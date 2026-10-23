// ============================================================================
//  Universe Sandbox 3D - Win32 window + WGL context (implementation)
// ============================================================================
#include "win32/window.h"
#include "gl/gl33.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace us3d {

namespace {

// WGL_EXT_swap_control (vsync) - resolved at runtime, optional.
typedef int (WINAPI *SwapIntervalEXT_t)(int);
static SwapIntervalEXT_t s_swapIntervalEXT = nullptr;

Window* s_instance = nullptr;

} // namespace

LRESULT CALLBACK Window::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* w = s_instance;
    switch (msg) {
    case WM_SIZE:
        w->width_  = LOWORD(lParam);
        w->height_ = HIWORD(lParam);
        if (w->onResize) w->onResize(w->width_, w->height_);
        return 0;

    case WM_CLOSE:
        if (w->onClose) w->onClose();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (w->onKey) w->onKey((int)wParam, true);
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (w->onKey) w->onKey((int)wParam, false);
        return 0;

    case WM_LBUTTONDOWN:
        if (w->onMouseDown) w->onMouseDown(0);
        return 0;
    case WM_MBUTTONDOWN:
        if (w->onMouseDown) w->onMouseDown(1);
        return 0;
    case WM_RBUTTONDOWN:
        if (w->onMouseDown) w->onMouseDown(2);
        return 0;
    case WM_LBUTTONUP:
        if (w->onMouseUp) w->onMouseUp(0);
        return 0;
    case WM_MBUTTONUP:
        if (w->onMouseUp) w->onMouseUp(1);
        return 0;
    case WM_RBUTTONUP:
        if (w->onMouseUp) w->onMouseUp(2);
        return 0;

    case WM_MOUSEMOVE: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        if (w->onMouseMove)
            w->onMouseMove(x, y, x - w->lastMouseX_, y - w->lastMouseY_);
        w->lastMouseX_ = x;
        w->lastMouseY_ = y;
        return 0;
    }

    case WM_MOUSEWHEEL:
        if (w->onWheel) w->onWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_CHAR:
        if (w->onChar && wParam >= 32 && wParam < 127)
            w->onChar((char)wParam);
        return 0;

    case WM_KILLFOCUS:
        if (w->onFocusLost) w->onFocusLost();
        return 0;

    case WM_GETMINMAXINFO: {
        // Don't let the window become smaller than 640x360.
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 640;
        mmi->ptMinTrackSize.y = 360;
        return 0;
    }

    case WM_DPICHANGED:
        // Simple approach: resize to the OS-suggested rect (Win10+; ignored
        // on Windows 7 which never posts this message).
        if (lParam) {
            RECT* r = reinterpret_cast<RECT*>(lParam);
            const int w = r->right - r->left;
            const int h = r->bottom - r->top;
            SetWindowPos(hwnd, nullptr, r->left, r->top, w, h,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void Window::setError(Window* w, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(w->errorBuf_, sizeof w->errorBuf_, fmt, ap);
    va_end(ap);
}

// ---------------------------------------------------------------------------
Window::~Window() { destroy(); }

bool Window::create(HINSTANCE hInstance, const char* title, int width, int height,
                    bool wantFullscreen) {
    // Resolve WGL entry points (needed before context creation).
    if (!gl33::wglCreateContext) gl33::load();

    const char clsName[] = "UniverseSandbox3D";
    WNDCLASSEX wc;
    std::memset(&wc, 0, sizeof wc);
    wc.cbSize        = sizeof wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = wndProcThunk;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = clsName;
    if (!RegisterClassEx(&wc)) {
        setError(this, "RegisterClassEx failed (error %lu)", (unsigned)GetLastError());
        return false;
    }

    RECT rc = { 0, 0, width, height };
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    if (wantFullscreen) {
        fullscreen_ = true;
        MONITORINFO mi;
        mi.cbSize = sizeof mi;
        if (GetMonitorInfoW(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTONEAREST), &mi))
            rc = mi.rcWork;
        style = WS_POPUP;
    } else {
        AdjustWindowRectEx(&rc, style, FALSE, 0);
    }

    hwnd_ = CreateWindowEx(0, clsName, title, style,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           rc.right - rc.left, rc.bottom - rc.top,
                           nullptr, nullptr, hInstance, this);
    if (!hwnd_) {
        setError(this, "CreateWindowEx failed (error %lu)", (unsigned)GetLastError());
        return false;
    }

    hdc_ = GetDC(hwnd_);
    if (!hdc_) {
        setError(this, "GetDC failed");
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        return false;
    }

    // ---- pixel format ----
    PIXELFORMATDESCRIPTOR pfd;
    std::memset(&pfd, 0, sizeof pfd);
    pfd.nSize      = sizeof pfd;
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int pf = ChoosePixelFormat(hdc_, &pfd);
    if (pf == 0) {
        setError(this, "ChoosePixelFormat failed (error %lu)", (unsigned)GetLastError());
        return false;
    }
    if (!SetPixelFormat(hdc_, pf, &pfd)) {
        setError(this, "SetPixelFormat failed (error %lu)", (unsigned)GetLastError());
        return false;
    }

    // ---- GL 3.3 context (MSDN bootstrap) ----
    // Resolve the always-available opengl32 entry points first. (Some GL
    // functions need a current context to resolve; they are re-resolved
    // after the context exists below.)
    gl33::load();

    std::string report;

    // Step 1: a temporary legacy context. Several drivers (notably Intel,
    // and some AMD/NVIDIA on Windows 7) only expose
    // wglCreateContextAttribsARB through wglGetProcAddress once *some*
    // context is current - the MSDN-recommended creation flow.
    void* legacyCtx = nullptr;
    if (gl33::wglCreateContext) {
        legacyCtx = gl33::wglCreateContext(hdc_);
    }
    if (!legacyCtx) {
        setError(this, "wglCreateContext failed (error %lu)",
                 (unsigned)GetLastError());
        return false;
    }
    if (!gl33::wglMakeCurrent(hdc_, legacyCtx)) {
        setError(this, "wglMakeCurrent failed (error %lu)", (unsigned)GetLastError());
        gl33::wglDeleteContext(legacyCtx);
        return false;
    }
    const std::string legacyVersion = gl33::versionString();
    report += "Context bootstrap:\n";
    report += "  1. legacy context: created, reports '" + legacyVersion + "'\n";

    // Step 2: a modern 3.3 context. Try several entry-point names and
    // profiles in order: real GPU drivers almost always expose
    // wglCreateContextAttribsARB with a core profile, but the
    // Mesa-for-Windows (llvmpipe) drop-in and some older drivers expose
    // only the Windows 8+ core name, or only a compatibility profile.
    // This app uses nothing core-only, so a 3.3 compatibility context is
    // equally acceptable.
    if (!gl33::wglCreateContextAttribsARB || !gl33::wglCreateContextAttribs) gl33::load();

    void* ctx = legacyCtx;
    struct Attempt {
        PFNWGLCREATECONTEXTATTRIBPROC fn;
        const char* label;
        int         profile;
    };
    const Attempt attempts[] = {
        { gl33::wglCreateContextAttribsARB, "wglCreateContextAttribsARB / 3.3 core",  WGL_CONTEXT_CORE_PROFILE_BIT_ARB },
        { gl33::wglCreateContextAttribsARB, "wglCreateContextAttribsARB / 3.3 compat", WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB },
        { gl33::wglCreateContextAttribs,    "wglCreateContextAttribs / 3.3 core",     WGL_CONTEXT_CORE_PROFILE_BIT_ARB },
        { gl33::wglCreateContextAttribs,    "wglCreateContextAttribs / 3.3 compat",   WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB },
    };
    bool modernOk = false;
    for (size_t i = 0; i < sizeof(attempts) / sizeof(attempts[0]) && !modernOk; ++i) {
        const std::string step = "  2." + std::to_string(i + 1) + " ";
        if (!attempts[i].fn) {
            report += step + attempts[i].label + ": entry point not resolved\n";
            continue;
        }
        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB,  attempts[i].profile,
            0,
        };
        void* newCtx = attempts[i].fn(hdc_, nullptr, attribs);
        if (newCtx) {
            if (gl33::wglMakeCurrent(hdc_, newCtx)) {
                gl33::wglDeleteContext(legacyCtx);
                ctx = newCtx;
                modernOk = true;
                report += step + attempts[i].label + ": OK\n";
            } else {
                report += step + attempts[i].label + ": created, wglMakeCurrent failed (error "
                        + std::to_string(GetLastError()) + ")\n";
                gl33::wglDeleteContext(newCtx);
            }
        } else {
            report += step + attempts[i].label + ": failed (wgl error "
                    + std::to_string(GetLastError()) + ")\n";
        }
    }
    if (!modernOk) {
        report += "  -> keeping legacy context (reported version: " + legacyVersion + ")\n";
    }
    report += "GL_VERSION : " + gl33::versionString() + "\n";
    report += "GL_VENDOR  : " + gl33::vendorString() + "\n";
    report += "GL_RENDERER: " + gl33::rendererString() + "\n";
    glReport_ = report;

    glctx_ = ctx;

    if (!gl33::load()) {
        setError(this, "Failed to resolve OpenGL entry points (wglGetProcAddress).");
        return false;
    }

    // Optional WGL_EXT_swap_control (vsync) - may be absent on old drivers.
    {
        typedef void* (WINAPI *GetProc_t)(const char*);
        GetProc_t wglp = nullptr;
        HMODULE o32 = GetModuleHandleA("opengl32.dll");
        if (o32) wglp = reinterpret_cast<GetProc_t>(GetProcAddress(o32, "wglGetProcAddress"));
        if (wglp)
            s_swapIntervalEXT = reinterpret_cast<SwapIntervalEXT_t>(
                wglp("wglSwapIntervalEXT"));
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

void Window::destroy() {
    if (glctx_) {
        gl33::wglMakeCurrent(nullptr, nullptr);
        gl33::wglDeleteContext(glctx_);
        glctx_ = nullptr;
    }
    if (hdc_) {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Window::swapBuffers() {
    if (hdc_) SwapBuffers(hdc_);
}

void Window::setVSync(bool on) {
    if (s_swapIntervalEXT) s_swapIntervalEXT(on ? 1 : 0);
}

bool Window::toggleFullscreen() {
    fullscreen_ = !fullscreen_;
    if (!hwnd_) return false;

    if (fullscreen_) {
        MONITORINFO mi;
        mi.cbSize = sizeof mi;
        GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLong(hwnd_, GWL_STYLE, WS_POPUP);
        SetWindowPos(hwnd_, nullptr,
                     mi.rcWork.left, mi.rcWork.top,
                     mi.rcWork.right - mi.rcWork.left,
                     mi.rcWork.bottom - mi.rcWork.top,
                     SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        SetWindowLong(hwnd_, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                     SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        ShowWindow(hwnd_, SW_SHOW);
    }
    return true;
}

// ---------------------------------------------------------------------------
//  Message pump (static, operates on the single window instance)
// ---------------------------------------------------------------------------
LRESULT CALLBACK Window::wndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        // Stash the instance pointer (passed as CreateWindowEx lpParam).
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        Window* w = static_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(w));
        s_instance = w;
    }
    return wndProc(hwnd, msg, wParam, lParam);
}

bool Window::pumpMessage() {
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

void Window::waitMessage() {
    MSG msg;
    do {
        GetMessageA(&msg, nullptr, 0, 0);
    } while (msg.message != WM_QUIT);
}

void Window::cursorPos(int& x, int& y) {
    POINT p;
    if (GetCursorPos(&p)) {
        HWND h = s_instance ? s_instance->hwnd_ : nullptr;
        if (h) ScreenToClient(h, &p);
        x = p.x;
        y = p.y;
    } else {
        x = y = 0;
    }
}

void Window::showCursor(bool show) {
    ShowCursor(show ? 1 : -1);
}

} // namespace us3d
