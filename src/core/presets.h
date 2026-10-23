// ============================================================================
//  Universe Sandbox 3D - scene presets
// ============================================================================
#pragma once

#include "core/system.h"

namespace us3d {

class Presets {
public:
    // A central star plus a ring of planets on near-circular coplanar orbits.
    static void solarSystem(System& s, int planets = 8);

    // Two stars orbiting a common barycenter, with a few planets.
    static void binary(System& s);

    // A star and a dense belt of asteroids that will slowly collide/merge.
    static void asteroidBelt(System& s, int asteroids = 400);

    // One star and a single planet + moon (tidal-ish demo).
    static void starAndMoon(System& s);

    // Random star with a random number of planets - "new universe".
    static void randomUniverse(System& s);

    // Circular orbital speed around a mass M at distance r.
    static float circularSpeed(float G, float M, float r);
};

} // namespace us3d
