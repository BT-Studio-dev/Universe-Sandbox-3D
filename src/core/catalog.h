// ============================================================================
//  Universe Sandbox 3D - Add-panel catalog + display/physics formatting
// ============================================================================
#pragma once

#include "core/body.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace us3d {

// ---------------------------------------------------------------------------
//  Add-panel catalog
// ---------------------------------------------------------------------------
struct CatalogItem {
    enum Kind { Body, Scenario } kind;
    const char* name;
    BodyType type;       // for Body
    int scenario;        // for Scenario (App::loadPreset index)

    float massLo, massHi;   // solar masses (random in range when lo < hi)
    float dispRLo, dispRHi; // display radius AU
    float realR;            // "physical" radius AU (info-panel maths)
    float hue, sat, lum;    // albedo colour (random hue when sat < 0)
    float bandFreq;         // 0 = none, -1 = 35% chance
    int   surface;          // 0 rock, 1 water, 2 ice, 3 air
    float tempC;
    float distLo, distHi;   // orbital distance AU from target
};

inline const CatalogItem& catalogBody(int i) {
    static const CatalogItem items[] = {
        { CatalogItem::Body, "Sun",          BodyType::Star, 0,
          1.0f, 1.0f, 0.09f, 0.09f, 0.00465f, 0.10f, 0.85f, 0.62f, 0.0f,
          0, 5516.0f, 3.0f, 3.0f },
        { CatalogItem::Body, "Mercury",      BodyType::Planet, 0,
          1.66e-7f, 1.66e-7f, 0.016f, 0.016f, 1.63e-5f, 0.08f, 0.30f, 0.52f, 0.0f,
          0, -173.0f, 0.39f, 0.39f },
        { CatalogItem::Body, "Venus",        BodyType::Planet, 0,
          2.45e-6f, 2.45e-6f, 0.024f, 0.024f, 4.05e-5f, 0.10f, 0.55f, 0.62f, 0.0f,
          3, 464.0f, 0.72f, 0.72f },
        { CatalogItem::Body, "Earth",        BodyType::Planet, 0,
          3.0e-6f, 3.0e-6f, 0.026f, 0.026f, 4.26e-5f, 0.55f, 0.65f, 0.52f, 0.0f,
          1, 15.0f, 1.00f, 1.00f },
        { CatalogItem::Body, "Mars",         BodyType::Planet, 0,
          3.23e-7f, 3.23e-7f, 0.020f, 0.020f, 2.27e-5f, 0.02f, 0.60f, 0.48f, 0.0f,
          0, -65.0f, 1.52f, 1.52f },
        { CatalogItem::Body, "Jupiter",      BodyType::Planet, 0,
          9.54e-4f, 9.54e-4f, 0.052f, 0.052f, 4.78e-4f, 0.12f, 0.45f, 0.56f, 5.0f,
          0, -110.0f, 2.60f, 2.60f },
        { CatalogItem::Body, "Saturn",       BodyType::Planet, 0,
          2.86e-4f, 2.86e-4f, 0.046f, 0.046f, 4.03e-4f, 0.13f, 0.42f, 0.60f, 3.0f,
          0, -140.0f, 4.10f, 4.10f },
        { CatalogItem::Body, "Uranus",       BodyType::Planet, 0,
          4.37e-5f, 4.37e-5f, 0.036f, 0.036f, 1.71e-4f, 0.52f, 0.35f, 0.62f, 1.5f,
          0, -195.0f, 5.80f, 5.80f },
        { CatalogItem::Body, "Neptune",      BodyType::Planet, 0,
          5.15e-5f, 5.15e-5f, 0.035f, 0.035f, 1.66e-4f, 0.62f, 0.55f, 0.50f, 1.5f,
          0, -200.0f, 7.20f, 7.20f },
        { CatalogItem::Body, "Pluto",        BodyType::Asteroid, 0,
          6.5e-9f, 6.5e-9f, 0.010f, 0.010f, 7.9e-6f, 0.08f, 0.30f, 0.55f, 0.0f,
          0, -230.0f, 8.6f, 8.6f },
        { CatalogItem::Body, "Random Moon",  BodyType::Asteroid, 0,
          2.0e-8f, 8.0e-8f, 0.008f, 0.014f, 1.16e-5f, 0.0f, -1.0f, 0.55f, 0.0f,
          0, -20.0f, 0.15f, 0.4f },
        { CatalogItem::Body, "Random Planet",BodyType::Planet, 0,
          0.3e-6f, 6.0e-6f, 0.010f, 0.032f, 4.0e-5f, 0.0f, -1.0f, 0.55f, -1.0f,
          0, 0.0f, 0.5f, 1.5f },
        { CatalogItem::Body, "Random Star",  BodyType::Star, 0,
          0.05f, 1.4f, 0.04f, 0.10f, 0.00465f, 0.10f, 0.80f, 0.60f, 0.0f,
          0, 0.0f, 3.0f, 8.0f },
        { CatalogItem::Body, "Proto A",      BodyType::Asteroid, 0,
          1.0e-8f, 1.0e-8f, 0.006f, 0.006f, 5.0e-6f, 0.08f, 0.25f, 0.50f, 0.0f,
          0, -100.0f, 1.0f, 1.0f },
        { CatalogItem::Body, "Proto B",      BodyType::Asteroid, 0,
          3.0e-8f, 3.0e-8f, 0.008f, 0.008f, 8.0e-6f, 0.06f, 0.25f, 0.48f, 0.0f,
          0, -120.0f, 1.6f, 1.6f },
        { CatalogItem::Body, "Proto C",      BodyType::Asteroid, 0,
          8.0e-8f, 8.0e-8f, 0.010f, 0.010f, 1.2e-5f, 0.05f, 0.25f, 0.46f, 0.0f,
          0, -140.0f, 2.2f, 2.2f },
    };
    return items[i];
}

