# Universe Sandbox 3D

A 3D gravitational universe sandbox written in **C++ / OpenGL 3.3 core** for
**Windows (x64)**. Create stars, fling planets around them, watch orbits,
collisions and mergers unfold in real time — then reverse time and undo it.

No external dependencies: no GLFW, no GLEW, no asset files. The window,
OpenGL context, font and every texture are created in code.

---

## Features

- **Real N-body gravity** (Newton, kick-drift-kick leapfrog integrator):
  stable orbits, reversible time, momentum-conserving collisions/merges.
- **Celestial bodies**: animated star surfaces, rocky planets, banded
  gas giants, moons, asteroid belts — all procedural (no textures on disk).
- **Full mouse UI** modelled on the original sandbox: bottom bar
  (Add / Tools / Find / Sim / View), a time bar, and tabbed panels —
  all drawn with a small immediate-mode GUI built on the GL quad pass.
- **Time machine**: pause, 0.1×–32× speed, and full **time reversal** (`N`).
- **Presets**: Solar System, Binary star, 400-body asteroid belt,
  star+moon, and random universes.
- **Save / load** the whole universe to a plain-text file (`Ctrl+S` / `Ctrl+O`).
- Orbit/pan/zoom camera, motion trails, twinkling starfield, nebula sky,
  sun glow, on-screen HUD with a built-in bitmap font.

## User interface

| Where | What |
|---|---|
| Bottom bar | **Add**, **Tools**, **Find** (center) and **Sim**, **View** (right) — each opens/closes its panel |
| Time bar (bottom-left) | Pause, reverse/forward, simulated date and time speed (`wheel` over it changes speed) |
| **Add** panel | Catalog of real bodies (Sun…Pluto), random moon/planet/star, proto-objects and saved scenarios. `Binary` spawns a gravitationally bound pair, `Auto orbit` places bodies on circular orbits, `Repeat` keeps the panel open |
| **Tools** panel | Terraform (cycle rock/water/ice/air), Heat Beam (right-click cools), Torque (right-click reverses), Detonate (breaks the focused world into fragments), Slingshot, Brake, Circularize, Stop Orbit, Follow, Reset View |
| **Find** panel | Searchable, tab-filtered list of every body (mass-sorted); click to select & fly to it; `wheel` scrolls |
| **Sim** panel | Scenario name/body count, Gravity/Impacts/Debris/Heat toggles, Overview / Advanced (time speed, trails, pause) / Stats (energy, speeds, FPS) |
| **View** panel | *Camera*: Looking at, Go to, See all, Reset; *Field of view* slider; *Worlds*: Atmospheres, Clouds, Charts (trails); *Colour by*: true colour / mass / speed / temperature; *Lamp*: on/off, side, strength, reach, colour temperature |
| **Info** panel (auto when a body is selected) | *Physical* (mass, radius, density, temperature, gravity, pressure, luminosity, speed, rotation, orbital period, age, spectral type), *Materials*, *Climate*, *Orbit* (distance, period, semi-major axis, eccentricity), *Appearance* (colour, radius, mass, spin) |
| Top-left | FPS + hamburger button (controls help) |

All panels work with mouse only; the keyboard shortcuts below still work.

## Controls

| Input                | Action                                  |
|----------------------|-----------------------------------------|
| Left drag            | Orbit camera                            |
| Right / middle drag  | Pan camera                              |
| Mouse wheel          | Zoom                                    |
| Left click           | Select body                             |
| Double click         | Focus camera + zoom to body             |
| `1` / `2` / `3`      | Spawn planet / star / asteroid          |
| `Del`                | Delete selected body                    |
| `Space`              | Pause / resume                          |
| `+` / `-`            | Time speed up / down (0.1×–32×)         |
| `N`                  | Reverse / restore time direction        |
| `F`                  | Toggle camera follow                    |
| `G`                  | Toggle motion trails                    |
| `H`                  | Toggle help overlay                     |
| `9` `8` `0` `7`      | Presets: solar / binary / belt / moon   |
| `R`                  | New random universe                     |
| `Ctrl+S` / `Ctrl+O`  | Save / load (`universe_save.txt`)       |
| `F11`                | Toggle fullscreen                       |
| `Esc`                | Close topmost panel, then quit          |

