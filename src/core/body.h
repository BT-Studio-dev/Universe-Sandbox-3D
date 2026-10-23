// ============================================================================
//  Universe Sandbox 3D - celestial body
//
//  Units (simulation):
//    mass   : solar masses
//    length : astronomical units (AU)
//    time   : years  ->  G = 4 * PI^2 in these units
//
//  `radius` is the *display* radius (AU). Real planetary radii are far too
//  small to see at system scale, so bodies carry an exaggerated visual size
//  that also drives collision detection (this is a sandbox, not a paper).
// ============================================================================
#pragma once

#include "core/math.h"
#include <string>

namespace us3d {

enum class BodyType : int {
    Star      = 0,
    Planet    = 1,
    Moon      = 2,
    Asteroid  = 3,
};

struct Body {
    int      id       = -1;      // compact index in System (renumbered on finalize)
    int      uid      = -1;      // stable unique id (never reused)
    BodyType type     = BodyType::Planet;
    std::string name  = "Body";

    Vec3 pos   = Vec3::Zero();   // AU
    Vec3 vel   = Vec3::Zero();   // AU / year
    Vec3 acc   = Vec3::Zero();   // AU / year^2
    float mass  = 0.0f;          // solar masses
    float radius = 0.0f;         // display radius, AU (also collision radius)
    float realRadiusAU = 0.0f;   // "real" radius used for info-panel physics

    Vec3 color = Vec3(0.8f, 0.8f, 0.8f);  // albedo / surface tint
    Vec3 displayColor;                    // effective albedo (colour-by overrides)
    float bandFreq = 0.0f;                // >0 gives banded gas-giant look
    float roughness = 0.5f;               // surface noise amount

    // Surface & thermals (UI: Tools / Info panels)
    int   surface   = 0;    // 0 rock, 1 water, 2 ice, 3 air/clouds
    float temperatureC = 15.0f;  // surface temperature
    float spin       = 0.0f;     // rad / year (visual rotation)

    bool dead = false;  // marked for removal (merged or deleted)

    float age = 0.0f;   // seconds since spawn (drives fade-in, HUD pulse)

    Body() { displayColor = color; }

    bool isStar() const { return type == BodyType::Star; }
};

inline const char* bodyTypeName(BodyType t) {
    switch (t) {
    case BodyType::Star:     return "star";
    case BodyType::Planet:   return "planet";
    case BodyType::Moon:     return "moon";
    case BodyType::Asteroid: return "asteroid";
    }
    return "body";
}

} // namespace us3d
