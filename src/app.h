// ============================================================================
//  Universe Sandbox 3D - application controller
//
//  Owns the window, simulation, camera, renderer, HUD and GUI; turns input
//  into simulation actions and drives the main loop.
// ============================================================================
#pragma once

#include "core/presets.h"
#include "core/savestate.h"
#include "core/system.h"
#include "render/camera.h"
#include "render/hud.h"
#include "render/renderer.h"
#include "ui/gui.h"
#include "ui/panels.h"
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
    friend class UIPanels;

    // ---- input callbacks (bound to Window) ----
    void onResize(int w, int h);
    void onClose();
    void onKey(int vk, bool down);
    void onChar(char c);
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
    void applyColourBy();
    void pushFlash(const std::string& msg);

    // ---- actions ----
    int pickBodyAtCursor();
    void selectAndFocus(int id, bool zoomIn);
    void spawnAroundFocus(BodyType type);
    void deleteSelected();
    void loadPreset(int which);
    void doSave();
    void doLoad();

    // ---- UI actions (called from UIPanels) ----
    void uiSpawnCatalog(int index, bool isScenario);
    void uiTool(int id);
    void uiToolRight(int id);
    void uiSelectUid(int uid);
    void uiCloseTopPanel();
    void detonateBody(int fid);
    void camGoTo();
    void camSeeAll();
    void camResetView();

    // ---- state ----
    Window window_;
    System sim_;
    Camera cam_;
    Renderer renderer_;
    Hud hud_;
    Gui gui_;

    HINSTANCE hInstance_ = nullptr;
    bool running_ = true;
    bool inited_ = false;

    // UI state (panels, toggles, sliders) + persistent text.
    UIState ui_;

    // Tracked by stable uid (survives renumbering after merges/deletes).
    int selectedUid_ = -1;
    int focusedUid_  = -1;
    bool follow_     = true;

    // Compact index helpers (uid -> current index, -1 if gone).
    int selId() const { return sim_.bodyByUid(selectedUid_); }
    int focId() const { return sim_.bodyByUid(focusedUid_); }

    bool paused_     = false;
    float timeSpeed_ = 1.0f;      // 0.1 .. 32
    bool showHelp_   = true;

    std::string simName_ = "Solar System";

    float timeSec_ = 0.0f;        // wall clock (shader animation)
    float fps_     = 60.0f;

    std::vector<std::pair<std::string, float>> flashes_; // text + ttl

    // mouse
    int mouseX_ = 0, mouseY_ = 0;
    bool mouseDown_[3] = { false, false, false };
    int  mouseButton_  = -1;
    float downX_ = 0.0f, downY_ = 0.0f;
    bool dragged_ = false;
    bool uiPress_[3] = { false, false, false };  // press started inside GUI
    int  lastClickTimeMs_ = -10000;
    bool keys_[256] = { false };
};

} // namespace us3d
