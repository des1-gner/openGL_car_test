#pragma once
#include "math3d.h"
#include "shader.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

class ShadowMap {
public:
    static constexpr int SIZE = 2048;

    void init();
    void beginPass(const Vec3& lightDir, const Vec3& sceneCenter, float radius);
    void endPass();
    void bind(int textureUnit) const;
    Mat4 getLightSpaceMatrix() const;
    const Shader& getShader() const { return depthShader; }

private:
    GLuint fbo, depthTex;
    Mat4 lightSpaceMat;
    Shader depthShader;
};
