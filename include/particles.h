#pragma once
#include "math3d.h"
#include "shader.h"
#include <vector>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

struct Particle {
    Vec3 position;
    Vec3 velocity;
    float life;
    float maxLife;
    float size;
    float alpha;
};

class ParticleSystem {
public:
    void init(int maxParticles);
    void emit(const Vec3& pos, const Vec3& vel, int count, float size = 0.3f, float life = 1.0f);
    void update(float dt);
    void draw(const Shader& shader, const Mat4& view, const Mat4& proj);

private:
    std::vector<Particle> particles;
    int maxParticles;
    GLuint vao, vbo;
};
