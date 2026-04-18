#include "game.h"
#include <cstdio>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════════
// PBR-ish World Shader — metallic/roughness workflow with shadows
// ═══════════════════════════════════════════════════════════════════════════════

static const char* pbrVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
layout(location=3) in float aMetallic;
layout(location=4) in float aRoughness;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
out float vMetallic;
out float vRoughness;
out vec4 vLightSpacePos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = normalize(mat3(model) * aNormal);
    vColor = aColor;
    vMetallic = aMetallic;
    vRoughness = aRoughness;
    vLightSpacePos = lightSpaceMatrix * wp;
    gl_Position = projection * view * wp;
}
)";

static const char* pbrFrag = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in float vMetallic;
in float vRoughness;
in vec4 vLightSpacePos;

layout(location=0) out vec4 FragColor;
layout(location=1) out vec4 BrightColor;

uniform vec3 camPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform float emissive;
uniform float draftGlow;
uniform float gameTime;
uniform sampler2DShadow shadowMap;

const float PI = 3.14159265;

// ── PBR functions ──

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N,V),0.0), roughness)
         * GeometrySchlickGGX(max(dot(N,L),0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ── Shadow calculation with PCF ──

float calcShadow(vec4 lsPos) {
    vec3 projCoords = lsPos.xyz / lsPos.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float bias = 0.002;
    float currentDepth = projCoords.z - bias;

    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            shadow += texture(shadowMap, vec3(projCoords.xy + offset, currentDepth));
        }
    }
    return shadow / 25.0;
}

void main() {
    if (emissive > 0.5) {
        // Emissive objects (headlights, taillights, lit windows)
        vec3 emit = vColor;
        FragColor = vec4(emit, 1.0);
        float brightness = dot(emit, vec3(0.2126, 0.7152, 0.0722));
        BrightColor = brightness > 1.0 ? vec4(emit, 1.0) : vec4(0.0);
        return;
    }

    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vWorldPos);
    vec3 L = normalize(sunDir);
    vec3 H = normalize(V + L);

    vec3 albedo = vColor;
    float metallic = vMetallic;
    float roughness = max(vRoughness, 0.04);

    // Draft glow
    if (draftGlow > 0.0) {
        albedo = mix(albedo, vec3(1.0, 0.5, 0.0), draftGlow * 0.25);
    }

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    // Shadow
    float shadow = calcShadow(vLightSpacePos);

    vec3 Lo = (kD * albedo / PI + specular) * sunColor * NdotL * shadow;

    // Ambient (hemisphere)
    vec3 skyColor = vec3(0.4, 0.5, 0.7);
    vec3 groundColor = vec3(0.15, 0.12, 0.1);
    float hemi = dot(N, vec3(0,1,0)) * 0.5 + 0.5;
    vec3 ambient = mix(groundColor, skyColor, hemi) * albedo * 0.35;

    // Ground ambient occlusion (darken near ground)
    float ao = smoothstep(0.0, 2.0, vWorldPos.y) * 0.5 + 0.5;
    ambient *= ao;

    vec3 color = ambient + Lo;

    // Wet road reflections (ground plane only, y ≈ 0)
    if (vWorldPos.y < 0.02 && metallic < 0.01) {
        float wetness = 0.4;
        vec3 reflDir = reflect(-V, N);
        // Fake sky reflection
        float skyFactor = max(reflDir.y, 0.0);
        vec3 skyRefl = mix(vec3(0.3, 0.35, 0.45), vec3(0.6, 0.7, 0.9), skyFactor);
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 4.0) * wetness;
        color = mix(color, skyRefl * sunColor * 0.5, fresnel);
    }

    // Distance fog
    float dist = length(vWorldPos - camPos);
    float fog = 1.0 - exp(-dist * dist * 0.00008);
    vec3 fogColor = mix(vec3(0.55, 0.62, 0.75), sunColor * 0.3, 0.2);
    color = mix(color, fogColor, clamp(fog, 0.0, 1.0));

    FragColor = vec4(color, 1.0);

    // Bright pass for bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = brightness > 1.2 ? vec4(color, 1.0) : vec4(0.0);
}
)";

