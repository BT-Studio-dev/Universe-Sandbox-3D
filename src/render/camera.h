// ============================================================================
//  Universe Sandbox 3D - orbit camera
//
//  The camera orbits a focus point (target). Left-drag rotates, right/middle
//  drag pans, wheel zooms. When "follow" is on and a body is focused, the
//  focus tracks that body every frame.
// ============================================================================
#pragma once

#include "core/math.h"

namespace us3d {

struct Ray {
    Vec3 origin = Vec3::Zero();
    Vec3 dir    = Vec3::UnitZ();
};

// Intersection of `ray` with a sphere (center, radius).
inline bool raySphere(const Ray& ray, const Vec3& center, float radius, float& tOut) {
    const Vec3 oc = ray.origin - center;
    const float b = dot(oc, ray.dir);
    const float c = lengthSq(oc) - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.0f) return false;
    const float sq = std::sqrt(disc);
    float t = -b - sq;
    if (t < 0.0f) t = -b + sq;
    if (t < 0.0f) return false;
    tOut = t;
    return true;
}

class Camera {
public:
    // Rotation / zoom / pan in pixel-independent amounts.
    void orbit(float yawRad, float pitchRad);
    void zoomBy(float factor);
    void pan(const Vec3& camRight, const Vec3& camUp, float dxPixels, float dyPixels);

    // Update focus (call once per frame with the current position of the
    // followed body, or Vec3::Zero() / no-focus to leave it alone).
    void setFocusPoint(const Vec3& p) { target_ = p; }
    void setFocusEnabled(bool on)     { follow_ = on; }
    bool focusEnabled() const         { return follow_; }

    void setDistance(float d) { dist_ = clampf(d, kMinDist, kMaxDist); }
    float distance() const    { return dist_; }

    void setPose(float yaw, float pitch, float dist) {
        yaw_ = yaw;
        pitch_ = clampf(pitch, -1.45f, 1.45f);
        dist_ = clampf(dist, kMinDist, kMaxDist);
    }

    Vec3 position() const {
        const float cy = std::cos(yaw_), sy = std::sin(yaw_);
        const float cp = std::cos(pitch_), sp = std::sin(pitch_);
        return target_ + Vec3(cy * cp, sp, sy * cp) * dist_;
    }
    Vec3 target() const { return target_; }
    Vec3 up() const { return Vec3::UnitY(); }

    Vec3 right() const {
        const Vec3 f = normalize(target_ - position());
        return normalize(cross(f, up()));
    }

    Mat4 view() const { return Mat4::LookAt(position(), target_, up()); }

    // Ray from a normalized device coord (-1..1) through the camera.
    Ray ray(float ndcX, float ndcY, float aspect, float fovyRad) const;

    float yaw() const   { return yaw_; }
    float pitch() const { return pitch_; }

    static constexpr float kMinDist = 0.02f;
    static constexpr float kMaxDist = 40.0f;

private:
    Vec3  target_ = Vec3::Zero();
    float yaw_    = 0.6f;
    float pitch_  = 0.42f;
    float dist_   = 2.4f;
    bool  follow_ = false;
};

} // namespace us3d
