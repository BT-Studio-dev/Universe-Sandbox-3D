// ============================================================================
//  Universe Sandbox 3D - N-body gravity system
//
//  * Kick-Drift-Kick (leapfrog / velocity-Verlet) symplectic integrator:
//    stable for orbits, reversible (run time backwards with dt < 0).
//  * O(N^2) pairwise gravity with mild softening (fine for N <= ~2000).
//  * Merges on contact: momentum + mass conserving.
//  * Per-body motion trails for rendering.
// ============================================================================
#pragma once

#include "core/body.h"
#include <vector>
#include <string>

namespace us3d {

struct SystemConfig {
    float G = 39.47841760435743f;   // 4 * PI^2  (solar masses / AU^3 / yr^2)
    float dt = 1.0f / 512.0f;       // base timestep, years
    float softening = 1.0f / 1e4f;  // AU, avoids close-encounter blowups
    int   maxStepsPerFrame = 512;
    int   trailLength = 512;        // stored points per body
    int   trailStride = 4;          // record a trail point every N steps
};

class System {
public:
    explicit System(const SystemConfig& cfg = SystemConfig()) : cfg_(cfg) {}

    // -- bodies -------------------------------------------------------------
    int addBody(Body b);
    void removeBody(int id);                     // marks body dead (finalized next step)
    const std::vector<Body>& bodies() const { return bodies_; }
    std::vector<Body>& bodies() { return bodies_; }
    Body* body(int id) {
        if (id < 0 || id >= (int)bodies_.size() || bodies_[id].dead) return nullptr;
        return &bodies_[id];
    }
    const Body* body(int id) const {
        if (id < 0 || id >= (int)bodies_.size() || bodies_[id].dead) return nullptr;
        return &bodies_[id];
    }

    // Compact index of the body with stable uid `uid`, or -1.
    int bodyByUid(int uid) const {
        for (const Body& b : bodies_)
            if (!b.dead && b.uid == uid) return b.id;
        return -1;
    }
    const Body* bodyOfUid(int uid) const {
        const int id = bodyByUid(uid);
        return (id >= 0) ? body(id) : nullptr;
    }
    int bodyCount() const {
        int n = 0;
        for (const Body& b : bodies_) if (!b.dead) ++n;
        return n;
    }

    // The most massive living body (the "sun" for lighting/picking bias).
    int primaryBody() const;

    // -- time ---------------------------------------------------------------
    float time() const        { return time_; }
    void  setTime(float t)    { time_ = t; }
    bool  reversed() const    { return reversed_; }
    void  setReversed(bool r) { reversed_ = r; }

    // Advance by `targetYears` of simulation time using cfg_.dt substeps
    // (sign handles direction). Returns number of substeps actually taken.
    int advance(float targetYears);

    // One explicit substep of cfg_.dt (used by tests / fine control).
    void step(float dt);

    // Remove dead bodies, compact ids, clear their trails.
    void finalize();

    // Total energy (kinetic + potential), for diagnostics/tests.
    float totalEnergy() const;

    // -- trails -------------------------------------------------------------
    // Trails[i] holds up to trailLength recent positions for body id i
    // (oldest first). Rebuilt live by the integrator.
    const std::vector<std::vector<Vec3>>& trails() const { return trails_; }

    void clearTrails();

    // -- presets / state ----------------------------------------------------
    void clearAll();
    std::string makeName(const char* style);  // random generated name

    // Recent event messages (merges / deletes). takeEvents() consumes them.
    std::vector<std::string> takeEvents() {
        std::vector<std::string> out;
        out.swap(events_);
        return out;
    }

    SystemConfig& config() { return cfg_; }

private:
    void computeAccelerations(std::vector<Vec3>& out);
    void recordTrails();
    void handleCollisions();

    SystemConfig cfg_;
    std::vector<Body> bodies_;
    std::vector<std::vector<Vec3>> trails_;
    std::vector<std::string> events_;
    float time_ = 0.0f;
    bool  reversed_ = false;
    int   nextId_ = 0;
    int   stepCount_ = 0;
};

} // namespace us3d
