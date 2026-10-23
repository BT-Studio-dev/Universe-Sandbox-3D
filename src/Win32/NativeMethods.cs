// ============================================================================
//  Universe Sandbox 3D (C#) - Win32 API declarations (P/Invoke)
//
//  Raw Win32 + WGL, no WinForms / no frameworks. Target: Windows 7 SP1 /
//  10 / 11 (x64), .NET Framework 4.5 (ships with Windows 7 SP1).
//  Written in C# 5 syntax so the stock csc.exe compiles it.
// ============================================================================
using System;
using System.Runtime.InteropServices;

namespace UniverseSandbox3D.Win32
{
    public static class NativeMethods
    {
        // ------------------------------------------------------------------
        //  Constants
        // ------------------------------------------------------------------
        public const int WM_DESTROY   = 0x0002;
        public const int WM_SIZE      = 0x0005;
        public const int WM_CLOSE     = 0x0010;
        public const int WM_PAINT     = 0x000F;
        public const uint WM_QUIT     = 0x0120;
        public const int WM_KEYDOWN   = 0x0100;
        public const int WM_CHAR      = 0x0102;
        public const int WM_LBUTTONDOWN = 0x0201;
        public const int WM_LBUTTONUP   = 0x0202;
        public const int WM_RBUTTONDOWN = 0x0204;
        public const int WM_RBUTTONUP   = 0x0205;
        public const int WM_MBUTTONDOWN = 0x0207;
        public const int WM_MBUTTONUP   = 0x0208;
        public const int WM_MOUSEMOVE   = 0x0200;
        public const int WM_MOUSEWHEEL  = 0x020A;
        public const int WM_KILLFOCUS   = 0x000A;

        public const uint WS_OVERLAPPEDWINDOW = 0x00CF0000;
        public const uint WS_CLIPSIBLINGS     = 0x04000000;
        public const uint WS_CLIPCHILDREN     = 0x02000000;
        public const uint WS_POPUP             = 0x80000000;
        public const uint CW_USEDEFAULT        = unchecked((uint)0x80000000);

        public const int SW_SHOW = 5;
        public const uint QS_ALLINPUT = 0x04FF;

        public const int IDI_APPLICATION = 32512;
        public const int IDC_ARROW       = 32512;
        public const int BLACK_BRUSH     = 4;

        public const uint MB_OK = 0;
        public const uint MB_ICONERROR = 0x10;

        // pixel format
        public const uint PFD_DRAW_TO_WINDOW = 0x00000001;
        public const uint PFD_SUPPORT_OPENGL = 0x00000020;
        public const uint PFD_DOUBLEBUFFER   = 0x00000100;
        public const int  PFD_TYPE_RGBA      = 0;
        public const int  PFD_MAIN_PLANE     = 0;

        // WGL (core-profile context creation)
        public const int WGL_CONTEXT_MAJOR_VERSION_ARB         = 0x2091;
        public const int WGL_CONTEXT_MINOR_VERSION_ARB         = 0x2092;
        public const int WGL_CONTEXT_PROFILE_MASK_ARB          = 0x9125;
        public const int WGL_CONTEXT_CORE_PROFILE_BIT_ARB      = 0x0001;
        public const int WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x0002;

