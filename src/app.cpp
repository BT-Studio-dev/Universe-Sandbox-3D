// ============================================================================
//  Universe Sandbox 3D - application controller (implementation)
// ============================================================================
#include "app.h"
#include "gl/gl33.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

namespace us3d {

namespace {
std::mt19937& rng() {
    static std::mt19937 g((unsigned)std::time(nullptr) ^ 0xC0FFEEu);
    return g;
}

float randRange(float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng());
}

std::string exeDir() {
    char buf[MAX_PATH];
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string s(buf, n);
    const size_t p = s.find_last_of("\\/");
    return (p == std::string::npos) ? std::string(".") : s.substr(0, p);
}
} // namespace

// ---------------------------------------------------------------------------
//  init / run / shutdown
// ---------------------------------------------------------------------------
bool App::init(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    // ---- window + GL context ----
    window_.onResize    = [this](int w, int h) { onResize(w, h); };
    window_.onClose     = [this]() { onClose(); };
    window_.onKey       = [this](int vk, bool d) { onKey(vk, d); };
    window_.onMouseDown = [this](int b) { onMouseDown(b); };
    window_.onMouseUp   = [this](int b) { onMouseUp(b); };
    window_.onMouseMove = [this](int x, int y, int dx, int dy) { onMouseMove(x, y, dx, dy); };
    window_.onWheel     = [this](int d) { onWheel(d); };
    window_.onFocusLost = [this]() { onFocusLost(); };

    if (!window_.create(hInstance, "Universe Sandbox 3D", 1280, 720, false)) {
        return false;
    }
    if (!gl33::isCompatible()) {
        const char* msg =
            "This computer's graphics driver reports an OpenGL version below 3.3.\n\n"
            "Universe Sandbox 3D needs a 3D card with an OpenGL 3.3-capable driver.\n"
            "Update the graphics driver and try again.";
        MessageBoxA(nullptr, msg, "Universe Sandbox 3D - incompatible GPU driver",
                    MB_OK | MB_ICONERROR);
        return false;
    }
    window_.setVSync(true);

    // ---- GL objects ----
    if (!renderer_.init()) {
        const char* msg = (std::string("Renderer initialization failed:\n") +
                           renderer_.error()).c_str();
        MessageBoxA(nullptr, msg, "Universe Sandbox 3D", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!hud_.init(renderer_.fontTexture())) {
        MessageBoxA(nullptr, "HUD initialization failed.", "Universe Sandbox 3D",
                    MB_OK | MB_ICONERROR);
        return false;
    }

    // ---- scene ----
    loadPreset(0); // solar system
    inited_ = true;
    pushFlash("Welcome! Press H for the controls.");
    return true;
}

void App::run() {
    if (!inited_) return;
    using clock = std::chrono::high_resolution_clock;
    auto prev = clock::now();
    while (running_) {
        if (!Window::pumpMessage()) break;
        const auto now = clock::now();
        float dt = std::chrono::duration<float>(now - prev).count();
        prev = now;
        dt = clampf(dt, 0.0f, 0.1f);

        processInput(dt);
        update(dt);
        renderFrame();
        window_.swapBuffers();
    }
}

void App::shutdown() {
    hud_.shutdown();
    renderer_.shutdown();
    window_.destroy();
    inited_ = false;
}

// ---------------------------------------------------------------------------
//  Input
// ---------------------------------------------------------------------------
void App::onResize(int w, int h) { (void)w; (void)h; }

void App::onClose() { requestQuit(); }

void App::onFocusLost() {
    for (int i = 0; i < 256; ++i) keys_[i] = false;
    for (int i = 0; i < 3; ++i) mouseDown_[i] = false;
    mouseButton_ = -1;
}

void App::onKey(int vk, bool down) {
    if (vk >= 256) return;
    if (down && keys_[vk]) return; // ignore auto-repeat
    keys_[vk] = down;
    if (!down) return;

    switch (vk) {
    case VK_SPACE:
        paused_ = !paused_;
        pushFlash(paused_ ? "Paused" : "Resumed");
        break;
    case VK_OEM_PLUS:
    case VK_ADD:
        timeSpeed_ = clampf(timeSpeed_ * 1.6f, 0.1f, 32.0f);
        break;
    case VK_OEM_MINUS:
    case VK_SUBTRACT:
        timeSpeed_ = clampf(timeSpeed_ / 1.6f, 0.1f, 32.0f);
        break;
    case 'N': {
        sim_.setReversed(!sim_.reversed());
        pushFlash(sim_.reversed() ? "Time reversed" : "Time forward");
        break;
    }
    case 'F':
        follow_ = !follow_;
        pushFlash(follow_ ? "Camera follows focus" : "Camera free");
        break;
    case 'G':
        showTrails_ = !showTrails_;
        break;
    case 'H':
        showHelp_ = !showHelp_;
        break;
    case '1': spawnAroundFocus(BodyType::Planet);   break;
    case '2': spawnAroundFocus(BodyType::Star);     break;
    case '3': spawnAroundFocus(BodyType::Asteroid); break;
    case VK_DELETE:
        deleteSelected();
        break;
    case '9': loadPreset(0); break; // solar system
    case '8': loadPreset(1); break; // binary
    case '0': loadPreset(2); break; // asteroid belt
    case '7': loadPreset(3); break; // star + moon
    case 'R': loadPreset(4); break; // random
    case VK_F11:
        window_.toggleFullscreen();
        break;
    case VK_ESCAPE:
        requestQuit();
        break;
    case 'S':
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) doSave();
        break;
    case 'O':
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) doLoad();
        break;
    default:
        break;
    }
}

