// ============================================================================
//  Universe Sandbox 3D - HUD (screen-space text + rects, built-in font)
// ============================================================================
#pragma once

#include "gl/gl33.h"
#include "render/renderer.h"
#include <string>
#include <vector>

namespace us3d {

class Hud {
public:
    bool init(GLuint fontTex);
    void shutdown();

    // Start/end a 2D pass over a w x h client area (pixels, top-left origin
    // for all draw calls in between).
    void begin(int w, int h);
    void end();

    void drawText(float x, float y, const std::string& text, float px,
                  float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    void drawTextCentered(float y, const std::string& text, float px,
                          float r, float g, float b, float a = 1.0f);
    void drawRect(float x, float y, float w, float h,
                  float r, float g, float b, float a);

    // Width in pixels of `text` at scale px.
    static float measureText(const std::string& text, float px);

private:
    Program progHolder_;   // reuse the Program helper for the UI shader
    GLuint  quadVao_ = 0, quadVbo_ = 0;
    GLuint  fontTex_ = 0;
    int     intWidth_ = 0, intHeight_ = 0;
    std::vector<float> verts_;  // interleaved: pos2, uv2, color4

    void flush();
};

} // namespace us3d
