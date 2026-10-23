// ============================================================================
//  Universe Sandbox 3D - STUB windows.h for compile-checking on non-Windows
//
//  This is NOT a functional header. It mirrors just enough of the Win32 /
//  GDI surface used by the project so the Windows code paths can be
//  compile-checked (syntax, types, declarations) on Linux CI or a dev box
//  without a Windows SDK. Nothing here is linked or executed off-Windows.
//
//  Usage (see tools/compile-check/check.sh):
//      g++ -fsyntax-only -std=c++17 -D_WIN32 -I stub_include -I src src/...
// ============================================================================
#pragma once

// ---- fundamental types -----------------------------------------------------
typedef unsigned long   DWORD;
typedef unsigned int    UINT;
typedef int             BOOL;
typedef long            LONG;
typedef long long       LONGLONG;
typedef LONG            LRESULT;
typedef unsigned long   WPARAM;
typedef long            LPARAM;
typedef int             SHORT;
typedef unsigned short  ATOM;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;

typedef void*           HWND;
typedef void*           HDC;
typedef void*           HINSTANCE;
typedef void*           HMODULE;
typedef void*           HMONITOR;
typedef void*           HBRUSH;
typedef void*           HCURSOR;
typedef void*           HICON;
typedef void*           HGDIOBJ;
typedef void*           HMENU;
typedef void*           FARPROC;
typedef void*           LPVOID;
typedef long long       LONG_PTR;
typedef unsigned long long ULONG_PTR;

typedef wchar_t*        LPWSTR;
typedef const wchar_t*  LPCWSTR;
typedef char*           LPSTR;
typedef const char*     LPCSTR;

#ifndef NULL
#  define NULL 0
#endif
#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#define WINAPI
#define CALLBACK
#define APIENTRY
#define MAX_PATH 260

#define LOWORD(l)  ((WORD)(((DWORD)(l)) & 0xFFFF))
#define HIWORD(l)  ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define GET_X_LPARAM(lp)       ((int)(short)(((LPARAM)(lp)) & 0xFFFF))
#define GET_Y_LPARAM(lp)       ((int)(short)(((LPARAM)(lp) >> 16) & 0xFFFF))
#define GET_WHEEL_DELTA_WPARAM(wp) ((short)HIWORD(wp))

// ---- structs ---------------------------------------------------------------
struct RECT { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };
struct MSG {
    HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam;
    DWORD time; POINT pt;
};
struct MINMAXINFO {
    POINT ptReserved1, ptMaxSize, ptMinTrackSize, ptMaxTrackSize;
};
struct MONITORINFO { DWORD cbSize; RECT rcMonitor; RECT rcWork; DWORD dwFlags; };
struct CREATESTRUCT {
    LPVOID lpCreateParams; HINSTANCE hInstance; HMENU hMenu; HWND hwndParent;
    HICON hIcon; LPCSTR lpszClassName; LPCSTR lpszWindowName;
    DWORD dwStyle; RECT rc; DWORD dwExStyle;
};
typedef CREATESTRUCT CREATESTRUCTW;
typedef LRESULT (WINAPI *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
struct WNDCLASSEX {
    UINT cbSize; DWORD style; WNDPROC lpfnWndProc;
    int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon;
    HCURSOR hCursor; HBRUSH hbrBackground; LPCSTR lpszMenuName;
    LPCSTR lpszClassName; HICON hIconSm;
};
typedef WNDCLASSEX WNDCLASSEXW;
typedef struct {
    UINT nSize; UINT nVersion; DWORD dwFlags; BYTE iPixelType;
    BYTE cColorBits; BYTE cRedBits; BYTE cRedShift;
    BYTE cGreenBits; BYTE cGreenShift; BYTE cBlueBits; BYTE cBlueShift;
    BYTE cAlphaBits; BYTE cAlphaShift;
    DWORD dwAuxBuffers; BYTE iLayerType; BYTE bReserved;
    DWORD dwDescription; DWORD cDepthBits; DWORD cStencilBits;
    DWORD dwMilliseconds; DWORD dwFlags2;
} PIXELFORMATDESCRIPTOR;

// ---- window class / style / message constants ------------------------------
#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_OWNDC            0x0020
#define WS_OVERLAPPED       0
#define WS_CAPTION          0x00C00000
#define WS_SYSMENU          0x00080000
#define WS_THICKFRAME       0x00040000
#define WS_MINIMIZEBOX      0x00020000
#define WS_MAXIMIZEBOX      0x00010000
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | \
                             WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUP            0x80000000
#define WS_CLIPSIBLINGS     0x04000000
#define WS_CLIPCHILDREN     0x02000000
#define CW_USEDEFAULT       0x80000000

#define GWL_STYLE           (-16)
#define GWLP_USERDATA       (-21)

#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
#define SWP_FRAMECHANGED    0x0020
#define SWP_SHOWWINDOW      0x0040
#define SWP_NOACTIVATE      0x0010

#define WM_NCCREATE         0x0081
#define WM_SIZE             0x0005
#define WM_CLOSE            0x0010
#define WM_DESTROY          0x0002
#define WM_QUIT             0x0012
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MOUSEWHEEL       0x020A
#define WM_KILLFOCUS        0x000A
#define WM_GETMINMAXINFO    0x0024
#define WM_DPICHANGED       0x02E0

#define PM_NOREMOVE         0
#define PM_REMOVE           1

// ---- virtual keys (character keys use their ASCII code) ----------------------
#define VK_SPACE    0x20
#define VK_DELETE   0x2E
#define VK_ESCAPE   0x1B
#define VK_F11      0x7A
#define VK_ADD      0x6B
#define VK_SUBTRACT 0x6D
#define VK_OEM_PLUS  0x2B
#define VK_OEM_MINUS 0x2D
#define VK_CONTROL  0x11

// ---- misc constants -----------------------------------------------------------
#define MB_OK            0x00000000
#define MB_ICONERROR     0x00000010
#define MONITOR_DEFAULTTONEAREST 2
#define IDC_ARROW        ((LPCSTR)(32512))
#define BLACK_BRUSH      4

#define PFD_DRAW_TO_WINDOW 0x00000001
#define PFD_SUPPORT_OPENGL 0x00000020
#define PFD_DOUBLEBUFFER   0x00000004
#define PFD_TYPE_RGBA      0
#define PFD_MAIN_PLANE     0

#define SW_SHOWNORMAL 1
#define SW_SHOW 5

// ---- function declarations ------------------------------------------------------
ATOM        RegisterClassEx(const WNDCLASSEX*);
ATOM        RegisterClassExW(const WNDCLASSEXW*);
HWND        CreateWindowEx(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int,
                            HWND, HMENU, HINSTANCE, LPVOID);
HWND        CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int,
                             HWND, HMENU, HINSTANCE, LPVOID);
