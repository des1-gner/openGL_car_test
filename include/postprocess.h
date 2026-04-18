#pragma once
#include "shader.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

class PostProcess {
public:
    void init(int width, int height);
    void resize(int width, int height);
    void beginScene();       // bind HDR framebuffer
    void endScene();         // unbind, render to screen with bloom + tonemap
    void destroy();

private:
    int width, height;

    // Main HDR framebuffer
    GLuint hdrFBO, hdrColorTex, hdrBrightTex, hdrDepthRBO;

    // Ping-pong blur framebuffers
    GLuint pingpongFBO[2], pingpongTex[2];

    // Fullscreen quad
    GLuint quadVAO, quadVBO;

    Shader bloomExtractShader;
    Shader blurShader;
    Shader tonemapShader;

    void createFramebuffers();
    void renderQuad();
};
