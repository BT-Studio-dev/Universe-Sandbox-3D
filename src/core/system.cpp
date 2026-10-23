// ============================================================================
//  Universe Sandbox 3D - N-body gravity system (implementation)
// ============================================================================
#include "core/system.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>

namespace us3d {

namespace {

std::mt19937& rng() {
    static std::mt19937 g(1337u);
    return g;
}

} // namespace

int System::addBody(Body b) {
    b.uid = nextId_++;
    b.id  = (int)bodies_.size();
    b.dead = false;
    bodies_.push_back(b);
    trails_.emplace_back();
    if ((int)trails_.back().capacity() < cfg_.trailLength)
        trails_.back().reserve(cfg_.trailLength);
    return b.id;
}

void System::removeBody(int id) {
    if (Body* b = body(id)) {
        b->dead = true;
        char msg[128];
        std::snprintf(msg, sizeof msg, "Deleted %s", b->name.c_str());
        events_.push_back(msg);
    }
}

int System::primaryBody() const {
    int best = -1;
    float bestMass = -1.0f;
    for (const Body& b : bodies_) {
        if (b.dead) continue;
        if (b.mass > bestMass) { bestMass = b.mass; best = b.id; }
    }
    return best;
}

void System::computeAccelerations(std::vector<Vec3>& out) {
    const int n = (int)bodies_.size();
    out.assign(n, Vec3::Zero());
    if (n == 0) return;

    const float eps2 = cfg_.softening * cfg_.softening;
    for (int i = 0; i < n; ++i) {
        const Body& bi = bodies_[i];
        if (bi.dead) continue;
        for (int j = i + 1; j < n; ++j) {
            const Body& bj = bodies_[j];
            if (bj.dead) continue;
            const Vec3 d = bj.pos - bi.pos;
            const float r2 = lengthSq(d) + eps2;
            const float invR = 1.0f / std::sqrt(r2);
            const float f = cfg_.G * invR * invR; // G / r^2, applied along unit dir
            const Vec3 dir = d * invR;            // unit vector i -> j
            out[i] += dir * (f * bj.mass);
            out[j] -= dir * (f * bi.mass);
        }
    }
}

void System::step(float dt) {
    // Kick-Drift-Kick leapfrog. Reversible when dt < 0.
    const int n = (int)bodies_.size();
    if (n == 0) return;

    for (Body& b : bodies_) {
        if (b.dead) continue;
        b.vel += b.acc * (0.5f * dt);
    }
    for (Body& b : bodies_) {
        if (b.dead) continue;
        b.pos += b.vel * dt;
    }
    std::vector<Vec3> acc;
    computeAccelerations(acc);
    for (int i = 0; i < n; ++i) {
        if (bodies_[i].dead) continue;
        bodies_[i].acc = acc[i];
        bodies_[i].vel += acc[i] * (0.5f * dt);
    }

    time_ += dt;
    stepCount_++;
    handleCollisions();
    if (stepCount_ % cfg_.trailStride == 0) recordTrails();
}

int System::advance(float targetYears) {
    if (targetYears == 0.0f || bodies_.empty()) return 0;
    // Substeps take the direction of the request itself.
    const float substep = (targetYears > 0.0f) ? cfg_.dt : -cfg_.dt;
    int n = (int)std::llabs((long long)std::llround(targetYears / substep));
    n = std::max(1, std::min(cfg_.maxStepsPerFrame, n));

    float remaining = targetYears;
    int taken = 0;
    for (int i = 0; i < n; ++i) {
        if (std::fabs(remaining) < 1e-12f) break;
        const float h = (std::fabs(remaining) < std::fabs(substep)) ? remaining : substep;
        step(h);
        remaining -= h;
        ++taken;
    }
    return taken;
}

void System::finalize() {
    if (bodies_.empty()) return;
    bool anyDead = false;
    for (const Body& b : bodies_) anyDead |= b.dead;
    if (!anyDead) return;

    std::vector<Body> kept;
    kept.reserve(bodies_.size());
    std::vector<std::vector<Vec3>> keptTrails;
    keptTrails.reserve(trails_.size());
    for (size_t i = 0; i < bodies_.size(); ++i) {
        if (bodies_[i].dead) continue;
        bodies_[i].id = (int)kept.size();
        kept.push_back(bodies_[i]);
        keptTrails.push_back(std::move(trails_[i]));
    }
    bodies_ = std::move(kept);
    trails_ = std::move(keptTrails);
}

void System::handleCollisions() {
    if (!impactsEnabled_) return;
    const int n = (int)bodies_.size();
    for (int i = 0; i < n; ++i) {
        Body& a = bodies_[i];
        if (a.dead) continue;
        for (int j = i + 1; j < n; ++j) {
            Body& b = bodies_[j];
            if (b.dead) continue;
            const float rr = (a.radius + b.radius) * 0.85f;
            const float d2 = lengthSq(b.pos - a.pos);
            if (d2 >= rr * rr) continue;

            // Merge: the more massive body survives.
            Body* big = &a;
            Body* small = &b;
            if (b.mass > a.mass) { big = &b; small = &a; }

            const float mSum = big->mass + small->mass;
            big->vel = (big->vel * big->mass + small->vel * small->mass) / mSum;
            big->pos = (big->pos * big->mass + small->pos * small->mass) / mSum;
            big->mass = mSum;
            big->radius = std::cbrt(big->radius * big->radius * big->radius +
                                    small->radius * small->radius * small->radius) * 0.92f;
            if (big->type != BodyType::Star && small->type == BodyType::Star)
                big->type = BodyType::Star;

            char msg[160];
            std::snprintf(msg, sizeof msg, "%s + %s -> %s",
                          a.name.c_str(), b.name.c_str(), big->name.c_str());
            events_.push_back(msg);

            small->dead = true;
        }
    }
    if (!events_.empty()) {
        // keep the HUD queue short
        while (events_.size() > 8) events_.erase(events_.begin());
    }
}

void System::recordTrails() {
    const int n = (int)bodies_.size();
    for (int i = 0; i < n; ++i) {
        if (bodies_[i].dead) continue;
        if (bodies_[i].type == BodyType::Star) continue; // no trail for sun
        std::vector<Vec3>& t = trails_[i];
        t.push_back(bodies_[i].pos);
        while ((int)t.size() > cfg_.trailLength) t.erase(t.begin());
    }
}

void System::clearTrails() {
    for (auto& t : trails_) t.clear();
}

float System::kineticEnergy() const {
    float ke = 0.0f;
    for (const Body& b : bodies_) {
        if (b.dead) continue;
        ke += 0.5f * b.mass * lengthSq(b.vel);
    }
    return ke;
}

float System::potentialEnergy() const {
    float pe = 0.0f;
    const float eps2 = cfg_.softening * cfg_.softening;
    const int n = (int)bodies_.size();
    for (int i = 0; i < n; ++i) {
        if (bodies_[i].dead) continue;
        for (int j = i + 1; j < n; ++j) {
            if (bodies_[j].dead) continue;
            const float r = std::sqrt(lengthSq(bodies_[j].pos - bodies_[i].pos) + eps2);
            pe -= cfg_.G * bodies_[i].mass * bodies_[j].mass / r;
        }
    }
    return pe;
}

float System::totalEnergy() const {
    return kineticEnergy() + potentialEnergy();
}

void System::clearAll() {
    bodies_.clear();
    trails_.clear();
    events_.clear();
    time_ = 0.0f;
    reversed_ = false;
    stepCount_ = 0;
    // nextId_ keeps counting; not an issue.
}

std::string System::makeName(const char* style) {
    static const char* syllA[] = { "Kor", "Vel", "Zar", "Tha", "Nyx", "Ora",
                                   "Bel", "Dra", "Fen", "Gor", "Hal", "Ith",
                                   "Jor", "Kel", "Lyr", "Mor", "Nan", "Oph",
                                   "Per", "Qua", "Rho", "Sol", "Tal", "Umb",
                                   "Vex", "Wyn", "Xar", "Yld", "Zef", "Ari" };
    static const char* suffix[] = { "", "-7", "-IX", " Nova", " Minor", " B",
                                    " III", " V", " Prime", "-12" };
    static const char* starN[]  = { "Helios", "Kestrel", "Aurel", "Vantari",
                                    "Solm", "Pyre", "Ignis", "Lumen" };

    std::uniform_int_distribution<size_t> d(0, sizeof(syllA) / sizeof(syllA[0]) - 1);
    std::uniform_int_distribution<size_t> ds(0, sizeof(starN) / sizeof(starN[0]) - 1);

    if (style && std::strcmp(style, "star") == 0) {
        std::string s = starN[ds(rng())];
        char num[16];
        std::uniform_int_distribution<int> dn(2, 97);
        std::snprintf(num, sizeof num, " %d", dn(rng()));
        s += num;
        return s;
    }

    // avoid duplicate names against current population
    for (int attempt = 0; attempt < 32; ++attempt) {
        std::string s = syllA[d(rng())];
        if (attempt > 4) {
            s += suffix[std::uniform_int_distribution<size_t>(1, sizeof(suffix) / sizeof(suffix[0]) - 1)(rng())];
        }
        bool dup = false;
        for (const Body& b : bodies_)
            if (!b.dead && b.name == s) { dup = true; break; }
        if (!dup) return s;
    }
    return "Body-" + std::to_string(nextId_);
}

} // namespace us3d
