// ============================================================================
//  Universe Sandbox 3D - orbit camera (implementation)
// ============================================================================
#include "render/camera.h"

namespace us3d {

void Camera::orbit(float yawRad, float pitchRad) {
    yaw_ += yawRad;
    pitch_ += pitchRad;
    const float lim = 1.55f; // keep away from exact pole
    if (pitch_ > lim) pitch_ = lim;
    if (pitch_ < -lim) pitch_ = -lim;
}

void Camera::zoomBy(float factor) {
    dist_ = clampf(dist_ * factor, kMinDist, kMaxDist);
}

void Camera::pan(const Vec3& camRight, const Vec3& camUp,
                 float dxPixels, float dyPixels) {
    // Pixels -> world units at the target distance.
    const float scale = dist_ * 0.0016f;
    target_ -= camRight * (dxPixels * scale);
    target_ += camUp * (dyPixels * scale);
}

Ray Camera::ray(float ndcX, float ndcY, float aspect, float fovyRad) const {
    const Vec3 pos = position();
    const Vec3 f = normalize(target_ - pos);
    const Vec3 r = normalize(cross(f, up()));
    const Vec3 u = cross(r, f);
    const float halfH = std::tan(fovyRad * 0.5f);
    Ray ray;
    ray.origin = pos;
    ray.dir = normalize(f + r * (ndcX * halfH * aspect) + u * (ndcY * halfH));
    return ray;
}

} // namespace us3d
