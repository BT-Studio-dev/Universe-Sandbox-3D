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
- **Interact with the universe**
  - Spawn planets / stars / asteroids (`1` `2` `3`)
  - Select & inspect bodies (click), focus & zoom (double-click)
  - Delete bodies (`Del`), trigger collisions, build binary stars
- **Time machine**: pause, 0.1×–32× speed, and full **time reversal** (`N`).
- **Presets**: Solar System, Binary star, 400-body asteroid belt,
  star+moon, and random universes.
- **Save / load** the whole universe to a plain-text file (`Ctrl+S` / `Ctrl+O`).
- Orbit/pan/zoom camera, motion trails, twinkling starfield, nebula sky,
  sun glow, on-screen HUD with a built-in bitmap font.

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
| `Esc`                | Quit                                    |

## Requirements

| | |
|---|---|
| OS | **Windows 7 SP1**, **Windows 10** or **Windows 11** — **64-bit (x64)** |
| Graphics | Any GPU/driver with **OpenGL 3.3** (nearly everything from 2010 onward; the app checks and reports a clear error otherwise) |
| RAM | 1 GB |

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
    savestate.h/.cpp     text save/load (US3D v1)
    shaders.h            all GLSL 3.30 shaders
  ui/font5x7.h           embedded 5x7 font (generated by tools/make_font.py)
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
US3D SAVE 1
time <years>
reversed <0|1>
body "<name>" <type> <x y z> <vx vy vz> <mass> <radius> <r g b> <bandFreq>
```

## Troubleshooting

- **“incompatible GPU driver” dialog** — your graphics driver reports OpenGL
  below 3.3. Update the driver (common on old Windows 7 machines with
  very old GPU drivers).
- **Black screen on Windows 7 with integrated graphics** — same fix: update
  the GPU driver; OpenGL 3.3 is the minimum.
- **Jittery at 32× speed** — that is 512 physics substeps per frame; it is
  intentional to keep orbits stable at high speed.
- **Where are saves?** Next to the executable (`universe_save.txt`).

## License

MIT — see [LICENSE](LICENSE).
