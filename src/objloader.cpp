#include "objloader.h"
#include <fstream>
#include <sstream>
#include <cstdio>

void Mesh::upload(const std::vector<PBRVertex>& verts) {
    vertexCount = (int)verts.size();
    if (vertexCount == 0) return;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(PBRVertex), verts.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PBRVertex), (void*)offsetof(PBRVertex, pos));
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PBRVertex), (void*)offsetof(PBRVertex, normal));
    glEnableVertexAttribArray(1);
    // color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PBRVertex), (void*)offsetof(PBRVertex, color));
    glEnableVertexAttribArray(2);
    // metallic
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(PBRVertex), (void*)offsetof(PBRVertex, metallic));
    glEnableVertexAttribArray(3);
    // roughness
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(PBRVertex), (void*)offsetof(PBRVertex, roughness));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (vertexCount == 0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void Mesh::destroy() {
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    vertexCount = 0;
}

bool loadOBJ(const std::string& path, std::vector<PBRVertex>& outVerts,
             Vec3 defaultColor, float defaultMetallic, float defaultRoughness) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::fprintf(stderr, "Cannot open OBJ: %s\n", path.c_str());
        return false;
    }

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texcoords;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vec3 p;
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (prefix == "vn") {
            Vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (prefix == "vt") {
            Vec2 t;
            iss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (prefix == "f") {
            // Parse face — supports v, v/vt, v/vt/vn, v//vn
            std::vector<int> faceVerts;
            std::vector<int> faceNormals;
            std::string token;
            while (iss >> token) {
                int vi = 0, ti = 0, ni = 0;
                // Replace '/' with spaces for parsing
                for (char& c : token) if (c == '/') c = ' ';
                std::istringstream tss(token);

                // Count original slashes to determine format
                int slashCount = 0;
                bool doubleSlash = false;
                for (size_t i = 0; i < line.size() - 1; i++) {
                    // This is approximate; we'll just try parsing
                }

                tss >> vi;
                if (tss >> ti) {
                    tss >> ni;
                } else {
                    // Could be v//vn format — re-parse
                    size_t s1 = token.find(' ');
                    if (s1 != std::string::npos) {
                        std::string rest = token.substr(s1 + 1);
                        // If rest starts with space, it was //
                        if (!rest.empty() && rest[0] == ' ') {
                            std::istringstream nss(rest);
                            nss >> ni;
                        }
                    }
                }

                faceVerts.push_back(vi > 0 ? vi - 1 : (int)positions.size() + vi);
                faceNormals.push_back(ni > 0 ? ni - 1 : -1);
            }

            // Triangulate (fan)
            for (size_t i = 2; i < faceVerts.size(); i++) {
                int indices[3] = { faceVerts[0], faceVerts[i-1], faceVerts[i] };
                int nindices[3] = { faceNormals[0], faceNormals[i-1], faceNormals[i] };

                Vec3 p0 = positions[indices[0]];
                Vec3 p1 = positions[indices[1]];
                Vec3 p2 = positions[indices[2]];

                // Compute face normal if vertex normals not available
                Vec3 faceNormal = (p1 - p0).cross(p2 - p0).normalized();

                for (int k = 0; k < 3; k++) {
                    PBRVertex v;
                    v.pos = positions[indices[k]];
                    v.normal = (nindices[k] >= 0 && nindices[k] < (int)normals.size())
                               ? normals[nindices[k]] : faceNormal;
                    v.color = defaultColor;
                    v.metallic = defaultMetallic;
                    v.roughness = defaultRoughness;
                    outVerts.push_back(v);
                }
            }
        }
    }

    std::printf("Loaded OBJ: %s (%d vertices)\n", path.c_str(), (int)outVerts.size());
    return !outVerts.empty();
}
