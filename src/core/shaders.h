// ============================================================================
//  Universe Sandbox 3D - GLSL 3.30 shaders (embedded)
// ============================================================================
#pragma once

namespace us3d {
namespace shaders {

// ---------------------------------------------------------------------------
//  Body (planets, moons, asteroids, stars)
// ---------------------------------------------------------------------------
extern const char* body_vs;

extern const char* body_fs;

// ---------------------------------------------------------------------------
//  Distant background sky (inverted sphere): nebula + hashed stars
// ---------------------------------------------------------------------------
extern const char* sky_vs;

extern const char* sky_fs;

// ---------------------------------------------------------------------------
//  Near starfield (GL points, twinkling)
// ---------------------------------------------------------------------------
extern const char* stars_vs;

extern const char* stars_fs;

// ---------------------------------------------------------------------------
//  Lines (trails): per-vertex position + color
// ---------------------------------------------------------------------------
extern const char* line_vs;

extern const char* line_fs;

// ---------------------------------------------------------------------------
//  UI (screen-space quads, optional font texture)
// ---------------------------------------------------------------------------
extern const char* ui_vs;

extern const char* ui_fs;

// ---------------------------------------------------------------------------
//  Glow billboard (sun corona), additive
// ---------------------------------------------------------------------------
extern const char* glow_vs;

extern const char* glow_fs;

// ---------------------------------------------------------------------------
//  GUI disc icons (Add-panel thumbnails, body avatars)
//  Screen-space quad; per-vertex colour + per-icon style params.
// ---------------------------------------------------------------------------
extern const char* icon_vs;

extern const char* icon_fs;

} // namespace shaders
} // namespace us3d
