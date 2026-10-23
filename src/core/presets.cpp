// ============================================================================
//  Universe Sandbox 3D - scene presets (implementation)
//
//  Distances in AU, masses in solar masses, G = 4*PI^2 (years).
//  Orbits start slightly off-exact-circular for visual life.
// ============================================================================
#include "core/presets.h"
#include <cmath>
#include <random>

namespace us3d {

namespace {

std::mt19937& rng() { static std::mt19937 g(9042u); return g; }

// Build a planet orbiting `sun` at radius r with a small inclination.
void addPlanet(System& s, const Body& sun, float r, float mass, float radius,
               Vec3 color, float bandFreq = 0.0f, float phase = 0.0f,
               float incDeg = 0.0f) {
    const float G = s.config().G;
    const float v = std::sqrt(G * sun.mass / r) * (1.0f + (float)std::sin(phase) * 0.01f);

    const float inc = incDeg * 0.0174532925f;
    const float c = std::cos(phase), sn = std::sin(phase);

    Body p;
    p.type = (mass < 1e-6f) ? BodyType::Asteroid : BodyType::Planet;
    p.name = s.makeName("planet");
    p.pos = Vec3(r * c, r * sn * std::sin(inc), r * sn * std::cos(inc));
    p.vel = Vec3(-v * sn, v * c * std::sin(inc), v * c * std::cos(inc));
    p.mass = mass;
    p.radius = radius;
    p.color = color;
    p.bandFreq = bandFreq;
    p.roughness = 0.4f + 0.4f * (float)std::fabs(std::sin(phase * 3.1f));
    s.addBody(p);
}

} // namespace

float Presets::circularSpeed(float G, float M, float r) {
    return std::sqrt(G * M / r);
}

void Presets::solarSystem(System& s, int planets) {
    s.clearAll();
    const float G = s.config().G;

    // The Sun.
    Body sun;
    sun.type = BodyType::Star;
    sun.name = "Sol";
    sun.pos = Vec3::Zero();
    // Give the star a tiny counter-velocity so the barycenter stays put.
    sun.vel = Vec3::Zero();
    sun.mass = 1.0f;
    sun.radius = 0.09f;
    sun.color = Vec3(1.0f, 0.85f, 0.55f);
    const int sunId = s.addBody(sun);
    const Body* sunPtr = s.body(sunId);

    struct P { float r; float mass; float rad; float hue; float band; };
    const P table[] = {
        //   r(AU)   mass(Msun)  radius  hue   band
        { 0.39f, 1.66e-7f, 0.016f, 0.07f, 0.0f },   // Mercury-ish
        { 0.72f, 2.45e-6f, 0.024f, 0.10f, 0.0f },   // Venus-ish
        { 1.00f, 3.00e-6f, 0.026f, 0.55f, 0.0f },   // Earth-ish
        { 1.52f, 3.23e-7f, 0.020f, 0.02f, 0.0f },   // Mars-ish
        { 2.60f, 9.54e-4f, 0.052f, 0.09f, 6.0f },   // Jupiter-ish
        { 4.10f, 2.86e-4f, 0.046f, 0.13f, 4.0f },   // Saturn-ish
        { 5.80f, 4.37e-5f, 0.036f, 0.52f, 2.0f },   // Uranus-ish
        { 7.20f, 5.15e-5f, 0.035f, 0.60f, 2.0f },   // Neptune-ish
    };

    for (int i = 0; i < planets && i < (int)(sizeof(table) / sizeof(table[0])); ++i) {
        const P& p = table[i];
        const float phase = (float)i * 2.39996f; // golden angle spread
        addPlanet(s, *sunPtr, p.r, p.mass, p.rad,
                  hsl2rgb(p.hue, 0.55f, 0.55f), p.band, phase,
                  (float)((i * 37) % 9) * 0.4f);
    }

    // A moon around the most massive planet (stays inside its Hill sphere).
    if (planets >= 2) {
        int best = -1;
        float bestMass = -1.0f;
        for (int id = 0; id < (int)s.bodies().size(); ++id) {
            const Body* b = s.body(id);
            if (!b || b->isStar()) continue;
            if (b->mass > bestMass) { bestMass = b->mass; best = id; }
        }
        if (best >= 0) {
            const Body* b = s.body(best);
            Body moon;
            moon.type = BodyType::Moon;
            moon.name = s.makeName("planet") + " moon";
            const float r = 0.06f;
            const float v = std::sqrt(G * b->mass / r); // circular
            const float ph = 1.1f;
            moon.pos = b->pos + Vec3(r * std::cos(ph), 0, r * std::sin(ph));
            moon.vel = b->vel + Vec3(-v * std::sin(ph), 0, v * std::cos(ph));
            moon.mass = 3.7e-8f;
            moon.radius = 0.010f;
            moon.color = Vec3(0.62f, 0.60f, 0.55f);
            s.addBody(moon);
        }
    }
}

void Presets::binary(System& s) {
    s.clearAll();
    const float G = s.config().G;

    // Two equal-mass stars separated by d, on circular orbits about the barycenter.
    const float m = 0.5f;
    const float d = 0.6f;                 // separation (AU)
    const float vRel = std::sqrt(G * (2.0f * m) / d);
    const float v = vRel * 0.5f;          // each star's speed

    Body a, b;
    a.type = b.type = BodyType::Star;
    a.name = "Alpha"; b.name = "Beta";
    a.pos = Vec3(-d / 2, 0, 0); b.pos = Vec3(d / 2, 0, 0);
    a.vel = Vec3(0, -v, 0);  b.vel = Vec3(0, v, 0);
    a.mass = b.mass = m;
    a.radius = 0.07f; b.radius = 0.06f;
    a.color = Vec3(1.0f, 0.8f, 0.5f);
    b.color = Vec3(0.65f, 0.75f, 1.0f);
    s.addBody(a);
    s.addBody(b);

    // A couple of wide planets orbiting the combined mass (barycenter).
    for (int i = 0; i < 3; ++i) {
        const float r = 2.2f + i * 1.3f;
        const float vC = std::sqrt(G * (2.0f * m) / r);
        const float ph = (float)i * 2.1f;
        Body p;
        p.type = BodyType::Planet;
        p.name = s.makeName("planet");
        p.pos = Vec3(r * std::cos(ph), 0, r * std::sin(ph));
        p.vel = Vec3(-vC * std::sin(ph), 0, vC * std::cos(ph));
        p.mass = 5e-6f + 5e-6f * i;
        p.radius = 0.024f + 0.004f * i;
        p.color = hsl2rgb(0.55f + 0.08f * i, 0.5f, 0.55f);
        s.addBody(p);
    }
}

void Presets::asteroidBelt(System& s, int asteroids) {
    s.clearAll();
    const float G = s.config().G;

    Body sun;
    sun.type = BodyType::Star;
    sun.name = "Core";
    sun.pos = Vec3::Zero();
    sun.mass = 1.0f;
    sun.radius = 0.09f;
    sun.color = Vec3(1.0f, 0.85f, 0.55f);
    const int sunId = s.addBody(sun);
    const Body* sunPtr = s.body(sunId);

    std::uniform_real_distribution<float> dr(2.2f, 3.4f);
    std::uniform_real_distribution<float> dph(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> dinc(-4.0f, 4.0f);
    std::uniform_real_distribution<float> dm(4e-9f, 9e-8f);
    std::uniform_real_distribution<float> hue(0.0f, 1.0f);

    for (int i = 0; i < asteroids; ++i) {
        const float r = dr(rng());
        const float ph = dph(rng());
        const float inc = dinc(rng()) * 0.0174532925f;
        const float v = std::sqrt(G * sunPtr->mass / r) * (0.98f + 0.04f * (float)std::sin(i));
        const float c = std::cos(ph), sn = std::sin(ph);
        Body a;
        a.type = BodyType::Asteroid;
        a.name = "Asteroid-" + std::to_string(i + 1);
        a.pos = Vec3(r * c, r * sn * std::sin(inc), r * sn * std::cos(inc));
        a.vel = Vec3(-v * sn, v * c * std::sin(inc), v * c * std::cos(inc));
        a.mass = dm(rng());
        a.radius = 0.006f + 0.008f * (float)std::fabs(std::sin(i * 1.7f));
        a.color = hsl2rgb(hue(rng()), 0.25f, 0.45f);
        s.addBody(a);
    }
}

void Presets::starAndMoon(System& s) {
    s.clearAll();
    const float G = s.config().G;

    Body sun;
    sun.type = BodyType::Star;
    sun.name = "Giant";
    sun.pos = Vec3::Zero();
    sun.mass = 1.0f;
    sun.radius = 0.12f;
    sun.color = Vec3(1.0f, 0.6f, 0.35f);
    const int sunId = s.addBody(sun);
    const Body* sunPtr = s.body(sunId);

    addPlanet(s, *sunPtr, 1.4f, 9.5e-4f, 0.05f, hsl2rgb(0.08f, 0.5f, 0.55f), 5.0f, 0.0f);

    // Grab that planet and put a big moon on it (circular, inside Hill sphere).
    for (int id = 0; id < (int)s.bodies().size(); ++id) {
        const Body* b = s.body(id);
        if (!b || b->isStar()) continue;
        Body moon;
        moon.type = BodyType::Moon;
        moon.name = s.makeName("planet") + " moon";
        const float r = 0.08f;
        const float v = std::sqrt(G * b->mass / r); // circular
        moon.pos = b->pos + Vec3(r, 0, 0);
        moon.vel = b->vel + Vec3(0, v, 0);
        moon.mass = 6e-7f;
        moon.radius = 0.014f;
        moon.color = Vec3(0.7f, 0.68f, 0.62f);
        s.addBody(moon);
        break;
    }
}

void Presets::randomUniverse(System& s) {
    s.clearAll();

    Body sun;
    sun.type = BodyType::Star;
    sun.name = s.makeName("star");
    sun.pos = Vec3::Zero();
    sun.mass = 0.4f + 1.6f * (float)std::rand() / RAND_MAX;
    sun.radius = 0.06f + 0.1f * (float)std::rand() / RAND_MAX;
    sun.color = hsl2rgb(0.08f + 0.06f * (float)std::rand() / RAND_MAX, 0.6f, 0.6f);
    const int sunId = s.addBody(sun);
    const Body* sunPtr = s.body(sunId);

    const int n = 3 + std::rand() % 8;
    for (int i = 0; i < n; ++i) {
        const float r = 0.5f + 1.6f * (float)i / (n - 1) + 0.3f * (float)std::rand() / RAND_MAX;
        const float mass = 1e-7f * (1.0f + (float)std::rand() / RAND_MAX * 3000.0f);
        const float rad = 0.014f + 0.05f * (float)std::rand() / RAND_MAX;
        const float hue = (float)std::rand() / RAND_MAX;
        const float band = (float)std::rand() / RAND_MAX < 0.4f ? (2.0f + 5.0f * (float)std::rand() / RAND_MAX) : 0.0f;
        addPlanet(s, *sunPtr, r, mass, rad, hsl2rgb(hue, 0.5f, 0.55f), band,
                  (float)i * 1.9f, (float)std::rand() / RAND_MAX * 6.0f);
    }
}

} // namespace us3d