BOOL        DestroyWindow(HWND);
BOOL        ShowWindow(HWND, int);
BOOL        UpdateWindow(HWND);
BOOL        PeekMessageA(MSG*, HWND, UINT, UINT, UINT);
BOOL        PeekMessageW(MSG*, HWND, UINT, UINT, UINT);
int         GetMessageA(MSG*, HWND, UINT, UINT);
int         GetMessageW(MSG*, HWND, UINT, UINT);
BOOL        TranslateMessage(const MSG*);
LRESULT     DispatchMessageA(MSG*);
LRESULT     DispatchMessageW(const MSG*);
void        PostQuitMessage(int);
LRESULT     DefWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT     DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
HCURSOR     LoadCursor(HINSTANCE, LPCSTR);
HDC         GetDC(HWND);
int         ReleaseDC(HWND, HDC);
BOOL        SetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR*);
int         ChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR*);
BOOL        SwapBuffers(HDC);
BOOL        SetWindowPos(HWND, HWND, int, int, int, int, UINT);
BOOL        AdjustWindowRectEx(RECT*, DWORD, int, DWORD);
LONG        SetWindowLong(HWND, int, LONG);
LONG_PTR    SetWindowLongPtrW(HWND, int, LONG_PTR);
LONG_PTR    GetWindowLongPtrW(HWND, int);
HMONITOR    MonitorFromWindow(HWND, UINT);
HMONITOR    MonitorFromPoint(POINT, UINT);
BOOL        GetMonitorInfoW(HMONITOR, MONITORINFO*);
BOOL        GetCursorPos(POINT*);
void        ScreenToClient(HWND, POINT*);
BOOL        ShowCursor(int);
int         MessageBoxA(HWND, LPCSTR, LPCSTR, UINT);
DWORD       GetLastError();
DWORD       GetTickCount();
SHORT       GetAsyncKeyState(int);
HMODULE     LoadLibraryA(LPCSTR);
HMODULE     GetModuleHandleA(LPCSTR);
HMODULE     GetModuleHandleW(LPCWSTR);
FARPROC     GetProcAddress(HMODULE, LPCSTR);
BOOL        FreeLibrary(HMODULE);
DWORD       GetModuleFileNameA(HMODULE, LPSTR, DWORD);
HCURSOR     LoadCursorW(HINSTANCE, LPCWSTR);
HGDIOBJ     GetStockObject(int);
