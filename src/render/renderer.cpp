// ============================================================================
//  Universe Sandbox 3D - renderer (implementation)
// ============================================================================
#include "render/renderer.h"

#include "core/shaders.h"
#include "ui/font5x7.h"
#include "gl/gl33.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace us3d {

// ---------------------------------------------------------------------------
//  Program
// ---------------------------------------------------------------------------
bool Program::create(const char* vsSrc, const char* fsSrc, const char* label,
                     std::string& errOut) {
    destroy();
    auto compileStage = [label](GLenum type, const char* src) -> GLuint {
        GLuint sh = gl33::glCreateShader(type);
        gl33::glShaderSource(sh, 1, &src, nullptr);
        gl33::glCompileShader(sh);
        int ok = 0;
        gl33::glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048];
            gl33::glGetShaderInfoLog(sh, sizeof log, nullptr, log);
            char msg[2200];
            std::snprintf(msg, sizeof msg,
                          "%s: shader compile failed:\n%s", label, log);
            std::fprintf(stderr, "%s\n", msg);
            gl33::glDeleteShader(sh);
            return 0;
        }
        return sh;
    };

    vs_ = compileStage(GL_VERTEX_SHADER, vsSrc);
    if (!vs_) { errOut = std::string(label) + ": vertex shader"; return false; }
    fs_ = compileStage(GL_FRAGMENT_SHADER, fsSrc);
    if (!fs_) { errOut = std::string(label) + ": fragment shader"; return false; }

    prog_ = gl33::glCreateProgram();
    gl33::glAttachShader(prog_, vs_);
    gl33::glAttachShader(prog_, fs_);
    gl33::glLinkProgram(prog_);
    int linked = 0;
    gl33::glGetProgramiv(prog_, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[2048];
        gl33::glGetProgramInfoLog(prog_, sizeof log, nullptr, log);
        char msg[2200];
        std::snprintf(msg, sizeof msg, "%s: link failed:\n%s", label, log);
        std::fprintf(stderr, "%s\n", msg);
        destroy();
        errOut = std::string(label) + ": link";
        return false;
    }
    gl33::glDeleteShader(vs_);
    gl33::glDeleteShader(fs_);
    vs_ = fs_ = 0;
    return true;
}

void Program::use() const { gl33::glUseProgram(prog_); }

void Program::destroy() {
    if (vs_) gl33::glDeleteShader(vs_);
    if (fs_) gl33::glDeleteShader(fs_);
    if (prog_) gl33::glDeleteProgram(prog_);
    vs_ = fs_ = prog_ = 0;
}

int Program::uniform(const char* name) {
    if (!prog_) return -1;
    return gl33::glGetUniformLocation(prog_, name);
}

// ---------------------------------------------------------------------------
//  Renderer
// ---------------------------------------------------------------------------
Renderer::~Renderer() { shutdown(); }

bool Renderer::init() {
    using namespace shaders;
    std::string err;

    if (!bodyProg_.create(body_vs, body_fs, "body", err))   { error_ = err; return false; }
    if (!skyProg_.create(sky_vs, sky_fs, "sky", err))       { error_ = err; return false; }
    if (!starsProg_.create(stars_vs, stars_fs, "stars", err)) { error_ = err; return false; }
    if (!lineProg_.create(line_vs, line_fs, "line", err))   { error_ = err; return false; }
    if (!glowProg_.create(glow_vs, glow_fs, "glow", err))   { error_ = err; return false; }

    buildSphereMesh();
    buildStarfield();
    buildGlowMesh();

    // Dynamic trail buffer: enough for 256 trails x 512 points.
    const unsigned trailPts = 256 * 512;
    lineCapacity_ = trailPts;
    gl33::glGenVertexArrays(1, &lineVao_);
    gl33::glGenBuffers(1, &lineVbo_);
    gl33::glBindVertexArray(lineVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, lineVbo_);
    gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(trailPts * 7 * sizeof(float)),
                       nullptr, GL_DYNAMIC_DRAW);
    const size_t stride = 7 * sizeof(float);
    gl33::glEnableVertexAttribArray(0);
    gl33::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride, (const void*)(size_t)0);
    gl33::glEnableVertexAttribArray(1);
    gl33::glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (GLsizei)stride, (const void*)(size_t)(3 * sizeof(float)));
    gl33::glBindVertexArray(0);

    if (!buildFontTexture()) { error_ = "font texture"; return false; }

    gl33::glEnable(GL_DEPTH_TEST);
    gl33::glDepthFunc(GL_LEQUAL);
    gl33::glEnable(GL_CULL_FACE);
    gl33::glCullFace(GL_BACK);
    gl33::glFrontFace(GL_CCW);
    gl33::glEnable(GL_BLEND);
    gl33::glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
    return true;
}

