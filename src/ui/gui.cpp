// ============================================================================
//  Universe Sandbox 3D - immediate-mode GUI (implementation)
// ============================================================================
#include "ui/gui.h"

#include "core/shaders.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace us3d {

namespace {
// Font atlas layout (must match Renderer::buildFontTexture / Hud).
const int kCols = 16, kRows = 6;
const int kCellW = 7, kCellH = 9;
const int kAtlasW = kCols * kCellW;   // 112
const int kAtlasH = kRows * kCellH;   // 54
} // namespace

Gui::~Gui() { shutdown(); }

bool Gui::init() {
    std::string err;
    if (!textProg_.create(shaders::ui_vs, shaders::ui_fs, "ui-text", err))
        return false;
    if (!iconProg_.create(shaders::icon_vs, shaders::icon_fs, "ui-icon", err))
        return false;

    gl33::glGenVertexArrays(1, &textVao_);
    gl33::glGenBuffers(1, &textVbo_);
    gl33::glBindVertexArray(textVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    gl33::glBufferData(GL_ARRAY_BUFFER, 2 * 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);
    const GLsizei stride = 8 * (GLsizei)sizeof(float);
    gl33::glEnableVertexAttribArray(0);
    gl33::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    gl33::glEnableVertexAttribArray(1);
    gl33::glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(size_t)(2 * sizeof(float)));
    gl33::glEnableVertexAttribArray(2);
    gl33::glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(size_t)(4 * sizeof(float)));
    gl33::glBindVertexArray(0);

    gl33::glGenVertexArrays(1, &iconVao_);
    gl33::glGenBuffers(1, &iconVbo_);
    gl33::glBindVertexArray(iconVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, iconVbo_);
    gl33::glBufferData(GL_ARRAY_BUFFER, 2 * 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);
    const GLsizei is = 10 * (GLsizei)sizeof(float);
    gl33::glEnableVertexAttribArray(0);
    gl33::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, is, (const void*)0);
    gl33::glEnableVertexAttribArray(1);
    gl33::glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, is, (const void*)(size_t)(2 * sizeof(float)));
    gl33::glEnableVertexAttribArray(2);
    gl33::glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, is, (const void*)(size_t)(4 * sizeof(float)));
    gl33::glEnableVertexAttribArray(3);
    gl33::glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, is, (const void*)(size_t)(8 * sizeof(float)));
    gl33::glBindVertexArray(0);
    return true;
}

void Gui::shutdown() {
    if (textVao_) gl33::glDeleteVertexArrays(1, &textVao_);
    if (textVbo_) gl33::glDeleteBuffers(1, &textVbo_);
    if (iconVao_) gl33::glDeleteVertexArrays(1, &iconVao_);
    if (iconVbo_) gl33::glDeleteBuffers(1, &iconVbo_);
    textVao_ = textVbo_ = iconVao_ = iconVbo_ = 0;
    textProg_.destroy();
    iconProg_.destroy();
}

void Gui::onPress(int btn, int x, int y) {
    if (btn < 0 || btn > 2) return;
    pressActive_[btn] = true;
    pressX_[btn] = (float)x;
    pressY_[btn] = (float)y;
    // Was the press inside an already painted panel?
    pressInUI_ = false;
    for (const BlockRect& b : blocks_) {
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
            pressInUI_ = true;
            break;
        }
    }
}

void Gui::onRelease(int btn, int x, int y) {
    if (btn < 0 || btn > 2) return;
    pressActive_[btn] = false;
    release_ = true;
    releaseBtn_ = btn;
    releaseX_ = (float)x;
    releaseY_ = (float)y;
}

void Gui::begin(int w, int h, GLuint fontTex,
                int mouseX, int mouseY, const bool mouseDown[3]) {
    w_ = w; h_ = h;
    fontTex_ = fontTex;
    mouseX_ = mouseX;
    mouseY_ = mouseY;
    for (int i = 0; i < 3; ++i) mouseDown_[i] = mouseDown[i];
    zCounter_ = 0;
    blocks_.clear();
    tv_.clear();
    iv_.clear();
    pressInUI_ = false;
    // NOTE: release_ / releaseUsed_ carry over from onRelease (called
    // between frames); they are cleared at the end of the frame.

    gl33::glDisable(GL_DEPTH_TEST);
    gl33::glDisable(GL_CULL_FACE);
    gl33::glEnable(GL_BLEND);
    gl33::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl33::glViewport(0, 0, w, h);
}