void App::onMouseDown(int button) {
    if (button < 0 || button > 2) return;
    mouseDown_[button] = true;
    mouseButton_ = button;
    Window::cursorPos((int&)mouseX_, (int&)mouseY_);
    downX_ = (float)mouseX_;
    downY_ = (float)mouseY_;
    dragged_ = false;
}

void App::onMouseUp(int button) {
    if (button < 0 || button > 2) return;
    if (!mouseDown_[button]) return;
    mouseDown_[button] = false;
    if (mouseButton_ != button) return;
    mouseButton_ = -1;

    if (button == 0 && !dragged_) {
        const int id = pickBodyAtCursor();
        const int nowMs = (int)GetTickCount();
        const bool dbl = (nowMs - lastClickTimeMs_ < 350);
        lastClickTimeMs_ = nowMs;
        if (id >= 0) {
            selectAndFocus(id, dbl);
        } else {
            selectedUid_ = -1;
            if (dbl) {
                // double-click empty space: refocus on the primary
                const int p = sim_.primaryBody();
                if (p >= 0) selectAndFocus(p, false);
            }
        }
    }
}

void App::onMouseMove(int x, int y, int dx, int dy) {
    mouseX_ = x;
    mouseY_ = y;
    if (mouseButton_ < 0) return;
    if (std::fabs(x - downX_) + std::fabs(y - downY_) > 4) dragged_ = true;

    if (mouseDown_[0]) {
        cam_.orbit(dx * 0.0055f, dy * 0.0055f);
    } else if (mouseDown_[1] || mouseDown_[2]) {
        const Vec3 f = normalize(cam_.target() - cam_.position());
        const Vec3 right = normalize(cross(f, Vec3::UnitY()));
        const Vec3 up = cross(right, f);
        cam_.pan(right, up, (float)dx, (float)dy);
    }
}

void App::onWheel(int delta) {
    cam_.zoomBy((float)std::exp(-delta * 0.0011f));
}

void App::processInput(float dt) { (void)dt; }

// ---------------------------------------------------------------------------
//  Update
// ---------------------------------------------------------------------------
void App::update(float dt) {
    timeSec_ += dt;

    if (!paused_) {
        // 1 year of simulation every 12 s at 1x speed.
        const float yearsPerSec = timeSpeed_ / 12.0f;
        sim_.advance(yearsPerSec * dt * (sim_.reversed() ? -1.0f : 1.0f));
    }
    for (Body& b : sim_.bodies()) b.age += dt;
    sim_.finalize();

    for (const std::string& e : sim_.takeEvents()) pushFlash(e);

    // Camera focus tracking (uid survives renumbering after merges).
    if (follow_ && focusedUid_ >= 0) {
        const int fid = focId();
        if (fid >= 0) {
            cam_.setFocusPoint(sim_.body(fid)->pos);
        } else {
            // Focused body merged away - follow the primary instead.
            const int p = sim_.primaryBody();
            if (p >= 0) focusedUid_ = sim_.body(p)->uid;
        }
    }

    // Flash decay.
    for (auto& f : flashes_) f.second -= dt;
    while (!flashes_.empty() && flashes_.back().second <= 0.0f) flashes_.pop_back();

    // FPS (exponential moving average).
    if (dt > 0) fps_ = fps_ * 0.95f + (1.0f / dt) * 0.05f;
}

