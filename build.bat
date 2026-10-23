@echo off
rem ============================================================================
rem  Universe Sandbox 3D - one-shot MSVC build (Visual Studio 2022 / 2019)
rem
rem  Produces build\Release\UniverseSandbox3D.exe (or us3d.exe) which runs on
rem  Windows 7 SP1, Windows 10 and Windows 11 (x64).
rem
rem  Requirements: CMake 3.16+ and Visual Studio 2019/2022 with the
rem  "Desktop development with C++" workload.
rem ============================================================================
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH. Install CMake 3.16+ first.
    exit /b 1
)

rem Pick the newest Visual Studio with the C++ toolset (2022 preferred).
set VS_GENERATOR=
for %%G in ("Visual Studio 17 2022" "Visual Studio 16 2019") do (
    if not defined VS_GENERATOR (
        rmdir /s /q build >nul 2>nul
        cmake -B build -G "%%~G" -A x64 >nul 2>nul
        if not errorlevel 1 set VS_GENERATOR=%%~G
    )
)

if not defined VS_GENERATOR (
    echo [ERROR] No Visual Studio 2019/2022 with the C++ toolset was found.
    echo         Install it, or use build_mingw.bat instead.
    exit /b 1
)

echo [INFO] Configuring with generator: %VS_GENERATOR%
cmake --build build --config Release --parallel || exit /b 1

echo.
echo ============================================================
echo  Build complete.
for %%F in (build\Release\us3d.exe build\Release\UniverseSandbox3D.exe) do (
    if exist "%%F" echo  Binary: %%F
)
echo  Runs on Windows 7 SP1 / 10 / 11 (x64) with an OpenGL 3.3 driver.
echo ============================================================
endlocal
