// ============================================================================
//  Universe Sandbox 3D - UI panels (implementation)
// ============================================================================
#include "ui/panels.h"

#include "app.h"
#include "ui/gui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>

namespace us3d {

namespace {

// Physical constants for info-panel display values.
const double kMsunKg   = 1.989e30;
const double kAUm      = 1.496e11;
const double kEarthRM  = 6.371e6;
const double kGsi      = 6.674e-11;
const double kMsunMearth = 332946.0;

std::string trunc(const std::string& s, int n) {
    return s.size() <= (size_t)n ? s : s.substr(0, n);
}

// Trim trailing zeros from a formatted number.
std::string trimNum(std::string s) {
    if (s.find('.') == std::string::npos) return s;
    while (!s.empty() && (s.back() == '0')) s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}

// ---------------------------------------------------------------------------
//  Tool definitions (Tools panel)
// ---------------------------------------------------------------------------
const ToolDef kToolsWorld[] = {
    { 0, "Terraform", "Cycle the surface:", "rock / water / ice / air", 0.3f, 0.65f, 0.9f },
    { 1, "Heat Beam", "Click: +800 C to the", "focused world", 1.0f, 0.55f, 0.15f },
    { 2, "Torque", "Click: spin the world.", "Right-click reverses", 0.95f, 0.8f, 0.3f },
    { 3, "Detonate", "Break the focused", "world into fragments", 0.95f, 0.3f, 0.2f },
};
const ToolDef kToolsLaunch[] = {
    { 4, "Slingshot", "Fling the world", "outward from its star", 0.3f, 0.9f, 0.4f },
    { 5, "Brake", "Cut 85% of the", "world's orbital speed", 0.3f, 0.85f, 0.75f },
};
const ToolDef kToolsArrange[] = {
    { 6, "Circularize", "Set a perfectly", "circular orbit", 0.7f, 0.5f, 0.95f },
    { 7, "Stop Orbit", "Freeze the world's", "motion relative to its star", 0.6f, 0.65f, 0.7f },
};
const ToolDef kToolsCamera[] = {
    { 8, "Follow", "Lock the camera onto", "the focused world", 0.85f, 0.9f, 1.0f },
    { 9, "Reset View", "Return the camera", "to its starting pose", 0.85f, 0.9f, 1.0f },
};

// "All" tab order: 3x3 grid.
const int kToolsAllOrder[9] = { 0, 1, 2, 3, 4, 6, 9, 8, 5 };

const ToolDef* findTool(int id) {
    static const ToolDef* const groups[4] = {
        kToolsWorld, kToolsLaunch, kToolsArrange, kToolsCamera };
    static const int groupSize[4] = { 4, 2, 2, 2 };
    for (int g = 0; g < 4; ++g)
        for (int i = 0; i < groupSize[g]; ++i)
            if (groups[g][i].id == id) return &groups[g][i];
    return nullptr;
}

} // namespace

void UIPanels::drawToolCard(Gui& gui, App& app, float x, float y, float w, float h,
                  const ToolDef& t, bool compact) {
    const bool hov = gui.mouseInside(x, y, w, h);
    gui.rect(x, y, w, h, hov ? 0.14f : 0.105f, hov ? 0.16f : 0.12f,
             hov ? 0.22f : 0.165f, 0.95f);
    gui.line(x, y + h, x + w, y + h, 0.2f, 0.23f, 0.31f, 0.7f, 1.0f);

    const float cx = x + (compact ? 24.0f : 30.0f);
    const float cy = y + 26.0f;
    gui.disc(cx, cy, compact ? 13.0f : 16.0f, t.r, t.g, t.b, 0.0f, 0.0f);
    gui.text(x + (compact ? 44.0f : 56.0f), y + 10.0f, t.title, 1.5f,
             0.92f, 0.95f, 1.0f);
    if (compact) {
        gui.text(x + 44.0f, y + 34.0f, trunc(t.d1, 20), 1.1f, 0.55f, 0.6f, 0.72f);
    } else {
        gui.text(x + 56.0f, y + 38.0f, trunc(t.d1, 24), 1.15f, 0.55f, 0.6f, 0.72f);
        gui.text(x + 56.0f, y + 54.0f, trunc(t.d2, 24), 1.15f, 0.55f, 0.6f, 0.72f);
    }

    if (gui.clickedAt(x, y, w, h))
        app.uiTool(t.id);
    else if (gui.hadRelease() && gui.releaseButton() == 2 &&
             gui.mouseInside(x, y, w, h))
        app.uiToolRight(t.id);
}

// ---------------------------------------------------------------------------
//  Info panel value computation
// ---------------------------------------------------------------------------
const Body* otherPrimary(const System& sim, const Body& self) {
    const Body* best = nullptr;
    for (const Body& b : sim.bodies()) {
        if (b.dead || &b == &self) continue;
        if (!best || b.mass > best->mass) best = &b;
    }
    return best;
}

InfoValues UIPanels::computeInfo(const App& app, const Body& b) {
    InfoValues v;
    const double mKg = b.mass * kMsunKg;
    // Physical radius: use the stored one, else estimate from mass
    // (Earth-radius * (M/Mearth)^(1/3)).
    double realRAU = b.realRadiusAU;
    if (realRAU <= 0.0f) {
        const float mEarth = b.mass * 332946.0f;
        realRAU = 4.26e-5 * std::cbrt(std::max(mEarth, 1e-9f));
    }
    const double rM  = realRAU * kAUm;
    const double vCm = (4.0 * 3.14159265358979 / 3.0) * std::pow(rM * 100.0, 3.0);

    v.mass = b.isStar()
        ? (fmtNum(b.mass, 2) + " Msun")
        : (trimNum(fmtNum(b.mass * (float)kMsunMearth, 1)) + " Mearth");
    v.radius = trimNum(fmtNum((float)(rM / kEarthRM), 1)) + " Earth radii";
    v.density = trimNum(fmtNum((float)(mKg / vCm / 1000.0), 3)) + " g/cm3";
    v.temperature = fmtNum(b.temperatureC, 1) + " C";

    const double g = kGsi * mKg / (rM * rM);
    v.gravity = trimNum(fmtNum((float)g, 2)) + " m/s2";
    if (b.isStar()) {
        v.pressure = "4.5e13 Pa";
    } else if (g < 0.5) {
        v.pressure = "0 Pa";
    } else {
        v.pressure = fmtSci((float)(101325.0 * g / 9.81)) + " Pa";
    }
    v.luminosity = b.isStar()
        ? (trimNum(fmtNum(b.mass * b.mass, 2)) + " lum") : "0.0 lum";
    v.speed = fmtNum(length(b.vel) * 4.74047f, 2) + " km/s";
    v.rotation = (b.spin > 1e-4f)
        ? (trimNum(fmtNum(6.2831853f / b.spin * 8766.0f, 1)) + " h") : "-";

    const Body* prim = otherPrimary(app.sim_, b);
    if (prim && b.mass > prim->mass) prim = nullptr; // we ARE the primary
    if (prim) {
        v.aroundName = prim->name;
        const OrbitInfo oi = orbitalInfo(b, *prim, app.sim_.config().G);
        v.hasOrbit = oi.bound;
        v.distAU = oi.distAU;
        v.speedRelKms = oi.speedKms;
        v.aAU = oi.aAU;
        v.ecc = oi.ecc;
        v.periodYr = oi.periodYr;
        v.orbital = oi.bound ? (trimNum(fmtNum(oi.periodYr, 2)) + " yr") : "-";
    }

    v.age = (b.isStar()
        ? fmtNum(1.0f + (float)(b.uid % 1000) / 1000.0f * 9.0f, 3)
        : "4.56") + " Gyr";
    v.stage = b.isStar() ? "Main sequence" : "Planet";
    v.spectral = b.isStar() ? spectralType(b.temperatureC + 273.0f) : "-";

    int frag = 0;
    for (const Body& o : app.sim_.bodies())
        if (!o.dead && o.type == BodyType::Asteroid) ++frag;
    v.fragments = std::to_string(frag);
    return v;
}

// ===========================================================================
//  Time bar (bottom-left)
// ===========================================================================
RectF UIPanels::timeBar(int w, int h) {
    (void)w;
    return { 12.0f, (float)h - 40.0f, 402.0f, 30.0f };
}

void UIPanels::drawTimeBar(Gui& gui, App& app) {
    const int w = gui.width(), h = gui.height();
    const RectF r = UIPanels::timeBar(w, h);

    gui.rect(r.x, r.y, r.w, r.h, 0.07f, 0.08f, 0.1f, 0.92f);
    gui.line(r.x, r.y + r.h, r.x + r.w, r.y + r.h, 0.2f, 0.24f, 0.32f, 0.9f, 1.0f);

    // pause / play
    const float by = r.y + 3.5f;
    const bool pauseHit = gui.button(r.x + 3.0f, by, 23.0f, 23.0f, "",
        app.paused_ ? Gui::IPlay : Gui::IPause, 1.6f,
        0.13f, 0.15f, 0.2f);
    if (pauseHit) {
        app.paused_ = !app.paused_;
        app.pushFlash(app.paused_ ? "Paused" : "Resumed");
    }
    // reverse / forward
    const bool revHit = gui.button(r.x + 30.0f, by, 23.0f, 23.0f, "",
        Gui::IRewind, 1.6f, 0.13f, 0.15f, 0.2f,
        app.sim_.reversed() ? 1.0f : 0.7f, 1.0f, 0.6f);
    if (revHit) app.sim_.setReversed(true);
    const bool fwdHit = gui.button(r.x + 57.0f, by, 23.0f, 23.0f, "",
        Gui::IForward, 1.6f, 0.13f, 0.15f, 0.2f,
        app.sim_.reversed() ? 0.7f : 1.0f, 1.0f, 0.6f);
    if (fwdHit) app.sim_.setReversed(false);

    char date[32];
    formatSimDate(app.sim_.time(), date, sizeof date);
    const float yrsPerSec = (app.timeSpeed_ / 12.0f) *
        (app.sim_.reversed() ? -1.0f : 1.0f);
    const std::string spd = speedLabel(yrsPerSec, app.paused_);

    gui.text(r.x + 88.0f, r.y + 9.0f, date, 1.4f, 0.92f, 0.95f, 1.0f);
    gui.text(r.x + 88.0f + Gui::measure(date, 1.4f) + 12.0f, r.y + 9.0f,
             spd, 1.4f, 0.55f, 0.75f, 1.0f);
}

// ===========================================================================
//  Bottom bar
// ===========================================================================
void UIPanels::drawBottomBar(Gui& gui, App& app) {
    const int w = gui.width(), h = gui.height();
    const UIState& ui = app.ui_;
    UIState& u = app.ui_;
    const float by = (float)h - 40.0f;

    auto barBtn = [&](float x, float bw, const char* label, Gui::IconKind ik,
                      bool open) {
        const float r = open ? 0.17f : 0.13f;
        const float g = open ? 0.38f : 0.15f;
        const float b = open ? 0.80f : 0.19f;
        return gui.button(x, by, bw, 28.0f, label, ik, 1.6f, r, g, b,
                          open ? 1.0f : 0.85f, open ? 1.0f : 0.88f,
                          open ? 1.0f : 0.95f);
    };

    const float cx = w * 0.5f - 132.0f;
    if (barBtn(cx, 84.0f, "Add", Gui::IPlus, ui.addOpen)) {
        u.addOpen = !u.addOpen;
    }
    if (barBtn(cx + 90.0f, 84.0f, "Tools", Gui::IWrench, ui.toolsOpen)) {
        u.toolsOpen = !u.toolsOpen;
    }
    if (barBtn(cx + 180.0f, 84.0f, "Find", Gui::ISearch, ui.findOpen)) {
        u.findOpen = !u.findOpen;
    }
    if (barBtn((float)w - 158.0f, 76.0f, "Sim", Gui::IGear, ui.simOpen)) {
        u.simOpen = !u.simOpen;
    }
    if (barBtn((float)w - 80.0f, 76.0f, "View", Gui::ISliders, ui.viewOpen)) {
        u.viewOpen = !u.viewOpen;
    }
}

// ===========================================================================
//  Add panel
// ===========================================================================
static RectF addRect() { return { 14.0f, 52.0f, 318.0f, 470.0f }; }

void UIPanels::drawAddPanel(Gui& gui, App& app) {
    const RectF R = addRect();
    UIState& u = app.ui_;

    gui.panel(R.x, R.y, R.w, R.h);
    gui.text(R.x + 12.0f, R.y + 9.0f, "Add", 2.0f, 0.95f, 0.97f, 1.0f);
    if (gui.button(R.x + R.w - 24.0f, R.y + 7.0f, 16.0f, 16.0f, "", Gui::IClose, 1.2f,
                   0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        u.addOpen = false;

    // tabs
    const char* tabs[4] = { "All", "Moons", "Saved", "Custom" };
    const float tw[4] = { 48.0f, 52.0f, 52.0f, 58.0f };
    float tx = R.x + 12.0f;
    for (int i = 0; i < 4; ++i) {
        if (gui.pill(tx, R.y + 36.0f, tw[i], 20.0f, tabs[i], u.addTab == i))
            u.addTab = i;
        tx += tw[i] + 6.0f;
    }

    // search
    gui.inputBox(R.x + 12.0f, R.y + 64.0f, R.w - 24.0f, 22.0f,
                 u.addQuery, u.addEditActive, "Search objects");

    // grid
    const float gx0 = R.x + 12.0f;
    const float gy0 = R.y + 92.0f;
    const float cw = (R.w - 24.0f - 3 * 8.0f) / 4.0f;
    const float chh = 68.0f;

    auto drawCell = [&](int gridPos, int spawnIndex, bool isScenario,
                        const CatalogItem& it) {
        const int col = gridPos % 4, rowI = gridPos / 4;
        const float x = gx0 + col * (cw + 8.0f);
        const float y = gy0 + rowI * (chh + 7.0f);

        const bool hov = gui.mouseInside(x, y, cw, chh);
        gui.rect(x, y, cw, chh, hov ? 0.15f : 0.105f, hov ? 0.17f : 0.115f,
                 hov ? 0.23f : 0.155f, 0.95f);

        // thumbnail colour
        float hr = 0.7f, hg = 0.7f, hb = 0.7f;
        float band = 0.0f;
        float emissive = 0.0f;
        if (isScenario) {
            hr = 0.5f; hg = 0.6f; hb = 0.8f;
        } else {
            if (it.sat < 0.0f) {
                const float hu = (it.type == BodyType::Star) ? 0.10f
                              : (it.type == BodyType::Asteroid) ? 0.55f : 0.45f;
                hr = hg = hb = 0;
                const Vec3 c = hsl2rgb(hu, it.type == BodyType::Star ? 0.8f : 0.5f, 0.55f);
                hr = c.x; hg = c.y; hb = c.z;
                if (it.type == BodyType::Star) emissive = 1.0f;
            } else {
                const Vec3 c = hsl2rgb(it.hue, it.sat, it.lum);
                hr = c.x; hg = c.y; hb = c.z;
                if (it.type == BodyType::Star) emissive = 1.0f;
            }
            band = (it.bandFreq > 0.0f) ? it.bandFreq : 0.0f;
        }
        gui.disc(x + cw * 0.5f, y + 27.0f, 19.0f, hr, hg, hb, band, emissive);
        gui.text(x + 4.0f, y + 52.0f, trunc(it.name, 10), 1.15f,
                 hov ? 1.0f : 0.8f, hov ? 1.0f : 0.84f, hov ? 1.0f : 0.9f);

        if (gui.clickedAt(x, y, cw, chh))
            app.uiSpawnCatalog(spawnIndex, isScenario);
    };

    auto matches = [&u](const char* name) {
        if (u.addQuery.empty()) return true;
        std::string n = name, q = u.addQuery;
        for (char& c : n) c = (char)std::tolower((unsigned char)c);
        for (char& c : q) c = (char)std::tolower((unsigned char)c);
        return n.find(q) != std::string::npos;
    };

    if (u.addTab == 0) {                       // All
        int pos = 0;
        for (int i = 0; i < catalogBodyCount(); ++i) {
            const CatalogItem& it = catalogBody(i);
            if (!matches(it.name)) continue;
            drawCell(pos++, i, false, it);
        }
    } else if (u.addTab == 1) {                // Moons
        static const int moonIdx[5] = { 10, 9, 13, 14, 15 };
        int pos = 0;
        for (int k = 0; k < 5; ++k) {
            const CatalogItem& it = catalogBody(moonIdx[k]);
            if (!matches(it.name)) continue;
            drawCell(pos++, moonIdx[k], false, it);
        }
    } else if (u.addTab == 2) {                // Saved scenarios
        int pos = 0;
        for (int i = 0; i < catalogScenarioCount(); ++i) {
            const CatalogItem& it = catalogScenario(i);
            if (!matches(it.name)) continue;
            drawCell(pos++, i, true, it);
        }
    } else {                                   // Custom
        gui.text(gx0 + 8.0f, gy0 + 20.0f, "No custom objects yet.", 1.4f,
                 0.6f, 0.66f, 0.78f);
        gui.text(gx0 + 8.0f, gy0 + 42.0f, "Spawn bodies with the Add panel", 1.3f,
                 0.45f, 0.5f, 0.6f);
        gui.text(gx0 + 8.0f, gy0 + 60.0f, "and shape them with the Tools.", 1.3f,
                 0.45f, 0.5f, 0.6f);
    }

    // bottom row: Target / Binary / Auto orbit / Repeat / Cancel
    const float fy = R.y + R.h - 64.0f;
    float fx = R.x + 12.0f;
    if (gui.button(fx, fy, 76.0f, 24.0f, "Target", Gui::INone, 1.35f,
                   0.12f, 0.14f, 0.18f)) {
        app.pushFlash("Adding around the focused world");
    }
    fx += 82.0f;
    if (gui.button(fx, fy, 72.0f, 24.0f, "Binary", Gui::INone, 1.35f,
                   u.addBinary ? 0.17f : 0.12f, u.addBinary ? 0.38f : 0.14f,
                   u.addBinary ? 0.80f : 0.18f,
                   u.addBinary ? 1.0f : 0.7f, u.addBinary ? 1.0f : 0.75f,
                   u.addBinary ? 1.0f : 0.85f)) {
        u.addBinary = !u.addBinary;
    }
    fx += 78.0f;
    if (gui.button(fx, fy, 90.0f, 24.0f, "Auto orbit", Gui::INone, 1.35f,
                   u.addAutoOrbit ? 0.17f : 0.12f, u.addAutoOrbit ? 0.38f : 0.14f,
                   u.addAutoOrbit ? 0.80f : 0.18f,
                   u.addAutoOrbit ? 1.0f : 0.7f, u.addAutoOrbit ? 1.0f : 0.75f,
                   u.addAutoOrbit ? 1.0f : 0.85f)) {
        u.addAutoOrbit = !u.addAutoOrbit;
    }
    const float fy2 = fy + 30.0f;
    fx = R.x + 12.0f;
    if (gui.button(fx, fy2, 76.0f, 24.0f, "Repeat", Gui::INone, 1.35f,
                   u.addRepeat ? 0.17f : 0.12f, u.addRepeat ? 0.38f : 0.14f,
                   u.addRepeat ? 0.80f : 0.18f,
                   u.addRepeat ? 1.0f : 0.7f, u.addRepeat ? 1.0f : 0.75f,
                   u.addRepeat ? 1.0f : 0.85f)) {
        u.addRepeat = !u.addRepeat;
    }
    fx += 82.0f;
    if (gui.button(fx, fy2, 76.0f, 24.0f, "Cancel", Gui::INone, 1.35f)) {
        u.addOpen = false;
    }
}

// ===========================================================================
//  Tools panel
// ===========================================================================
static RectF toolsRect(int w, int h) {
    return { (float)w * 0.5f - 195.0f, (float)h - 40.0f - 252.0f, 390.0f, 252.0f };
}

void UIPanels::drawToolsPanel(Gui& gui, App& app) {
    const int w = gui.width(), h = gui.height();
    const RectF R = toolsRect(w, h);
    UIState& u = app.ui_;

    gui.panel(R.x, R.y, R.w, R.h);
    gui.text(R.x + 12.0f, R.y + 8.0f, "Tools", 2.0f, 0.95f, 0.97f, 1.0f);
    if (gui.button(R.x + R.w - 24.0f, R.y + 6.0f, 16.0f, 16.0f, "", Gui::IClose,
                   1.2f, 0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        u.toolsOpen = false;
    gui.text(R.x + 12.0f, R.y + 30.0f, "Tools that act on the focused world",
             1.25f, 0.5f, 0.56f, 0.7f);

    const char* tabs[5] = { "All", "World", "Launch", "Arrange", "Camera" };
    const float tw[5] = { 48.0f, 52.0f, 56.0f, 60.0f, 60.0f };
    float tx = R.x + 12.0f;
    for (int i = 0; i < 5; ++i) {
        if (gui.pill(tx, R.y + 50.0f, tw[i], 20.0f, tabs[i], u.toolsTab == i))
            u.toolsTab = i;
        tx += tw[i] + 6.0f;
    }

    const float gy = R.y + 80.0f;
    if (u.toolsTab == 0) {
        // 3 x 3 compact grid
        const float cw = (R.w - 24.0f - 2 * 8.0f) / 3.0f;
        for (int k = 0; k < 9; ++k) {
            const ToolDef* t = findTool(kToolsAllOrder[k]);
            const float x = R.x + 12.0f + (k % 3) * (cw + 8.0f);
            const float y = gy + (k / 3) * 56.0f;
            drawToolCard(gui, app, x, y, cw, 52.0f, *t, true);
        }
    } else {
        const ToolDef* tools[4] = { 0, 0, 0, 0 };
        int n = 0;
        if (u.toolsTab == 1) { for (auto& t : kToolsWorld)   tools[n++] = &t; }
        if (u.toolsTab == 2) { for (auto& t : kToolsLaunch)  tools[n++] = &t; }
        if (u.toolsTab == 3) { for (auto& t : kToolsArrange) tools[n++] = &t; }
        if (u.toolsTab == 4) { for (auto& t : kToolsCamera)  tools[n++] = &t; }
        const float cw = (R.w - 24.0f - 8.0f) / 2.0f;
        for (int k = 0; k < n; ++k) {
            const float x = R.x + 12.0f + (k % 2) * (cw + 8.0f);
            const float y = gy + (k / 2) * 78.0f;
            drawToolCard(gui, app, x, y, cw, 72.0f, *tools[k], false);
        }
    }
}

// ===========================================================================
//  Find panel
// ===========================================================================
RectF UIPanels::findPanel(int w, int h) {
    (void)h;
    return { (float)w * 0.5f - 210.0f, 52.0f, 420.0f, 396.0f };
}

void UIPanels::drawFindPanel(Gui& gui, App& app) {
    const int w = gui.width();
    const RectF R = UIPanels::findPanel(w, gui.height());
    UIState& u = app.ui_;
    const System& sim = app.sim_;

    gui.panel(R.x, R.y, R.w, R.h);
    gui.text(R.x + 12.0f, R.y + 8.0f, "Find", 2.0f, 0.95f, 0.97f, 1.0f);
    if (gui.button(R.x + R.w - 24.0f, R.y + 6.0f, 16.0f, 16.0f, "", Gui::IClose,
                   1.2f, 0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        u.findOpen = false;

    gui.inputBox(R.x + 12.0f, R.y + 34.0f, R.w - 24.0f, 22.0f,
                 u.findQuery, u.findEditActive, "Search bodies...");

    const char* tabs[5] = { "All", "Stars", "Planets", "Moons", "Small" };
    const float tw[5] = { 42.0f, 50.0f, 62.0f, 50.0f, 54.0f };
    float tx = R.x + 12.0f;
    for (int i = 0; i < 5; ++i) {
        if (gui.pill(tx, R.y + 64.0f, tw[i], 20.0f, tabs[i], u.findTab == i))
            u.findTab = i;
        tx += tw[i] + 6.0f;
    }

    // gather + filter + sort by mass desc
    std::vector<const Body*> list;
    for (const Body& b : sim.bodies()) {
        if (b.dead) continue;
        switch (u.findTab) {
        case 1: if (!b.isStar()) continue; break;
        case 2: if (b.type != BodyType::Planet) continue; break;
        case 3: if (b.type != BodyType::Asteroid) continue; break;
        case 4: if (b.type != BodyType::Asteroid && b.mass > 1e-7f) continue; break;
        default: break;
        }
        if (!u.findQuery.empty()) {
            std::string name = b.name, q = u.findQuery;
            for (char& c : name) c = (char)std::tolower((unsigned char)c);
            for (char& c : q) c = (char)std::tolower((unsigned char)c);
            if (name.find(q) == std::string::npos) continue;
        }
        list.push_back(&b);
    }
    std::sort(list.begin(), list.end(),
              [](const Body* a, const Body* b) { return a->mass > b->mass; });

    const int nRows = 9;
    const float ry0 = R.y + 94.0f;
    const float rh = 28.0f;
    const int fid = app.focId();
    const Vec3 ref = (fid >= 0) ? sim.body(fid)->pos : Vec3::Zero();

    for (int k = 0; k < nRows; ++k) {
        const int idx = u.findScroll + k;
        const float y = ry0 + k * rh;
        if (idx >= (int)list.size()) break;
        const Body* b = list[idx];

        const bool hov = gui.mouseInside(R.x + 10.0f, y, R.w - 20.0f, rh);
        const bool sel = b->uid == app.selectedUid_;
        if (gui.row(R.x + 10.0f, y, R.w - 20.0f, rh, hov))
            app.uiSelectUid(b->uid);

        if (sel) gui.rect(R.x + 10.0f, y, R.w - 20.0f, rh,
                          0.17f, 0.38f, 0.8f, 0.4f);

        gui.disc(R.x + 24.0f, y + 14.0f, 9.0f, b->displayColor.x,
                 b->displayColor.y, b->displayColor.z, b->bandFreq,
                 b->isStar() ? 1.0f : 0.0f);
        gui.text(R.x + 40.0f, y + 8.0f, trunc(b->name, 16), 1.4f,
                 0.9f, 0.93f, 1.0f);
        gui.text(R.x + 176.0f, y + 8.0f, bodyTypeName(b->type), 1.3f,
                 0.55f, 0.6f, 0.72f);
        const std::string m = b->isStar()
            ? (fmtNum(b->mass, 1) + " Msun")
            : (trimNum(fmtNum(b->mass * (float)kMsunMearth, 0)) + " Mearth");
        gui.text(R.x + 248.0f, y + 8.0f, m, 1.3f, 0.55f, 0.6f, 0.72f);
        const std::string d = fmtNum(length(b->pos - ref), 2) + " AU";
        gui.text(R.x + 330.0f, y + 8.0f, d, 1.3f, 0.55f, 0.6f, 0.72f);
    }
    const int extra = (int)list.size() - u.findScroll - nRows;
    if (extra > 0)
        gui.text(R.x + 14.0f, ry0 + nRows * rh + 4.0f,
                 trunc(std::to_string(extra) + " more - scroll", 30), 1.2f,
                 0.45f, 0.5f, 0.6f);
    if ((int)list.size() == 0)
        gui.text(R.x + 14.0f, ry0 + 8.0f, "No bodies match.", 1.4f,
                 0.5f, 0.55f, 0.68f);
}

// ===========================================================================
//  Sim panel
// ===========================================================================
static RectF simRect(int w, int h) {
    return { (float)w * 0.5f - 235.0f, (float)h - 40.0f - 292.0f, 470.0f, 292.0f };
}

void UIPanels::drawSimPanel(Gui& gui, App& app) {
    const int w = gui.width(), h = gui.height();
    const RectF R = simRect(w, h);
    UIState& u = app.ui_;
    System& sim = app.sim_;

    gui.panel(R.x, R.y, R.w, R.h);
    gui.text(R.x + 12.0f, R.y + 8.0f, "Simulation", 2.0f, 0.95f, 0.97f, 1.0f);
    char sub[64];
    std::snprintf(sub, sizeof sub, "%s - %d bodies",
                  app.simName_.c_str(), sim.bodyCount());
    gui.text(R.x + 142.0f, R.y + 12.0f, sub, 1.3f, 0.55f, 0.6f, 0.75f);
    if (gui.button(R.x + R.w - 24.0f, R.y + 6.0f, 16.0f, 16.0f, "", Gui::IClose,
                   1.2f, 0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        u.simOpen = false;

    // toggle pills (top-right)
    float px = R.x + R.w - 10.0f - 4 * 84.0f;
    const float py = R.y + 26.0f;
    if (gui.pill(px, py, 80.0f, 20.0f, "Gravity", true))
        app.pushFlash("Gravity can't be disabled in this build");
    px += 84.0f;
    if (gui.pill(px, py, 80.0f, 20.0f, "Impacts", u.impactsOn)) {
        u.impactsOn = !u.impactsOn;
        sim.setImpactsEnabled(u.impactsOn);
    }
    px += 84.0f;
    if (gui.pill(px, py, 80.0f, 20.0f, "Debris", u.debrisOn))
        u.debrisOn = !u.debrisOn;
    px += 84.0f;
    if (gui.pill(px, py, 80.0f, 20.0f, "Heat", u.heatOn))
        u.heatOn = !u.heatOn;

    // sidebar
    const char* views[3] = { "Overview", "Advanced", "Stats" };
    const float sy = R.y + 60.0f;
    for (int i = 0; i < 3; ++i) {
        if (gui.pill(R.x + 12.0f, sy + i * 26.0f, 90.0f, 22.0f, views[i],
                     u.simView == i))
            u.simView = i;
    }

    const float cx = R.x + 114.0f;
    const float cw = R.w - 126.0f;
    float cy = sy + 4.0f;

    if (u.simView == 0) {                     // Overview
        gui.text(cx, cy, "Name", 1.35f, 0.6f, 0.66f, 0.8f);
        cy += 20.0f;
        gui.field(cx, cy, cw, app.simName_, app.simName_);
        cy += 30.0f;
        gui.text(cx, cy, "A live sandbox of planets, stars, moons and", 1.3f,
                 0.5f, 0.55f, 0.68f);
        cy += 16.0f;
        gui.text(cx, cy, "debris. Spawn worlds, detonate them, and watch", 1.3f,
                 0.5f, 0.55f, 0.68f);
        cy += 16.0f;
        gui.text(cx, cy, "gravity do the rest.", 1.3f, 0.5f, 0.55f, 0.68f);
        cy += 30.0f;
        gui.field(cx, cy, cw, "Bodies", std::to_string(sim.bodyCount()));
        cy += 30.0f;
        if (gui.button(cx, cy, 130.0f, 24.0f, "Open the stats", Gui::INone,
                       1.4f))
            u.simView = 2;
    } else if (u.simView == 1) {              // Advanced
        gui.text(cx, cy, "Time speed", 1.35f, 0.6f, 0.66f, 0.8f);
        char val[24];
        std::snprintf(val, sizeof val, "%.1fx", app.timeSpeed_);
        gui.text(cx + cw - Gui::measure(val, 1.35f), cy, val, 1.35f,
                 0.9f, 0.95f, 1.0f);
        cy += 18.0f;
        gui.slider(cx, cy, cw, app.timeSpeed_, 0.1f, 32.0f);
        cy += 30.0f;
        gui.text(cx, cy, "Trail length", 1.35f, 0.6f, 0.66f, 0.8f);
        std::snprintf(val, sizeof val, "%d", sim.config().trailLength);
        gui.text(cx + cw - Gui::measure(val, 1.35f), cy, val, 1.35f,
                 0.9f, 0.95f, 1.0f);
        cy += 18.0f;
        float tl = (float)sim.config().trailLength;
        if (gui.slider(cx, cy, cw, tl, 64.0f, 1024.0f))
            sim.config().trailLength = (int)tl;
        cy += 30.0f;
        if (gui.button(cx, cy, 90.0f, 24.0f, app.paused_ ? "Resume" : "Pause",
                       Gui::INone, 1.4f)) {
            app.paused_ = !app.paused_;
        }
        if (gui.button(cx + 98.0f, cy, 110.0f, 24.0f, sim.reversed()
                       ? "Forward" : "Reverse", Gui::INone, 1.4f)) {
            sim.setReversed(!sim.reversed());
        }
        if (gui.button(cx + 216.0f, cy, 110.0f, 24.0f, "Clear trails",
                       Gui::INone, 1.4f)) {
            sim.clearTrails();
        }
    } else {                                  // Stats
        struct Row { const char* label; std::string value; };
        Row rows[7];
        rows[0] = Row{ "Bodies", std::to_string(sim.bodyCount()) };
        {
            char b[32];
            std::snprintf(b, sizeof b, "%+.3f yr", sim.time());
            rows[1] = Row{ "Time", b };
        }
        {
            char b[48];
            std::snprintf(b, sizeof b, "%.3e", (double)sim.kineticEnergy());
            rows[2] = Row{ "Kinetic energy", b };
        }
        {
            char b[48];
            std::snprintf(b, sizeof b, "%.3e", (double)sim.potentialEnergy());
            rows[3] = Row{ "Potential energy", b };
        }
        {
            char b[48];
            std::snprintf(b, sizeof b, "%.3e", (double)sim.totalEnergy());
            rows[4] = Row{ "Total energy", b };
        }
        {
            char b[48];
            float v = 0.0f;
            int n = 0;
            for (const Body& bd : sim.bodies())
                if (!bd.dead) { v += length(bd.vel); ++n; }
            std::snprintf(b, sizeof b, "%.2f AU/yr", n ? v / n : 0.0f);
            rows[5] = Row{ "Average speed", b };
        }
        {
            char b[24];
            std::snprintf(b, sizeof b, "%.0f", app.fps_);
            rows[6] = Row{ "FPS", b };
        }
        for (const Row& r : rows) {
            gui.field(cx, cy, cw, r.label, r.value);
            cy += 26.0f;
        }
    }
}

// ===========================================================================
//  View panel  (the rework target)
// ===========================================================================
static RectF viewRect(int w) {
    return { (float)w - 228.0f, 52.0f, 216.0f, 530.0f };
}

void UIPanels::drawViewPanel(Gui& gui, App& app) {
    const int w = gui.width();
    const RectF R = viewRect(w);
    UIState& u = app.ui_;

    gui.panel(R.x, R.y, R.w, R.h);
    gui.text(R.x + 12.0f, R.y + 8.0f, "View", 2.0f, 0.95f, 0.97f, 1.0f);
    if (gui.button(R.x + R.w - 24.0f, R.y + 6.0f, 16.0f, 16.0f, "", Gui::IClose,
                   1.2f, 0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        u.viewOpen = false;

    float y = R.y + 36.0f;
    const float ix = R.x + 12.0f;
    const float iw = R.w - 24.0f;

    gui.section(ix, y, "Camera");
    y += 24.0f;
    const int fid = app.focId();
    const std::string looking = (fid >= 0)
        ? app.sim_.body(fid)->name : std::string("None");
    gui.field(ix, y, iw, "Looking at", looking);
    y += 26.0f;
    gui.field(ix, y, iw, "Camera", app.follow_ ? "Free + follow" : "Free camera");
    y += 32.0f;

    if (gui.button(ix, y, 62.0f, 24.0f, "Go to", Gui::ICamera, 1.4f))
        app.camGoTo();
    if (gui.button(ix + 68.0f, y, 62.0f, 24.0f, "See all", Gui::INone, 1.4f))
        app.camSeeAll();
    if (gui.button(ix + 136.0f, y, 62.0f, 24.0f, "Reset", Gui::INone, 1.4f))
        app.camResetView();
    y += 38.0f;

    gui.text(ix, y, "Field of view", 1.35f, 0.6f, 0.66f, 0.8f);
    char fovv[16];
    std::snprintf(fovv, sizeof fovv, "%.0f deg", u.fovDeg);
    gui.text(ix + iw - Gui::measure(fovv, 1.35f), y, fovv, 1.35f,
             0.9f, 0.95f, 1.0f);
    y += 18.0f;
    if (gui.slider(ix, y, iw, u.fovDeg, 30.0f, 90.0f))
        app.renderer_.setFovDeg(u.fovDeg);
    y += 28.0f;

    gui.section(ix, y, "Worlds");
    y += 24.0f;
    if (gui.chip(ix, y, 104.0f, 20.0f, "Atmospheres", u.showAtmospheres)) {}
    if (gui.chip(ix + 110.0f, y, 52.0f, 20.0f, "Clouds", u.showClouds)) {}
    y += 26.0f;
    if (gui.chip(ix, y, 56.0f, 20.0f, "Charts", u.showCharts)) {}
    y += 30.0f;

    gui.section(ix, y, "Colour by");
    y += 24.0f;
    static const char* colourOpts[4] = { "True colour", "Mass", "Speed",
                                         "Temperature" };
    if (gui.button(ix, y, iw, 24.0f, colourOpts[u.colourBy], Gui::IChevron,
                   1.4f))
        u.colourByMenu = !u.colourByMenu;
    y += 32.0f;

    gui.section(ix, y, "Lamp");
    y += 24.0f;
    if (gui.chip(ix, y, 56.0f, 20.0f, "Lamp", u.lampOn)) {}
    if (gui.chip(ix + 62.0f, y, 100.0f, 20.0f, "On at right",
                 u.lampFromRight)) {}
    y += 28.0f;

    gui.text(ix, y, "Strength", 1.35f, 0.6f, 0.66f, 0.8f);
    char sv[16];
    std::snprintf(sv, sizeof sv, "%.1fx", u.lampStrength);
    gui.text(ix + iw - Gui::measure(sv, 1.35f), y, sv, 1.35f, 0.9f, 0.95f, 1.0f);
    y += 18.0f;
    gui.slider(ix, y, iw, u.lampStrength, 0.2f, 5.0f);
    y += 28.0f;

    gui.text(ix, y, "Reach", 1.35f, 0.6f, 0.66f, 0.8f);
    std::snprintf(sv, sizeof sv, "%.1f AU", u.lampReach);
    gui.text(ix + iw - Gui::measure(sv, 1.35f), y, sv, 1.35f, 0.9f, 0.95f, 1.0f);
    y += 18.0f;
    gui.slider(ix, y, iw, u.lampReach, 0.5f, 8.0f);
    y += 28.0f;

    gui.text(ix, y, "Colour", 1.35f, 0.6f, 0.66f, 0.8f);
    std::snprintf(sv, sizeof sv, "%.0f K", u.lampKelvin);
    gui.text(ix + iw - Gui::measure(sv, 1.35f), y, sv, 1.35f, 0.9f, 0.95f, 1.0f);
    y += 18.0f;
    gui.slider(ix, y, iw, u.lampKelvin, 2000.0f, 10000.0f);
}

// Colour-by dropdown menu (drawn last so it sits on top).
bool UIPanels::drawColourMenu(Gui& gui, App& app) {
    const int w = gui.width();
    const RectF R = viewRect(w);
    UIState& u = app.ui_;
    if (!u.colourByMenu) return false;

    static const char* opts[4] = { "True colour", "Mass", "Speed",
                                   "Temperature" };
    const float mx = R.x + 12.0f;
    const float my = R.y + 334.0f;
    gui.panel(mx, my, 150.0f, 4 * 22.0f + 8.0f);
    bool consumed = false;
    for (int i = 0; i < 4; ++i) {
        const bool hov = gui.mouseInside(mx + 4.0f, my + 4.0f + i * 22.0f,
                                         142.0f, 22.0f);
        if (hov) gui.rect(mx + 4.0f, my + 4.0f + i * 22.0f, 142.0f, 22.0f,
                          0.15f, 0.3f, 0.65f, 0.6f);
        if (gui.row(mx + 4.0f, my + 4.0f + i * 22.0f, 142.0f, 22.0f, hov)) {
            u.colourBy = i;
            u.colourByMenu = false;
            consumed = true;
        }
        gui.text(mx + 12.0f, my + 9.0f + i * 22.0f, opts[i], 1.35f,
                 u.colourBy == i ? 1.0f : 0.75f, u.colourBy == i ? 1.0f : 0.8f,
                 u.colourBy == i ? 1.0f : 0.9f);
    }
    // A click anywhere (release on this frame) that the menu rows did not
    // consume closes the menu.
    if (gui.hadRelease() && !consumed && u.colourByMenu)
        u.colourByMenu = false;
    return consumed;
}

// ===========================================================================
//  Info panel (selected body)
// ===========================================================================
static RectF infoRect(int w) {
    return { (float)w - 352.0f, 52.0f, 340.0f, 590.0f };
}

void UIPanels::drawInfoPanel(Gui& gui, App& app) {
    const int w = gui.width();
    const RectF R = infoRect(w);
    UIState& u = app.ui_;
    const int sid = app.selId();
    if (sid < 0) return;
    Body& b = *app.sim_.body(sid);

    gui.panel(R.x, R.y, R.w, R.h);

    // header
    gui.disc(R.x + 34.0f, R.y + 32.0f, 20.0f, b.displayColor.x,
             b.displayColor.y, b.displayColor.z, b.bandFreq,
             b.isStar() ? 1.0f : 0.0f);
    gui.text(R.x + 66.0f, R.y + 16.0f, b.name, 2.2f, 0.97f, 0.98f, 1.0f);
    gui.text(R.x + 66.0f, R.y + 40.0f, bodyTypeName(b.type), 1.4f,
             0.55f, 0.62f, 0.78f);
    if (gui.button(R.x + R.w - 26.0f, R.y + 12.0f, 18.0f, 18.0f, "",
                   Gui::IClose, 1.3f, 0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        u.infoOpen = false;

    const char* tabs[5] = { "Physical", "Materials", "Climate", "Orbit",
                            "Appearance" };
    const float tw[5] = { 60.0f, 62.0f, 48.0f, 38.0f, 72.0f };
    float tx = R.x + 12.0f;
    for (int i = 0; i < 5; ++i) {
        if (gui.pill(tx, R.y + 64.0f, tw[i], 20.0f, tabs[i], u.infoTab == i))
            u.infoTab = i;
        tx += tw[i] + 4.0f;
    }

    const InfoValues v = computeInfo(app, b);
    const float ix = R.x + 12.0f;
    const float iw = R.w - 24.0f;
    float y = R.y + 94.0f;
    auto sec = [&](const char* s) {
        gui.section(ix, y, s);
        y += 26.0f;
    };

    if (u.infoTab == 0) {                     // Physical
        sec("Size");
        gui.field(ix, y, iw, "Mass", v.mass);             y += 24.0f;
        gui.field(ix, y, iw, "Radius", v.radius);         y += 24.0f;
        gui.field(ix, y, iw, "Density", v.density);       y += 26.0f;

        sec("Conditions");
        gui.field(ix, y, iw, "Temperature", v.temperature);   y += 24.0f;
        gui.field(ix, y, iw, "Surface gravity", v.gravity);   y += 24.0f;
        gui.field(ix, y, iw, "Pressure", v.pressure);         y += 24.0f;
        gui.field(ix, y, iw, "Luminosity", v.luminosity);     y += 26.0f;

        sec("Motion");
        gui.field(ix, y, iw, "Speed", v.speed);                y += 24.0f;
        gui.field(ix, y, iw, "Rotation period", v.rotation);   y += 24.0f;
        gui.field(ix, y, iw, "Orbital period", v.orbital);     y += 24.0f;
        gui.field(ix, y, iw, "Age", v.age);                    y += 24.0f;
        gui.field(ix, y, iw, "Stage", v.stage);                y += 24.0f;
        gui.field(ix, y, iw, "Spectral type", v.spectral);     y += 26.0f;

        sec("Debris");
        gui.field(ix, y, iw, "Fragments", v.fragments);        y += 24.0f;
        gui.field(ix, y, iw, "Far-flung", "0.0");              y += 24.0f;
    }
    else if (u.infoTab == 1) {                // Materials
        sec("Surface");
        const char* names[4] = { "Rock", "Water", "Ice", "Air" };
        float cx0 = ix;
        for (int i = 0; i < 4; ++i) {
            const bool on = (b.surface == i);
            if (on) gui.rect(cx0, y, 62.0f, 20.0f, 0.17f, 0.4f, 0.85f, 1.0f);
            if (gui.button(cx0, y, 62.0f, 20.0f, names[i], Gui::INone, 1.3f,
                           on ? 0.17f : 0.12f, on ? 0.4f : 0.13f,
                           on ? 0.85f : 0.17f,
                           on ? 1.0f : 0.7f, on ? 1.0f : 0.75f,
                           on ? 1.0f : 0.85f)) {
                b.surface = i;
            }
            cx0 += 68.0f;
            if (i == 1) { cx0 = ix; y += 26.0f; }
        }
        y += 26.0f;
        gui.text(ix, y, "Roughness", 1.35f, 0.6f, 0.66f, 0.8f);
        y += 18.0f;
        gui.slider(ix, y, iw, b.roughness, 0.0f, 1.0f);
        y += 30.0f;
        gui.field(ix, y, iw, "Density", v.density);
    }
    else if (u.infoTab == 2) {                // Climate
        sec("Climate");
        gui.text(ix, y, "Temperature", 1.35f, 0.6f, 0.66f, 0.8f);
        char tv[24];
        std::snprintf(tv, sizeof tv, "%.0f C", b.temperatureC);
        gui.text(ix + iw - Gui::measure(tv, 1.35f), y, tv, 1.35f, 0.9f, 0.95f, 1.0f);
        y += 18.0f;
        gui.slider(ix, y, iw, b.temperatureC, -273.0f, 6000.0f);
        y += 30.0f;
        gui.field(ix, y, iw, "Pressure", v.pressure);
        y += 28.0f;
        bool atm = (b.surface == 3);
        if (gui.chip(ix, y, 120.0f, 20.0f, "Atmosphere", atm)) {
            b.surface = atm ? 0 : 3;
        }
        y += 28.0f;
        gui.field(ix, y, iw, "Temperature (shown)", v.temperature);
    }
    else if (u.infoTab == 3) {                // Orbit
        sec("Orbit");
        gui.field(ix, y, iw, "Orbit around",
                  v.aroundName.empty() ? std::string("-") : v.aroundName);
        y += 24.0f;
        gui.field(ix, y, iw, "Distance",
                  v.hasOrbit || v.distAU > 0.0f
                      ? (trimNum(fmtNum(v.distAU, 3)) + " AU") : "-");
        y += 24.0f;
        gui.field(ix, y, iw, "Relative speed",
                  trimNum(fmtNum(v.speedRelKms, 2)) + " km/s");
        y += 24.0f;
        gui.field(ix, y, iw, "Period",
                  v.hasOrbit ? (trimNum(fmtNum(v.periodYr, 2)) + " yr") : "-");
        y += 24.0f;
        gui.field(ix, y, iw, "Semi-major axis",
                  v.hasOrbit ? (trimNum(fmtNum(v.aAU, 3)) + " AU") : "-");
        y += 24.0f;
        gui.field(ix, y, iw, "Eccentricity", trimNum(fmtNum(v.ecc, 3)));
        y += 30.0f;
        if (gui.button(ix, y, 130.0f, 24.0f, "Circularize orbit",
                       Gui::INone, 1.4f))
            app.uiTool(6);
    }
    else {                                    // Appearance
        sec("Appearance");
        gui.disc(ix + 16.0f, y + 16.0f, 16.0f, b.color.x, b.color.y,
                 b.color.z, b.bandFreq, b.isStar() ? 1.0f : 0.0f);
        gui.text(ix + 44.0f, y + 6.0f, "Colour", 1.35f, 0.6f, 0.66f, 0.8f);
        float rr = b.color.x, gg = b.color.y, bb = b.color.z;
        if (gui.slider(ix + 44.0f, y + 24.0f, iw - 44.0f, rr, 0.0f, 1.0f))
            b.color.x = rr;
        if (gui.slider(ix + 44.0f, y + 44.0f, iw - 44.0f, gg, 0.0f, 1.0f))
            b.color.y = gg;
        if (gui.slider(ix + 44.0f, y + 64.0f, iw - 44.0f, bb, 0.0f, 1.0f))
            b.color.z = bb;
        gui.text(ix + 44.0f, y + 20.0f, "R", 1.3f, 0.9f, 0.4f, 0.4f);
        gui.text(ix + 44.0f, y + 40.0f, "G", 1.3f, 0.4f, 0.9f, 0.4f);
        gui.text(ix + 44.0f, y + 60.0f, "B", 1.3f, 0.4f, 0.5f, 0.9f);
        y += 96.0f;

        gui.text(ix, y, "Display radius", 1.35f, 0.6f, 0.66f, 0.8f);
        char rv[24];
        std::snprintf(rv, sizeof rv, "%.4f AU", b.radius);
        gui.text(ix + iw - Gui::measure(rv, 1.35f), y, rv, 1.35f, 0.9f, 0.95f, 1.0f);
        y += 18.0f;
        gui.slider(ix, y, iw, b.radius, 0.003f, 0.12f);
        y += 28.0f;

        gui.text(ix, y, "Mass (log)", 1.35f, 0.6f, 0.66f, 0.8f);
        std::snprintf(rv, sizeof rv, "%.2e Msun", (double)b.mass);
        gui.text(ix + iw - Gui::measure(rv, 1.35f), y, rv, 1.35f, 0.9f, 0.95f, 1.0f);
        y += 18.0f;
        float logm = std::log10(std::max(b.mass, 1e-12f)) + 9.0f; // 0..~21
        if (gui.slider(ix, y, iw, logm, 0.0f, 21.0f))
            b.mass = std::pow(10.0f, logm - 9.0f);
        y += 28.0f;

        gui.text(ix, y, "Spin (rad/yr)", 1.35f, 0.6f, 0.66f, 0.8f);
        std::snprintf(rv, sizeof rv, "%.2f", b.spin);
        gui.text(ix + iw - Gui::measure(rv, 1.35f), y, rv, 1.35f, 0.9f, 0.95f, 1.0f);
        y += 18.0f;
        gui.slider(ix, y, iw, b.spin, -8.0f, 8.0f);
    }
}

// ===========================================================================
//  Top-left chrome: FPS + hamburger help
// ===========================================================================
void UIPanels::drawTopLeft(Gui& gui, App& app) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.0f FPS", app.fps_);
    gui.text(10.0f, 10.0f, buf, 1.4f, 0.5f, 0.95f, 0.55f);

    // hamburger button
    const bool hov = gui.mouseInside(10.0f, 28.0f, 22.0f, 20.0f);
    gui.rect(10.0f, 28.0f, 22.0f, 20.0f,
             hov ? 0.15f : 0.1f, hov ? 0.17f : 0.11f,
             hov ? 0.23f : 0.15f, 0.9f);
    gui.line(14.0f, 34.0f, 28.0f, 34.0f, 0.8f, 0.85f, 1.0f, 0.9f, 1.5f);
    gui.line(14.0f, 38.0f, 28.0f, 38.0f, 0.8f, 0.85f, 1.0f, 0.9f, 1.5f);
    gui.line(14.0f, 42.0f, 28.0f, 42.0f, 0.8f, 0.85f, 1.0f, 0.9f, 1.5f);
    if (gui.clickedAt(10.0f, 28.0f, 22.0f, 20.0f))
        app.ui_.helpOpen = !app.ui_.helpOpen;
}

void UIPanels::drawHelpPanel(Gui& gui, App& app) {
    if (!app.ui_.helpOpen) return;
    const float x = 10.0f, y = 54.0f, w = 330.0f, h = 320.0f;
    gui.panel(x, y, w, h);
    gui.text(x + 12.0f, y + 8.0f, "Controls", 1.8f, 0.95f, 0.97f, 1.0f);
    if (gui.button(x + w - 24.0f, y + 6.0f, 16.0f, 16.0f, "", Gui::IClose,
                   1.2f, 0.11f, 0.12f, 0.16f, 0.8f, 0.85f, 1.0f))
        app.ui_.helpOpen = false;

    static const char* lines[] = {
        "DRAG          orbit camera",
        "R-DRAG        pan camera",
        "WHEEL         zoom",
        "CLICK         select body",
        "DBL CLICK     focus + zoom",
        "1 / 2 / 3     spawn planet / star / rock",
        "DEL           delete selected",
        "SPACE         pause / resume",
        "+ / -         time speed",
        "N             reverse time",
        "F             camera follow",
        "G             motion trails",
        "9  solar      8  binary",
        "0  asteroid   7  star + moon",
        "R             new random universe",
        "CTRL+S save   CTRL+O load",
        "F11 fullscreen          ESC close panels / quit",
    };
    float ty = y + 36.0f;
    for (const char* s : lines) {
        gui.text(x + 12.0f, ty, s, 1.25f, 0.68f, 0.74f, 0.86f);
        ty += 15.0f;
    }
}

// ===========================================================================
//  Entry point
// ===========================================================================
bool UIPanels::anyPanelOpen(const UIState& s) {
    return s.addOpen || s.toolsOpen || s.findOpen || s.simOpen ||
           s.viewOpen || s.helpOpen;
}

bool UIPanels::pointInPanel(const UIState& s, int w, int h, int px, int py) {
    if (s.addOpen && RectF(14.0f, 52.0f, 318.0f, 470.0f).contains(px, py))
        return true;
    if (s.toolsOpen && toolsRect(w, h).contains(px, py)) return true;
    if (s.findOpen && findPanel(w, h).contains(px, py)) return true;
    if (s.simOpen && simRect(w, h).contains(px, py)) return true;
    if (s.helpOpen && RectF(10.0f, 54.0f, 330.0f, 320.0f).contains(px, py))
        return true;
    if (s.viewOpen || s.infoOpen) {
        if (RectF((float)w - 352.0f, 52.0f, 352.0f, 590.0f).contains(px, py))
            return true;
    }
    return false;
}

void UIPanels::drawAll(Gui& gui, App& app) {
    const int w = gui.width(), h = gui.height();
    UIState& u = app.ui_;

    // side panels first (they sit under the bottom chrome)
    if (u.addOpen)   drawAddPanel(gui, app);
    if (u.toolsOpen) drawToolsPanel(gui, app);
    if (u.findOpen)  drawFindPanel(gui, app);
    if (u.simOpen)   drawSimPanel(gui, app);

    // right column: info when a body is selected, else the view panel
    const int sid = app.selId();
    if (sid >= 0 && u.infoOpen) {
        drawInfoPanel(gui, app);
    } else if (u.viewOpen) {
        drawViewPanel(gui, app);
    }

    // bottom chrome on top
    drawBottomBar(gui, app);
    drawTimeBar(gui, app);

    // help + top-left chrome
    drawHelpPanel(gui, app);
    drawTopLeft(gui, app);

    // colour-by menu floats above everything
    drawColourMenu(gui, app);
}

} // namespace us3d