void Renderer::shutdown() {
    if (sphereVao_) { gl33::glDeleteVertexArrays(1, &sphereVao_); sphereVao_ = 0; }
    if (sphereIbo_) { gl33::glDeleteBuffers(1, &sphereIbo_);     sphereIbo_ = 0; }
    if (starVao_)   { gl33::glDeleteVertexArrays(1, &starVao_);   starVao_ = 0; }
    if (glowVao_)   { gl33::glDeleteVertexArrays(1, &glowVao_);   glowVao_ = 0; }
    if (lineVao_)   { gl33::glDeleteVertexArrays(1, &lineVao_);   lineVao_ = 0; }
    if (lineVbo_)   { gl33::glDeleteBuffers(1, &lineVbo_);        lineVbo_ = 0; }
    if (fontTex_)   { gl33::glDeleteTextures(1, &fontTex_);       fontTex_ = 0; }
    bodyProg_.destroy();
    skyProg_.destroy();
    starsProg_.destroy();
    lineProg_.destroy();
    glowProg_.destroy();
}

void Renderer::buildSphereMesh() {
    const int lat = 44, lon = 88;
    std::vector<float> pos, nrm, uv;
    std::vector<unsigned> idx;
    pos.reserve((lat + 1) * (lon + 1));

    for (int i = 0; i <= lat; ++i) {
        const float theta = 3.14159265f * i / lat;      // 0..PI
        const float st = std::sin(theta), ct = std::cos(theta);
        for (int j = 0; j <= lon; ++j) {
            const float phi = 2.0f * 3.14159265f * j / lon;
            const float sp = std::sin(phi), cp = std::cos(phi);
            pos.push_back(st * cp); pos.push_back(ct); pos.push_back(st * sp);
            nrm.push_back(st * cp); nrm.push_back(ct); nrm.push_back(st * sp);
            uv.push_back(j / (float)lon);
            uv.push_back(i / (float)lat);
        }
    }
    for (int i = 0; i < lat; ++i) {
        for (int j = 0; j < lon; ++j) {
            const unsigned a = (unsigned)(i * (lon + 1) + j);
            const unsigned b = a + (unsigned)(lon + 1);
            idx.push_back(a);   idx.push_back(b);   idx.push_back(a + 1);
            idx.push_back(b);   idx.push_back(b + 1); idx.push_back(a + 1);
        }
    }
    sphereIndexCount_ = (unsigned)idx.size();

    gl33::glGenVertexArrays(1, &sphereVao_);
    gl33::glGenBuffers(1, &sphereIbo_);
    gl33::glBindVertexArray(sphereVao_);

    {
        GLuint vbo;
        gl33::glGenBuffers(1, &vbo);
        gl33::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(pos.size() * sizeof(float)),
                           pos.data(), GL_STATIC_DRAW);
        gl33::glEnableVertexAttribArray(0);
        gl33::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
    }
    {
        GLuint vbo;
        gl33::glGenBuffers(1, &vbo);
        gl33::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nrm.size() * sizeof(float)),
                           nrm.data(), GL_STATIC_DRAW);
        gl33::glEnableVertexAttribArray(1);
        gl33::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
    }
    {
        GLuint vbo;
        gl33::glGenBuffers(1, &vbo);
        gl33::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(uv.size() * sizeof(float)),
                           uv.data(), GL_STATIC_DRAW);
        gl33::glEnableVertexAttribArray(2);
        gl33::glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)0);
    }
    gl33::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIbo_);
    gl33::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                       (GLsizeiptr)(idx.size() * sizeof(unsigned)),
                       idx.data(), GL_STATIC_DRAW);
    gl33::glBindVertexArray(0);
}