inline int catalogBodyCount() {
    return 16;
}

inline const CatalogItem& catalogScenario(int i) {
    struct Sc { const char* name; int scenario; };
    static const Sc names[] = {
        { "Solar System", 0 },
        { "Binary Stars", 1 },
        { "Asteroid Belt", 2 },
        { "Star + Moon", 3 },
        { "Random Universe", 4 },
    };
    static CatalogItem items[5];
    static bool inited = false;
    if (!inited) {
        for (int i = 0; i < 5; ++i) {
            items[i].kind = CatalogItem::Scenario;
            items[i].name = names[i].name;
            items[i].type = BodyType::Star;
            items[i].scenario = names[i].scenario;
        }
        inited = true;
    }
    return items[i];
}

inline int catalogScenarioCount() { return 5; }

// ---------------------------------------------------------------------------
//  Formatting helpers
// ---------------------------------------------------------------------------
inline std::string fmtNum(float v, int dec) {
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.*f", dec, v);
    return buf;
}

inline std::string fmtSci(float v) {
    if (v == 0.0f) return "0";
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.3ge", v);
    return buf;
}

// Simulation time (years, epoch 2000-01-01 00:00) -> "YYYY-MM-DD HH:MM".
inline void formatSimDate(float years, char* out, size_t n) {
    const float daysF = years * 365.25f;
    long long days = (long long)std::floor(daysF);
    float frac = daysF - (float)days;
    int hh = (int)(frac * 24.0f);
    int mm = (int)((frac * 24.0f - (float)hh) * 60.0f);
    if (mm < 0) mm = 0;

    // Hinnant's civil-from-days (proleptic Gregorian), epoch 1970-01-01.
    long long z = days + 10957 + 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365u;
    unsigned doy = doe - (365u * yoe + yoe / 4 - yoe / 100);
    unsigned mp  = (5 * doy + 2) / 153;
    unsigned d   = doy - (153 * mp + 2) / 5 + 1;
    unsigned m   = mp + (mp < 10 ? 3 : -9);
    unsigned y   = yoe + (unsigned)(era * 400) + (m <= 2 ? 1 : 0);

    std::snprintf(out, n, "%04u-%02u-%02u %02d:%02d", y, m, d, hh, mm);
}

