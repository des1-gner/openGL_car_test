#pragma once
#include <cmath>
#include <cstring>

static constexpr float PI = 3.14159265358979323846f;
static constexpr float DEG2RAD = PI / 180.0f;
static constexpr float RAD2DEG = 180.0f / PI;

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
    float length() const { return std::sqrt(x*x + y*y); }
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3(float s) : x(s), y(s), z(s) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator*(const Vec3& o) const { return {x*o.x, y*o.y, z*o.z}; }
    Vec3 operator/(float s) const { return {x/s, y/s, z/s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vec3& operator*=(float s) { x*=s; y*=s; z*=s; return *this; }

    float length() const { return std::sqrt(x*x + y*y + z*z); }
    float lengthSq() const { return x*x + y*y + z*z; }
    Vec3 normalized() const {
        float l = length();
        if (l < 1e-7f) return {0,0,0};
        return {x/l, y/l, z/l};
    }
    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
    }
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }
    static Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - n * (2.0f * v.dot(n));
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return {s*v.x, s*v.y, s*v.z}; }

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};

struct Mat4 {
    float m[16]; // column-major

    Mat4() { identity(); }

    void identity() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    const float* data() const { return m; }
    float& operator()(int row, int col) { return m[col*4+row]; }
    float operator()(int row, int col) const { return m[col*4+row]; }

    static Mat4 perspective(float fovDeg, float aspect, float near, float far) {
        Mat4 r;
        std::memset(r.m, 0, sizeof(r.m));
        float f = 1.0f / std::tan(fovDeg * 0.5f * DEG2RAD);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (far + near) / (near - far);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * far * near) / (near - far);
        return r;
    }

    static Mat4 ortho(float l, float r, float b, float t, float n, float f) {
        Mat4 res;
        std::memset(res.m, 0, sizeof(res.m));
        res.m[0]  = 2.0f / (r - l);
        res.m[5]  = 2.0f / (t - b);
        res.m[10] = -2.0f / (f - n);
        res.m[12] = -(r + l) / (r - l);
        res.m[13] = -(t + b) / (t - b);
        res.m[14] = -(f + n) / (f - n);
        res.m[15] = 1.0f;
        return res;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);
        Mat4 r;
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;  r.m[12] = -s.dot(eye);
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;  r.m[13] = -u.dot(eye);
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z; r.m[14] = f.dot(eye);
        r.m[3] = 0;    r.m[7] = 0;    r.m[11] = 0;    r.m[15] = 1.0f;
        return r;
    }

    static Mat4 translate(float x, float y, float z) {
        Mat4 r; r.m[12] = x; r.m[13] = y; r.m[14] = z; return r;
    }
    static Mat4 translate(const Vec3& v) { return translate(v.x, v.y, v.z); }

    static Mat4 scale(float x, float y, float z) {
        Mat4 r; r.m[0] = x; r.m[5] = y; r.m[10] = z; return r;
    }
    static Mat4 scale(float s) { return scale(s, s, s); }

    static Mat4 rotateY(float angleDeg) {
        Mat4 r;
        float rad = angleDeg * DEG2RAD;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[8]  = s;
        r.m[2] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotateX(float angleDeg) {
        Mat4 r;
        float rad = angleDeg * DEG2RAD;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c;  r.m[9]  = -s;
        r.m[6] = s;  r.m[10] = c;
        return r;
    }

    static Mat4 rotateZ(float angleDeg) {
        Mat4 r;
        float rad = angleDeg * DEG2RAD;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[4] = -s;
        r.m[1] = s;  r.m[5] = c;
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int c = 0; c < 4; c++)
            for (int row = 0; row < 4; row++) {
                r.m[c*4+row] = 0;
                for (int k = 0; k < 4; k++)
                    r.m[c*4+row] += m[k*4+row] * o.m[c*4+k];
            }
        return r;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12]*v.w,
            m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13]*v.w,
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
        };
    }
};

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float smoothstep(float edge0, float edge1, float x) {
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float randf() {
    return (float)rand() / (float)RAND_MAX;
}

inline float randf(float lo, float hi) {
    return lo + randf() * (hi - lo);
}