void Gui::end() {
    flushText();
    flushIcons();
    release_ = false;
    releaseBtn_ = -1;
    releaseUsed_ = false;
    gl33::glEnable(GL_DEPTH_TEST);
    gl33::glDisable(GL_BLEND);
}

float Gui::measure(const std::string& s, float px) {
    return (float)s.size() * 6.0f * px;
}

static inline void toNdc(int w, int h, float x, float y, float& nx, float& ny) {
    nx = (x / (float)w) * 2.0f - 1.0f;
    ny = 1.0f - (y / (float)h) * 2.0f;
}

void Gui::pushText(float x, float y, float u, float t,
                   float r, float g, float b, float a) {
    tv_.push_back(x); tv_.push_back(y); tv_.push_back(u); tv_.push_back(t);
    tv_.push_back(r); tv_.push_back(g); tv_.push_back(b); tv_.push_back(a);
}

void Gui::flushText() {
    if (tv_.empty()) return;
    textProg_.use();
    gl33::glActiveTexture(GL_TEXTURE0);
    gl33::glBindTexture(GL_TEXTURE_2D, fontTex_);
    gl33::glUniform1i(textProg_.uniform("uTex"), 0);
    gl33::glBindVertexArray(textVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    gl33::glBufferSubData(GL_ARRAY_BUFFER, 0,
                          (GLsizeiptr)(tv_.size() * sizeof(float)), tv_.data());
    gl33::glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(tv_.size() / 8));
    gl33::glBindVertexArray(0);
    tv_.clear();
}

void Gui::flushIcons() {
    if (iv_.empty()) return;
    iconProg_.use();
    gl33::glBindVertexArray(iconVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, iconVbo_);
    gl33::glBufferSubData(GL_ARRAY_BUFFER, 0,
                          (GLsizeiptr)(iv_.size() * sizeof(float)), iv_.data());
    gl33::glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(iv_.size() / 10));
    gl33::glBindVertexArray(0);
    iv_.clear();
}

void Gui::panel(float x, float y, float w, float h,
                float bgR, float bgG, float bgB, float bgA) {
    blocks_.push_back({ x, y, w, h, zCounter_++ });
    rect(x, y, w, h, bgR, bgG, bgB, bgA);
    // 1px border
    const float br = 0.20f, bg = 0.24f, bb = 0.32f;
    line(x, y, x + w, y, br, bg, bb, 0.8f, 1.0f);
    line(x, y + h, x + w, y + h, br, bg, bb, 0.8f, 1.0f);
    line(x, y, x, y + h, br, bg, bb, 0.8f, 1.0f);
    line(x + w, y, x + w, y + h, br, bg, bb, 0.8f, 1.0f);
}

void Gui::text(float x, float y, const std::string& s, float px,
               float r, float g, float b, float a) {
    const float cw = 5.0f * px;
    const float ch = 7.0f * px;
    for (char c : s) {
        const unsigned char code = (unsigned char)c;
        if (code < 32 || code > 126) { x += cw + px; continue; }
        const int col = (code - 32) % kCols;
        const int row = (code - 32) / kCols;
        const float u0 = (col * kCellW + 1) / (float)kAtlasW;
        const float u1 = (col * kCellW + 6) / (float)kAtlasW;
        const float vTop = (kAtlasH - (row * kCellH + 1)) / (float)kAtlasH;
        const float vBot = (kAtlasH - (row * kCellH + 8)) / (float)kAtlasH;

        const float x1 = x + cw, y1 = y + ch;
        float a0x, a0y, a1x, a1y;
        toNdc(w_, h_, x, y, a0x, a0y);
        toNdc(w_, h_, x1, y1, a1x, a1y);
        pushText(a0x, a0y, u0, vTop, r, g, b, a);
        pushText(a1x, a0y, u1, vTop, r, g, b, a);
        pushText(a0x, a1y, u0, vBot, r, g, b, a);
        pushText(a1x, a0y, u1, vTop, r, g, b, a);
        pushText(a1x, a1y, u1, vBot, r, g, b, a);
        pushText(a0x, a1y, u0, vBot, r, g, b, a);
        x += cw + px;
    }
}