// ---------------------------------------------------------------------------
//  Actions
// ---------------------------------------------------------------------------
int App::pickBodyAtCursor() {
    const int w = window_.clientWidth();
    const int h = window_.clientHeight();
    if (w <= 0 || h <= 0) return -1;
    const float ndcX = (float)mouseX_ / w * 2.0f - 1.0f;
    const float ndcY = 1.0f - (float)mouseY_ / h * 2.0f;
    const Ray ray = cam_.ray(ndcX, ndcY, (float)w / h, renderer_.fovRad());

    int bestId = -1;
    float bestT = 1e30f;
    for (const Body& b : sim_.bodies()) {
        if (b.dead) continue;
        const float pickR = std::max(b.radius, 0.006f);
        float t;
        if (raySphere(ray, b.pos, pickR, t) && t < bestT) {
            bestT = t;
            bestId = b.id;
        }
    }
    return bestId;
}

void App::selectAndFocus(int id, bool zoomIn) {
    const Body* b = sim_.body(id);
    if (!b) return;
    selectedUid_ = b->uid;
    focusedUid_  = b->uid;
    follow_ = true;
    if (zoomIn) {
        const float want = std::max(b->radius * 12.0f, 0.12f);
        if (cam_.distance() > want * 1.5f) cam_.setDistance(want);
    }
}

void App::spawnAroundFocus(BodyType type) {
    const int anchorId = (focId() >= 0) ? focId() : sim_.primaryBody();
    const Body* anchor = (anchorId >= 0) ? sim_.body(anchorId) : nullptr;
    const Vec3 anchorPos = anchor ? anchor->pos : Vec3::Zero();
    const Vec3 anchorVel = anchor ? anchor->vel : Vec3::Zero();
    const float anchorMass = anchor ? anchor->mass : 0.0f;
    const float G = sim_.config().G;

    const float ang = randRange(0.0f, 2.0f * 3.14159265f);
    const float inc = randRange(-0.12f, 0.12f);
    const Vec3 radial(std::cos(ang), 0.0f, std::sin(ang));
    Vec3 tangent(-std::sin(ang), 0.0f, std::cos(ang));
    // tilt orbital plane slightly
    const Vec3 up(0.0f, 1.0f, 0.0f);
    const Vec3 n = normalize(cross(radial, tangent) + up * inc);
    tangent = normalize(cross(n, radial));

    Body b;
    b.type = type;
    b.name = sim_.makeName(type == BodyType::Star ? "star" : "planet");

    float dist = cam_.distance() * 0.5f;
    switch (type) {
    case BodyType::Star:
        b.mass  = randRange(0.05f, 1.4f);
        b.radius = 0.05f + 0.07f * std::cbrt(b.mass);
        b.color = hsl2rgb(randRange(0.05f, 0.14f), 0.55f, 0.62f);
        dist = clampf(cam_.distance() * 0.9f, 1.2f, 12.0f);
        b.pos = anchorPos + radial * dist;
        b.vel = anchorVel * 0.3f + radial * 0.04f;
        break;
    case BodyType::Asteroid:
        b.mass = randRange(4e-9f, 8e-8f);
        b.radius = 0.006f + randRange(0.0f, 0.006f);
        b.color = hsl2rgb(randRange(0.0f, 1.0f), 0.2f, 0.45f);
        dist = clampf(randRange(0.08f, 0.6f), 0.05f, 1.5f);
        b.pos = anchorPos + radial * dist;
        b.vel = anchorVel + tangent * std::sqrt(G * std::max(anchorMass, 1e-6f) / dist) * 0.96f;
        break;
    default: // Planet / Moon
        b.mass = randRange(0.3e-6f, 6.0e-6f);
        b.radius = clampf(0.005f * std::cbrt(b.mass / 3.0e-6f), 0.006f, 0.032f);
        b.color = hsl2rgb(randRange(0.0f, 1.0f), 0.45f, 0.55f);
        b.bandFreq = (randRange(0.0f, 1.0f) < 0.35f) ? randRange(3.0f, 7.0f) : 0.0f;
        dist = clampf(dist, 0.15f, 6.0f);
        b.pos = anchorPos + radial * dist;
        b.vel = anchorVel + tangent * std::sqrt(G * std::max(anchorMass, 1e-6f) / dist);
        break;
    }
    b.roughness = randRange(0.2f, 0.8f);

    const int id = sim_.addBody(b);
    selectAndFocus(id, false);
    char msg[96];
    std::snprintf(msg, sizeof msg, "+ %s (%s)", b.name.c_str(), bodyTypeName(b.type));
    pushFlash(msg);
}