        // ------------------------------------------------------------------
        //  Structures
        // ------------------------------------------------------------------
        [StructLayout(LayoutKind.Sequential)]
        public struct RECT
        {
            public int Left, Top, Right, Bottom;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct POINT
        {
            public int x, y;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MSG
        {
            public IntPtr hwnd;
            public uint message;
            public IntPtr wParam;
            public IntPtr lParam;
            public uint time;
            public int ptX;
            public int ptY;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PAINTSTRUCT
        {
            public IntPtr hdc;
            [MarshalAs(UnmanagedType.Bool)] public bool fErase;
            public RECT rcPaint;
            [MarshalAs(UnmanagedType.Bool)] public bool fRestore;
            [MarshalAs(UnmanagedType.Bool)] public bool fIncUpdate;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
            public byte[] rgbReserved;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MONITORINFO
        {
            public int cbSize;
            public RECT rcMonitor;
            public RECT rcWork;
            public uint dwFlags;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PIXELFORMATDESCRIPTOR
        {
            public byte nSize;
            public byte nVersion;
            public uint dwFlags;
            public byte iPixelType;
            public byte cColorBits;
            public byte cRedBits;
            public byte cRedShift;
            public byte cGreenBits;
            public byte cGreenShift;
            public byte cBlueBits;
            public byte cBlueShift;
            public byte cAlphaBits;
            public byte cAlphaShift;
            public uint dwAuxBuffers;
            public byte iLayerType;
            public byte bColorIndexSupport;
            public byte cPlaneBits;
            public byte Reserved;
            public uint dwGenCaps;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct WNDCLASSEX
        {
            public int cbSize;
            public uint style;
            public IntPtr lpfnWndProc;      // marshaled WndProcDelegate
            public int cbClsExtra;
            public int cbWndExtra;
            public IntPtr hInstance;
            public IntPtr hIcon;
            public IntPtr hCursor;
            public IntPtr hbrBackground;
            public string lpszMenuName;
            public string lpszClassName;
            public IntPtr hIconSm;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct FILETIME
        {
            public ushort wLowDateTime;
            public ushort wHighDateTime;
        }

        // ------------------------------------------------------------------
        //  Delegates
        // ------------------------------------------------------------------
        public delegate IntPtr WndProcDelegate(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        // WGL: wglCreateContextAttribsARB is NOT an export of Win7's
        // opengl32.dll; it must be resolved at runtime (see Window.cs).
        public delegate IntPtr WglCreateContextAttribsARBDelegate(IntPtr hDC, IntPtr hShareContext, int[] attribList);
        public delegate IntPtr WglGetProcAddressDelegate(string name);

        // ------------------------------------------------------------------
        //  user32.dll
        // ------------------------------------------------------------------
        [DllImport("user32.dll")]
        public static extern IntPtr DefWindowProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Ansi)]
        public static extern ushort RegisterClassEx([In] ref WNDCLASSEX lpwcx);

        [DllImport("user32.dll", CharSet = CharSet.Ansi)]
        public static extern IntPtr CreateWindowEx(uint dwExStyle, string lpClassName, string lpWindowName,
            uint dwStyle, int x, int y, int nWidth, int nHeight, IntPtr hWndParent,
            IntPtr hMenu, IntPtr hInstance, IntPtr lpParam);

        [DllImport("user32.dll")]
        public static extern bool DestroyWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [DllImport("user32.dll")]
        public static extern bool UpdateWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool PeekMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax);

        [DllImport("user32.dll")]
        public static extern int GetMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax);

        [DllImport("user32.dll")]
        public static extern bool TranslateMessage(ref MSG lpMsg);

        [DllImport("user32.dll", CharSet = CharSet.Ansi)]
        public static extern IntPtr DispatchMessage(ref MSG lpMsg);

        [DllImport("user32.dll")]
        public static extern bool PostQuitMessage(int nExitCode);

        [DllImport("user32.dll")]
        public static extern bool IsWindow(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Ansi)]
        public static extern bool SetWindowText(IntPtr hWnd, string lpString);

        [DllImport("user32.dll", CharSet = CharSet.Ansi)]
        public static extern int MessageBox(IntPtr hWnd, string lpText, string lpCaption, uint uType);

        [DllImport("user32.dll")]
        public static extern IntPtr GetWindowLong(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll")]
        public static extern IntPtr SetWindowLong(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

        [DllImport("user32.dll")]
        public static extern bool AdjustWindowRectEx(ref RECT lpRect, uint dwStyle, [MarshalAs(UnmanagedType.Bool)] bool bMenu, uint dwExStyle);

        [DllImport("user32.dll")]
        public static extern IntPtr MonitorFromPoint(POINT pt, uint dwFlags);

        [DllImport("user32.dll")]
        public static extern bool GetMonitorInfo(IntPtr hMonitor, ref MONITORINFO lpmi);

        [DllImport("user32.dll")]
        public static extern bool GetCursorPos(out POINT lpPoint);

        [DllImport("user32.dll")]
        public static extern bool ScreenToClient(IntPtr hWnd, ref POINT lpPoint);

        [DllImport("user32.dll")]
        public static extern IntPtr GetDC(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);

        [DllImport("user32.dll")]
        public static extern IntPtr LoadCursor(IntPtr hInstance, IntPtr lpCursorName);

        [DllImport("user32.dll")]
        public static extern IntPtr BeginPaint(IntPtr hWnd, out PAINTSTRUCT lpps);

        [DllImport("user32.dll")]
        public static extern bool EndPaint(IntPtr hWnd, ref PAINTSTRUCT lpPaint);

        [DllImport("user32.dll")]
        public static extern bool InvalidateRect(IntPtr hWnd, ref RECT lpRect, [MarshalAs(UnmanagedType.Bool)] bool bErase);

        [DllImport("user32.dll")]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
            int X, int Y, int cx, int cy, uint uFlags);

        [DllImport("user32.dll")]
        public static extern uint MsgWaitForMultipleObjects(int nCount, IntPtr[] pHandles,
            [MarshalAs(UnmanagedType.Bool)] bool bWaitAll, int dwMilliseconds, uint dwMask);

        public const uint MONITOR_DEFAULTTONEAREST = 2;
        public const uint SWP_NOSIZE = 0x0001;
        public const uint SWP_NOMOVE = 0x0002;
        public const uint SWP_NOZORDER = 0x0004;
        public const uint SWP_FRAMECHANGED = 0x0020;
        public const uint SWP_SHOWWINDOW = 0x0040;

        // ------------------------------------------------------------------
        //  gdi32.dll
        // ------------------------------------------------------------------
        [DllImport("gdi32.dll")]
        public static extern int ChoosePixelFormat(IntPtr hDC, ref PIXELFORMATDESCRIPTOR ppfd);

        [DllImport("gdi32.dll")]
        public static extern bool SetPixelFormat(IntPtr hDC, int iPixelFormat, ref PIXELFORMATDESCRIPTOR ppfd);

        [DllImport("gdi32.dll")]
        public static extern bool SwapBuffers(IntPtr hDC);

        [DllImport("gdi32.dll")]
        public static extern IntPtr GetStockObject(int oi);

        // ------------------------------------------------------------------
        //  kernel32.dll
        // ------------------------------------------------------------------
        [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
        public static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
        public static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32.dll")]
        public static extern uint GetLastError();

        [DllImport("kernel32.dll")]
        public static extern void GetSystemTimeAsFileTime(out FILETIME lpSystemTime);

        // ------------------------------------------------------------------
        //  opengl32.dll (the core exports - present on every Windows)
        // ------------------------------------------------------------------
        [DllImport("opengl32.dll")]
        public static extern IntPtr wglCreateContext(IntPtr hDC);

        [DllImport("opengl32.dll")]
        public static extern bool wglMakeCurrent(IntPtr hDC, IntPtr hglrc);

        [DllImport("opengl32.dll")]
        public static extern bool wglDeleteContext(IntPtr hglrc);

        [DllImport("opengl32.dll")]
        public static extern IntPtr wglGetCurrentContext();
    }
}