void Gui::rect(float x, float y, float w, float h,
               float r, float g, float b, float a) {
    float x0, y0, x1, y1;
    toNdc(w_, h_, x, y, x0, y0);
    toNdc(w_, h_, x + w, y + h, x1, y1);
    // u = 2.0 marks solid-color vertices in the shader.
    pushText(x0, y0, 2.0f, 0, r, g, b, a);
    pushText(x1, y0, 2.0f, 0, r, g, b, a);
    pushText(x0, y1, 2.0f, 0, r, g, b, a);
    pushText(x1, y0, 2.0f, 0, r, g, b, a);
    pushText(x1, y1, 2.0f, 0, r, g, b, a);
    pushText(x0, y1, 2.0f, 0, r, g, b, a);
}

void Gui::line(float x0, float y0, float x1, float y1,
               float r, float g, float b, float a, float thick) {
    // Build a thin quad between the two points.
    const float dx = x1 - x0, dy = y1 - y0;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) return;
    const float nx = -dy / len * thick * 0.5f;
    const float ny = dx / len * thick * 0.5f;

    float ax, ay, bx, by, cx, cy, dxn, dyn;
    toNdc(w_, h_, x0 + nx, y0 + ny, ax, ay);
    toNdc(w_, h_, x1 + nx, y1 + ny, bx, by);
    toNdc(w_, h_, x0 - nx, y0 - ny, cx, cy);
    toNdc(w_, h_, x1 - nx, y1 - ny, dxn, dyn);
    pushText(ax, ay, 2.0f, 0, r, g, b, a);
    pushText(bx, by, 2.0f, 0, r, g, b, a);
    pushText(cx, cy, 2.0f, 0, r, g, b, a);
    pushText(bx, by, 2.0f, 0, r, g, b, a);
    pushText(dxn, dyn, 2.0f, 0, r, g, b, a);
    pushText(cx, cy, 2.0f, 0, r, g, b, a);
}

void Gui::pushIconQuad(float x, float y, float u, float t,
                       float r, float g, float b,
                       float p0, float p1) {
    iv_.push_back(x); iv_.push_back(y); iv_.push_back(u); iv_.push_back(t);
    iv_.push_back(r); iv_.push_back(g); iv_.push_back(b); iv_.push_back(1.0f);
    iv_.push_back(p0); iv_.push_back(p1);
}

void Gui::disc(float cx, float cy, float rPx, float cr, float cg, float cb,
               float bandFreq, float emissive) {
    float x0, y0, x1, y1;
    toNdc(w_, h_, cx - rPx, cy - rPx, x0, y0);
    toNdc(w_, h_, cx + rPx, cy + rPx, x1, y1);
    pushIconQuad(x0, y0, 0.0f, 0.0f, cr, cg, cb, bandFreq, emissive);
    pushIconQuad(x1, y0, 1.0f, 0.0f, cr, cg, cb, bandFreq, emissive);
    pushIconQuad(x0, y1, 0.0f, 1.0f, cr, cg, cb, bandFreq, emissive);
    pushIconQuad(x1, y0, 1.0f, 0.0f, cr, cg, cb, bandFreq, emissive);
    pushIconQuad(x1, y1, 1.0f, 1.0f, cr, cg, cb, bandFreq, emissive);
    pushIconQuad(x0, y1, 0.0f, 1.0f, cr, cg, cb, bandFreq, emissive);
}

