// ============================================================================
//  Universe Sandbox 3D - GLSL 3.30 shaders (embedded)
// ============================================================================
#pragma once

namespace us3d {
namespace shaders {

// ---------------------------------------------------------------------------
//  Body (planets, moons, asteroids, stars)
// ---------------------------------------------------------------------------
static const char* body_vs = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUv;

void main() {
    vNormal   = mat3(uModel) * aNormal;
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
    vUv       = aUv;
    gl_Position = uProj * uView * vec4(vWorldPos, 1.0);
}
)glsl";

static const char* body_fs = R"glsl(
#version 330 core
precision highp float;

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUv;

uniform vec3  uCamPos;
uniform vec3  uCenter;      // body center (world)
uniform vec3  uLightPos;    // primary star (world)
uniform vec3  uLightColor;
uniform vec3  uAlbedo;
uniform float uEmissive;    // 1 = star
uniform float uBandFreq;    // >0.5 gives gas-giant banding
uniform float uRoughness;
uniform float uTime;
uniform float uSelected;
uniform float uPulse;
uniform float uFade;        // spawn/fade-in
// View panel (worlds / colour by / lamp)
uniform float uSurface;     // 0 rock, 1 water, 2 ice, 3 air/clouds
uniform float uTempGlow;    // 0..1 heat glow (Heat Beam tool)
uniform float uClouds;      // global clouds toggle
uniform float uAtmos;       // rim/atmosphere glow 0..1
uniform vec3  uLampPos;     // lamp point light (world)
uniform vec3  uLampColor;
uniform float uLampOn;

out vec4 fragColor;

float hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x),
                   mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
                   mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; i++) {
        s += a * noise(p);
        p *= 2.03;
        a *= 0.5;
    }
    return s;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 S = normalize(vWorldPos - uCenter); // sphere direction from center
    vec3 L = normalize(uLightPos - vWorldPos);

    if (uEmissive > 0.5) {
        // ---- animated star surface ----
        float n1 = fbm(S * 9.0 + vec3(0.0, uTime * 0.10, 0.0));
        float n2 = fbm(S * 22.0 - vec3(uTime * 0.13, 0.0, 0.0));
        float t  = clamp(n1 * 0.65 + n2 * 0.55, 0.0, 1.0);
        vec3 c = mix(vec3(1.0, 0.32, 0.04), vec3(1.0, 0.96, 0.72), smoothstep(0.25, 0.85, t));
        float limb = pow(clamp(dot(N, V) + 0.15, 0.0, 1.0), 0.55);
        vec3 col = c * (0.72 + 0.55 * limb);
        if (uSelected > 0.5) col += vec3(1.0, 0.9, 0.5) * 0.10 * uPulse;
        fragColor = vec4(col * uFade, 1.0);
        return;
    }

    // ---- rocky / banded planet surface ----
    float mottle = fbm(S * (3.5 + uRoughness * 5.0));
    vec3 base = uAlbedo * (0.80 + 0.40 * mottle);

    // Terraform surface tints (View: surface state)
    if (uSurface > 0.5 && uSurface < 1.5) {          // water
        base = mix(base, vec3(0.06, 0.22, 0.45), 0.55);
    } else if (uSurface > 1.5 && uSurface < 2.5) {   // ice
        base = mix(base, vec3(0.85, 0.92, 1.00), 0.65);
    }

    if (uBandFreq > 0.5) {
        float lat = asin(clamp(S.y, -1.0, 1.0));
        float bands = sin(lat * uBandFreq * 3.0 + fbm(S * 5.0) * 3.5);
        vec3 banded = base * (0.82 + 0.30 * bands) * vec3(1.06, 0.98, 0.92);
        base = banded;
    }

    // Clouds (global toggle or air surface)
    if (uClouds > 0.5 || (uSurface > 2.5)) {
        float cl = smoothstep(0.42, 0.72, fbm(S * 6.5 + vec3(0.0, uTime * 0.05, 0.0)));
        base = mix(base, vec3(0.92, 0.94, 0.98) * (0.75 + 0.25 * mottle), cl * 0.85);
    }

    float diff = max(dot(N, L), 0.0);
    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.22;
    float rim  = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);

    vec3 col = base * (0.055 + uLightColor * diff)
             + uLightColor * spec
             + uAlbedo * rim * 0.14 * uAtmos;

    // Lamp (View panel point light)
    if (uLampOn > 0.5) {
        vec3  L2   = normalize(uLampPos - vWorldPos);
        float d2   = clamp(dot(N, L2), 0.0, 1.0);
        float att  = 1.0 / (1.0 + 0.15 * dot(uLampPos - vWorldPos, uLampPos - vWorldPos));
        col += uLampColor * d2 * att;
    }

    // Heat Beam glow
    if (uTempGlow > 0.004) {
        vec3 hot = mix(vec3(1.0, 0.25, 0.02), vec3(1.0, 0.95, 0.75), uTempGlow * 0.7);
        col += hot * uTempGlow * (0.25 + 0.75 * (1.0 - diff));
    }

    if (uSelected > 0.5) col += vec3(1.0, 0.85, 0.35) * 0.12 * uPulse;
    fragColor = vec4(col * uFade, 1.0);
}
)glsl";

