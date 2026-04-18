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

struct Building {
    Vec3 position;
    float width, depth, height;
    Vec3 color;
    float metallic, roughness;
    int style; // 0=office, 1=residential, 2=glass tower, 3=brick, 4=modern
};

class City {
public:
    void generate(int gridSize, float blockSize, float roadWidth);
    void buildMesh();
    void draw(const Shader& shader, const Mat4& view, const Mat4& proj) const;
    void drawEmissive(const Shader& shader, const Mat4& view, const Mat4& proj) const;
    void drawShadow(const Shader& shader, const Mat4& lightSpace) const;

    bool collides(const Vec3& pos, float radius) const;
    Vec3 pushOut(const Vec3& pos, float radius) const;

    float roadWidth;
    float blockSize;
    int gridSize;
    std::vector<Building> buildings;

private:
    Mesh groundMesh, roadMarkingMesh, sidewalkMesh;
    Mesh buildingMesh;
    Mesh emissiveMesh;  // windows, street lamps, traffic lights
    Mesh propMesh;      // street lamps, trees, benches, etc.

    void addQuad(std::vector<PBRVertex>& verts,
                 Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                 Vec3 normal, Vec3 color, float metallic, float roughness);

    void addBox(std::vector<PBRVertex>& verts,
                Vec3 mn, Vec3 mx, Vec3 color, float metallic, float roughness);

    void generateStreetLamp(std::vector<PBRVertex>& props, std::vector<PBRVertex>& emissive,
                            float x, float z, float height);

    void generateTree(std::vector<PBRVertex>& props, float x, float z);

    void generateCylinder(std::vector<PBRVertex>& verts, Vec3 base, float radius,
                          float height, int segments, Vec3 color, float met, float rough);
};