// Small pixel/vector glyphs from rects + lines.
void Gui::icon(float x, float y, float s, IconKind kind,
               float r, float g, float b, float a) {
    const float cx = x + s * 0.5f, cy = y + s * 0.5f;
    switch (kind) {
    case IPlus:
        rect(x + s * 0.40f, y + s * 0.15f, s * 0.20f, s * 0.70f, r, g, b, a);
        rect(x + s * 0.15f, y + s * 0.40f, s * 0.70f, s * 0.20f, r, g, b, a);
        break;
    case IWrench:
        rect(x + s * 0.42f, y + s * 0.08f, s * 0.16f, s * 0.50f, r, g, b, a);
        rect(x + s * 0.18f, y + s * 0.40f, s * 0.64f, s * 0.16f, r, g, b, a);
        rect(x + s * 0.42f, y + s * 0.56f, s * 0.16f, s * 0.36f, r, g, b, a);
        break;
    case ISearch: {
        // circle approximated by an octagon of short lines
        const float rr = s * 0.26f;
        for (int i = 0; i < 8; ++i) {
            const float a0 = (float)i / 8.0f * 6.2831853f;
            const float a1 = (float)(i + 1) / 8.0f * 6.2831853f;
            line(cx + std::cos(a0) * rr, cy + std::sin(a0) * rr,
                 cx + std::cos(a1) * rr, cy + std::sin(a1) * rr,
                 r, g, b, a, s * 0.09f);
        }
        line(cx + rr * 0.7f, cy + rr * 0.7f, x + s * 0.92f, y + s * 0.92f,
             r, g, b, a, s * 0.10f);
        break;
    }
    case ISliders:
        rect(x + s * 0.10f, y + s * 0.22f, s * 0.80f, s * 0.09f, r, g, b, a);
        rect(x + s * 0.10f, y + s * 0.60f, s * 0.80f, s * 0.09f, r, g, b, a);
        rect(x + s * 0.20f, y + s * 0.14f, s * 0.14f, s * 0.26f, r, g, b, a);
        rect(x + s * 0.60f, y + s * 0.52f, s * 0.14f, s * 0.26f, r, g, b, a);
        break;
    case IPlay: {
        const float lx0 = x + s * 0.30f, lx1 = x + s * 0.75f;
        const float ty = y + s * 0.22f, by = y + s * 0.78f;
        line(lx0, ty, lx1, cy, r, g, b, a, s * 0.10f);
        line(lx1, cy, lx0, by, r, g, b, a, s * 0.10f);
        line(lx0, by, lx0, ty, r, g, b, a, s * 0.10f);
        break;
    }
    case IPause:
        rect(x + s * 0.28f, y + s * 0.22f, s * 0.16f, s * 0.56f, r, g, b, a);
        rect(x + s * 0.56f, y + s * 0.22f, s * 0.16f, s * 0.56f, r, g, b, a);
        break;
    case IRewind:
    case IForward: {
        const bool rev = (kind == IRewind);
        const float x0 = rev ? x + s * 0.72f : x + s * 0.12f;
        const float x1 = rev ? x + s * 0.32f : x + s * 0.52f;
        line(x0, y + s * 0.24f, x1, cy, r, g, b, a, s * 0.10f);
        line(x1, cy, x0, y + s * 0.76f, r, g, b, a, s * 0.10f);
        line(x0, y + s * 0.76f, x0, y + s * 0.24f, r, g, b, a, s * 0.10f);
        // second triangle
        const float x2 = rev ? x + s * 0.44f : x + s * 0.40f;
        const float x3 = rev ? x + s * 0.04f : x + s * 0.84f;
        line(x2, y + s * 0.24f, x3, cy, r, g, b, a, s * 0.10f);
        line(x3, cy, x2, y + s * 0.76f, r, g, b, a, s * 0.10f);
        line(x2, y + s * 0.76f, x2, y + s * 0.24f, r, g, b, a, s * 0.10f);
        break;
    }
    case IClose:
        line(x + s * 0.2f, y + s * 0.2f, x + s * 0.8f, y + s * 0.8f, r, g, b, a, s * 0.10f);
        line(x + s * 0.8f, y + s * 0.2f, x + s * 0.2f, y + s * 0.8f, r, g, b, a, s * 0.10f);
        break;
    case IChevron:
        line(x + s * 0.3f, y + s * 0.3f, x + s * 0.7f, cy, r, g, b, a, s * 0.11f);
        line(x + s * 0.7f, cy, x + s * 0.3f, y + s * 0.7f, r, g, b, a, s * 0.11f);
        break;
    case IEye: {
        const float rr = s * 0.30f;
        for (int i = 0; i < 10; ++i) {
            const float a0 = (float)i / 10.0f * 6.2831853f;
            const float a1 = (float)(i + 1) / 10.0f * 6.2831853f;
            // eye = squashed circle
            line(cx + std::cos(a0) * rr, cy + std::sin(a0) * rr * 0.62f,
                 cx + std::cos(a1) * rr, cy + std::sin(a1) * rr * 0.62f,
                 r, g, b, a, s * 0.08f);
        }
        rect(cx - s * 0.10f, cy - s * 0.10f, s * 0.20f, s * 0.20f, r, g, b, a);
        break;
    }
    case IGear: {
        const float rr = s * 0.34f;
        for (int i = 0; i < 12; ++i) {
            const float a0 = (float)i / 12.0f * 6.2831853f;
            const float a1 = (float)(i + 1) / 12.0f * 6.2831853f;
            const float r0 = (i % 2 == 0) ? rr : rr * 0.78f;
            const float r1 = (i % 2 == 0) ? rr : rr * 0.78f;
            line(cx + std::cos(a0) * r0, cy + std::sin(a0) * r0,
                 cx + std::cos(a1) * r1, cy + std::sin(a1) * r1,
                 r, g, b, a, s * 0.09f);
        }
        break;
    }
    case ICamera: {
        rect(x + s * 0.14f, y + s * 0.30f, s * 0.52f, s * 0.40f, r, g, b, a);
        const float rr = s * 0.13f;
        for (int i = 0; i < 8; ++i) {
            const float a0 = (float)i / 8.0f * 6.2831853f;
            const float a1 = (float)(i + 1) / 8.0f * 6.2831853f;
            line(cx - s * 0.12f + std::cos(a0) * rr, cy + std::sin(a0) * rr,
                 cx - s * 0.12f + std::cos(a1) * rr, cy + std::sin(a1) * rr,
                 r, g, b, a, s * 0.07f);
        }
        rect(x + s * 0.66f, y + s * 0.40f, s * 0.20f, s * 0.20f, r, g, b, a);
        break;
    }
    case INone:
    default:
        break;
    }
}