// ── Particle shader ──────────────────────────────────────────────────────────

static const char* particleVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in float aAlpha;

out vec2 vUV;
out float vAlpha;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vUV = aUV;
    vAlpha = aAlpha;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

static const char* particleFrag = R"(
#version 330 core
in vec2 vUV;
in float vAlpha;
out vec4 FragColor;

void main() {
    // Soft circle
    float dist = length(vUV - vec2(0.5));
    float alpha = smoothstep(0.5, 0.2, dist) * vAlpha;
    vec3 smokeColor = vec3(0.7, 0.7, 0.72);
    FragColor = vec4(smokeColor, alpha);
}
)";

// ═══════════════════════════════════════════════════════════════════════════════
// Game
// ═══════════════════════════════════════════════════════════════════════════════

Game::Game() : window(nullptr), winWidth(1280), winHeight(720),
               fbWidth(1280), fbHeight(720),
               lastTime(0), fps(0), gameTime(0) {
    sunDir = Vec3(0.4f, 0.8f, 0.3f).normalized();
    sunColor = {1.2f, 1.1f, 0.95f};
}

bool Game::init(int width, int height, const char* title) {
    winWidth = width;
    winHeight = height;

    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to init GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    window = glfwCreateWindow(winWidth, winHeight, title, nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferCallback);
    glfwSwapInterval(1);

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_MULTISAMPLE);

    // Load shaders
    pbrShader.load(pbrVert, pbrFrag);
    particleShader.load(particleVert, particleFrag);

    // Init shadow map
    shadowMap.init();

    // Init post-processing
    postProcess.init(fbWidth, fbHeight);

    // Init particles
    tireSmoke.init(2000);

    // Generate city
    city.generate(8, 10.0f, 7.0f);
    city.buildMesh();

    // Player car
    player.init({0, 0, 3}, 0, {0.85f, 0.12f, 0.08f});

    // AI cars
    Vec3 aiColors[] = {
        {0.08f, 0.35f, 0.85f},
        {0.08f, 0.75f, 0.20f},
        {0.90f, 0.70f, 0.05f},
        {0.60f, 0.08f, 0.75f},
        {0.10f, 0.10f, 0.10f},
    };
    float aiStarts[][3] = {
        { 5, 0,  5}, {-5, 0,  8}, {10, 0, -5},
        {-8, 0, -3}, { 0, 0, 12},
    };
    for (int i = 0; i < 5; i++) {
        Car ai;
        ai.init({aiStarts[i][0], aiStarts[i][1], aiStarts[i][2]},
                (float)(i * 72), aiColors[i]);
        ai.maxSpeed = 24.0f + i * 2.0f;
        aiCars.push_back(ai);
    }

    // HUD
    hud.init(fbWidth, fbHeight);

    lastTime = (float)glfwGetTime();

    std::printf("\n");
    std::printf("╔══════════════════════════════════════╗\n");
    std::printf("║     CITY DRIFT — Racing Game         ║\n");
    std::printf("╠══════════════════════════════════════╣\n");
    std::printf("║  W / ↑        Accelerate             ║\n");
    std::printf("║  S / ↓        Brake / Reverse        ║\n");
    std::printf("║  A / ←        Steer Left             ║\n");
    std::printf("║  D / →        Steer Right            ║\n");
    std::printf("║  SPACE        Handbrake (DRIFT!)     ║\n");
    std::printf("║  ESC          Quit                   ║\n");
    std::printf("╠══════════════════════════════════════╣\n");
    std::printf("║  Draft behind cars for speed boost   ║\n");
    std::printf("║  Hold SPACE + steer to drift!        ║\n");
    std::printf("╚══════════════════════════════════════╝\n\n");

    return true;
}

void Game::framebufferCallback(GLFWwindow* w, int width, int height) {
    Game* g = static_cast<Game*>(glfwGetWindowUserPointer(w));
    if (g) {
        g->fbWidth = width;
        g->fbHeight = height;
        g->postProcess.resize(width, height);
        g->hud.resize(width, height);
        glViewport(0, 0, width, height);
    }
}

