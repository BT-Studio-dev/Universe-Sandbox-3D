# Universe Sandbox 3D

A real-time **N-body gravity universe sandbox** rendered in OpenGL, written
in **C#** (.NET Framework 4.5, raw Win32 + WGL — no WinForms, no external
packages). Spawn stars, planets, moons and asteroid belts; bend gravity;
reverse time; save your universe.

Runs on **Windows 7 SP1, Windows 10 and Windows 11 — 64-bit (x64)**.

> **Status:** C# rewrite in progress.
> **Phase 1 (foundation) is complete**: Win32 window, the full WGL 3.3
> core/compat context bootstrap (including the Mesa/llvmpipe-friendly
> fallback path and the self-diagnosing "OpenGL 3.3 not available" dialog),
> self-contained GL 3.3 loader, ~60 FPS render loop.
> The complete C++ v1 implementation is preserved at git tag
> [`v1-cpp`](https://github.com/BT-Studio-dev/Universe-Sandbox-3D/releases/tag/v1-cpp)
> (and in the CI artifacts) for reference or fallback.

## Roadmap

| Phase | Scope | Status |
|-------|-------|--------|
| 1 | Win32 window · WGL 3.3 bootstrap · GL 3.3 loader · render loop | ✅ done |
| 2 | N-body core (leapfrog KDK, collisions/merges) + headless tests | next |
| 3 | Renderer: shaders (GLSL 330), starfield, bodies, glow, trails, rings | — |
| 4 | HUD + immediate-mode GUI (Add / Tools / Find / Sim / View panels) | — |
| 5 | Presets · save/load · time controls · fullscreen | — |
| 6 | CI build + Win7 PE/import audit · NSIS installer · final polish | — |

## Features (target, as in C++ v1)

- **Real N-body gravity** (Newton, kick-drift-kick leapfrog, G = 4π²):
  stable orbits, reversible time, momentum-conserving collisions/merges.
- **Celestial bodies**: animated star surfaces, rocky planets, banded gas
  giants, moons, asteroid belts — all procedural.
- **Time machine**: pause, 0.1×–32× speed, full time reversal.
- **Presets**: Solar System, Binary star, 400-body asteroid belt,
  Star + Moon, random universes. **Save/load** to plain text.
- Orbit/pan/zoom camera, motion trails, twinkling starfield, sun glow,
  on-screen HUD — full mouse UI (bottom bar, time bar, tabbed panels).

## Requirements

| | |
|---|---|
| OS | **Windows 7 SP1**, **Windows 10** or **Windows 11** — **64-bit (x64)** |
| Graphics | Any GPU/driver with **OpenGL 3.3** (or Mesa llvmpipe software rendering) |
| Runtime | **.NET Framework 4.5** — preinstalled on Windows 7 SP1 / 8 / 10 / 11 |
| RAM | 1 GB |

The C# build is a single self-contained exe with **no runtime
prerequisites** beyond .NET 4.5 (no VC++ redistributable needed, unlike
the C++ cross build).

## Building (Windows, x64)

### Option A — one-click, no Visual Studio needed
```bat
build_csc.bat
```
Uses the stock .NET Framework `csc.exe`. Output: `UniverseSandbox3D.exe`.

### Option B — Visual Studio
Open `UniverseSandbox3D.csproj` (.NET Framework 4.5, x64, WinExe) and
build.

### Run
```bat
UniverseSandbox3D.exe
```
Phase 1 shows a dark window titled with the detected GL version and
renderer (e.g. `… - 4.6.0 NVIDIA 566.36 | NVIDIA GeForce …`).

## Software rendering (VMs / no GPU driver)

Same as v1: put the *Mesa for Windows* x64 DLLs (`opengl32.dll` +
`libgallium_wgl.dll`, <https://www.mesa3d.org/download.html>) next to the
exe and start with **`run_mesa.bat`**.

## Project layout

```
src/
  Program.cs            entry point
  App.cs                app controller, init + render loop (phase 1)
  Win32/
    NativeMethods.cs    Win32/WGL P/Invoke declarations
    Window.cs           window + WGL 3.3 bootstrap + message loop
    GL33.cs             self-contained OpenGL 3.3 loader
  Core/                 (phase 2) N-body system, presets, save/load
  Render/               (phase 3) renderer, camera, HUD, shaders
  Ui/                   (phase 4) immediate-mode GUI + panels
site/                   self-contained landing page (see below)
```

## Website

A self-contained landing page lives in `site/` (single `index.html`, no
dependencies). Serve it with any static file server, e.g.
`python3 -m http.server --directory site`. The hero animation is a live
9-body N-simulation rendered on `<canvas>` with the same physics as the
app.

## C++ v1

The complete C++/OpenGL implementation (Windows 7/10/11, installer, CI)
is preserved at tag **`v1-cpp`** — checkout that tag to rebuild it:
```bat
git checkout v1-cpp
build.bat
```

## Troubleshooting

- **“OpenGL 3.3 not available” dialog** — the dialog prints a full
  *Context bootstrap* report (every 3.3-context attempt with its WGL
  error code, plus GL version/vendor/renderer). Update the GPU driver,
  or use the Mesa/llvmpipe fallback above.
- **“UniverseSandbox3D.exe is not recognized…” on clean Windows 7** —
  install .NET Framework 4.5+ (preinstalled on Win7 SP1 with updates).

## License

MIT — see [LICENSE](LICENSE).
