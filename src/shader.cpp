#include "shader.h"
#include <cstdio>

GLuint Shader::compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::fprintf(stderr, "Shader compile error (%s):\n%s\n",
                     type == GL_VERTEX_SHADER ? "vert" : "frag", log);
    }
    return s;
}

void Shader::load(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);
    programID = glCreateProgram();
    glAttachShader(programID, vs);
    glAttachShader(programID, fs);
    glLinkProgram(programID);
    GLint ok;
    glGetProgramiv(programID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(programID, 1024, nullptr, log);
        std::fprintf(stderr, "Shader link error:\n%s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Shader::use() const { glUseProgram(programID); }

void Shader::setMat4(const char* name, const float* mat) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name), 1, GL_FALSE, mat);
}
void Shader::setVec2(const char* name, float x, float y) const {
    glUniform2f(glGetUniformLocation(programID, name), x, y);
}
void Shader::setVec3(const char* name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(programID, name), x, y, z);
}
void Shader::setVec4(const char* name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(programID, name), x, y, z, w);
}
void Shader::setFloat(const char* name, float val) const {
    glUniform1f(glGetUniformLocation(programID, name), val);
}
void Shader::setInt(const char* name, int val) const {
    glUniform1i(glGetUniformLocation(programID, name), val);
}

Shader::~Shader() {
    if (programID) glDeleteProgram(programID);
}
