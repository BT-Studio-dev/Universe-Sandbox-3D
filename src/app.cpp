// ============================================================================
//  Universe Sandbox 3D - application controller (implementation)
// ============================================================================
#include "app.h"
#include "gl/gl33.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

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

const char* presetName(int which) {
    switch (which) {
    case 1:  return "Binary Stars";
    case 2:  return "Asteroid Belt";
    case 3:  return "Star + Moon";
    case 4:  return "Random Universe";
    default: return "Solar System";
    }
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
    window_.onChar      = [this](char c) { onChar(c); };
    window_.onMouseDown = [this](int b) { onMouseDown(b); };
    window_.onMouseUp   = [this](int b) { onMouseUp(b); };
    window_.onMouseMove = [this](int x, int y, int dx, int dy) { onMouseMove(x, y, dx, dy); };
    window_.onWheel     = [this](int d) { onWheel(d); };
    window_.onFocusLost = [this]() { onFocusLost(); };

    if (!window_.create(hInstance, "Universe Sandbox 3D", 1280, 720, false)) {
        return false;
    }
    if (!gl33::isCompatible()) {
        const std::string msg =
            "This computer's graphics driver reports an OpenGL version below 3.3.\n\n"
            + window_.glReport() + "\n"
            "Universe Sandbox 3D needs OpenGL 3.3 (core or compatibility profile).\n"
            "Options:\n"
            " - On a physical PC: update the graphics driver and try again.\n"
            " - On a VM or a PC without a working 3D driver: put the\n"
            "   Mesa-for-Windows DLLs (opengl32.dll + libgallium_wgl.dll) next\n"
            "   to us3d.exe and start the app with the included run_mesa.bat.\n"
            " - The 'Context bootstrap' lines above show which 3.3-context\n"
            "   attempt succeeded or failed - useful when reporting the issue.";
        MessageBoxA(nullptr, msg.c_str(),
                    "Universe Sandbox 3D - OpenGL 3.3 not available",
                    MB_OK | MB_ICONERROR);
        return false;
    }
    window_.setVSync(true);

    // ---- GL objects ----
    if (!renderer_.init()) {
        const std::string msg =
            "Renderer initialization failed:\n" + renderer_.error();
        MessageBoxA(nullptr, msg.c_str(), "Universe Sandbox 3D", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!hud_.init(renderer_.fontTexture())) {
        MessageBoxA(nullptr, "HUD initialization failed.", "Universe Sandbox 3D",
                    MB_OK | MB_ICONERROR);
        return false;
    }
    if (!gui_.init()) {
        MessageBoxA(nullptr, "GUI initialization failed.", "Universe Sandbox 3D",
                    MB_OK | MB_ICONERROR);
        return false;
    }

    // ---- scene ----
    loadPreset(0); // solar system
    inited_ = true;
    pushFlash("Welcome! Use Add, Tools, Find, Sim and View below.");
    return true;
}

void App::run() {
    if (!inited_) return;
    // Frame timing via QueryPerformanceCounter - a Windows 2000 API that
    // is monotonic and high-resolution. Deliberately NOT
    // std::chrono::high_resolution_clock: the MinGW/zig libc
    // implementation of <chrono> imports GetSystemTimePreciseAsFileTime
    // (a Windows 8+ kernel32 API) at load time, which makes the
    // cross-built exe unlaunchable on Windows 7.
    LARGE_INTEGER freq, prev;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);
    while (running_) {
        if (!Window::pumpMessage()) break;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = (float)((now.QuadPart - prev.QuadPart) / (double)freq.QuadPart);
        prev = now;
        dt = clampf(dt, 0.0f, 0.1f);

        processInput(dt);
        update(dt);
        renderFrame();
        window_.swapBuffers();
    }
}

void App::shutdown() {
    gui_.shutdown();
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
    for (int i = 0; i < 3; ++i) mouseDown_[i] = false;
    mouseButton_ = -1;
}

