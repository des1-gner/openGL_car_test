#pragma once
#include "math3d.h"
#include "shader.h"
#include "objloader.h"
#include <vector>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

struct CarInput {
    bool accelerate = false;
    bool brake = false;
    bool turnLeft = false;
    bool turnRight = false;
    bool handbrake = false;
};

class Car {
public:
    Vec3 position;
    float heading;
    float speed;
    Vec3 velocity;

    float slipAngle;
    float driftFactor;
    float angularVel;
    bool isDrifting;
    float driftTimer;
    float tireTemp;

    float draftBoost;
    bool inDraft;

    float length = 2.2f;
    float width  = 1.1f;
    float height = 0.55f;

    float maxSpeed      = 32.0f;
    float engineForce   = 16.0f;
    float brakeForce    = 22.0f;
    float maxSteerAngle = 35.0f;
    float wheelBase     = 1.8f;
    float mass          = 1200.0f;
    float gripFront     = 1.0f;
    float gripRear      = 1.0f;
    float cornerStiff   = 8.0f;

    Vec3 bodyColor;

    Car();
    void init(Vec3 pos, float hdg, Vec3 col);
    void update(const CarInput& input, float dt);
    void loadModel();  // loads OBJ files
    void draw(const Shader& shader, const Mat4& view, const Mat4& proj) const;
    void drawShadow(const Shader& shader, const Mat4& lightSpace) const;

    Vec3 forward() const;
    Vec3 right() const;
    Vec3 rearLeftWheel() const;
    Vec3 rearRightWheel() const;

private:
    Mesh bodyMesh, glassMesh, wheelMesh, lightMesh, detailMesh;
    bool modelLoaded = false;

    // Fallback box mesh if OBJ not found
    Mesh fallbackMesh;
    void buildFallback();
};
