#pragma once
#include "math3d.h"
#include <vector>
#include <string>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// Vertex format matching our PBR pipeline: pos3 + normal3 + color3 + metallic1 + roughness1
struct PBRVertex {
    Vec3 pos;
    Vec3 normal;
    Vec3 color;
    float metallic;
    float roughness;
};

struct Mesh {
    GLuint vao = 0, vbo = 0;
    int vertexCount = 0;

    void upload(const std::vector<PBRVertex>& verts);
    void draw() const;
    void destroy();
};

// Load an OBJ file. Returns true on success.
// Assigns a default color/metallic/roughness to all vertices.
bool loadOBJ(const std::string& path, std::vector<PBRVertex>& outVerts,
             Vec3 defaultColor = {0.5f, 0.5f, 0.5f},
             float defaultMetallic = 0.3f, float defaultRoughness = 0.4f);
