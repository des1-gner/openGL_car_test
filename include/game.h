#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "shader.h"
#include "car.h"
#include "city.h"
#include "camera.h"
#include "physics.h"
#include "hud.h"
#include "shadows.h"
#include "postprocess.h"
#include "particles.h"
#include <vector>

class Game {
public:
    Game();
    bool init(int width, int height, const char* title);
    void run();
    void cleanup();

private:
    GLFWwindow* window;
    int winWidth, winHeight;
    int fbWidth, fbHeight;

    // Shaders
    Shader pbrShader;
    Shader particleShader;

    // Scene
    Car player;
    std::vector<Car> aiCars;
    City city;
    Camera camera;
    HUD hud;

    // Effects
    ShadowMap shadowMap;
    PostProcess postProcess;
    ParticleSystem tireSmoke;

    // Sun
    Vec3 sunDir;
    Vec3 sunColor;

    CarInput input;
    float lastTime;
    float fps;
    float gameTime;

    void processInput();
    void update(float dt);
    void renderShadowPass();
    void renderScene(const Mat4& view, const Mat4& proj);
    void render();

    static void framebufferCallback(GLFWwindow* w, int width, int height);
};
