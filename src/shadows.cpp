#include "shadows.h"
#include <cstdio>

static const char* depthVert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

static const char* depthFrag = R"(
#version 330 core
void main() {
    // depth is written automatically
}
)";

void ShadowMap::init() {
    depthShader.load(depthVert, depthFrag);

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SIZE, SIZE, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "Shadow FBO incomplete!\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::beginPass(const Vec3& lightDir, const Vec3& sceneCenter, float radius) {
    Vec3 ld = lightDir.normalized();
    Vec3 lightPos = sceneCenter - ld * radius;
    Mat4 lightView = Mat4::lookAt(lightPos, sceneCenter, {0, 1, 0});
    Mat4 lightProj = Mat4::ortho(-radius, radius, -radius, radius, 0.1f, radius * 2.5f);
    lightSpaceMat = lightProj * lightView;

    glViewport(0, 0, SIZE, SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT); // reduce peter-panning
}

void ShadowMap::endPass() {
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bind(int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthTex);
}

Mat4 ShadowMap::getLightSpaceMatrix() const {
    return lightSpaceMat;
}
