// ============================================================================
//  Universe Sandbox 3D - minimal math library (Vec3 / Mat4)
//  Column-major Mat4, layout-compatible with OpenGL uniformMatrix4fv.
//  Header-only, dependency-free, C++11.
// ============================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace us3d {

// ---------------------------------------------------------------------------
//  Vec3
// ---------------------------------------------------------------------------
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    static Vec3 Zero()   { return Vec3(0, 0, 0); }
    static Vec3 UnitX()  { return Vec3(1, 0, 0); }
    static Vec3 UnitY()  { return Vec3(0, 1, 0); }
    static Vec3 UnitZ()  { return Vec3(0, 0, 1); }

    Vec3  operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3  operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3  operator-() const              { return Vec3(-x, -y, -z); }
    Vec3  operator*(float s) const       { return Vec3(x * s, y * s, z * s); }
    Vec3  operator/(float s) const       { return Vec3(x / s, y / s, z / s); }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s)     { x *= s;   y *= s;   z *= s;   return *this; }
    bool  operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3  cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}
inline float lengthSq(const Vec3& v) { return dot(v, v); }
inline float length(const Vec3& v)   { return std::sqrt(std::max(0.0f, dot(v, v))); }

inline Vec3 normalize(const Vec3& v) {
    const float l = length(v);
    return (l > 1e-12f) ? (v / l) : Vec3(0, 0, 1);
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }

// ---------------------------------------------------------------------------
//  Mat4 (column-major: m[col][row])
// ---------------------------------------------------------------------------
struct Mat4 {
    float m[4][4];

    Mat4() { setIdentity(); }

    void setIdentity() {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                m[c][r] = (c == r) ? 1.0f : 0.0f;
    }

    static Mat4 Identity() { return Mat4(); }

    static Mat4 Translate(const Vec3& t) {
        Mat4 r; r.m[3][0] = t.x; r.m[3][1] = t.y; r.m[3][2] = t.z; return r;
    }

    static Mat4 Scale(float s) {
        Mat4 r; r.m[0][0] = s; r.m[1][1] = s; r.m[2][2] = s; return r;
    }

    static Mat4 Scale(const Vec3& s) {
        Mat4 r; r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z; return r;
    }

    // Standard OpenGL-style perspective projection (right-handed).
    static Mat4 Perspective(float fovyRad, float aspect, float zn, float zf) {
        Mat4 r;
        const float f = 1.0f / std::tan(fovyRad * 0.5f);
        const float rangeInv = 1.0f / (zn - zf);
        r.m[0][0] = f / aspect;  r.m[1][0] = 0.0f;  r.m[2][0] = 0.0f;  r.m[3][0] = 0.0f;
        r.m[0][1] = 0.0f;        r.m[1][1] = f;     r.m[2][1] = 0.0f;  r.m[3][1] = 0.0f;
        r.m[0][2] = 0.0f;        r.m[1][2] = 0.0f;  r.m[2][2] = (zf + zn) * rangeInv; r.m[3][2] = 2.0f * zf * zn * rangeInv;
        r.m[0][3] = 0.0f;        r.m[1][3] = 0.0f;  r.m[2][3] = -1.0f; r.m[3][3] = 0.0f;
        return r;
    }

    // Axis-aligned orthographic box (used for the HUD).
    static Mat4 Ortho(float xl, float xr, float yb, float yt, float zn, float zf) {
        Mat4 r;
        r.m[0][0] = 2.0f / (xr - xl);
        r.m[1][1] = 2.0f / (yt - yb);
        r.m[2][2] = -2.0f / (zf - zn);
        r.m[3][0] = -(xr + xl) / (xr - xl);
        r.m[3][1] = -(yt + yb) / (yt - yb);
        r.m[3][2] = -(zf + zn) / (zf - zn);
        return r;
    }

    static Mat4 Multiply(const Mat4& a, const Mat4& b) {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int rr = 0; rr < 4; ++rr) {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k) s += a.m[k][rr] * b.m[c][k];
                r.m[c][rr] = s;
            }
        return r;
    }

    Vec3 TransformPoint(const Vec3& p) const {
        const float w = m[3][0] * p.x + m[3][1] * p.y + m[3][2] * p.z + m[3][3];
        return Vec3(
            (m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[3][0]) / (w != 0 ? w : 1),
            (m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[3][1]) / (w != 0 ? w : 1),
            (m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[3][2]) / (w != 0 ? w : 1));
    }

    Vec3 TransformDir(const Vec3& d) const {
        return Vec3(
            m[0][0] * d.x + m[0][1] * d.y + m[0][2] * d.z,
            m[1][0] * d.x + m[1][1] * d.y + m[1][2] * d.z,
            m[2][0] * d.x + m[2][1] * d.y + m[2][2] * d.z);
    }

    // lookAt: eye looking at target, up. Produces a view matrix (world -> view).
    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        const Vec3 f = normalize(target - eye);
        const Vec3 s = normalize(cross(f, up));
        const Vec3 u = cross(s, f);
        Mat4 r;
        r.m[0][0] =  s.x;  r.m[1][0] =  s.y;  r.m[2][0] =  s.z;  r.m[3][0] = 0;
        r.m[0][1] =  u.x;  r.m[1][1] =  u.y;  r.m[2][1] =  u.z;  r.m[3][1] = 0;
        r.m[0][2] = -f.x;  r.m[1][2] = -f.y;  r.m[2][2] = -f.z;  r.m[3][2] = 0;
        r.m[3][0] = 0;     r.m[3][1] = 0;     r.m[3][2] = 0;     r.m[3][3] = 1;
        // translate by -eye
        r.m[3][0] -= dot(s, eye);
        r.m[3][1] -= dot(u, eye);
        r.m[3][2] += dot(f, eye);
        return r;
    }
};

// ---------------------------------------------------------------------------
//  Small helpers
// ---------------------------------------------------------------------------
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float signf(float v) { return (v > 0) - (v < 0); }
inline float deg2rad(float d) { return d * 0.017453292519943295f; }
inline float rad2deg(float r) { return r * 57.29577951308232f; }

// HSL (h in [0,1), s in [0,1], l in [0,1]) -> RGB in [0,1]
inline Vec3 hsl2rgb(float h, float s, float l) {
    if (s <= 0.0f) return Vec3(l, l, l);
    const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    const float p = 2.0f * l - q;
    auto hue2rgb = [&](float tt) {
        if (tt < 0.0f) tt += 1.0f;
        if (tt > 1.0f) tt -= 1.0f;
        if (tt < 1.0f / 6.0f) return p + (q - p) * 6.0f * tt;
        if (tt < 0.5f)        return q;
        if (tt < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - tt) * 6.0f;
        return p;
    };
    return Vec3(hue2rgb(h + 1.0f / 3.0f), hue2rgb(h), hue2rgb(h - 1.0f / 3.0f));
}

} // namespace us3d
