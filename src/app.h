// ============================================================================
//  Universe Sandbox 3D - application controller
//
//  Owns the window, simulation, camera, renderer and HUD; turns input into
//  simulation actions and drives the main loop.
// ============================================================================
#pragma once

#include "core/presets.h"
#include "core/savestate.h"
#include "core/system.h"
#include "render/camera.h"
#include "render/hud.h"
#include "render/renderer.h"
#include "win32/window.h"

#include <string>
#include <vector>

namespace us3d {

class App {
public:
    App() = default;

    bool init(HINSTANCE hInstance);
    void run();
    void shutdown();
    void requestQuit() { running_ = false; }

private:
    // ---- input callbacks (bound to Window) ----
    void onResize(int w, int h);
    void onClose();
    void onKey(int vk, bool down);
    void onMouseDown(int button);
    void onMouseUp(int button);
    void onMouseMove(int x, int y, int dx, int dy);
    void onWheel(int delta);
    void onFocusLost();

    // ---- per-frame ----
    void processInput(float dt);
    void update(float dt);
    void renderFrame();
    void drawHud();
    void pushFlash(const std::string& msg);

    // ---- actions ----
    int pickBodyAtCursor();
    void selectAndFocus(int id, bool zoomIn);
    void spawnAroundFocus(BodyType type);
    void deleteSelected();
    void loadPreset(int which);
    void doSave();
    void doLoad();
    void resetCameraForPreset();

    // ---- state ----
    Window window_;
    System sim_;
    Camera cam_;
    Renderer renderer_;
    Hud hud_;

    HINSTANCE hInstance_ = nullptr;
    bool running_ = true;
    bool inited_ = false;

    // Tracked by stable uid (survives renumbering after merges/deletes).
    int selectedUid_ = -1;
    int focusedUid_  = -1;
    bool follow_     = true;

    // Compact index helpers (uid -> current index, -1 if gone).
    int selId() const { return sim_.bodyByUid(selectedUid_); }
    int focId() const { return sim_.bodyByUid(focusedUid_); }

    bool paused_     = false;
    float timeSpeed_ = 1.0f;      // 0.1 .. 32
    bool showTrails_ = true;
    bool showHelp_   = true;

    float timeSec_ = 0.0f;        // wall clock (shader animation)
    float fps_     = 60.0f;

    std::vector<std::pair<std::string, float>> flashes_; // text + ttl

    // mouse
    int mouseX_ = 0, mouseY_ = 0;
    bool mouseDown_[3] = { false, false, false };
    int  mouseButton_  = -1;
    float downX_ = 0.0f, downY_ = 0.0f;
    bool dragged_ = false;
    int  lastClickTimeMs_ = -10000;
    bool keys_[256] = { false };
};

} // namespace us3d