// ---------------------------------------------------------------------------
//  Distant background sky (inverted sphere): nebula + hashed stars
// ---------------------------------------------------------------------------
static const char* sky_vs = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vDir;
void main() {
    vDir = aPos;
    gl_Position = uProj * uView * vec4(aPos * 900.0, 1.0);
}
)glsl";

static const char* sky_fs = R"glsl(
#version 330 core
precision highp float;
in vec3 vDir;
uniform float uTime;
out vec4 fragColor;

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.7);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x),
                   mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
                   mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; i++) {
        s += a * noise(p);
        p *= 2.03;
        a *= 0.5;
    }
    return s;
}

void main() {
    vec3 d = normalize(vDir);
    vec3 col = vec3(0.010, 0.010, 0.020);

    float n1 = fbm(d * 3.0 + vec3(7.0));
    float n2 = fbm(d * 6.0 - vec3(3.0));
    col += vec3(0.10, 0.055, 0.17) * smoothstep(0.42, 0.85, n1) * 0.55;
    col += vec3(0.045, 0.085, 0.13) * smoothstep(0.50, 0.90, n2) * 0.45;

    // hashed background stars
    vec3 g = d * 130.0;
    vec3 id = floor(g);
    float h = hash(id);
    if (h > 0.9968) {
        vec3 rnd = vec3(hash(id + 1.7), hash(id + 2.3), hash(id + 3.1));
        vec3 f = fract(g) - 0.5 - (rnd - 0.5) * 0.7;
        float r = length(f);
        float s = smoothstep(0.16, 0.0, r);
        float tw = 0.75 + 0.25 * sin(uTime * (0.6 + 2.5 * rnd.x) + h * 40.0);
        float big = 1.0 + 2.5 * (h - 0.9968) / 0.0032;
        col += vec3(0.75, 0.82, 1.0) * s * s * tw * big;
    }
    fragColor = vec4(col, 1.0);
}
)glsl";

// ---------------------------------------------------------------------------
//  Near starfield (GL points, twinkling)
// ---------------------------------------------------------------------------
static const char* stars_vs = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aSize;
layout(location = 2) in float aPhase;
uniform mat4 uView;
uniform mat4 uProj;
uniform float uTime;
out float vTw;
void main() {
    vec4 mv = uView * vec4(aPos, 1.0);
    gl_Position = uProj * mv;
    gl_PointSize = aSize * (420.0 / max(1.0, -mv.z));
    vTw = 0.65 + 0.35 * sin(uTime * (0.4 + aPhase * 0.8) + aPhase * 21.0);
}
)glsl";

static const char* stars_fs = R"glsl(
#version 330 core
precision mediump float;
in float vTw;
uniform vec3 uColor;
uniform float uFade;
out vec4 fragColor;
void main() {
    vec2 c = gl_PointCoord - 0.5;
    float a = smoothstep(0.5, 0.05, length(c));
    fragColor = vec4(uColor * uFade, a * vTw);
}
)glsl";

// ---------------------------------------------------------------------------
//  Lines (trails): per-vertex position + color
// ---------------------------------------------------------------------------
static const char* line_vs = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uView;
uniform mat4 uProj;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
)glsl";

static const char* line_fs = R"glsl(
#version 330 core
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)glsl";

// ---------------------------------------------------------------------------
//  UI (screen-space quads, optional font texture)
// ---------------------------------------------------------------------------
static const char* ui_vs = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;    // NDC
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec4 aColor;
out vec2 vUv;
out vec4 vColor;
void main() {
    vUv = aUv;
    vColor = aColor;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)glsl";

