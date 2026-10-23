# ============================================================================
#  Universe Sandbox 3D - CMake build
#
#  Windows (the primary target, Win7 SP1 / 10 / 11 x64):
#      cmake -B build -G "Visual Studio 17 2022" -A x64     (MSVC)
#      cmake -B build -G "MinGW Makefiles"                   (MinGW-w64)
#      cmake --build build --config Release
#
#  Anywhere (Linux/macOS dev boxes):
#      cmake -B build && cmake --build build
#        - builds test_sim  : headless physics test suite (run it!)
#        - builds stub_check: compile-checks the Windows layer vs stub headers
# ============================================================================
cmake_minimum_required(VERSION 3.16)
project(UniverseSandbox3D VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ----------------------------------------------------------------------------
#  Common flags
# ----------------------------------------------------------------------------
if(MSVC)
    # Target Windows 7 and later; keep the Win10/11 SDK happy.
    set(CMAKE_SYSTEM_VERSION 7)
    add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601 NOMINMAX)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ----------------------------------------------------------------------------
#  test_sim - headless simulation tests (all platforms)
# ----------------------------------------------------------------------------
set(US3D_CORE_SOURCES
    src/core/system.cpp
    src/core/presets.cpp
    src/core/savestate.cpp
)

add_executable(test_sim
    tests/test_sim.cpp
    ${US3D_CORE_SOURCES}
)
target_include_directories(test_sim PRIVATE src)

enable_testing()
add_test(NAME test_sim COMMAND test_sim)

# ----------------------------------------------------------------------------
#  us3d - the game (Windows only)
# ----------------------------------------------------------------------------
if(WIN32)
    add_executable(us3d WIN32
        src/main.cpp
        src/app.cpp
        src/win32/window.cpp
        src/gl/gl33.cpp
        src/render/renderer.cpp
        src/render/camera.cpp
        src/render/hud.cpp
        ${US3D_CORE_SOURCES}
    )
    target_include_directories(us3d PRIVATE src)
    target_link_libraries(us3d PRIVATE user32 gdi32 opengl32)

    if(MSVC)
        # wWinMain entry (no console window).
        target_link_options(us3d PRIVATE /SUBSYSTEM:WINDOWS)
    else()
        # MinGW-w64: use the wide entry point.
        target_link_options(us3d PRIVATE -municode)
    endif()
endif()

# ----------------------------------------------------------------------------
#  stub_check - compile the Windows code paths against stub <windows.h>
#  (useful on Linux CI; not needed on Windows itself)
# ----------------------------------------------------------------------------
if(NOT WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/tools/compile-check/check.sh")
    add_custom_target(stub_check
        COMMAND sh ${CMAKE_SOURCE_DIR}/tools/compile-check/check.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Compile-checking the Windows layer against stub headers"
    )
endif()
