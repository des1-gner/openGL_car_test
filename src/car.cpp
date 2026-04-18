#include "car.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

Car::Car()
    : position(0,0,0), heading(0), speed(0), velocity(0,0,0),
      slipAngle(0), driftFactor(0), angularVel(0), isDrifting(false),
      driftTimer(0), tireTemp(0), draftBoost(0), inDraft(false),
      bodyColor(1,0,0) {}

void Car::init(Vec3 pos, float hdg, Vec3 col) {
    position = pos;
    heading = hdg;
    bodyColor = col;
    speed = 0;
    velocity = {0,0,0};
    slipAngle = 0;
    driftFactor = 0;
    angularVel = 0;
    isDrifting = false;
    driftTimer = 0;
    tireTemp = 0;
    loadModel();
}

Vec3 Car::forward() const {
    float rad = heading * DEG2RAD;
    return {std::sin(rad), 0, std::cos(rad)};
}

Vec3 Car::right() const {
    float rad = heading * DEG2RAD;
    return {std::cos(rad), 0, -std::sin(rad)};
}

Vec3 Car::rearLeftWheel() const {
    return position - forward() * (length * 0.35f) - right() * (width * 0.5f);
}
Vec3 Car::rearRightWheel() const {
    return position - forward() * (length * 0.35f) + right() * (width * 0.5f);
}

void Car::update(const CarInput& input, float dt) {
    float effectiveMax = maxSpeed * (1.0f + draftBoost * 0.3f);
    float effectiveEngine = engineForce * (1.0f + draftBoost * 0.2f);

    float throttle = 0;
    if (input.accelerate) throttle = 1.0f;
    if (input.brake) throttle = -1.0f;

    float engineF = 0;
    if (throttle > 0) {
        engineF = effectiveEngine * throttle;
        float speedRatio = std::fabs(speed) / effectiveMax;
        engineF *= (1.0f - speedRatio * speedRatio * 0.8f);
    } else if (throttle < 0) {
        if (speed > 0.5f) engineF = -brakeForce;
        else engineF = -engineForce * 0.4f;
    }

    float steerInput = 0;
    if (input.turnLeft)  steerInput =  1.0f;
    if (input.turnRight) steerInput = -1.0f;

    float speedFactor = clampf(std::fabs(speed) / effectiveMax, 0, 1);
    float maxSteer = maxSteerAngle * (1.0f - speedFactor * 0.6f);
    float steerAngle = steerInput * maxSteer;

    bool handbraking = input.handbrake && std::fabs(speed) > 3.0f;
    float rearGrip = gripRear;
    if (handbraking) rearGrip *= 0.2f;

    Vec3 fwd = forward();
    Vec3 rt = right();
    float lateralSpeed = velocity.dot(rt);
    float forwardSpeed = velocity.dot(fwd);

    if (std::fabs(forwardSpeed) > 0.5f) {
        slipAngle = std::atan2(lateralSpeed, std::fabs(forwardSpeed)) * RAD2DEG;
    } else {
        slipAngle *= 0.95f;
    }

    isDrifting = std::fabs(slipAngle) > 8.0f || handbraking;
    if (isDrifting) {
        driftTimer += dt;
        tireTemp = clampf(tireTemp + dt * 2.0f, 0, 1);
    } else {
        driftTimer = 0;
        tireTemp = clampf(tireTemp - dt * 1.5f, 0, 1);
    }
    driftFactor = clampf(std::fabs(slipAngle) / 45.0f, 0, 1);

    Vec3 forceForward = fwd * engineF;

    float frontSlipAngle = steerAngle - std::atan2(lateralSpeed + angularVel * DEG2RAD * wheelBase * 0.5f,
                                                     std::fabs(forwardSpeed) + 0.1f) * RAD2DEG;
    float rearSlipAngle = -std::atan2(lateralSpeed - angularVel * DEG2RAD * wheelBase * 0.5f,
                                       std::fabs(forwardSpeed) + 0.1f) * RAD2DEG;

    float frontLateralForce = clampf(frontSlipAngle * cornerStiff * gripFront, -20.0f, 20.0f);
    float rearLateralForce  = clampf(rearSlipAngle  * cornerStiff * rearGrip,  -20.0f, 20.0f);

    float drag = 0.4f * speed * std::fabs(speed);
    float rolling = 3.0f * speed;
    Vec3 resistForce = fwd * (-(drag + rolling));

    Vec3 totalForce = forceForward + resistForce + rt * (frontLateralForce + rearLateralForce);

    Vec3 accel = totalForce / mass * 1000.0f;
    velocity += accel * dt;

    float latDamp = handbraking ? 0.985f : 0.94f;
    float latVel = velocity.dot(rt);
    velocity -= rt * (latVel * (1.0f - latDamp));

    speed = velocity.dot(fwd);
    if (std::fabs(speed) > effectiveMax) {
        velocity = velocity.normalized() * effectiveMax;
        speed = clampf(speed, -effectiveMax * 0.3f, effectiveMax);
    }

    float yawTorque = frontLateralForce * wheelBase * 0.5f - rearLateralForce * wheelBase * 0.5f;
    angularVel += yawTorque / mass * 1000.0f * dt;

    if (std::fabs(speed) > 1.0f) {
        float turnSign = speed > 0 ? 1.0f : -1.0f;
        float arcadeTurn = steerAngle * (std::fabs(speed) / effectiveMax) * 2.5f * turnSign;
        angularVel = lerpf(angularVel, arcadeTurn * 60.0f, 3.0f * dt);
    }

    if (isDrifting && !handbraking && std::fabs(steerInput) < 0.1f) {
        angularVel *= (1.0f - 2.0f * dt);
    }
    angularVel *= (1.0f - 3.0f * dt);
    heading += angularVel * dt;

    position += velocity * dt;
    position.y = 0;
}