bool Gui::clickedAt(float x, float y, float w, float h) {
    if (wasClickedAt(x, y, w, h)) {
        releaseUsed_ = true;
        return true;
    }
    return false;
}

bool Gui::wasClickedAt(float x, float y, float w, float h) const {
    if (releaseUsed_ || !release_) return false;
    if (releaseBtn_ != 0) return false; // left click only
    const bool pressInside =
        (pressX_[0] >= x && pressX_[0] < x + w &&
         pressY_[0] >= y && pressY_[0] < y + h);
    if (!pressInside) return false;
    return (releaseX_ >= x && releaseX_ < x + w &&
            releaseY_ >= y && releaseY_ < y + h);
}

bool Gui::button(float x, float y, float w, float h, const std::string& label,
                 IconKind icon, float fontPx,
                 float r, float g, float b,
                 float textR, float textG, float textB) {
    const bool hov = mouseInside(x, y, w, h);
    float br = r, bg = g, bb = b;
    if (hov) { br += 0.07f; bg += 0.08f; bb += 0.12f; }

    bool consumed = false;
    if (wasClickedAt(x, y, w, h)) {
        releaseUsed_ = true;
        consumed = true;
    }

    rect(x, y, w, h, br, bg, bb, 1.0f);
    line(x, y, x + w, y, 0.24f, 0.28f, 0.38f, 0.9f, 1.0f);
    float tx = x + 8;
    if (icon != INone) {
        Gui::icon(tx, y + (h - 14) * 0.5f, 14, icon, textR, textG, textB,
                  hov ? 1.0f : 0.85f);
        tx += 20;
    }
    const float th = 7.0f * fontPx;
    text(tx, y + (h - th) * 0.5f, label, fontPx, textR, textG, textB,
         hov ? 1.0f : 0.85f);
    return consumed;
}

bool Gui::pill(float x, float y, float w, float h, const std::string& label,
               bool active) {
    bool consumed = false;
    if (wasClickedAt(x, y, w, h)) {
        releaseUsed_ = true;
        consumed = true;
    }

    const bool hov = mouseInside(x, y, w, h);
    if (active) {
        rect(x, y, w, h, 0.17f, 0.38f, 0.80f, 1.0f);
        text(x + (w - measure(label, 1.4f)) * 0.5f, y + (h - 9.8f) * 0.5f,
             label, 1.4f, 1.0f, 1.0f, 1.0f);
    } else {
        rect(x, y, w, h, hov ? 0.14f : 0.10f, hov ? 0.16f : 0.11f,
             hov ? 0.21f : 0.15f, 0.9f);
        text(x + (w - measure(label, 1.4f)) * 0.5f, y + (h - 9.8f) * 0.5f,
             label, 1.4f, 0.62f, 0.68f, 0.80f, 0.9f);
    }
    return consumed;
}

