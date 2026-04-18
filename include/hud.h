#pragma once
#include "shader.h"
#include "car.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

class HUD {
public:
    void init(int screenW, int screenH);
    void draw(const Car& player, float fps);
    void resize(int w, int h);

private:
    Shader hudShader;
    GLuint vao, vbo;
    int screenW, screenH;

    void drawBar(float x, float y, float w, float h,
                 float fill, Vec3 bgColor, Vec3 fgColor, float alpha = 0.85f);
    void drawRect(float x, float y, float w, float h, Vec4 color);
};