void Renderer::buildStarfield() {
    const int count = 2200;
    std::vector<float> data;
    data.reserve((size_t)count * 5);
    unsigned seed = 1234u;
    auto rnd = [&seed]() -> float {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) / 16777216.0f;
    };
    for (int i = 0; i < count; ++i) {
        const float u = rnd() * 2.0f - 1.0f;
        const float a = rnd() * 2.0f * 3.14159265f;
        const float r = 400.0f + rnd() * 450.0f;
        const float s = std::sqrt(std::max(0.0f, 1.0f - u * u));
        data.push_back(r * s * std::cos(a));
        data.push_back(r * u);
        data.push_back(r * s * std::sin(a));
        data.push_back(1.0f + rnd() * 2.4f);
        data.push_back(rnd());
    }
    starCount_ = (unsigned)count;

    gl33::glGenVertexArrays(1, &starVao_);
    GLuint vbo;
    gl33::glGenBuffers(1, &vbo);
    gl33::glBindVertexArray(starVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)),
                       data.data(), GL_STATIC_DRAW);
    const GLsizei stride = (GLsizei)(5 * sizeof(float));
    gl33::glEnableVertexAttribArray(0);
    gl33::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    gl33::glEnableVertexAttribArray(1);
    gl33::glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, (const void*)(size_t)(3 * sizeof(float)));
    gl33::glEnableVertexAttribArray(2);
    gl33::glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (const void*)(size_t)(4 * sizeof(float)));
    gl33::glBindVertexArray(0);
}

void Renderer::buildGlowMesh() {
    const float corners[12] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
    };
    gl33::glGenVertexArrays(1, &glowVao_);
    GLuint vbo;
    gl33::glGenBuffers(1, &vbo);
    gl33::glBindVertexArray(glowVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl33::glBufferData(GL_ARRAY_BUFFER, sizeof corners, corners, GL_STATIC_DRAW);
    gl33::glEnableVertexAttribArray(0);
    gl33::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
    gl33::glBindVertexArray(0);
}

bool Renderer::buildFontTexture() {
    const int cellW = 7, cellH = 9;   // 5x7 glyph + padding
    const int cols = 16, rows = 6;
    const int W = cols * cellW, H = rows * cellH;
    std::vector<unsigned char> px((size_t)W * H * 4, 0);

    for (int c = 32; c <= 126; ++c) {
        const int col = (c - 32) % cols;
        const int row = (c - 32) / cols;
        const unsigned char* g = fontGlyph((char)c);
        for (int r = 0; r < kFontH; ++r) {
            const unsigned bits = g[r]; // bit 4 = leftmost
            for (int x = 0; x < kFontW; ++x) {
                const bool on = (bits >> (kFontW - 1 - x)) & 1u;
                if (!on) continue;
                const int tx = col * cellW + 1 + x;
                const int ty = H - 1 - (row * cellH + 1 + r); // GL origin bottom-left
                size_t i = ((size_t)ty * W + tx) * 4;
                px[i] = 255; px[i + 1] = 255; px[i + 2] = 255; px[i + 3] = 255;
            }
        }
    }

    gl33::glGenTextures(1, &fontTex_);
    gl33::glBindTexture(GL_TEXTURE_2D, fontTex_);
    gl33::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    gl33::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl33::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl33::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl33::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl33::glBindTexture(GL_TEXTURE_2D, 0);
    return fontTex_ != 0;
}

