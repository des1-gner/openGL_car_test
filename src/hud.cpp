#include "hud.h"
#include <cmath>

static const char* hudVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

static const char* hudFragSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
)";

void HUD::init(int w, int h) {
    screenW = w; screenH = h;
    hudShader.load(hudVertSrc, hudFragSrc);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void HUD::resize(int w, int h) { screenW = w; screenH = h; }

void HUD::drawRect(float x, float y, float w, float h, Vec4 color) {
    float proj[16] = {0};
    proj[0] = 2.0f/screenW; proj[5] = 2.0f/screenH;
    proj[10] = -1; proj[12] = -1; proj[13] = -1; proj[15] = 1;

    hudShader.use();
    hudShader.setMat4("projection", proj);
    hudShader.setVec4("uColor", color.x, color.y, color.z, color.w);

    float verts[] = { x,y, x+w,y, x+w,y+h, x,y, x+w,y+h, x,y+h };
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void HUD::drawBar(float x, float y, float w, float h,
                   float fill, Vec3 bgColor, Vec3 fgColor, float alpha) {
    drawRect(x, y, w, h, {bgColor.x, bgColor.y, bgColor.z, alpha * 0.5f});
    drawRect(x, y, w * clampf(fill, 0, 1), h, {fgColor.x, fgColor.y, fgColor.z, alpha});
    // Border
    drawRect(x, y, w, 1, {1,1,1,0.2f});
    drawRect(x, y+h-1, w, 1, {1,1,1,0.2f});
    drawRect(x, y, 1, h, {1,1,1,0.2f});
    drawRect(x+w-1, y, 1, h, {1,1,1,0.2f});
}

void HUD::draw(const Car& player, float fps) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float m = 25.0f;
    float barW = 220.0f;
    float barH = 16.0f;
    float gap = 6.0f;
    float baseY = screenH - m;

    // Speed label background + bar
    float speedFill = std::fabs(player.speed) / player.maxSpeed;
    Vec3 speedColor = {0.1f, 0.85f, 0.3f};
    if (speedFill > 0.8f) speedColor = {0.9f, 0.3f, 0.1f};
    drawBar(m, baseY - barH, barW, barH, speedFill, {0.1f,0.1f,0.1f}, speedColor);

    // Speed text area (km/h indicator)
    float kmh = std::fabs(player.speed) * 3.6f;
    // Simple numeric display using bars
    int speedInt = (int)kmh;
    float digitW = 8.0f;
    for (int d = 2; d >= 0; d--) {
        int digit = speedInt % 10;
        speedInt /= 10;
        float dx = m + barW + 10 + d * (digitW + 2);
        // 7-segment style using rectangles
        float dy = baseY - barH;
        float segH = barH * 0.45f;
        float segW = digitW;
        Vec4 on = {0.9f, 0.9f, 0.9f, 0.9f};
        Vec4 off = {0.2f, 0.2f, 0.2f, 0.3f};
        // Simplified: just show filled proportion
        float fillD = digit / 9.0f;
        drawRect(dx, dy, segW, barH, {0.9f, 0.9f, 0.9f, 0.15f + fillD * 0.7f});
    }

    // Draft boost bar
    float draftFill = player.draftBoost;
    drawBar(m, baseY - barH*2 - gap, barW, barH, draftFill,
            {0.1f,0.1f,0.15f}, {0.2f, 0.5f, 1.0f});

    // Drift indicator
    if (player.isDrifting) {
        float driftIntensity = player.driftFactor;
        Vec3 driftColor = Vec3::lerp({1.0f, 0.6f, 0.0f}, {1.0f, 0.1f, 0.0f}, driftIntensity);
        drawBar(m, baseY - barH*3 - gap*2, barW, barH * 0.7f, driftIntensity,
                {0.15f,0.05f,0.0f}, driftColor, 0.9f);
    }

    // Draft active glow
    if (player.inDraft) {
        float pulse = 0.7f + 0.3f * std::sin(player.draftBoost * 10.0f);
        drawRect(m - 3, baseY - barH*2 - gap - 3, barW + 6, barH + 6,
                 {0.2f, 0.5f, 1.0f, 0.15f * pulse});
    }

    // Tire temperature
    if (player.tireTemp > 0.05f) {
        drawBar(m, baseY - barH*4 - gap*3, barW * 0.6f, barH * 0.5f, player.tireTemp,
                {0.1f,0.1f,0.1f}, {1.0f, 0.4f, 0.1f}, 0.7f);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
