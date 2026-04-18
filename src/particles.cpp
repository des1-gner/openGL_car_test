#include "particles.h"
#include <algorithm>

void ParticleSystem::init(int max) {
    maxParticles = max;
    particles.reserve(max);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // Each particle billboard: 6 verts * (pos3 + uv2 + alpha1) = 6 floats per vert
    glBufferData(GL_ARRAY_BUFFER, max * 6 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(5*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void ParticleSystem::emit(const Vec3& pos, const Vec3& vel, int count, float size, float life) {
    for (int i = 0; i < count && (int)particles.size() < maxParticles; i++) {
        Particle p;
        p.position = pos + Vec3(randf(-0.2f, 0.2f), randf(0, 0.1f), randf(-0.2f, 0.2f));
        p.velocity = vel + Vec3(randf(-0.5f, 0.5f), randf(0.3f, 1.0f), randf(-0.5f, 0.5f));
        p.life = life + randf(-0.2f, 0.2f);
        p.maxLife = p.life;
        p.size = size + randf(-0.05f, 0.1f);
        p.alpha = 0.6f;
        particles.push_back(p);
    }
}

void ParticleSystem::update(float dt) {
    for (auto& p : particles) {
        p.velocity.y += 0.5f * dt; // slight upward drift
        p.velocity *= (1.0f - 1.5f * dt); // drag
        p.position += p.velocity * dt;
        p.life -= dt;
        p.alpha = (p.life / p.maxLife) * 0.5f;
        p.size += dt * 0.8f; // expand
    }
    // Remove dead
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                        [](const Particle& p) { return p.life <= 0; }),
        particles.end());
}

void ParticleSystem::draw(const Shader& shader, const Mat4& view, const Mat4& proj) {
    if (particles.empty()) return;

    // Extract camera right and up from view matrix
    Vec3 camRight = {view.m[0], view.m[4], view.m[8]};
    Vec3 camUp    = {view.m[1], view.m[5], view.m[9]};

    std::vector<float> verts;
    verts.reserve(particles.size() * 36);

    for (auto& p : particles) {
        float hs = p.size * 0.5f;
        Vec3 r = camRight * hs;
        Vec3 u = camUp * hs;

        Vec3 bl = p.position - r - u;
        Vec3 br = p.position + r - u;
        Vec3 tr = p.position + r + u;
        Vec3 tl = p.position - r + u;

        auto push = [&](Vec3 pos, float uvx, float uvy) {
            verts.push_back(pos.x); verts.push_back(pos.y); verts.push_back(pos.z);
            verts.push_back(uvx); verts.push_back(uvy);
            verts.push_back(p.alpha);
        };

        push(bl, 0, 0); push(br, 1, 0); push(tr, 1, 1);
        push(bl, 0, 0); push(tr, 1, 1); push(tl, 0, 1);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());

    shader.use();
    shader.setMat4("view", view.data());
    shader.setMat4("projection", proj.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glDrawArrays(GL_TRIANGLES, 0, (int)(verts.size() / 6));

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