// ---------------------------------------------------------------------------
//  Frame
// ---------------------------------------------------------------------------
void Renderer::render(const Camera& cam, const System& sim, int selectedId,
                      bool showTrails, float timeSec, int w, int h,
                      const ViewOptions& vo) {
    gl33::glViewport(0, 0, w, h);
    gl33::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    aspect_ = (float)w / (float)std::max(1, h);
    view_ = cam.view();
    proj_ = Mat4::Perspective(fov_, aspect_, 0.0005f, 2000.0f);

    // Primary star position drives lighting.
    const int primary = sim.primaryBody();
    const Vec3 lightPos = (primary >= 0) ? sim.bodies()[primary].pos : Vec3::Zero();
    const Vec3 lightColor = (primary >= 0)
        ? sim.bodies()[primary].color * 1.25f : Vec3(0.9f, 0.9f, 1.0f);

    // ---- sky (background sphere) ----
    skyProg_.use();
    gl33::glDisable(GL_CULL_FACE);
    gl33::glDepthMask(GL_FALSE);
    gl33::glUniformMatrix4fv(skyProg_.uniform("uView"), 1, 0, &view_.m[0][0]);
    gl33::glUniformMatrix4fv(skyProg_.uniform("uProj"), 1, 0, &proj_.m[0][0]);
    gl33::glUniform1f(skyProg_.uniform("uTime"), timeSec);
    gl33::glBindVertexArray(sphereVao_);
    gl33::glDrawElements(GL_TRIANGLES, (GLsizei)sphereIndexCount_,
                         GL_UNSIGNED_INT, (const void*)0);
    gl33::glDepthMask(GL_TRUE);
    gl33::glEnable(GL_CULL_FACE);

    // ---- near starfield (points) ----
    gl33::glDepthMask(GL_FALSE);
    gl33::glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    starsProg_.use();
    gl33::glUniformMatrix4fv(starsProg_.uniform("uView"), 1, 0, &view_.m[0][0]);
    gl33::glUniformMatrix4fv(starsProg_.uniform("uProj"), 1, 0, &proj_.m[0][0]);
    gl33::glUniform1f(starsProg_.uniform("uTime"), timeSec);
    gl33::glUniform3f(starsProg_.uniform("uColor"), 0.85f, 0.88f, 1.0f);
    gl33::glUniform1f(starsProg_.uniform("uFade"), 1.0f);
    gl33::glBindVertexArray(starVao_);
    gl33::glDrawArrays(GL_POINTS, 0, (GLsizei)starCount_);

    // ---- bodies ----
    drawBodies(cam, sim, selectedId, timeSec, lightPos, lightColor, vo);

    // ---- sun glow (additive) ----
    drawGlow(sim, timeSec);

    // ---- trails ----
    if (showTrails) drawTrails(sim);

    // ---- selection ring ----
    if (selectedId >= 0) drawSelectionRing(cam, sim, selectedId);

    gl33::glDepthMask(GL_TRUE);
    gl33::glDisable(GL_BLEND);
    gl33::glFlush();
}