void App::deleteSelected() {
    const int sid = selId();
    if (sid < 0) return;
    sim_.removeBody(sid);
    selectedUid_ = -1;
    if (focId() < 0) {
        const int p = sim_.primaryBody();
        if (p >= 0) focusedUid_ = sim_.body(p)->uid;
    }
}

void App::resetCameraForPreset() {
    cam_.setFocusPoint(Vec3::Zero());
    cam_.setDistance(2.4f);
    // nudge the view a little so presets don't all look identical
    cam_.orbit(randRange(-0.4f, 0.4f), 0.0f);
}

void App::loadPreset(int which) {
    switch (which) {
    case 0:  Presets::solarSystem(sim_);        break;
    case 1:  Presets::binary(sim_);             break;
    case 2:  Presets::asteroidBelt(sim_, 400);  break;
    case 3:  Presets::starAndMoon(sim_);        break;
    default: Presets::randomUniverse(sim_);     break;
    }
    selectedUid_ = -1;
    {
        const int p = sim_.primaryBody();
        focusedUid_ = (p >= 0) ? sim_.body(p)->uid : -1;
    }
    follow_ = true;
    paused_ = false;
    sim_.setReversed(false);
    timeSpeed_ = 1.0f;
    showTrails_ = true;
    sim_.clearTrails();
    for (Body& b : sim_.bodies()) b.age = 0.0f;

    switch (which) {
    case 1:  cam_.setDistance(3.4f); break;
    case 2:  cam_.setDistance(4.2f); break;
    case 3:  cam_.setDistance(1.9f); break;
    default: cam_.setDistance(2.4f); break;
    }
    cam_.setFocusPoint(Vec3::Zero());
    const int p = sim_.primaryBody();
    if (p >= 0) cam_.setFocusPoint(sim_.body(p)->pos);
    pushFlash(which == 4 ? "New random universe" : "Preset loaded");
}

void App::doSave() {
    const std::string path = exeDir() + "/universe_save.txt";
    const SaveResult r = saveSystem(sim_, path);
    pushFlash(r.message);
}

void App::doLoad() {
    const std::string path = exeDir() + "/universe_save.txt";
    SaveResult r = loadSystem(sim_, path);
    pushFlash(r.message);
    if (r.ok) {
        selectedUid_ = -1;
        const int p = sim_.primaryBody();
        focusedUid_ = (p >= 0) ? sim_.body(p)->uid : -1;
        for (Body& b : sim_.bodies()) b.age = 1.0f;
        sim_.clearTrails();
        if (p >= 0) cam_.setFocusPoint(sim_.body(p)->pos);
    }
}

void App::pushFlash(const std::string& msg) {
    flashes_.push_back({ msg, 2.6f });
    while (flashes_.size() > 4) flashes_.erase(flashes_.begin());
}

// ---------------------------------------------------------------------------
//  Rendering
// ---------------------------------------------------------------------------
void App::renderFrame() {
    const int w = window_.clientWidth();
    const int h = window_.clientHeight();
    if (w <= 0 || h <= 0) return;

    renderer_.render(cam_, sim_, selId(), showTrails_, timeSec_, w, h);
    drawHud();
}

