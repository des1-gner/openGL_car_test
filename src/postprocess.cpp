#include "postprocess.h"
#include <cstdio>

// ── Fullscreen quad vertex shader (shared) ───────────────────────────────────
static const char* quadVert = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// ── Bloom bright-pass extraction ─────────────────────────────────────────────
static const char* bloomExtractFrag = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D scene;
uniform float threshold;
void main() {
    vec3 c = texture(scene, vUV).rgb;
    float brightness = dot(c, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold)
        FragColor = vec4(c, 1.0);
    else
        FragColor = vec4(0.0);
}
)";

// ── Gaussian blur ────────────────────────────────────────────────────────────
static const char* blurFrag = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D image;
uniform bool horizontal;
uniform float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
void main() {
    vec2 texelSize = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, vUV).rgb * weight[0];
    if (horizontal) {
        for (int i = 1; i < 5; i++) {
            result += texture(image, vUV + vec2(texelSize.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, vUV - vec2(texelSize.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; i++) {
            result += texture(image, vUV + vec2(0.0, texelSize.y * float(i))).rgb * weight[i];
            result += texture(image, vUV - vec2(0.0, texelSize.y * float(i))).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
)";

// ── Final tonemap + bloom composite ──────────────────────────────────────────
static const char* tonemapFrag = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D hdrScene;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform float bloomStrength;
uniform float vignetteStrength;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(hdrScene, vUV).rgb;
    vec3 bloom = texture(bloomBlur, vUV).rgb;
    hdr += bloom * bloomStrength;

    // Exposure + ACES tonemapping
    vec3 mapped = ACESFilm(hdr * exposure);

    // Vignette
    vec2 uv = vUV * 2.0 - 1.0;
    float vig = 1.0 - dot(uv, uv) * vignetteStrength;
    mapped *= clamp(vig, 0.0, 1.0);

    // Gamma
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
)";

// ── Implementation ───────────────────────────────────────────────────────────

void PostProcess::init(int w, int h) {
    width = w;
    height = h;

    bloomExtractShader.load(quadVert, bloomExtractFrag);
    blurShader.load(quadVert, blurFrag);
    tonemapShader.load(quadVert, tonemapFrag);

    // Fullscreen quad
    float quadVerts[] = {
        -1, -1,  0, 0,
         1, -1,  1, 0,
         1,  1,  1, 1,
        -1, -1,  0, 0,
         1,  1,  1, 1,
        -1,  1,  0, 1,
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    createFramebuffers();
}

void PostProcess::createFramebuffers() {
    // HDR framebuffer with 2 color attachments (scene + bright pass)
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(1, &hdrColorTex);
    glBindTexture(GL_TEXTURE_2D, hdrColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorTex, 0);

    glGenTextures(1, &hdrBrightTex);
    glBindTexture(GL_TEXTURE_2D, hdrBrightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, hdrBrightTex, 0);

    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    glGenRenderbuffers(1, &hdrDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "HDR FBO incomplete!\n");

    // Ping-pong blur buffers (half res for performance)
    int bw = width / 2, bh = height / 2;
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongTex);
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bw, bh, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongTex[i], 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::resize(int w, int h) {
    // Destroy old
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(1, &hdrColorTex);
    glDeleteTextures(1, &hdrBrightTex);
    glDeleteRenderbuffers(1, &hdrDepthRBO);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongTex);

    width = w;
    height = h;
    createFramebuffers();
}

void PostProcess::beginScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcess::endScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);

    int bw = width / 2, bh = height / 2;

    // 1) Extract bright areas from hdrBrightTex into pingpong[0]
    //    Actually we wrote bright to attachment1 in the PBR shader, so use hdrBrightTex directly
    //    But let's do a proper extract from the main color for simplicity
    glViewport(0, 0, bw, bh);
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]);
    bloomExtractShader.use();
    bloomExtractShader.setInt("scene", 0);
    bloomExtractShader.setFloat("threshold", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorTex);
    renderQuad();

    // 2) Gaussian blur ping-pong (6 passes)
    bool horizontal = true;
    for (int i = 0; i < 6; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal ? 1 : 0]);
        blurShader.use();
        blurShader.setInt("image", 0);
        blurShader.setInt("horizontal", horizontal ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pingpongTex[horizontal ? 0 : 1]);
        renderQuad();
        horizontal = !horizontal;
    }

    // 3) Final composite: tonemap + bloom
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    tonemapShader.use();
    tonemapShader.setInt("hdrScene", 0);
    tonemapShader.setInt("bloomBlur", 1);
    tonemapShader.setFloat("exposure", 1.2f);
    tonemapShader.setFloat("bloomStrength", 0.35f);
    tonemapShader.setFloat("vignetteStrength", 0.25f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongTex[0]);
    renderQuad();

    glEnable(GL_DEPTH_TEST);
}

void PostProcess::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcess::destroy() {
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(1, &hdrColorTex);
    glDeleteTextures(1, &hdrBrightTex);
    glDeleteRenderbuffers(1, &hdrDepthRBO);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongTex);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
}