void Renderer::drawBodies(const Camera& cam, const System& sim, int selectedId,
                          float timeSec, const Vec3& lightPos,
                          const Vec3& lightColor, const ViewOptions& vo) {
    bodyProg_.use();
    gl33::glUniformMatrix4fv(bodyProg_.uniform("uView"), 1, 0, &view_.m[0][0]);
    gl33::glUniformMatrix4fv(bodyProg_.uniform("uProj"), 1, 0, &proj_.m[0][0]);
    gl33::glUniform3f(bodyProg_.uniform("uCamPos"),
                      cam.position().x, cam.position().y, cam.position().z);
    gl33::glUniform3f(bodyProg_.uniform("uLightPos"), lightPos.x, lightPos.y, lightPos.z);
    gl33::glUniform3f(bodyProg_.uniform("uLightColor"),
                      lightColor.x, lightColor.y, lightColor.z);
    gl33::glUniform1f(bodyProg_.uniform("uTime"), timeSec);
    gl33::glUniform1f(bodyProg_.uniform("uPulse"), 0.5f + 0.5f * std::sin(timeSec * 4.0f));
    gl33::glUniform1f(bodyProg_.uniform("uClouds"), vo.clouds);
    gl33::glUniform1f(bodyProg_.uniform("uAtmos"), vo.atmos);
    gl33::glUniform3f(bodyProg_.uniform("uLampPos"),
                      vo.lampPos.x, vo.lampPos.y, vo.lampPos.z);
    gl33::glUniform3f(bodyProg_.uniform("uLampColor"),
                      vo.lampColor.x * vo.lampStrength,
                      vo.lampColor.y * vo.lampStrength,
                      vo.lampColor.z * vo.lampStrength);
    gl33::glUniform1f(bodyProg_.uniform("uLampOn"), vo.lampOn ? 1.0f : 0.0f);

    gl33::glBindVertexArray(sphereVao_);
    gl33::glDepthMask(GL_TRUE);
    for (const Body& b : sim.bodies()) {
        if (b.dead) continue;
        if (!vo.showDebris && b.type == BodyType::Asteroid) continue;

        // Model: translate * spin (visual rotation) * scale.
        Mat4 model = Mat4::Translate(b.pos);
        if (b.spin != 0.0f) {
            const float ang = b.spin * sim.time() + (float)(b.uid % 997) * 0.0063f;
            const float c = std::cos(ang), s = std::sin(ang);
            Mat4 rot;
            rot.m[0][0] = c;   rot.m[0][2] = s;
            rot.m[2][0] = -s;  rot.m[2][2] = c;
            model = Mat4::Multiply(model, rot);
        }
        model = Mat4::Multiply(model, Mat4::Scale(b.radius));
        gl33::glUniformMatrix4fv(bodyProg_.uniform("uModel"), 1, 0, &model.m[0][0]);
        gl33::glUniform3f(bodyProg_.uniform("uCenter"), b.pos.x, b.pos.y, b.pos.z);
        gl33::glUniform3f(bodyProg_.uniform("uAlbedo"),
                          b.displayColor.x, b.displayColor.y, b.displayColor.z);
        gl33::glUniform1f(bodyProg_.uniform("uEmissive"), b.isStar() ? 1.0f : 0.0f);
        gl33::glUniform1f(bodyProg_.uniform("uBandFreq"), b.bandFreq);
        gl33::glUniform1f(bodyProg_.uniform("uRoughness"), b.roughness);
        gl33::glUniform1f(bodyProg_.uniform("uSelected"),
                          b.id == selectedId ? 1.0f : 0.0f);
        gl33::glUniform1f(bodyProg_.uniform("uFade"), std::min(1.0f, b.age * 2.0f));
        gl33::glUniform1f(bodyProg_.uniform("uSurface"), (float)b.surface);
        gl33::glUniform1f(bodyProg_.uniform("uTempGlow"),
                          clampf((b.temperatureC - 300.0f) / 2700.0f, 0.0f, 1.0f)
                          * vo.heatGlow);
        gl33::glDrawElements(GL_TRIANGLES, (GLsizei)sphereIndexCount_,
                             GL_UNSIGNED_INT, (const void*)0);
    }
}

