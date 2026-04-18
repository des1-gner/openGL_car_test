#pragma once
#include "math3d.h"
#include "objloader.h"
#include <vector>

// Procedural high-poly sports car generator
// Creates a smooth, curved car body using cross-section lofting
class CarModelGenerator {
public:
    // Generate a complete sports car mesh
    // Returns separate meshes for body, glass, lights, wheels, undercarriage
    struct CarMeshes {
        std::vector<PBRVertex> body;
        std::vector<PBRVertex> glass;
        std::vector<PBRVertex> lights;     // emissive headlights + taillights
        std::vector<PBRVertex> wheels;
        std::vector<PBRVertex> details;    // spoiler, mirrors, grille, etc.
        std::vector<PBRVertex> undercarriage;
    };

    static CarMeshes generate(Vec3 bodyColor, float scale = 1.0f);

private:
    // Cross-section profile at a given Z position along the car
    struct Profile {
        float z;           // position along car length
        float halfWidth;   // half-width at this section
        float floorY;      // bottom of body
        float roofY;       // top of body
        float beltY;       // beltline (window bottom)
        float roofWidth;   // half-width of roof
    };

    static void loftBody(const std::vector<Profile>& profiles, int circumSegments,
                         std::vector<PBRVertex>& out, Vec3 color, float metallic, float roughness);

    static void generateWheel(Vec3 center, float radius, float width, int segments,
                              std::vector<PBRVertex>& tireOut, bool leftSide);

    static void addSmoothQuad(std::vector<PBRVertex>& out,
                              Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                              Vec3 color, float metallic, float roughness);

    static void addTriangle(std::vector<PBRVertex>& out,
                            Vec3 p0, Vec3 p1, Vec3 p2, Vec3 n,
                            Vec3 color, float metallic, float roughness);

    static Vec3 calcNormal(Vec3 p0, Vec3 p1, Vec3 p2);
};