void Game::processInput() {
    input.accelerate = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                       glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    input.brake      = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
                       glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    input.turnLeft   = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                       glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    input.turnRight  = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
                       glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    input.handbrake  = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void Game::update(float dt) {
    gameTime += dt;

    // Player
    player.update(input, dt);
    Physics::resolveCollisions(player, city);

    // AI
    for (auto& ai : aiCars) {
        Physics::updateAI(ai, player, city, dt);
        Physics::resolveCollisions(ai, city);
    }

    // Drafting
    std::vector<Car> all;
    all.push_back(player);
    for (auto& ai : aiCars) all.push_back(ai);
    Physics::updateDrafting(all);
    player.draftBoost = all[0].draftBoost;
    player.inDraft = all[0].inDraft;
    for (size_t i = 0; i < aiCars.size(); i++) {
        aiCars[i].draftBoost = all[i+1].draftBoost;
        aiCars[i].inDraft = all[i+1].inDraft;
    }

    // Camera
    camera.follow(player.position, player.heading, player.speed,
                  player.maxSpeed, player.driftFactor, dt);

    // Tire smoke particles
    auto emitSmoke = [&](Car& car) {
        if (car.isDrifting && std::fabs(car.speed) > 3.0f) {
            int count = (int)(car.driftFactor * 4);
            Vec3 smokeVel = car.velocity * -0.1f + Vec3(0, 0.5f, 0);
            tireSmoke.emit(car.rearLeftWheel(), smokeVel, count, 0.25f + car.driftFactor * 0.3f);
            tireSmoke.emit(car.rearRightWheel(), smokeVel, count, 0.25f + car.driftFactor * 0.3f);
        }
    };
    emitSmoke(player);
    for (auto& ai : aiCars) emitSmoke(ai);

    tireSmoke.update(dt);
}

void Game::renderShadowPass() {
    shadowMap.beginPass(sunDir, player.position, 50.0f);
    Mat4 lsm = shadowMap.getLightSpaceMatrix();
    const Shader& ds = shadowMap.getShader();

    city.drawShadow(ds, lsm);
    player.drawShadow(ds, lsm);
    for (auto& ai : aiCars) ai.drawShadow(ds, lsm);

    shadowMap.endPass();
}

void Game::renderScene(const Mat4& view, const Mat4& proj) {
    // Bind shadow map
    pbrShader.use();
    shadowMap.bind(0);
    pbrShader.setInt("shadowMap", 0);
    pbrShader.setMat4("lightSpaceMatrix", shadowMap.getLightSpaceMatrix().data());
    pbrShader.setVec3("camPos", camera.position.x, camera.position.y, camera.position.z);
    pbrShader.setVec3("sunDir", sunDir.x, sunDir.y, sunDir.z);
    pbrShader.setVec3("sunColor", sunColor.x, sunColor.y, sunColor.z);
    pbrShader.setFloat("gameTime", gameTime);

    // Draw city (non-emissive)
    city.draw(pbrShader, view, proj);

    // Draw cars
    player.draw(pbrShader, view, proj);
    for (auto& ai : aiCars) {
        ai.draw(pbrShader, view, proj);
    }

    // Draw city emissive (windows, lamps)
    city.drawEmissive(pbrShader, view, proj);

    // Particles
    tireSmoke.draw(particleShader, view, proj);
}

void Game::render() {
    float aspect = (float)fbWidth / (float)fbHeight;
    Mat4 proj = Mat4::perspective(camera.getFOV(), aspect, 0.1f, 300.0f);
    Mat4 view = camera.getViewMatrix();

    // 1) Shadow pass
    renderShadowPass();

    // 2) Main scene into HDR framebuffer
    postProcess.beginScene();
    glClearColor(0.55f, 0.62f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, fbWidth, fbHeight);

    renderScene(view, proj);

    postProcess.endScene();

    // 3) HUD on top (after tonemapping, directly to screen)
    hud.draw(player, fps);
}

void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f;
        fps = 1.0f / dt;

        processInput();
        update(dt);
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Game::cleanup() {
    postProcess.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}
