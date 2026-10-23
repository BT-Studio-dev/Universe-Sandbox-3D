// ============================================================================
//  Universe Sandbox 3D - Win32 window + WGL 3.3 core context
//
//  Creates an OpenGL 3.3 core-profile context on the window. Works on
//  Windows 7 SP1 / 8 / 10 / 11 (x64). Requires a driver with OpenGL 3.3.
// ============================================================================
#pragma once

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM
#endif

#include <functional>
#include <string>

namespace us3d {

class Window {
public:
    Window() = default;
    ~Window();

    // Registers the class, creates the window, sets the pixel format and
    // creates a 3.3 core context. On failure returns false and error() is
    // filled with a human readable reason.
    bool create(HINSTANCE hInstance, const char* title, int width, int height,
                bool fullscreen = false);

    void destroy();

    HWND  handle() const { return hwnd_; }
    HDC   dc() const     { return hdc_; }

    int   clientWidth() const  { return width_; }
    int   clientHeight() const { return height_; }

    void swapBuffers();

    // vsync control (WGL_EXT_swap_control; no-op if unavailable).
    void setVSync(bool on);

    bool toggleFullscreen();

    const char* error() const { return errorBuf_; }

    // Multi-line diagnostics captured during create(): GL version /
    // vendor / renderer and the outcome of every 3.3-context attempt.
    // Shown in the "incompatible GPU driver" dialog so failures on
    // software renderers (Mesa/llvmpipe) or VMs are self-explaining.
    const std::string& glReport() const { return glReport_; }

    // Callbacks driven by the message loop.
    std::function<void(int w, int h)>        onResize;
    std::function<void()>                     onClose;
    std::function<void(int vk, bool down)>    onKey;
    std::function<void(int button)>           onMouseDown; // 0 left, 1 middle, 2 right
    std::function<void(int button)>           onMouseUp;
    std::function<void(int x, int y, int dx, int dy)> onMouseMove;
    std::function<void(int delta)>            onWheel;
    std::function<void()>                     onFocusLost;
    std::function<void(char)>                 onChar;    // printable (32..126)

    // Process one pending message; returns false when WM_QUIT is seen.
    static bool pumpMessage();
    static void waitMessage();   // blocks until a message arrives

    // Cursor position in client pixels (screen-independent).
    static void cursorPos(int& x, int& y);

    // Restore the default cursor after we hide it, etc.
    static void showCursor(bool show);

private:
    static LRESULT CALLBACK wndProcThunk(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    static void setError(Window* w, const char* fmt, ...);

    HWND hwnd_ = nullptr;
    HDC  hdc_  = nullptr;
    void* glctx_ = nullptr;   // HGLRC
    std::string glReport_;
    int  width_ = 0, height_ = 0;
    int  lastMouseX_ = 0, lastMouseY_ = 0;
    bool fullscreen_ = false;
    char errorBuf_[256] = "";
};

} // namespace us3d