// Time speed (years per simulated second) -> "38.5 minutes / s" style.
inline std::string speedLabel(float yearsPerSec, bool paused) {
    if (paused) return "paused";
    const float a = std::fabs(yearsPerSec);
    struct Unit { float per; const char* name; };
    static const Unit units[] = {
        { 1.0f, "years / s" },
        { 365.25f, "days / s" },
        { 8766.0f, "hours / s" },
        { 525960.0f, "minutes / s" },
        { 31557600.0f, "seconds / s" },
    };
    for (const Unit& u : units) {
        if (a * u.per >= 10.0f) {
            char buf[48];
            std::snprintf(buf, sizeof buf, "%.1f %s", yearsPerSec * u.per, u.name);
            return buf;
        }
    }
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.1f %s", yearsPerSec * 31557600.0f, "seconds / s");
    return buf;
}

// Blackbody approximation (Tanner Helland).
inline Vec3 kelvin2rgb(float kelvin) {
    const float t = kelvin / 100.0f;
    float r, g, b;
    if (t <= 66.0f) r = 255.0f;
    else r = 329.698727446f * std::pow(t - 60.0f, -0.1332047592f);
    if (t <= 66.0f) g = 99.4708025861f * std::log(t) - 161.1195681661f;
    else g = 288.1221695283f * std::pow(t - 60.0f, -0.0755148492f);
    if (t >= 66.0f) b = 255.0f;
    else if (t <= 19.0f) b = 0.0f;
    else b = 138.5177312231f * std::log(t - 10.0f) - 305.0447927307f;
    return Vec3(clampf(r, 0.0f, 255.0f) / 255.0f,
                clampf(g, 0.0f, 255.0f) / 255.0f,
                clampf(b, 0.0f, 255.0f) / 255.0f);
}

// Spectral class (OBAFGKM) from effective temperature in Kelvin.
inline std::string spectralType(float tempK) {
    struct Band { const char* cls; float minK; float maxK; };
    static const Band bands[] = {
        { "O", 30000.0f, 1.0e9f },
        { "B", 10000.0f, 30000.0f },
        { "A",  7500.0f, 10000.0f },
        { "F",  6000.0f,  7500.0f },
        { "G",  5200.0f,  6000.0f },
        { "K",  3700.0f,  5200.0f },
        { "M",  2400.0f,  3700.0f },
    };
    int idx = 6;
    for (int i = 0; i < 7; ++i) {
        if (tempK >= bands[i].minK) { idx = i; break; }
    }
    const float span = bands[idx].maxK - bands[idx].minK;
    int sub = (span > 1.0f)
        ? (int)clampf((tempK - bands[idx].minK) / span * 9.0f + 0.5f, 0.0f, 9.0f)
        : 5;
    char buf[16];
    std::snprintf(buf, sizeof buf, "%s%d V", bands[idx].cls, sub);
    return buf;
}

// ---------------------------------------------------------------------------
//  Two-body orbital elements (for the Info panel)
// ---------------------------------------------------------------------------
struct OrbitInfo {
    bool bound = false;
    float distAU = 0.0f;
    float speedKms = 0.0f;
    float periodYr = 0.0f;
    float aAU = 0.0f;
    float ecc = 0.0f;
};

inline OrbitInfo orbitalInfo(const Body& b, const Body& primary, float G) {
    OrbitInfo o;
    const Vec3 r = b.pos - primary.pos;
    const Vec3 v = b.vel - primary.vel;
    const float rlen = std::max(length(r), 1e-6f);
    const float vlen = length(v);
    o.distAU = rlen;
    o.speedKms = vlen * 4.74047f;   // 1 AU/yr = 4.74047 km/s

    const float mu = G * (b.mass + primary.mass);
    const float inv = 2.0f / rlen - (vlen * vlen) / mu;
    if (inv > 1e-8f) {
        o.bound = true;
        o.aAU = 1.0f / inv;
        o.periodYr = 6.2831853f * std::sqrt(o.aAU * o.aAU * o.aAU / mu);
    }
    const Vec3 h = cross(r, v);
    const Vec3 e = (cross(v, h) / mu) - (r / rlen);
    o.ecc = length(e);
    return o;
}

} // namespace us3d
