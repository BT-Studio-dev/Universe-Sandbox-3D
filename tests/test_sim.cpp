// ============================================================================
//  Universe Sandbox 3D - headless simulation tests
//
//  Platform-independent: exercises the physics core (N-body integrator,
//  collisions, reversibility, save/load) without any graphics.
//  Build: see CMakeLists.txt (target: test_sim).
// ============================================================================
#include "core/presets.h"
#include "core/savestate.h"
#include "core/system.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

using namespace us3d;

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (cond) {                                                            \
            std::printf("  [PASS] %s\n", msg);                                 \
        } else {                                                               \
            std::printf("  [FAIL] %s\n", msg);                                 \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static void testOrbitStability() {
    std::printf("[test] orbit stability (solar system, 5 years)\n");
    System s;
    Presets::solarSystem(s, 8);

    // Find the body closest to 1.0 AU (the Earth-ish planet).
    int earth = -1;
    const int sun = s.primaryBody();
    for (const Body& b : s.bodies()) {
        if (b.isStar()) continue;
        const float d = length(b.pos);
        if (std::fabs(d - 1.0f) < 0.25f) earth = b.id;
    }
    CHECK(earth >= 0, "found 1 AU planet");

    const float e0 = length(s.body(earth)->vel);
    s.advance(5.0f);
    CHECK(s.body(earth) != nullptr, "planet still exists after 5 years");
    if (s.body(earth)) {
        const float d = length(s.body(earth)->pos - s.body(sun)->pos);
        const float e = length(s.body(earth)->vel);
        std::printf("         earth-ish: r=%.3f AU (want ~1.0), v=%.3f (started %.3f)\n",
                    d, e, e0);
        CHECK(d > 0.90f && d < 1.10f, "semi-major axis within 10% after 5 years");
        CHECK(std::fabs(e - e0) < 0.15f, "speed within 15% after 5 years");
    }
}

static void testEnergy() {
    std::printf("[test] energy conservation (1 year, leapfrog)\n");
    System s;
    Presets::solarSystem(s, 8);
    const float e0 = s.totalEnergy();
    s.advance(1.0f);
    const float e1 = s.totalEnergy();
    const float drift = std::fabs(e1 - e0) / std::fabs(e0);
    std::printf("         E0=%+.4f E1=%+.4f drift=%.4f%%\n", e0, e1, drift * 100.0f);
    CHECK(drift < 0.01f, "total energy drift < 1% over 1 year");
}

static void testCollision() {
    std::printf("[test] collision merges conserve mass + momentum\n");
    System s;
    s.clearAll();

    Body a; a.name = "A"; a.mass = 3.0f; a.radius = 0.02f;
    a.pos = Vec3(-0.01f, 0, 0); a.vel = Vec3(0.1f, 0, 0);
    Body b; b.name = "B"; b.mass = 1.0f; b.radius = 0.02f;
    b.pos = Vec3(0.01f, 0, 0); b.vel = Vec3(-0.1f, 0, 0);
    const int ia = s.addBody(a);
    const int ib = s.addBody(b);
    (void)ia; (void)ib;

    const float mass0 = 4.0f;
    const float px0 = 3.0f * 0.1f + 1.0f * (-0.1f); // = 0.2
    for (int i = 0; i < 40 && s.bodyCount() > 1; ++i) s.step(0.001f);

    CHECK(s.bodyCount() == 1, "bodies merged into one");
    if (s.bodyCount() == 1) {
        const Body& m = s.bodies()[0];
        std::printf("         merged: m=%.3f vx=%.4f (want m=4, vx=%.4f)\n",
                    m.mass, m.vel.x, px0 / mass0);
        CHECK(std::fabs(m.mass - mass0) < 1e-3f, "mass conserved");
        CHECK(std::fabs(m.vel.x - px0 / mass0) < 1e-3f, "momentum conserved");
        CHECK(std::fabs(m.pos.x) < 0.05f, "merged near contact point");
    }
}

static void testReversibility() {
    std::printf("[test] time reversal (200 steps forward, 200 back)\n");
    System s;
    Presets::solarSystem(s, 8);
    s.advance(0.5f); // let the system settle a bit

    std::vector<Vec3> p0, v0;
    for (const Body& b : s.bodies()) { p0.push_back(b.pos); v0.push_back(b.vel); }

    s.advance(200 * s.config().dt);
    s.advance(-200 * s.config().dt);

    float maxErr = 0.0f;
    for (size_t i = 0; i < p0.size(); ++i) {
        const Body& b = s.bodies()[i];
        maxErr = std::max(maxErr, length(b.pos - p0[i]));
        maxErr = std::max(maxErr, length(b.vel - v0[i]));
    }
    std::printf("         max deviation after round trip: %.3e AU\n", maxErr);
    CHECK(maxErr < 1e-3f, "state recovered within 1e-3 AU");
}

static void testSaveLoad() {
    std::printf("[test] save / load round trip\n");
    System s;
    Presets::binary(s);
    s.advance(0.37f);
    const float t0 = s.time();

    const char* path = "us3d_test_save.txt";
    const SaveResult rs = saveSystem(s, path);
    CHECK(rs.ok, "save succeeded");

    System s2;
    const SaveResult rl = loadSystem(s2, path);
    CHECK(rl.ok, "load succeeded");
    CHECK(s2.bodyCount() == s.bodyCount(), "body count matches");
    CHECK(std::fabs(s2.time() - t0) < 1e-5f, "sim time matches");

    float maxErr = 0.0f;
    bool namesOk = true;
    for (size_t i = 0; i < s.bodies().size() && i < s2.bodies().size(); ++i) {
        maxErr = std::max(maxErr, length(s2.bodies()[i].pos - s.bodies()[i].pos));
        maxErr = std::max(maxErr, length(s2.bodies()[i].vel - s.bodies()[i].vel));
        if (s2.bodies()[i].name != s.bodies()[i].name) namesOk = false;
    }
    std::printf("         max state error: %.3e\n", maxErr);
    CHECK(maxErr < 1e-5f, "positions/velocities round-trip");
    CHECK(namesOk, "names round-trip (including spaces)");
    std::remove(path);
}

static void testV1Compat() {
    std::printf("[test] legacy v1 save file still loads\n");
    const char* path = "us3d_test_v1.txt";
    {
        std::ofstream f(path);
        f << "US3D SAVE 1\n"
          << "time 1.25\n"
          << "reversed 0\n"
          << "body \"Old Sun\" 0 0.5 0.0 0.0 0.0 0.0 0.0 0.0 1.0 0.09 1.0 0.9 0.5 0\n"
          << "body \"Old Planet\" 1 1.0 0.0 0.0 0.0 6.283185 0.0 3e-6 0.02 0.3 0.5 0.9 0\n";
    }
    System s;
    const SaveResult r = loadSystem(s, path);
    CHECK(r.ok, "v1 file accepted");
    CHECK(s.bodyCount() == 2, "two bodies loaded");
    CHECK(std::fabs(s.time() - 1.25f) < 1e-5f, "time loaded");
    const Body* planet = nullptr;
    for (const Body& b : s.bodies())
        if (b.name == "Old Planet") planet = &b;
    CHECK(planet != nullptr, "spaced name round-trips (quoted token)");
    if (planet) {
        CHECK(planet->surface == 0, "missing v2 fields default to rock");
        CHECK(std::fabs(planet->vel.y - 6.283185f) < 1e-4f, "v1 velocity parsed");
    }
    std::remove(path);
}

static void testAsteroidBelt() {
    std::printf("[test] asteroid belt (400 bodies) stays sane over 2 years\n");
    System s;
    Presets::asteroidBelt(s, 400);
    const int n0 = s.bodyCount();
    const float e0 = s.totalEnergy();
    s.advance(2.0f);
    const int n1 = s.bodyCount();
    CHECK(n1 >= n0 - 60, "most asteroids survive (fewer than ~15% lost to merges)");
    std::printf("         bodies: %d -> %d\n", n0, n1);
    const float e1 = s.totalEnergy();
    const float drift = std::fabs(e1 - e0) / std::fabs(e0);
    CHECK(drift < 0.5f, "energy doesn't explode (drift < 50%)");
}

int main() {
    std::printf("Universe Sandbox 3D - headless simulation tests\n");
    std::printf("================================================\n");
    testOrbitStability();
    testEnergy();
    testCollision();
    testReversibility();
    testSaveLoad();
    testV1Compat();
    testAsteroidBelt();
    std::printf("================================================\n");
    if (g_failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", g_failures);
    return 1;
}