// ── Model loading ────────────────────────────────────────────────────────────

void Car::loadModel() {
    std::vector<PBRVertex> bodyVerts, glassVerts, wheelVerts, lightVerts, detailVerts;

    bool ok = loadOBJ("assets/car_body.obj", bodyVerts, bodyColor, 0.3f, 0.35f);
    if (ok) {
        loadOBJ("assets/car_glass.obj", glassVerts, {0.12f, 0.14f, 0.20f}, 0.1f, 0.05f);
        loadOBJ("assets/car_wheels.obj", wheelVerts, {0.08f, 0.08f, 0.08f}, 0.0f, 0.85f);
        loadOBJ("assets/car_lights.obj", lightVerts, {5.0f, 4.5f, 3.5f}, 0.0f, 0.0f);
        loadOBJ("assets/car_details.obj", detailVerts, bodyColor * 0.7f, 0.4f, 0.3f);

        // Color the rim parts of wheels differently
        for (auto& v : wheelVerts) {
            // Vertices closer to center are rim (metallic)
            float distFromCenter = std::sqrt(v.pos.y * v.pos.y + v.pos.z * v.pos.z);
            // This is approximate — inner verts get metallic treatment
        }

        bodyMesh.upload(bodyVerts);
        glassMesh.upload(glassVerts);
        wheelMesh.upload(wheelVerts);
        lightMesh.upload(lightVerts);
        detailMesh.upload(detailVerts);
        modelLoaded = true;
        std::printf("Car model loaded: %d body + %d glass + %d wheel + %d light + %d detail verts\n",
                    (int)bodyVerts.size(), (int)glassVerts.size(), (int)wheelVerts.size(),
                    (int)lightVerts.size(), (int)detailVerts.size());
    } else {
        std::printf("OBJ not found, using fallback box car\n");
        buildFallback();
    }
}

void Car::buildFallback() {
    // Simple box car as fallback
    std::vector<PBRVertex> verts;
    float hw = width * 0.5f, hl = length * 0.5f, h = height;

    auto quad = [&](Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 n, Vec3 c, float met, float rough) {
        verts.push_back({p0, n, c, met, rough});
        verts.push_back({p1, n, c, met, rough});
        verts.push_back({p2, n, c, met, rough});
        verts.push_back({p0, n, c, met, rough});
        verts.push_back({p2, n, c, met, rough});
        verts.push_back({p3, n, c, met, rough});
    };

    quad({-hw,0.08f,-hl},{hw,0.08f,-hl},{hw,h,-hl},{-hw,h,-hl}, {0,0,-1}, bodyColor, 0.3f, 0.35f);
    quad({-hw,0.08f,hl},{hw,0.08f,hl},{hw,h,hl},{-hw,h,hl}, {0,0,1}, bodyColor, 0.3f, 0.35f);
    quad({-hw,0.08f,-hl},{-hw,0.08f,hl},{-hw,h,hl},{-hw,h,-hl}, {-1,0,0}, bodyColor*0.8f, 0.3f, 0.35f);
    quad({hw,0.08f,hl},{hw,0.08f,-hl},{hw,h,-hl},{hw,h,hl}, {1,0,0}, bodyColor*0.8f, 0.3f, 0.35f);
    quad({-hw,h,-hl},{hw,h,-hl},{hw,h,hl},{-hw,h,hl}, {0,1,0}, bodyColor, 0.3f, 0.35f);

    fallbackMesh.upload(verts);
}

void Car::draw(const Shader& shader, const Mat4& view, const Mat4& proj) const {
    Mat4 model = Mat4::translate(position) * Mat4::rotateY(heading);

    shader.use();
    shader.setMat4("model", model.data());
    shader.setMat4("view", view.data());
    shader.setMat4("projection", proj.data());
    shader.setFloat("draftGlow", inDraft ? draftBoost : 0.0f);

    if (modelLoaded) {
        shader.setFloat("emissive", 0.0f);
        bodyMesh.draw();
        glassMesh.draw();
        wheelMesh.draw();
        detailMesh.draw();

        shader.setFloat("emissive", 1.0f);
        lightMesh.draw();
    } else {
        shader.setFloat("emissive", 0.0f);
        fallbackMesh.draw();
    }
}

void Car::drawShadow(const Shader& shader, const Mat4& lightSpace) const {
    Mat4 model = Mat4::translate(position) * Mat4::rotateY(heading);
    shader.use();
    shader.setMat4("model", model.data());
    shader.setMat4("lightSpaceMatrix", lightSpace.data());

    if (modelLoaded) {
        bodyMesh.draw();
        wheelMesh.draw();
        detailMesh.draw();
    } else {
        fallbackMesh.draw();
    }
}
