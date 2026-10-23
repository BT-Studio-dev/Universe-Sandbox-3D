// ============================================================================
//  Universe Sandbox 3D - renderer
//
//  Owns all GL objects: shader programs, meshes, starfield, dynamic trail
//  buffer. Draws one full frame from (camera, simulation, selection state).
// ============================================================================
#pragma once

#include "core/system.h"
#include "gl/gl33.h"
#include "render/camera.h"
#include <string>

namespace us3d {

// View-panel options applied to a frame.
struct ViewOptions {
    float clouds = 0.0f;        // 0/1 global clouds layer
    float atmos = 1.0f;         // 0/1 atmosphere rim glow
    float heatGlow = 1.0f;      // 0/1 show Heat-Beam glow
    bool  lampOn = false;
    Vec3  lampPos = Vec3::Zero();
    Vec3  lampColor = Vec3(1.0f, 1.0f, 1.0f);
    float lampStrength = 1.0f;
    bool  showDebris = true;    // render small bodies (asteroids)
};

// Small helper: compile + link a 2-stage program.
class Program {
public:
    bool create(const char* vsSrc, const char* fsSrc, const char* label,
                std::string& errOut);
    void destroy();
    void use() const;

    int  uniform(const char* name);
    GLuint handle() const { return prog_; }

private:
    GLuint vs_ = 0, fs_ = 0, prog_ = 0;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init();
    void shutdown();
    const std::string& error() const { return error_; }

    // Draws a full frame.
    void render(const Camera& cam, const System& sim, int selectedId,
                bool showTrails, float timeSec, int w, int h,
                const ViewOptions& vo = ViewOptions());

    // View panel: field of view in degrees.
    void setFovDeg(float deg) { fov_ = deg2rad(clampf(deg, 25.0f, 100.0f)); }

    // Font atlas texture (RGBA8, luminance in .r) for the HUD.
    GLuint fontTexture() const { return fontTex_; }
    // Atlas metrics: columns, rows, glyph cell UV advance.
    void fontMetrics(int& cols, int& rows) const { cols = 16; rows = 6; }

    float fovRad() const { return fov_; }

private:
    void buildSphereMesh();
    void buildStarfield();
    void buildGlowMesh();
    bool buildFontTexture();
    void drawBodies(const Camera& cam, const System& sim, int selectedId,
                    float timeSec, const Vec3& lightPos, const Vec3& lightColor,
                    const ViewOptions& vo);
    void drawTrails(const System& sim);
    void drawGlow(const System& sim, float timeSec);
    void drawSelectionRing(const Camera& cam, const System& sim, int selectedId);

    // programs
    Program bodyProg_, skyProg_, starsProg_, lineProg_, glowProg_;

    // meshes
    GLuint sphereVao_ = 0, sphereIbo_ = 0;
    unsigned sphereIndexCount_ = 0;
    GLuint starVao_ = 0; unsigned starCount_ = 0;
    GLuint glowVao_ = 0;

    // dynamic trails
    GLuint lineVao_ = 0, lineVbo_ = 0;
    unsigned lineCapacity_ = 0;

    GLuint fontTex_ = 0;

    float fov_ = 0.87f;    // ~50 degrees
    float aspect_ = 16.0f / 9.0f;
    Mat4 view_ = Mat4::Identity();
    Mat4 proj_ = Mat4::Identity();
    std::string error_;

    // per-frame selection ring scratch
    std::vector<float> ringScratch_;
};

} // namespace us3d