void Renderer::drawGlow(const System& sim, float timeSec) {
    glowProg_.use();
    gl33::glUniformMatrix4fv(glowProg_.uniform("uView"), 1, 0, &view_.m[0][0]);
    gl33::glUniformMatrix4fv(glowProg_.uniform("uProj"), 1, 0, &proj_.m[0][0]);
    gl33::glUniform1f(glowProg_.uniform("uTime"), timeSec);
    const Vec3 right(view_.m[0][0], view_.m[1][0], view_.m[2][0]);
    const Vec3 up(view_.m[0][1], view_.m[1][1], view_.m[2][1]);
    gl33::glUniform3f(glowProg_.uniform("uCamRight"), right.x, right.y, right.z);
    gl33::glUniform3f(glowProg_.uniform("uCamUp"), up.x, up.y, up.z);

    gl33::glBindVertexArray(glowVao_);
    gl33::glDepthMask(GL_FALSE);
    gl33::glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (const Body& b : sim.bodies()) {
        if (b.dead || !b.isStar()) continue;
        gl33::glUniform3f(glowProg_.uniform("uCenter"), b.pos.x, b.pos.y, b.pos.z);
        gl33::glUniform1f(glowProg_.uniform("uSize"), b.radius * 6.5f);
        gl33::glUniform3f(glowProg_.uniform("uColor"),
                          b.color.x * 0.85f, b.color.y * 0.70f, b.color.z * 0.45f);
        gl33::glUniform1f(glowProg_.uniform("uIntensity"), 0.5f * std::min(1.0f, b.age * 2.0f));
        gl33::glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void Renderer::drawTrails(const System& sim) {
    lineProg_.use();
    gl33::glUniformMatrix4fv(lineProg_.uniform("uView"), 1, 0, &view_.m[0][0]);
    gl33::glUniformMatrix4fv(lineProg_.uniform("uProj"), 1, 0, &proj_.m[0][0]);

    // Build interleaved pos+color line segments (one strip per body).
    struct Seg { unsigned offset; unsigned count; };
    std::vector<Seg> segs;
    std::vector<float> data;
    data.reserve(lineCapacity_ * 7);

    const auto& bodies = sim.bodies();
    const auto& trails = sim.trails();
    for (size_t i = 0; i < bodies.size() && i < trails.size(); ++i) {
        const Body& b = bodies[i];
        if (b.dead || b.isStar()) continue;
        const std::vector<Vec3>& t = trails[i];
        if (t.size() < 2) continue;
        if (data.size() / 7 + t.size() > lineCapacity_) break; // buffer full
        segs.push_back({(unsigned)(data.size() / 7), (unsigned)t.size()});
        for (size_t k = 0; k < t.size(); ++k) {
            const float f = (float)k / (float)(t.size() - 1);
            const float a = 0.04f + 0.30f * f * f;
            data.push_back(t[k].x); data.push_back(t[k].y); data.push_back(t[k].z);
            data.push_back(b.color.x * (0.4f + 0.6f * f));
            data.push_back(b.color.y * (0.4f + 0.6f * f));
            data.push_back(b.color.z * (0.4f + 0.6f * f));
            data.push_back(a);
        }
    }
    if (data.empty()) return;

    gl33::glBindVertexArray(lineVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, lineVbo_);
    gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)),
                       data.data(), GL_DYNAMIC_DRAW);
    gl33::glDepthMask(GL_FALSE);
    gl33::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl33::glLineWidth(1.0f);
    for (const Seg& s : segs)
        gl33::glDrawArrays(GL_LINE_STRIP, (GLint)s.offset, (GLsizei)s.count);
    gl33::glDepthMask(GL_TRUE);
}

void Renderer::drawSelectionRing(const Camera& cam, const System& sim, int selectedId) {
    const Body* b = sim.body(selectedId);
    if (!b) return;

    const int n = 72;
    ringScratch_.clear();
    ringScratch_.reserve((size_t)n * 7);
    const Vec3 camPos = cam.position();
    const Vec3 f = normalize(b->pos - camPos);
    const Vec3 right = normalize(cross(f, Vec3::UnitY()));
    const Vec3 up = cross(right, f);
    const float r = b->radius * 1.7f;
    const float pulse = 0.75f + 0.25f * std::sin(sim.time() * 6.0f + 1.0f);
    for (int i = 0; i < n; ++i) {
        const float a = 2.0f * 3.14159265f * i / (n - 1);
        const Vec3 p = b->pos + (right * std::cos(a) + up * std::sin(a)) * r;
        ringScratch_.push_back(p.x); ringScratch_.push_back(p.y); ringScratch_.push_back(p.z);
        ringScratch_.push_back(1.00f * pulse);
        ringScratch_.push_back(0.85f * pulse);
        ringScratch_.push_back(0.35f * pulse);
        ringScratch_.push_back(0.9f);
    }

    lineProg_.use();
    gl33::glUniformMatrix4fv(lineProg_.uniform("uView"), 1, 0, &view_.m[0][0]);
    gl33::glUniformMatrix4fv(lineProg_.uniform("uProj"), 1, 0, &proj_.m[0][0]);
    gl33::glBindVertexArray(lineVao_);
    gl33::glBindBuffer(GL_ARRAY_BUFFER, lineVbo_);
    gl33::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(ringScratch_.size() * sizeof(float)),
                       ringScratch_.data(), GL_DYNAMIC_DRAW);
    gl33::glDepthMask(GL_FALSE);
    gl33::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl33::glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)n);
    gl33::glDepthMask(GL_TRUE);
}

} // namespace us3d
