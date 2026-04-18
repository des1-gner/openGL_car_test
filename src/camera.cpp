#include "camera.h"
#include <cmath>

Camera::Camera() : position(0, 10, -10), target(0, 0, 0) {}

void Camera::follow(const Vec3& carPos, float carHeading, float carSpeed,
                    float maxSpeed, float driftFactor, float dt) {
    float rad = carHeading * DEG2RAD;
    Vec3 back = {-std::sin(rad), 0, -std::cos(rad)};

    // Pull camera back at higher speeds
    float speedRatio = clampf(std::fabs(carSpeed) / maxSpeed, 0, 1);
    float dynDist = distance + speedRatio * 4.0f;
    float dynHeight = heightOffset + speedRatio * 1.5f;

    // Offset camera sideways during drift for dramatic angle
    Vec3 carRight = {std::cos(rad), 0, -std::sin(rad)};
    float driftOffset = driftFactor * 2.0f * (carSpeed > 0 ? 1.0f : -1.0f);

    Vec3 desiredPos = carPos + back * dynDist + Vec3(0, dynHeight, 0) + carRight * driftOffset;
    Vec3 desiredTarget = carPos + Vec3(0, 0.8f, 0);

    float t = 1.0f - std::exp(-smoothing * dt);
    position = Vec3::lerp(position, desiredPos, t);
    target = Vec3::lerp(target, desiredTarget, t * 1.5f);

    // FOV increases with speed (speed rush effect)
    float targetFOV = fovBase + speedRatio * 15.0f + driftFactor * 5.0f;
    fovCurrent = lerpf(fovCurrent, targetFOV, 3.0f * dt);

    // Camera shake during drift
    if (driftFactor > 0.3f) {
        float intensity = driftFactor * 0.08f;
        position.x += randf(-intensity, intensity);
        position.y += randf(-intensity * 0.5f, intensity * 0.5f);
    }
}

Mat4 Camera::getViewMatrix() const {
    return Mat4::lookAt(position, target, {0, 1, 0});
}
