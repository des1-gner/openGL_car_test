#pragma once
#include "math3d.h"

class Camera {
public:
    Vec3 position;
    Vec3 target;
    float distance    = 7.0f;
    float heightOffset = 3.0f;
    float smoothing   = 4.0f;

    // Dynamic camera — pulls back at speed, shifts during drift
    float fovBase     = 60.0f;
    float fovCurrent  = 60.0f;
    float shake       = 0.0f;

    Camera();
    void follow(const Vec3& carPos, float carHeading, float carSpeed,
                float maxSpeed, float driftFactor, float dt);
    Mat4 getViewMatrix() const;
    float getFOV() const { return fovCurrent; }
};
