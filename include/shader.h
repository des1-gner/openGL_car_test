#pragma once
#include <string>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

class Shader {
public:
    GLuint programID = 0;

    Shader() = default;
    // prevent accidental copies that would double-delete the GL program
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept : programID(o.programID) { o.programID = 0; }
    Shader& operator=(Shader&& o) noexcept {
        if (this != &o) { if (programID) glDeleteProgram(programID); programID = o.programID; o.programID = 0; }
        return *this;
    }

    void load(const char* vertexSrc, const char* fragmentSrc);
    void use() const;
    void setMat4(const char* name, const float* mat) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setFloat(const char* name, float val) const;
    void setInt(const char* name, int val) const;
    ~Shader();

private:
    GLuint compile(GLenum type, const char* src);
};