static const char* ui_fs = R"glsl(
#version 330 core
precision mediump float;
in vec2 vUv;
in vec4 vColor;
uniform sampler2D uTex;
out vec4 fragColor;
void main() {
    // Vertices with u >= 1.5 are solid-color rects; others sample the font.
    if (vUv.x < 1.5) {
        float a = texture(uTex, vUv).r * vColor.a;
        fragColor = vec4(vColor.rgb, a);
    } else {
        fragColor = vColor;
    }
}
)glsl";

// ---------------------------------------------------------------------------
//  Glow billboard (sun corona), additive
// ---------------------------------------------------------------------------
static const char* glow_vs = R"glsl(
#version 330 core
layout(location = 0) in vec3 aCorner; // x = right, y = up, in [-0.5, 0.5]
uniform vec3  uCenter;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform float uSize;
uniform mat4  uView;
uniform mat4  uProj;
out vec2 vUv;
void main() {
    vec3 world = uCenter
               + uCamRight * (aCorner.x * uSize)
               + uCamUp    * (aCorner.y * uSize);
    vUv = aCorner + 0.5;
    gl_Position = uProj * uView * vec4(world, 1.0);
}
)glsl";

static const char* glow_fs = R"glsl(
#version 330 core
precision mediump float;
in vec2 vUv;
uniform vec3  uColor;
uniform float uIntensity;
uniform float uTime;
out vec4 fragColor;
void main() {
    float d = length(vUv - 0.5) * 2.0;
    float core = exp(-d * 3.5);
    float halo = exp(-d * 1.4) * 0.30;
    float flick = 1.0 + 0.035 * sin(uTime * 6.3) + 0.025 * sin(uTime * 11.9 + 1.7);
    float a = clamp(core + halo, 0.0, 1.0);
    fragColor = vec4(uColor * a * uIntensity * flick, a);
}
)glsl";

// ---------------------------------------------------------------------------
//  GUI disc icons (Add-panel thumbnails, body avatars)
//  Screen-space quad; per-vertex colour + per-icon style params.
// ---------------------------------------------------------------------------
static const char* icon_vs = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;      // NDC corners of the icon quad
layout(location = 1) in vec2 aUv;       // 0..1 over the quad
layout(location = 2) in vec4 aColor;    // albedo (star: core colour)
layout(location = 3) in vec2 aParams;   // x = bandFreq, y = emissive
out vec2 vUv;
out vec4 vColor;
out vec2 vParams;
void main() {
    vUv = aUv;
    vColor = aColor;
    vParams = aParams;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)glsl";

static const char* icon_fs = R"glsl(
#version 330 core
precision mediump float;
in vec2 vUv;
in vec4 vColor;
in vec2 vParams;
out vec4 fragColor;

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    vec2 c = (vUv - 0.5) * 2.0;          // -1..1, circle space
    float d = length(c);
    if (d > 1.0) discard;

    vec3 S = vec3(c, sqrt(max(1.0 - d * d, 0.0)));

    if (vParams.y > 0.5) {
        // ---- star: hot radial core with mottled surface ----
        float m = hash12(S.xy * 7.0 + S.zz * 3.0) * 0.5 + hash12(S.xy * 15.0) * 0.5;
        float t = clamp(d * 1.35 - m * 0.35, 0.0, 1.0);
        vec3 col = mix(vColor.rgb * 1.5, vec3(1.0, 0.42, 0.06), t);
        col += vec3(1.0, 0.8, 0.4) * exp(-d * 5.0) * 0.9;
        fragColor = vec4(col, 1.0);
        return;
    }

    // ---- planet: lit sphere with bands + mottle ----
    vec3 N = normalize(S);
    vec3 L = normalize(vec3(-0.55, 0.35, 0.75));
    float diff = max(dot(N, L), 0.0);
    float mottle = hash12(S.xy * 9.0 + S.yz * 4.0);
    vec3 col = vColor.rgb * (0.28 + 1.05 * diff) * (0.85 + 0.3 * mottle);
    if (vParams.x > 0.5) {
        float lat = asin(clamp(N.y, -1.0, 1.0));
        float bands = sin(lat * vParams.x * 3.0);
        col *= 0.85 + 0.25 * bands;
    }
    float rim = pow(1.0 - N.z, 2.0);
    col += vColor.rgb * rim * 0.12;
    fragColor = vec4(col, 1.0);
}
)glsl";

} // namespace shaders
} // namespace us3d