## Requirements

| | |
|---|---|
| OS | **Windows 7 SP1**, **Windows 10** or **Windows 11** — **64-bit (x64)** |
| Graphics | Any GPU/driver with **OpenGL 3.3** (nearly everything from 2010 onward; the app checks and reports a clear error otherwise) |
| RAM | 1 GB |
| C runtime | The **installer / MSVC exe** is fully self-contained (static runtime).
  The **cross-built exe** additionally needs the **Universal C Runtime on
  Windows 7**, i.e. any VC++ 2015–2022 redistributable (most Win7 SP1
  machines have it; install `vc_redist.x64.exe` if not). Windows 10/11
  always have it built in. |

## Download a ready build (GitHub Actions)

No build tools needed — CI compiles the app on every commit and publishes
downloadable artifacts. Go to the [Actions](https://github.com/BT-Studio-dev/Universe-Sandbox-3D/actions)
tab, open the latest green run, and grab the artifact you want:

| Artifact (on the run page → *Artifacts*) | Contents |
|---|---|
| `UniverseSandbox3D-Setup-win64` | **`UniverseSandbox3D-Setup-1.0.0-win64.exe`** — NSIS installer (adds a Start-menu entry, desktop shortcut and an uninstaller), plus the raw `us3d.exe`. Built with MSVC. |
| `UniverseSandbox3D-exe-cross-win64` | Just `us3d.exe`, built with the MinGW-w64 cross compiler. |

Both binaries run on **Windows 7 SP1 / 10 / 11 (x64)**. To install, run the
Setup exe (accept the UAC prompt); or just copy `us3d.exe` anywhere and
double-click it — it has no dependencies beyond a 64-bit OS and an
OpenGL 3.3 driver.

## Building (Windows, x64)

### Option A — one-click MSVC build (recommended)

1. Install **CMake 3.16+** and **Visual Studio 2022** (or 2019) with the
   *“Desktop development with C++”* workload.
2. Double-click **`build.bat`** (or run it in a terminal).

Output: `build\Release\us3d.exe`

The project is compiled with `_WIN32_WINNT=0x0601`, so the binary targets
Windows 7 while building against the Windows 10/11 SDK.

### Option B — open the Visual Studio solution

Open **`vs\UniverseSandbox.sln`** (x64, Debug/Release) and build.
Output: `vs\bin\x64\Release\us3d.exe`.

### Option C — MinGW-w64 (also the way to build *on* Windows 7)

1. Install **CMake 3.16+** and a 64-bit **MinGW-w64** toolchain
   (`x86_64-w64-mingw32-g++` in `PATH`).
2. Run **`build_mingw.bat`**.

Output: `build\us3d.exe`

### Raw CMake

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Building / testing anywhere (Linux, macOS)

The physics core is platform-independent, so you can build and run the
headless test suite and the Win32 compile-check on any machine:

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure    # runs the physics tests
cmake --build build --target stub_check       # compile-checks the Win32 layer
```

- **`test_sim`** — verifies orbit stability (5 years), energy conservation
  (<1% drift/year), collision mass/momentum conservation, time-reversal
  round-trips and save/load round-trips.
- **`stub_check`** — compiles every Windows-only translation unit against a
  minimal stub `<windows.h>` (`tools/compile-check/`), catching real
  errors in the Win32 code path without a Windows SDK.

GitHub Actions (`.github/workflows/ci.yml`) runs all of the above plus real
MSVC and MinGW-w64 Windows builds on every push.

## Project layout

```
src/
  main.cpp               entry point (wWinMain, DPI awareness)
  app.h/.cpp             application controller: input, actions, main loop
  win32/window.h/.cpp    Win32 window + WGL 3.3 core context, input callbacks
  gl/gl33.h/.cpp         self-contained OpenGL 3.3 loader (no GLEW)
  render/
    renderer.h/.cpp      shaders, meshes, starfield, trails, glow, font atlas
    camera.h/.cpp        orbit camera + picking ray
    hud.h/.cpp           2D overlay (text/rects)
  core/
    math.h               Vec3/Mat4 (column-major, GL-compatible)
    body.h               celestial body
    system.h/.cpp        N-body integrator, collisions, trails
    presets.h/.cpp       solar system, binary, asteroid belt, random
    savestate.h/.cpp     text save/load (US3D v2, reads v1)
    catalog.h            Add-panel catalog + display/physics formatting
    shaders.h/.cpp       GLSL 3.30 shaders (declarations / sources)
  ui/
    gui.h/.cpp           immediate-mode GUI (panels, buttons, sliders, discs)
    panels.h/.cpp        the UI panels (Add/Tools/Find/Sim/View/Info/time bar)
    font5x7.h            embedded 5x7 font (generated by tools/make_font.py)
tests/test_sim.cpp       headless physics test suite
tools/compile-check/     stub <windows.h> + check.sh (Linux compile check)
vs/                      Visual Studio solution/project
```

## Simulation notes

- **Units**: solar masses, AU, years → `G = 4π²`.
- **Integrator**: kick-drift-kick leapfrog at 512 steps/year, substepped to
  match the time speed. It is symplectic (orbits don't decay) and exactly
  reversible — press `N` and watch the solar system un-happen.
- **Collisions**: bodies that touch merge conserving mass and momentum;
  the survivor grows.
- **Sizes**: display radii are exaggerated (real planetary radii are ~0.00004
  AU — invisible at system scale) and also drive collision detection. This
  is a sandbox, not a paper.

## Save file format

```
US3D SAVE 2
time <years>
reversed <0|1>
body "<name>" <type> <x y z> <vx vy vz> <mass> <radius> <r g b> <bandFreq> \
       <realRadiusAU> <surface> <temperatureC> <spin>
```

`surface`: 0 rock, 1 water, 2 ice, 3 air. v1 files (without the last four
fields) are still loaded.

## Troubleshooting

- **“OpenGL 3.3 not available” dialog** — your graphics driver reports OpenGL
  below 3.3 (common on old Windows 7 machines with very old GPU drivers, on
  VMs, or on driver-less hardware). The dialog now prints a *Context
  bootstrap* report showing exactly which 3.3-context attempt succeeded or
  failed, plus the GL version / vendor / renderer — include it when
  reporting the issue.

  Two ways forward:

  1. **Update the graphics driver** (real GPU, physical PC).
  2. **Software rendering with Mesa/llvmpipe** (VMs, headless boxes, old
     integrated GPUs). This is fully supported:
     - Get the *Mesa for Windows* x64 build
       (<https://www.mesa3d.org/download.html>) and place `opengl32.dll`
       and `libgallium_wgl.dll` in the **same folder as `us3d.exe`**.
     - Start the app with the included **`run_mesa.bat`** (it forces Mesa to
       advertise an OpenGL 3.3 Core / GLSL 330 context). You can also do it
       by hand in a terminal:
       ```bat
       set MESA_GL_VERSION_OVERRIDE=3.3FC
       set MESA_GLSL_VERSION_OVERRIDE=330
       us3d.exe
       ```
     Expect software-rendered frame rates (tens of FPS for a modest
     particle count, not hundreds).

- **Black screen on Windows 7 with integrated graphics** — same as above:
  update the GPU driver, or use the Mesa/llvmpipe fallback; OpenGL 3.3 is
  the minimum.
- **Jittery at 32× speed** — that is 512 physics substeps per frame; it is
  intentional to keep orbits stable at high speed.
- **Where are saves?** Next to the executable (`universe_save.txt`).

## License

MIT — see [LICENSE](LICENSE).

## Website

A self-contained landing page lives in `site/` (single `index.html`, no
dependencies, no build step). Serve it with any static file server, e.g.
`python3 -m http.server --directory site`. The hero animation is a live
9-body N-simulation rendered on `<canvas>` with the same physics as the app.
