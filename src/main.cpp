// ============================================================================
//  Universe Sandbox 3D - entry point (Windows 7 / 10 / 11, x64)
//
//  Subsystem: WINDOWS (no console). Entry: wWinMain.
// ============================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "app.h"

#ifdef _MSC_VER
#  pragma comment(lib, "user32")
#  pragma comment(lib, "gdi32")
#  pragma comment(lib, "opengl32")
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    LPWSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    // Per-monitor DPI: we render in physical pixels; keep the OS from
    // bitmap-stretching us on high-DPI displays (no-op on Windows 7).
    typedef BOOL (WINAPI *SetDpiAware_t)(void);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        SetDpiAware_t setAware =
            reinterpret_cast<SetDpiAware_t>(
                reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDPIAware")));
        if (setAware) setAware();
    }

    us3d::App app;
    if (!app.init(hInstance)) {
        // Window may exist; report if the error is meaningful.
        MessageBoxA(nullptr, "Failed to start Universe Sandbox 3D.\n\n"
                             "Check that a graphics driver with OpenGL 3.3 is available.",
                    "Universe Sandbox 3D", MB_OK | MB_ICONERROR);
        app.shutdown();
        return 1;
    }

    app.run();
    app.shutdown();
    return 0;
}
