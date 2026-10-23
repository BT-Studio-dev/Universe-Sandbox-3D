@echo off
rem ============================================================================
rem  Universe Sandbox 3D - one-shot MinGW-w64 build
rem
rem  Use this on Windows 7 itself (or anywhere) when Visual Studio is not
rem  available. Requires CMake 3.16+ and a 64-bit MinGW-w64 toolchain in
rem  PATH (g++.exe must be x86_64-w64-mingw32-g++).
rem
rem  Produces build\us3d.exe, which runs on Windows 7 / 10 / 11 (x64).
rem ============================================================================
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH. Install CMake 3.16+ first.
    exit /b 1
)

g++ --version >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ not found in PATH. Install MinGW-w64 (x86_64) first.
    exit /b 1
)

cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build --parallel || exit /b 1

echo.
echo ============================================================
if exist "build\us3d.exe" (
    echo  Build complete: build\us3d.exe
    echo  Runs on Windows 7 / 10 / 11 (x64) with an OpenGL 3.3 driver.
) else (
    echo  Build finished - look for us3d.exe under build\.
)
echo ============================================================
endlocal