void App::drawHud() {
    const int w = window_.clientWidth();
    const int h = window_.clientHeight();
    hud_.begin(w, h);

    const float px1 = 1.4f;  // small
    const float px2 = 2.0f;  // normal

    // ---- top-left status panel ----
    {
        const float x = 10, y = 10;
        const float pw = 484, ph = 118;
        hud_.drawRect(x, y, pw, ph, 0.0f, 0.0f, 0.0f, 0.42f);
        hud_.drawText(x + 10, y + 8, "UNIVERSE SANDBOX 3D", px2, 1.0f, 0.85f, 0.4f);

        char line[160];
        std::snprintf(line, sizeof line, "FPS %3d   BODIES %d   T %+.2f yr   SPEED %.1fx%s%s",
                      (int)std::lround(fps_), sim_.bodyCount(), sim_.time(),
                      timeSpeed_, sim_.reversed() ? " REV" : "", paused_ ? "  [PAUSED]" : "");
        hud_.drawText(x + 10, y + 34, line, px1, 0.75f, 0.8f, 0.9f);

        const int sid = selId();
        if (sid >= 0 && sim_.body(sid)) {
            const Body* b = sim_.body(sid);
            const float v = length(b->vel);
            const int fid = focId();
            const float d = (fid >= 0 && sim_.body(fid))
                ? length(b->pos - sim_.body(fid)->pos) : length(b->pos);
            std::snprintf(line, sizeof line,
                          "SEL %s  %s  m %.2e  v %.2f AU/yr  d %.2f AU",
                          b->name.c_str(), bodyTypeName(b->type),
                          b->mass, v, d);
            hud_.drawText(x + 10, y + 54, line, px1, 0.9f, 0.95f, 1.0f);
        } else {
            hud_.drawText(x + 10, y + 54, "Click a body to inspect it", px1, 0.6f, 0.65f, 0.75f);
        }

        std::snprintf(line, sizeof line, "GL %s", gl33::versionString().c_str());
        hud_.drawText(x + 10, y + 74, line, px1, 0.45f, 0.5f, 0.6f);
        hud_.drawText(x + 10, y + 92, "1/2/3 spawn   DEL remove   N reverse   SPACE pause",
                      px1, 0.45f, 0.5f, 0.6f);
    }

    // ---- top-right help ----
    if (showHelp_) {
        static const char* help[] = {
            "DRAG      orbit camera",
            "R-DRAG    pan camera",
            "WHEEL     zoom",
            "CLICK     select body",
            "DBL CLICK focus + zoom",
            "1 / 2 / 3 spawn planet / star / rock",
            "DEL       delete selected",
            "SPACE     pause / resume",
            "+ / -     time speed",
            "N         reverse time",
            "F         camera follow",
            "G         motion trails",
            "H         this help",
            "9  solar     8  binary",
            "0  asteroid  7  planet+moon",
            "R       new random universe",
            "CTRL+S  save     CTRL+O load",
            "F11     fullscreen     ESC quit",
        };
        const float px = 1.25f;
        const float lh = 13.5f * px;
        float maxw = 0;
        for (const char* s : help) maxw = std::max(maxw, Hud::measureText(s, px));
        const float bw = maxw + 16;
        const float bh = lh * (float)(sizeof help / sizeof help[0]) + 10;
        const float x = w - bw - 10, y = 10;
        hud_.drawRect(x, y, bw, bh, 0.0f, 0.0f, 0.0f, 0.38f);
        float ty = y + 7;
        for (const char* s : help) {
            hud_.drawText(x + 8, ty, s, px, 0.7f, 0.75f, 0.85f);
            ty += lh;
        }
    } else {
        hud_.drawText(w - 190, 12, "PRESS H FOR HELP", 1.2f, 0.55f, 0.6f, 0.7f);
    }

    // ---- center flashes ----
    {
        float fy = h * 0.5f - (float)flashes_.size() * 14.0f;
        for (const auto& f : flashes_) {
            const float a = std::min(1.0f, f.second / 0.6f);
            hud_.drawTextCentered(fy, f.first, 2.0f, 1.0f, 0.9f, 0.55f, a);
            fy += 22.0f;
        }
    }

    // ---- pause banner ----
    if (paused_) {
        hud_.drawTextCentered(60, "PAUSED - PRESS SPACE", 3.0f, 1.0f, 0.9f, 0.4f, 0.9f);
    }

    hud_.end();
}

} // namespace us3d
