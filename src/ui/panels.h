// ============================================================================
//  Universe Sandbox 3D - UI panels
//
//  The full mouse UI modelled on the reference layout:
//    * time bar (bottom-left): pause / reverse / date / speed
//    * bottom bar: Add, Tools, Find (center) - Sim, View (right)
//    * Add panel     - object catalog + Target/Binary/Auto orbit/Repeat
//    * Tools panel   - Terraform, Heat Beam, Torque, Detonate, ...
//    * Find panel    - searchable body list
//    * Sim panel     - scenario info, toggles, stats
//    * View panel    - camera, FOV, worlds, colour-by, lamp
//    * Info panel    - selected body: physical / materials / climate /
//                      orbit / appearance
// ============================================================================
#pragma once

#include "core/catalog.h"
#include <string>

namespace us3d {

class App;
class Gui;

// Tool card (Tools panel).
struct ToolDef {
    int id;
    const char* title;
    const char* d1;
    const char* d2;
    float r, g, b;
};

// Pre-computed Info-panel values.
struct InfoValues {
    std::string mass, radius, density, temperature, gravity, pressure,
                luminosity, speed, rotation, orbital, age, stage, spectral,
                fragments, aroundName;
    float distAU = 0.0f;
    float speedRelKms = 0.0f;
    float periodYr = 0.0f;
    float aAU = 0.0f;
    float ecc = 0.0f;
    bool hasOrbit = false;
};

struct RectF {
    float x, y, w, h;
    RectF() : x(0), y(0), w(0), h(0) {}
    RectF(float _x, float _y, float _w, float _h)
        : x(_x), y(_y), w(_w), h(_h) {}
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// All persistent UI settings + transient panel state.
struct UIState {
    // open flags
    bool addOpen   = false;
    bool toolsOpen = false;
    bool findOpen  = false;
    bool simOpen   = false;
    bool viewOpen  = false;
    bool infoOpen  = true;     // info shows while a body is selected
    bool helpOpen  = false;

    // Add panel
    int   addTab = 0;          // 0 All, 1 Moons, 2 Saved, 3 Custom
    bool  addBinary = false;
    bool  addAutoOrbit = true;
    bool  addRepeat = false;
    std::string addQuery;
    bool  addEditActive = false;

    // Tools panel
    int   toolsTab = 0;        // 0 All, 1 World, 2 Launch, 3 Arrange, 4 Camera

    // Find panel
    int   findTab = 0;         // 0 All, 1 Stars, 2 Planets, 3 Moons, 4 Small
    std::string findQuery;
    bool  findEditActive = false;
    int   findScroll = 0;

    // View panel
    float fovDeg = 50.0f;
    bool  showAtmospheres = true;
    bool  showClouds = false;
    bool  showCharts = true;   // motion trails
    int   colourBy = 0;        // 0 true colour, 1 mass, 2 speed, 3 temperature
    bool  colourByMenu = false;
    bool  lampOn = false;
    bool  lampFromRight = true;
    float lampStrength = 1.0f;
    float lampReach = 2.0f;    // AU
    float lampKelvin = 6500.0f;

    // Sim panel
    bool  impactsOn = true;
    bool  debrisOn = true;
    bool  heatOn = false;      // show heat glow
    int   simView = 0;         // 0 Overview, 1 Advanced, 2 Stats

    // Info panel
    int   infoTab = 0;         // 0 Physical, 1 Materials, 2 Climate,
                               // 3 Orbit, 4 Appearance
};

class UIPanels {
public:
    // Draws every visible panel. Call with the Gui inside its begin/end.
    static void drawAll(Gui& gui, App& app);

    // Layout queries (used by App for wheel routing).
    static RectF timeBar(int w, int h);
    static RectF findPanel(int w, int h);
    static bool anyPanelOpen(const UIState& s);
    static bool pointInPanel(const UIState& s, int w, int h, int px, int py);

private:
    static void drawTimeBar(Gui& gui, App& app);
    static void drawBottomBar(Gui& gui, App& app);
    static void drawAddPanel(Gui& gui, App& app);
    static void drawToolsPanel(Gui& gui, App& app);
    static void drawFindPanel(Gui& gui, App& app);
    static void drawSimPanel(Gui& gui, App& app);
    static void drawViewPanel(Gui& gui, App& app);
    static bool drawColourMenu(Gui& gui, App& app);
    static void drawInfoPanel(Gui& gui, App& app);
    static void drawTopLeft(Gui& gui, App& app);
    static void drawHelpPanel(Gui& gui, App& app);
    static void drawToolCard(Gui& gui, App& app, float x, float y, float w,
                             float h, const ToolDef& t, bool compact);
    static InfoValues computeInfo(const App& app, const Body& b);
};

} // namespace us3d