void App::onKey(int vk, bool down) {
    if (vk >= 256) return;
    if (down && (keys_[vk] = down)) return; // ignore auto-repeat
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
    case 'N':
        sim_.setReversed(!sim_.reversed());
        pushFlash(sim_.reversed() ? "Time reversed" : "Time forward");
        break;
    case 'F':
        follow_ = !follow_;
        pushFlash(follow_ ? "Camera follows focus" : "Camera free");
        break;
    case 'G':
        ui_.showCharts = !ui_.showCharts;
        break;
    case 'H':
        ui_.helpOpen = !ui_.helpOpen;
        break;
    case VK_BACK:
        if (ui_.findEditActive && !ui_.findQuery.empty())
            ui_.findQuery.pop_back();
        else if (ui_.addEditActive && !ui_.addQuery.empty())
            ui_.addQuery.pop_back();
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
        if (UIPanels::anyPanelOpen(ui_) || ui_.findEditActive || ui_.addEditActive)
            uiCloseTopPanel();
        else
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

void App::onChar(char c) {
    if (ui_.findEditActive) {
        if (ui_.findQuery.size() < 24) ui_.findQuery += c;
    } else if (ui_.addEditActive) {
        if (ui_.addQuery.size() < 24) ui_.addQuery += c;
    }
}

void App::onMouseDown(int button) {
    if (button < 0 || button > 2) return;
    Window::cursorPos((int&)mouseX_, (int&)mouseY_);
    mouseDown_[button] = true;
    mouseButton_ = button;
    downX_ = (float)mouseX_;
    downY_ = (float)mouseY_;
    dragged_ = false;

    gui_.onPress(button, mouseX_, mouseY_);
    uiPress_[button] = gui_.pressInUI();
}

void App::onMouseUp(int button) {
    if (button < 0 || button > 2) return;
    if (!mouseDown_[button]) return;
    Window::cursorPos((int&)mouseX_, (int&)mouseY_);
    mouseDown_[button] = false;
    const bool wasMouseBtn = (mouseButton_ == button);
    if (wasMouseBtn) mouseButton_ = -1;

    gui_.onRelease(button, mouseX_, mouseY_);

    if (button == 0 && wasMouseBtn && !uiPress_[0] && !dragged_) {
        const int id = pickBodyAtCursor();
        const int nowMs = (int)GetTickCount();
        const bool dbl = (nowMs - lastClickTimeMs_ < 350);
        lastClickTimeMs_ = nowMs;
        if (id >= 0) {
            selectAndFocus(id, dbl);
            ui_.infoOpen = true;
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

    // A press that started inside the GUI never drives the camera.
    if (uiPress_[0] || uiPress_[1] || uiPress_[2]) return;

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
    const int w = window_.clientWidth();
    const int h = window_.clientHeight();

    // Wheel over the Find list scrolls it.
    if (ui_.findOpen &&
        UIPanels::findPanel(w, h).contains(mouseX_, mouseY_)) {
        ui_.findScroll = std::max(0, ui_.findScroll + (delta > 0 ? -1 : 1));
        return;
    }
    // Wheel over the time bar adjusts speed.
    if (UIPanels::timeBar(w, h).contains(mouseX_, mouseY_)) {
        timeSpeed_ = clampf(timeSpeed_ * (delta > 0 ? 1.4f : 1.0f / 1.4f),
                            0.1f, 32.0f);
        return;
    }
    // Wheel over any open panel: no camera zoom.
    if (UIPanels::pointInPanel(ui_, w, h, mouseX_, mouseY_))
        return;

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
        b.realRadiusAU = 0.00465f * std::sqrt(b.mass);
        b.color = hsl2rgb(randRange(0.05f, 0.14f), 0.55f, 0.62f);
        b.temperatureC = 5500.0f;
        dist = clampf(cam_.distance() * 0.9f, 1.2f, 12.0f);
        b.pos = anchorPos + radial * dist;
        b.vel = anchorVel * 0.3f + radial * 0.04f;
        break;
    case BodyType::Asteroid:
        b.mass = randRange(4e-9f, 8e-8f);
        b.radius = 0.006f + randRange(0.0f, 0.006f);
        b.realRadiusAU = b.radius * 0.002f;
        b.color = hsl2rgb(randRange(0.0f, 1.0f), 0.2f, 0.45f);
        b.temperatureC = -100.0f;
        dist = clampf(randRange(0.08f, 0.6f), 0.05f, 1.5f);
        b.pos = anchorPos + radial * dist;
        b.vel = anchorVel + tangent * std::sqrt(G * std::max(anchorMass, 1e-6f) / dist) * 0.96f;
        break;
    default: // Planet / Moon
        b.mass = randRange(0.3e-6f, 6.0e-6f);
        b.radius = clampf(0.005f * std::cbrt(b.mass / 3.0e-6f), 0.006f, 0.032f);
        b.realRadiusAU = 4.26e-5f * std::cbrt(b.mass / 3.0e-6f);
        b.color = hsl2rgb(randRange(0.0f, 1.0f), 0.45f, 0.55f);
        b.bandFreq = (randRange(0.0f, 1.0f) < 0.35f) ? randRange(3.0f, 7.0f) : 0.0f;
        b.temperatureC = randRange(-50.0f, 50.0f);
        dist = clampf(dist, 0.15f, 6.0f);
        b.pos = anchorPos + radial * dist;
        b.vel = anchorVel + tangent * std::sqrt(G * std::max(anchorMass, 1e-6f) / dist);
        break;
    }
    b.roughness = randRange(0.2f, 0.8f);
    b.spin = randRange(0.3f, 1.0f);

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

void App::loadPreset(int which) {
    switch (which) {
    case 0:  Presets::solarSystem(sim_);        break;
    case 1:  Presets::binary(sim_);             break;
    case 2:  Presets::asteroidBelt(sim_, 400);  break;
    case 3:  Presets::starAndMoon(sim_);        break;
    default: Presets::randomUniverse(sim_);     break;
    }
    simName_ = presetName(which);
    selectedUid_ = -1;
    {
        const int p = sim_.primaryBody();
        focusedUid_ = (p >= 0) ? sim_.body(p)->uid : -1;
    }
    follow_ = true;
    paused_ = false;
    sim_.setReversed(false);
    timeSpeed_ = 1.0f;
    sim_.setImpactsEnabled(ui_.impactsOn);
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
//  UI actions
// ---------------------------------------------------------------------------
void App::uiCloseTopPanel() {
    if (ui_.findEditActive || ui_.addEditActive) {
        ui_.findEditActive = false;
        ui_.addEditActive = false;
    } else if (ui_.helpOpen) {
        ui_.helpOpen = false;
    } else if (ui_.addOpen) {
        ui_.addOpen = false;
    } else if (ui_.toolsOpen) {
        ui_.toolsOpen = false;
    } else if (ui_.findOpen) {
        ui_.findOpen = false;
    } else if (ui_.simOpen) {
        ui_.simOpen = false;
    } else if (ui_.viewOpen) {
        ui_.viewOpen = false;
    } else if (selId() >= 0) {
        ui_.infoOpen = false;
    }
}

void App::uiSelectUid(int uid) {
    const int id = sim_.bodyByUid(uid);
    if (id < 0) return;
    selectAndFocus(id, true);
    ui_.infoOpen = true;
}

void App::uiSpawnCatalog(int index, bool isScenario) {
    if (isScenario) {
        loadPreset(index);
        pushFlash("Loaded " + simName_);
        if (!ui_.addRepeat) ui_.addOpen = false;
        return;
    }

    const CatalogItem& it = catalogBody(index);
    const float G = sim_.config().G;
    const int anchorId = (focId() >= 0) ? focId() : sim_.primaryBody();
    Body* anchor = (anchorId >= 0) ? sim_.body(anchorId) : nullptr;
    const Vec3 anchorPos = anchor ? anchor->pos : Vec3::Zero();
    const Vec3 anchorVel = anchor ? anchor->vel : Vec3::Zero();
    const float M1 = anchor ? anchor->mass : 0.0f;

    Body b;
    b.type = it.type;
    b.name = it.name;
    b.mass = (it.massLo < it.massHi) ? randRange(it.massLo, it.massHi) : it.massLo;

    if (it.type == BodyType::Star) {
        b.radius = 0.045f + 0.06f * std::cbrt(b.mass);
        b.realRadiusAU = 0.00465f * std::sqrt(b.mass);
        b.temperatureC = (b.mass < 0.2f) ? 4300.0f
                          : (b.mass < 0.8f ? 5800.0f : 7200.0f);
        b.color = hsl2rgb(randRange(0.03f, 0.14f), 0.8f, 0.6f);
        b.spin = randRange(0.2f, 0.6f);
    } else {
        b.radius = (it.dispRLo < it.dispRHi)
            ? randRange(it.dispRLo, it.dispRHi) : it.dispRLo;
        b.realRadiusAU = it.realR;
        if (it.sat < 0.0f) {
            b.color = hsl2rgb(randRange(0.0f, 1.0f), 0.45f, 0.55f);
            b.bandFreq = (it.type == BodyType::Planet &&
                          randRange(0.0f, 1.0f) < 0.35f)
                ? randRange(3.0f, 6.0f) : 0.0f;
            b.temperatureC = randRange(-40.0f, 60.0f);
        } else {
            b.color = hsl2rgb(it.hue, it.sat, it.lum);
            b.bandFreq = (it.bandFreq > 0.0f) ? it.bandFreq : 0.0f;
            b.temperatureC = it.tempC;
        }
        b.spin = randRange(0.4f, 1.2f);
    }
    b.surface = it.surface;
    b.roughness = randRange(0.2f, 0.8f);

    const float ang = randRange(0.0f, 2.0f * 3.14159265f);
    const Vec3 radial(std::cos(ang), 0.0f, std::sin(ang));
    const Vec3 tangent(-std::sin(ang), 0.0f, std::cos(ang));
    const float dist = randRange(it.distLo, it.distHi);
    b.pos = anchorPos + radial * dist;

    if (ui_.addBinary && M1 > 0.0f) {
        // Mutual circular orbit about the barycenter (momentum conserved).
        const float vRel = std::sqrt(G * (M1 + b.mass) / dist);
        b.vel = anchorVel - tangent * vRel * (M1 / (M1 + b.mass));
        anchor->vel += tangent * vRel * (b.mass / (M1 + b.mass));
    } else if (ui_.addAutoOrbit && M1 > 0.0f) {
        b.vel = anchorVel + tangent * std::sqrt(G * M1 / dist);
    } else {
        b.vel = anchorVel;
    }

    const int id = sim_.addBody(b);
    selectAndFocus(id, false);
    char msg[96];
    std::snprintf(msg, sizeof msg, "+ %s (%s)", b.name.c_str(), bodyTypeName(b.type));
    pushFlash(msg);
    if (!ui_.addRepeat) ui_.addOpen = false;
}

void App::uiTool(int id) {
    switch (id) {
    case 8:
        follow_ = true;
        pushFlash("Camera follows focus");
        return;
    case 9:
        camResetView();
        return;
    default:
        break;
    }

    const int fid = (focId() >= 0) ? focId() : sim_.primaryBody();
    Body* b = (fid >= 0) ? sim_.body(fid) : nullptr;
    if (!b) {
        pushFlash("No focused world");
        return;
    }

    switch (id) {
    case 0: // Terraform
        if (b->isStar()) {
            pushFlash("Stars have no surface");
            break;
        }
        b->surface = (b->surface + 1) % 4;
        {
            static const char* names[4] = { "rock", "water", "ice", "air" };
            char msg[48];
            std::snprintf(msg, sizeof msg, "Terraformed to %s", names[b->surface]);
            pushFlash(msg);
        }
        break;
    case 1: // Heat Beam
        b->temperatureC = clampf(b->temperatureC + 800.0f, -273.0f, 12000.0f);
        pushFlash("Heated +800 C");
        break;
    case 2: // Torque
        b->spin = clampf(b->spin + 0.5f, -8.0f, 8.0f);
        pushFlash("Torque applied");
        break;
    case 3: // Detonate
        detonateBody(fid);
        break;
    case 4: { // Slingshot
        const int p = sim_.primaryBody();
        Vec3 out = Vec3(1.0f, 0.0f, 0.0f);
        const Vec3 pv = (p >= 0 && p != fid && sim_.body(p))
            ? sim_.body(p)->vel : Vec3::Zero();
        if (p >= 0 && p != fid && sim_.body(p))
            out = normalize(b->pos - sim_.body(p)->pos);
        const float sp = std::max(length(b->vel - pv), 0.05f);
        b->vel += out * sp * 0.5f;
        pushFlash("Slingshot");
        break;
    }
    case 5: { // Brake
        const int p = sim_.primaryBody();
        if (p >= 0 && p != fid && sim_.body(p)) {
            const Vec3 pv = sim_.body(p)->vel;
            b->vel = (b->vel - pv) * 0.15f + pv;
        } else {
            b->vel *= 0.15f;
        }
        pushFlash("Braked");
        break;
    }
    case 6: { // Circularize
        const int p = sim_.primaryBody();
        if (p < 0 || p == fid || !sim_.body(p)) break;
        const Body* pr = sim_.body(p);
        const Vec3 r = b->pos - pr->pos;
        const float d = length(r);
        if (d < 1e-4f) break;
        const Vec3 rad = r / d;
        const Vec3 vrel = b->vel - pr->vel;
        Vec3 tang = vrel - rad * dot(vrel, rad);
        if (length(tang) < 1e-4f) tang = normalize(cross(Vec3(0, 1, 0), rad));
        tang = normalize(tang);
        const float G = sim_.config().G;
        b->vel = pr->vel + tang * std::sqrt(G * pr->mass / d);
        pushFlash("Orbit circularized");
        break;
    }
    case 7: { // Stop orbit
        const int p = sim_.primaryBody();
        if (p >= 0 && p != fid && sim_.body(p))
            b->vel = sim_.body(p)->vel;
        else
            b->vel = Vec3::Zero();
        pushFlash("Orbit stopped");
        break;
    }
    default:
        break;
    }
}

void App::uiToolRight(int id) {
    const int fid = (focId() >= 0) ? focId() : sim_.primaryBody();
    Body* b = (fid >= 0) ? sim_.body(fid) : nullptr;
    if (!b) return;
    switch (id) {
    case 1: // Heat Beam (cool)
        b->temperatureC = clampf(b->temperatureC - 800.0f, -273.0f, 12000.0f);
        pushFlash("Cooled -800 C");
        break;
    case 2: // Torque (reverse)
        b->spin = clampf(b->spin - 0.5f, -8.0f, 8.0f);
        pushFlash("Torque reversed");
        break;
    default:
        break;
    }
}

void App::detonateBody(int fid) {
    const Body* src = sim_.body(fid);
    if (!src) return;

    const int n = 8 + (src->uid % 7);
    std::vector<Vec3> dirs((size_t)n);
    Vec3 mean = Vec3::Zero();
    for (int i = 0; i < n; ++i) {
        const float u = randRange(-1.0f, 1.0f);
        const float a = randRange(0.0f, 2.0f * 3.14159265f);
        const float s = std::sqrt(std::max(0.0f, 1.0f - u * u));
        dirs[(size_t)i] = Vec3(s * std::cos(a), u, s * std::sin(a));
        mean += dirs[(size_t)i];
    }
    mean = mean / (float)n;

    const float fragMass = src->mass / (float)n;
    const float baseR =
        std::max(src->radius * std::cbrt(1.0f / (float)n) * 0.8f, 0.003f);

    sim_.removeBody(fid);
    if (selectedUid_ == src->uid) selectedUid_ = -1;
    if (focusedUid_ == src->uid) {
        const int p = sim_.primaryBody();
        focusedUid_ = (p >= 0) ? sim_.body(p)->uid : -1;
    }

    for (int i = 0; i < n; ++i) {
        Body f;
        f.type = (fragMass < 1e-5f) ? BodyType::Asteroid : BodyType::Planet;
        f.name = src->name + "-" + std::to_string(i + 1);
        f.mass = fragMass;
        f.radius = baseR * randRange(0.7f, 1.3f);
        f.realRadiusAU = src->realRadiusAU * std::cbrt(1.0f / (float)n);
        f.color = src->color * randRange(0.8f, 1.2f);
        f.surface = 0;
        f.temperatureC = src->temperatureC * 0.5f + 300.0f;
        f.roughness = randRange(0.4f, 0.9f);

        Vec3 d = dirs[(size_t)i] - mean * 0.8f;
        if (length(d) < 1e-3f)
            d = Vec3(randRange(-1.f, 1.f), randRange(-1.f, 1.f), randRange(-1.f, 1.f));
        d = normalize(d);
        f.pos = src->pos + d * src->radius * 0.4f;
        f.vel = src->vel + d * (0.02f + 0.10f * src->radius + randRange(0.0f, 0.03f));
        sim_.addBody(f);
    }
    char msg[96];
    std::snprintf(msg, sizeof msg, "Detonated %s into %d fragments",
                  src->name.c_str(), n);
    pushFlash(msg);
}

void App::camGoTo() {
    const int tid = (selId() >= 0) ? selId()
                     : (focId() >= 0 ? focId() : sim_.primaryBody());
    if (tid < 0) {
        pushFlash("Nothing to go to");
        return;
    }
    const Body* b = sim_.body(tid);
    focusedUid_ = b->uid;
    follow_ = true;
    cam_.setFocusPoint(b->pos);
    cam_.setDistance(std::max(b->radius * 10.0f, 0.1f));
    pushFlash("Went to " + b->name);
}

void App::camSeeAll() {
    Vec3 c = Vec3::Zero();
    int n = 0;
    for (const Body& b : sim_.bodies()) {
        if (b.dead) continue;
        c += b.pos;
        ++n;
    }
    if (n == 0) return;
    c = c / (float)n;
    float maxR = 0.0f;
    for (const Body& b : sim_.bodies()) {
        if (b.dead) continue;
        maxR = std::max(maxR, length(b.pos - c) + b.radius);
    }
    follow_ = false;
    cam_.setFocusPoint(c);
    cam_.setDistance(clampf(maxR * 2.6f, 0.3f, Camera::kMaxDist));
    pushFlash("Viewing the whole system");
}

void App::camResetView() {
    cam_.setPose(0.6f, 0.42f, 2.4f);
    const int p = sim_.primaryBody();
    if (p >= 0) {
        cam_.setFocusPoint(sim_.body(p)->pos);
        focusedUid_ = sim_.body(p)->uid;
        follow_ = true;
    }
    pushFlash("Camera reset");
}

// ---------------------------------------------------------------------------
//  Rendering
// ---------------------------------------------------------------------------
void App::applyColourBy() {
    if (ui_.colourBy == 0) {
        for (Body& b : sim_.bodies())
            if (!b.dead) b.displayColor = b.color;
        return;
    }
    for (Body& b : sim_.bodies()) {
        if (b.dead) continue;
        if (ui_.colourBy == 1) {
            const float t = clampf((std::log10(std::max(b.mass, 1e-12f)) + 9.0f) / 21.0f,
                                   0.0f, 1.0f);
            b.displayColor = lerp(Vec3(0.15f, 0.3f, 0.9f),
                                  Vec3(1.0f, 1.0f, 1.0f), t);
        } else if (ui_.colourBy == 2) {
            const float t = clampf(length(b.vel) / 8.0f, 0.0f, 1.0f);
            b.displayColor = lerp(Vec3(0.9f, 0.3f, 0.1f),
                                  Vec3(1.0f, 1.0f, 1.0f), t);
        } else {
            // thermal ramp: dark -> red -> yellow -> white
            const float t = clampf((b.temperatureC + 273.0f) / 6000.0f, 0.0f, 1.0f);
            if (t < 0.33f)
                b.displayColor = lerp(Vec3(0.05f, 0.02f, 0.05f),
                                      Vec3(1.0f, 0.15f, 0.05f), t / 0.33f);
            else if (t < 0.66f)
                b.displayColor = lerp(Vec3(1.0f, 0.15f, 0.05f),
                                      Vec3(1.0f, 0.85f, 0.2f),
                                      (t - 0.33f) / 0.33f);
            else
                b.displayColor = lerp(Vec3(1.0f, 0.85f, 0.2f),
                                      Vec3(1.0f, 1.0f, 1.0f),
                                      (t - 0.66f) / 0.34f);
        }
    }
}

void App::renderFrame() {
    const int w = window_.clientWidth();
    const int h = window_.clientHeight();
    if (w <= 0 || h <= 0) return;

    applyColourBy();
    renderer_.setFovDeg(ui_.fovDeg);

    ViewOptions vo;
    vo.clouds = ui_.showClouds ? 1.0f : 0.0f;
    vo.atmos = ui_.showAtmospheres ? 1.0f : 0.0f;
    vo.heatGlow = ui_.heatOn ? 1.0f : 0.0f;
    vo.showDebris = ui_.debrisOn;
    if (ui_.lampOn) {
        vo.lampOn = true;
        const int fid = focId();
        const Vec3 base = (fid >= 0) ? sim_.body(fid)->pos : cam_.target();
        vo.lampPos = ui_.lampFromRight ? (base + cam_.right() * ui_.lampReach)
                                       : cam_.position();
        vo.lampColor = kelvin2rgb(ui_.lampKelvin);
        vo.lampStrength = ui_.lampStrength;
    }

    renderer_.render(cam_, sim_, selId(), ui_.showCharts, timeSec_, w, h, vo);

    // UI panels (mouse UI)
    gui_.begin(w, h, renderer_.fontTexture(), mouseX_, mouseY_, mouseDown_);
    UIPanels::drawAll(gui_, *this);
    gui_.end();

    // Flashes + pause banner (on top)
    drawHud();
}

void App::drawHud() {
    const int w = window_.clientWidth();
    const int h = window_.clientHeight();
    hud_.begin(w, h);

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