bool Gui::chip(float x, float y, float w, float h, const std::string& label,
               bool& on) {
    bool consumed = false;
    if (wasClickedAt(x, y, w, h)) {
        releaseUsed_ = true;
        consumed = true;
        on = !on;
    }

    const bool hov = mouseInside(x, y, w, h);
    if (on) {
        rect(x, y, w, h, 0.17f, 0.40f, 0.85f, 1.0f);
        text(x + 6, y + (h - 9.8f) * 0.5f, label, 1.35f, 1.0f, 1.0f, 1.0f);
    } else {
        rect(x, y, w, h, hov ? 0.15f : 0.11f, hov ? 0.17f : 0.12f,
             hov ? 0.23f : 0.17f, 0.9f);
        text(x + 6, y + (h - 9.8f) * 0.5f, label, 1.35f, 0.6f, 0.66f, 0.8f, 0.9f);
    }
    return consumed;
}

bool Gui::slider(float x, float y, float w, float& value, float lo, float hi) {
    const float th = 14.0f;
    const float cy = y + th * 0.5f;
    const float t = (hi > lo) ? (value - lo) / (hi - lo) : 0.0f;
    const float knobX = x + t * w;

    bool dragging = false;
    const bool over = mouseInside(x, y, w, th);
    if (over && mouseDown_[0]) {
        // press started on the slider, or we started dragging it
        if (wasClickedAt(x - 4, y, w + 8, th) ||
            (pressX_[0] >= x && pressX_[0] <= x + w &&
             pressY_[0] >= y - 4 && pressY_[0] <= y + th + 4)) {
            releaseUsed_ = true;
            dragging = true;
        }
    }
    if (dragging && (over || mouseDown_[0])) {
        value = lo + clampf((mouseX_ - x) / w, 0.0f, 1.0f) * (hi - lo);
    }

    // track
    rect(x, cy - 1.5f, w, 3.0f, 0.16f, 0.18f, 0.24f, 1.0f);
    rect(x, cy - 1.5f, knobX, 3.0f, 0.22f, 0.48f, 0.95f, 1.0f);
    // knob
    rect(knobX - 2.5f, y, 5.0f, th, 0.85f, 0.9f, 1.0f, 1.0f);
    return dragging;
}

void Gui::field(float x, float y, float w, const std::string& label,
                const std::string& value) {
    const float lw = w * 0.44f;
    text(x, y + 4, label, 1.35f, 0.62f, 0.68f, 0.82f);
    const float vx = x + lw;
    const float vw = w - lw;
    rect(vx, y, vw, 18.0f, 0.11f, 0.12f, 0.16f, 0.95f);
    line(vx, y, vx + vw, y, 0.22f, 0.25f, 0.33f, 0.8f, 1.0f);
    text(vx + 6, y + 4, value, 1.35f, 0.92f, 0.95f, 1.0f);
}

bool Gui::row(float x, float y, float w, float h, bool hovered) {
    bool consumed = false;
    if (wasClickedAt(x, y, w, h)) {
        releaseUsed_ = true;
        consumed = true;
    }
    if (hovered) rect(x, y, w, h, 0.14f, 0.30f, 0.65f, 0.55f);
    return consumed;
}

void Gui::inputBox(float x, float y, float w, float h, std::string& buf,
                   bool& active, const std::string& placeholder) {
    if (wasClickedAt(x, y, w, h)) {
        releaseUsed_ = true;
        active = !active;
    }
    const bool hov = mouseInside(x, y, w, h);
    rect(x, y, w, h, active ? 0.10f : 0.09f, active ? 0.12f : 0.10f,
         active ? 0.16f : 0.13f, 0.95f);
    line(x, y + h, x + w, y + h, active ? 0.22f : 0.30f, active ? 0.45f : 0.34f,
         active ? 0.95f : 0.45f, 0.9f, 1.0f);
    if (buf.empty())
        text(x + 8, y + (h - 9.8f) * 0.5f, placeholder, 1.4f, 0.42f, 0.46f, 0.56f);
    else {
        std::string show = buf;
        if (measure(show, 1.4f) > w - 16)
            while (show.size() > 1 && measure(show, 1.4f) > w - 16)
                show = show.substr(1);
        text(x + 8, y + (h - 9.8f) * 0.5f, show, 1.4f, 0.9f, 0.93f, 1.0f);
        (void)hov;
    }
}

void Gui::section(float x, float y, const std::string& s) {
    text(x, y, s, 1.3f, 0.5f, 0.56f, 0.72f);
    line(x, y + 16, x + 200, y + 16, 0.18f, 0.20f, 0.27f, 0.8f, 1.0f);
}

} // namespace us3d
