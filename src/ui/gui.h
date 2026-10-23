// ============================================================================
//  Universe Sandbox 3D - immediate-mode GUI (panels, buttons, sliders...)
//
//  Draw order is Z order: widgets drawn later sit on top and block clicks.
//  Panels register a "block rect" when their background is painted, so
//  widgets beneath never receive hover/click events landing on a panel
//  that is in front of them.
// ============================================================================
#pragma once

#include "gl/gl33.h"
#include "core/math.h"
#include "render/renderer.h"   // Program helper
#include <string>
#include <vector>

namespace us3d {

class Gui {
public:
    enum IconKind {
        INone = 0, IPlus, IWrench, ISearch, ISliders, IPlay, IPause,
        IRewind, IForward, IClose, IChevron, IEye, IGear, ICamera
    };

    Gui() = default;
    ~Gui();

    bool init();   // creates the text + icon GL programs
    void shutdown();

    // Per-frame setup. mouseX/mouseY in client px (top-left origin).
    void begin(int w, int h, GLuint fontTex,
               int mouseX, int mouseY, const bool mouseDown[3]);
    void end();

    // App feeds raw button events between frames (client px).
    void onPress(int btn, int x, int y);
    void onRelease(int btn, int x, int y);

    // Consumes a left click that started and ended inside the rect, without
    // drawing anything. Returns true when this call got the click.
    bool clickedAt(float x, float y, float w, float h);

    // Input state (for popup menus).
    bool hadRelease() const { return release_; }
    bool releaseConsumed() const { return releaseUsed_; }
    int  releaseButton() const { return releaseBtn_; }

    // ---- geometry / input state -------------------------------------
    int  width()  const { return w_; }
    int  height() const { return h_; }
    bool mouseInside(float x, float y, float ww, float hh) const {
        return mouseX_ >= x && mouseX_ < x + ww &&
               mouseY_ >= y && mouseY_ < y + hh;
    }

    // True when the current mouse-down press started inside an already
    // painted panel (suppresses 3D camera / pick actions).
    bool pressInUI() const { return pressInUI_; }

    // ---- primitives ---------------------------------------------------
    void panel(float x, float y, float w, float h,
               float bgR = 0.075f, float bgG = 0.085f, float bgB = 0.105f,
               float bgA = 0.94f);

    void text(float x, float y, const std::string& s, float px,
              float r, float g, float b, float a = 1.0f);
    static float measure(const std::string& s, float px);
    void rect(float x, float y, float w, float h,
              float r, float g, float b, float a);
    void line(float x0, float y0, float x1, float y1,
              float r, float g, float b, float a, float thick = 1.0f);
    void icon(float x, float y, float size, IconKind kind,
              float r, float g, float b, float a = 1.0f);

    // Shaded sphere "disc" (planet thumbnail) - center (cx,cy), radius px.
    void disc(float cx, float cy, float rPx, float cr, float cg, float cb,
              float bandFreq, float emissive);

    // ---- widgets --------------------------------------------------------
    // Clickable button; returns true when clicked this frame.
    bool button(float x, float y, float w, float h, const std::string& label,
                IconKind icon = INone, float fontPx = 1.6f,
                float r = 0.13f, float g = 0.15f, float b = 0.19f,
                float textR = 0.85f, float textG = 0.88f, float textB = 0.95f);

    // Pill / tab: active state highlighted.
    bool pill(float x, float y, float w, float h, const std::string& label,
              bool active);

    // Toggle chip.
    bool chip(float x, float y, float w, float h, const std::string& label,
              bool& on);

    // Slider bound to [lo,hi]; returns true while the user drags it.
    bool slider(float x, float y, float w, float& value, float lo, float hi);

    // Value field row (Info panel): label + boxed value text.
    void field(float x, float y, float w, const std::string& label,
               const std::string& value);

    // List row; returns true when clicked.
    bool row(float x, float y, float w, float h, bool hovered);

    // Text input box; when active it should receive characters (App routes
    // them into `buf`). Clicking the box toggles `active`.
    void inputBox(float x, float y, float w, float h, std::string& buf,
                  bool& active, const std::string& placeholder);

    void section(float x, float y, const std::string& s);

private:
    struct BlockRect { float x, y, w, h; int z; };

    bool wasClickedAt(float x, float y, float w, float h) const;

    void pushText(float x, float y, float u, float t,
                  float r, float g, float b, float a);
    void flushText();
    void pushIconQuad(float x, float y, float u, float t,
                      float r, float g, float b,
                      float p0, float p1);
    void flushIcons();

    Program textProg_;
    Program iconProg_;
    GLuint textVao_ = 0, textVbo_ = 0;
    GLuint iconVao_ = 0, iconVbo_ = 0;
    GLuint fontTex_ = 0;

    std::vector<float> tv_;   // text verts: pos2 uv2 col4
    std::vector<float> iv_;   // icon verts: pos2 uv2 col4 params2

    int w_ = 1280, h_ = 720;
    int mouseX_ = -1, mouseY_ = -1;
    bool mouseDown_[3] = { false, false, false };
    int zCounter_ = 0;

    // press tracking
    bool  pressActive_[3] = { false, false, false };
    float pressX_[3] = { -1e9f, -1e9f, -1e9f };
    float pressY_[3] = { -1e9f, -1e9f, -1e9f };

    // release this frame
    bool  release_ = false;
    int   releaseBtn_ = -1;
    float releaseX_ = 0, releaseY_ = 0;
    bool  releaseUsed_ = false;

    bool pressInUI_ = false;
    std::vector<BlockRect> blocks_;
};

} // namespace us3d
