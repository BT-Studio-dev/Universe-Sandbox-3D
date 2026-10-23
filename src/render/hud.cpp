// ============================================================================
//  Universe Sandbox 3D - HUD (implementation)
// ============================================================================
#include "render/hud.h"

#include "core/shaders.h"

#include <cmath>
#include <cstdio>

namespace us3d {

// Font atlas layout (must match Renderer::buildFontTexture).
static const int kCols = 16, kRows = 6;
static const int kCellW = 7, kCellH = 9;
static const int kAtlasW = kCols * kCellW;   // 112
static const int kAtlasH = kRows * kCellH;   // 54

bool Hud::init(GLuint fontTex) {
    std::string err;
    if (!progHolder_.create(shaders::ui_vs, shaders::ui_fs, "ui", err))
        return false;
    fontTex_ = fontTex;

    gl33::glGenVertexArrays(1, &quadVao_);
    gl33::glGenBuffers(1, &quadVbo_);
    gl33::glBindVertexArray(quadVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    gl33::glBufferData(GL_ARRAY_BUFFER, 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);
    const GLsizei stride = 8 * (GLsizei)sizeof(float);
    gl33::glEnableVertexAttribArray(0);
    gl33::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    gl33::glEnableVertexAttribArray(1);
    gl33::glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(size_t)(2 * sizeof(float)));
    gl33::glEnableVertexAttribArray(2);
    gl33::glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(size_t)(4 * sizeof(float)));
    gl33::glBindVertexArray(0);
    return true;
}

void Hud::shutdown() {
    if (quadVao_) gl33::glDeleteVertexArrays(1, &quadVao_);
    if (quadVbo_) gl33::glDeleteBuffers(1, &quadVbo_);
    quadVao_ = quadVbo_ = 0;
    progHolder_.destroy();
}

float Hud::measureText(const std::string& text, float px) {
    // 6 font-pixels per character (5 + 1 tracking).
    return (float)text.size() * 6.0f * px;
}

void Hud::begin(int w, int h) {
    intWidth_ = w;
    intHeight_ = h;
    verts_.clear();
    gl33::glDisable(GL_DEPTH_TEST);
    gl33::glDisable(GL_CULL_FACE);
    gl33::glEnable(GL_BLEND);
    gl33::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    progHolder_.use();
    gl33::glActiveTexture(GL_TEXTURE0);
    gl33::glBindTexture(GL_TEXTURE_2D, fontTex_);
    gl33::glUniform1i(progHolder_.uniform("uTex"), 0);
    gl33::glViewport(0, 0, w, h);
}

void Hud::end() {
    flush();
    gl33::glEnable(GL_DEPTH_TEST);
    gl33::glEnable(GL_CULL_FACE);
    gl33::glDisable(GL_BLEND);
}

void Hud::flush() {
    if (verts_.empty()) return;
    gl33::glBindVertexArray(quadVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    gl33::glBufferSubData(GL_ARRAY_BUFFER, 0,
                          (GLsizeiptr)(verts_.size() * sizeof(float)), verts_.data());
    gl33::glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts_.size() / 8));
    gl33::glBindVertexArray(0);
    verts_.clear();
}

static inline void pushVert(std::vector<float>& v,
                            float x, float y, float u, float t,
                            float r, float g, float b, float a) {
    v.push_back(x); v.push_back(y);
    v.push_back(u); v.push_back(t);
    v.push_back(r); v.push_back(g); v.push_back(b); v.push_back(a);
}

void Hud::drawText(float x, float y, const std::string& text, float px,
                   float r, float g, float b, float a) {
    const float cw = 5.0f * px;   // glyph advance
    const float ch = 7.0f * px;

    for (char c : text) {
        const unsigned char code = (unsigned char)c;
        if (code < 32 || code > 126) { x += cw + px; continue; }
        const int col = (code - 32) % kCols;
        const int row = (code - 32) / kCols;
        const float u0 = (col * kCellW + 1) / (float)kAtlasW;
        const float u1 = (col * kCellW + 6) / (float)kAtlasW;
        const float vTop = (kAtlasH - (row * kCellH + 1)) / (float)kAtlasH;
        const float vBot = (kAtlasH - (row * kCellH + 8)) / (float)kAtlasH;

        const float x1 = x + cw;
        const float y1 = y + ch;
        // NDC (screen y grows downward)
        const float ndcX0 = (x / intWidth_) * 2.0f - 1.0f;
        const float ndcX1 = (x1 / intWidth_) * 2.0f - 1.0f;
        const float ndcY0 = 1.0f - (y / intHeight_) * 2.0f;
        const float ndcY1 = 1.0f - (y1 / intHeight_) * 2.0f;

        pushVert(verts_, ndcX0, ndcY0, u0, vTop, r, g, b, a);
        pushVert(verts_, ndcX1, ndcY0, u1, vTop, r, g, b, a);
        pushVert(verts_, ndcX0, ndcY1, u0, vBot, r, g, b, a);
        pushVert(verts_, ndcX1, ndcY0, u1, vTop, r, g, b, a);
        pushVert(verts_, ndcX1, ndcY1, u1, vBot, r, g, b, a);
        pushVert(verts_, ndcX0, ndcY1, u0, vBot, r, g, b, a);
        x += cw + px;
    }
}

void Hud::drawTextCentered(float y, const std::string& text, float px,
                           float r, float g, float b, float a) {
    const float w = measureText(text, px);
    drawText((intWidth_ - w) * 0.5f, y, text, px, r, g, b, a);
}

void Hud::drawRect(float x, float y, float w, float h,
                   float r, float g, float b, float a) {
    const float x1 = x + w, y1 = y + h;
    const float ndcX0 = (x / intWidth_) * 2.0f - 1.0f;
    const float ndcX1 = (x1 / intWidth_) * 2.0f - 1.0f;
    const float ndcY0 = 1.0f - (y / intHeight_) * 2.0f;
    const float ndcY1 = 1.0f - (y1 / intHeight_) * 2.0f;

    // u = 2.0 marks these as solid-color vertices in the shader.
    pushVert(verts_, ndcX0, ndcY0, 2.0f, 0, r, g, b, a);
    pushVert(verts_, ndcX1, ndcY0, 2.0f, 0, r, g, b, a);
    pushVert(verts_, ndcX0, ndcY1, 2.0f, 0, r, g, b, a);
    pushVert(verts_, ndcX1, ndcY0, 2.0f, 0, r, g, b, a);
    pushVert(verts_, ndcX1, ndcY1, 2.0f, 0, r, g, b, a);
    pushVert(verts_, ndcX0, ndcY1, 2.0f, 0, r, g, b, a);
}

} // namespace us3d
