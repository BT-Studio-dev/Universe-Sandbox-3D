// ============================================================================
//  Universe Sandbox 3D (C#) - Win32 window + WGL 3.3 context
//
//  Port of the battle-tested C++ v1 bootstrap (MSDN creation flow):
//   1. temporary legacy context (some drivers only expose
//      wglCreateContextAttribsARB through wglGetProcAddress once some
//      context is current),
//   2. try four ways to create a 3.3 context - wglCreateContextAttribsARB
//      and the Windows 8+ core name, each with core and compatibility
//      profile (the app uses nothing core-only),
//   3. full diagnostic report (GlReport) shown in the "OpenGL 3.3 not
//      available" dialog so driver failures are self-explaining.
//
//  Works on Windows 7 SP1 / 8 / 10 / 11 (x64) with real GPU drivers and
//  with the Mesa-for-Windows (llvmpipe) drop-in.
// ============================================================================
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace UniverseSandbox3D.Win32
{
    public class Window
    {
        // ------------------------------------------------------------------
        //  Public state
        // ------------------------------------------------------------------
        public IntPtr HWnd;
        public IntPtr Hdc;
        public string Error = "";
        public string GlReport = "";
        public int ClientWidth { get; private set; }
        public int ClientHeight { get; private set; }

        // Callbacks driven by the message loop.
        public Action<int, int> OnResize;      // width, height
        public Action OnPaintRequested;
        public Action<uint, bool> OnKeyDown;   // vk, down
        public Action<char> OnChar;
        public Action<int> OnMouseDown;        // 0 left, 1 middle, 2 right
        public Action<int> OnMouseUp;
        public Action<int, int, int, int> OnMouseMove; // x, y, dx, dy
        public Action<int> OnWheel;            // signed delta
        public Action OnFocusLost;

        // ------------------------------------------------------------------
        //  State
        // ------------------------------------------------------------------
        IntPtr glctx_;
        IntPtr hInstance_;
        NativeMethods.WndProcDelegate wndProcRef_; // keep alive (GC!)
        int lastMouseX_, lastMouseY_;
        static Window s_instance;

        // ------------------------------------------------------------------
        //  Creation
        // ------------------------------------------------------------------
        public bool Create(string title, int width, int height)
        {
            hInstance_ = Marshal.GetHINSTANCE(typeof(NativeMethods).Module);

            const string clsName = "UniverseSandbox3D";
            NativeMethods.WNDCLASSEX wc = new NativeMethods.WNDCLASSEX();
            wc.cbSize = Marshal.SizeOf(typeof(NativeMethods.WNDCLASSEX));
            wc.style = 0x0001 | 0x0002 | 0x0100; // CS_VREDRAW | CS_HREDRAW | CS_OWNDC
            wndProcRef_ = new NativeMethods.WndProcDelegate(WndProc);
            wc.lpfnWndProc = Marshal.GetFunctionPointerForDelegate(wndProcRef_);
            wc.hInstance = hInstance_;
            wc.hCursor = NativeMethods.LoadCursor(IntPtr.Zero, (IntPtr)NativeMethods.IDC_ARROW);
            wc.hbrBackground = (IntPtr)NativeMethods.GetStockObject(NativeMethods.BLACK_BRUSH);
            wc.lpszClassName = clsName;

            if (NativeMethods.RegisterClassEx(ref wc) == 0)
            {
                SetError("RegisterClassEx failed (error " + NativeMethods.GetLastError().ToString() + ")");
                return false;
            }

            NativeMethods.RECT rc = new NativeMethods.RECT();
            rc.Left = 0; rc.Top = 0; rc.Right = width; rc.Bottom = height;
            uint style = NativeMethods.WS_OVERLAPPEDWINDOW
                       | NativeMethods.WS_CLIPSIBLINGS
                       | NativeMethods.WS_CLIPCHILDREN;
            NativeMethods.AdjustWindowRectEx(ref rc, style, false, 0);

            int w = rc.Right - rc.Left;
            int h = rc.Bottom - rc.Top;

            HWnd = NativeMethods.CreateWindowEx(0, clsName, title, style,
                unchecked((int)NativeMethods.CW_USEDEFAULT), unchecked((int)NativeMethods.CW_USEDEFAULT),
                w, h, IntPtr.Zero, IntPtr.Zero, hInstance_, IntPtr.Zero);
            if (HWnd == IntPtr.Zero)
            {
                SetError("CreateWindowEx failed (error " + NativeMethods.GetLastError().ToString() + ")");
                return false;
            }
            s_instance = this;
            ClientWidth = w;
            ClientHeight = h;

            Hdc = NativeMethods.GetDC(HWnd);
            if (Hdc == IntPtr.Zero)
            {
                SetError("GetDC failed");
                return false;
            }

            // ---- pixel format ----
            NativeMethods.PIXELFORMATDESCRIPTOR pfd = new NativeMethods.PIXELFORMATDESCRIPTOR();
            pfd.nSize = (byte)Marshal.SizeOf(typeof(NativeMethods.PIXELFORMATDESCRIPTOR));
            pfd.nVersion = 1;
            pfd.dwFlags = NativeMethods.PFD_DRAW_TO_WINDOW
                        | NativeMethods.PFD_SUPPORT_OPENGL
                        | NativeMethods.PFD_DOUBLEBUFFER;
            pfd.iPixelType = NativeMethods.PFD_TYPE_RGBA;
            pfd.cColorBits = 24;
            pfd.cDepthBits = 24;
            pfd.iLayerType = NativeMethods.PFD_MAIN_PLANE;

            int pf = NativeMethods.ChoosePixelFormat(Hdc, ref pfd);
            if (pf == 0)
            {
                SetError("ChoosePixelFormat failed (error " + NativeMethods.GetLastError().ToString() + ")");
                return false;
            }
            if (!NativeMethods.SetPixelFormat(Hdc, pf, ref pfd))
            {
                SetError("SetPixelFormat failed (error " + NativeMethods.GetLastError().ToString() + ")");
                return false;
            }

            // ---- GL 3.3 context (MSDN bootstrap) ----
            if (!GL33.Load())
            {
                SetError("Failed to resolve basic OpenGL entry points: " + GL33.LastError);
                return false;
            }

            StringBuilder report = new StringBuilder();
            report.AppendLine("Context bootstrap:");

            // Step 1: temporary legacy context. Several drivers (notably
            // Intel, and some AMD/NVIDIA on Windows 7) only expose
            // wglCreateContextAttribsARB through wglGetProcAddress once
            // *some* context is current.
            IntPtr legacyCtx = NativeMethods.wglCreateContext(Hdc);
            if (legacyCtx == IntPtr.Zero)
            {
                SetError("wglCreateContext failed (error " + NativeMethods.GetLastError().ToString() + ")");
                return false;
            }
            if (!NativeMethods.wglMakeCurrent(Hdc, legacyCtx))
            {
                SetError("wglMakeCurrent failed (error " + NativeMethods.GetLastError().ToString() + ")");
                NativeMethods.wglDeleteContext(legacyCtx);
                return false;
            }
            string legacyVersion = GL33.VersionString();
            report.AppendLine("  1. legacy context: created, reports '" + legacyVersion + "'");

            // Step 2: a modern 3.3 context. Try several entry-point names
            // and profiles: real GPU drivers almost always expose
            // wglCreateContextAttribsARB with a core profile; the
            // Mesa-for-Windows (llvmpipe) drop-in and some older drivers
            // expose only the Windows 8+ core name, or only a
            // compatibility profile.
            NativeMethods.WglCreateContextAttribsARBDelegate ccArb =
                ResolveContextAttribs("wglCreateContextAttribsARB");
            NativeMethods.WglCreateContextAttribsARBDelegate ccCore =
                ResolveContextAttribs("wglCreateContextAttribs");

            glctx_ = legacyCtx;
            struct Attempt
            {
                public NativeMethods.WglCreateContextAttribsARBDelegate Fn;
                public string Label;
                public int Profile;
            }
            Attempt[] attempts = new Attempt[]
            {
                new Attempt { Fn = ccArb, Label = "wglCreateContextAttribsARB / 3.3 core",   Profile = NativeMethods.WGL_CONTEXT_CORE_PROFILE_BIT_ARB },
                new Attempt { Fn = ccArb, Label = "wglCreateContextAttribsARB / 3.3 compat", Profile = NativeMethods.WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB },
                new Attempt { Fn = ccCore, Label = "wglCreateContextAttribs / 3.3 core",     Profile = NativeMethods.WGL_CONTEXT_CORE_PROFILE_BIT_ARB },
                new Attempt { Fn = ccCore, Label = "wglCreateContextAttribs / 3.3 compat",   Profile = NativeMethods.WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB },
            };

            bool modernOk = false;
            for (int i = 0; i < attempts.Length && !modernOk; i++)
            {
                string step = "  2." + (i + 1) + " " + attempts[i].Label + ": ";
                if (attempts[i].Fn == null)
                {
                    report.AppendLine(step + "entry point not resolved");
                    continue;
                }
                int[] attribs = new int[]
                {
                    NativeMethods.WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                    NativeMethods.WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                    NativeMethods.WGL_CONTEXT_PROFILE_MASK_ARB,  attempts[i].Profile,
                    0,
                };
                IntPtr newCtx = attempts[i].Fn(Hdc, IntPtr.Zero, attribs);
                if (newCtx != IntPtr.Zero)
                {
                    if (NativeMethods.wglMakeCurrent(Hdc, newCtx))
                    {
                        NativeMethods.wglDeleteContext(legacyCtx);
                        glctx_ = newCtx;
                        modernOk = true;
                        report.AppendLine(step + "OK");
                    }
                    else
                    {
                        report.AppendLine(step + "created, wglMakeCurrent failed (error "
                            + NativeMethods.GetLastError().ToString() + ")");
                        NativeMethods.wglDeleteContext(newCtx);
                    }
                }
                else
                {
                    report.AppendLine(step + "failed (wgl error "
                        + NativeMethods.GetLastError().ToString() + ")");
                }
            }
            if (!modernOk)
            {
                report.AppendLine("  -> keeping legacy context (reported version: " + legacyVersion + ")");
            }
            report.AppendLine("GL_VERSION : " + GL33.VersionString());
            report.AppendLine("GL_VENDOR  : " + GL33.VendorString());
            report.AppendLine("GL_RENDERER: " + GL33.RendererString());
            GlReport = report.ToString();

            // Re-resolve now that a modern context is current (drivers may
            // expose extension entry points only then).
            GL33.Load();

            NativeMethods.ShowWindow(HWnd, NativeMethods.SW_SHOW);
            NativeMethods.UpdateWindow(HWnd);
            return true;
        }

        /// <summary>Resolve wglCreateContextAttribs[ARB] (opengl32 export first, then wglGetProcAddress).</summary>
        static NativeMethods.WglCreateContextAttribsARBDelegate ResolveContextAttribs(string name)
        {
            IntPtr p = IntPtr.Zero;
            IntPtr o32 = NativeMethods.GetModuleHandle("opengl32.dll");
            if (o32 != IntPtr.Zero)
                p = NativeMethods.GetProcAddress(o32, name);
            if (p == IntPtr.Zero)
                p = GL33.WglGetProc(name);
            if (p == IntPtr.Zero) return null;
            try
            {
                return (NativeMethods.WglCreateContextAttribsARBDelegate)
                    Marshal.GetDelegateForFunctionPointer(p, typeof(NativeMethods.WglCreateContextAttribsARBDelegate));
            }
            catch
            {
                return null;
            }
        }

        public void SetTitle(string title)
        {
            if (HWnd != IntPtr.Zero) NativeMethods.SetWindowText(HWnd, title);
        }

        public void Destroy()
        {
            if (glctx_ != IntPtr.Zero)
            {
                NativeMethods.wglMakeCurrent(IntPtr.Zero, IntPtr.Zero);
                NativeMethods.wglDeleteContext(glctx_);
                glctx_ = IntPtr.Zero;
            }
            if (Hdc != IntPtr.Zero)
            {
                NativeMethods.ReleaseDC(HWnd, Hdc);
                Hdc = IntPtr.Zero;
            }
            if (HWnd != IntPtr.Zero && NativeMethods.IsWindow(HWnd))
            {
                NativeMethods.DestroyWindow(HWnd);
            }
            HWnd = IntPtr.Zero;
            s_instance = null;
        }

        public void SwapBuffers()
        {
            if (Hdc != IntPtr.Zero) NativeMethods.SwapBuffers(Hdc);
        }

        public void SetVSync(bool on)
        {
            // WGL_EXT_swap_control - resolved lazily, optional.
            // (Wired up in the renderer phase.)
        }

        // ------------------------------------------------------------------
        //  Message loop
        // ------------------------------------------------------------------

        /// <summary>
        /// Process all pending messages; then wait up to one frame (~16 ms)
        /// for the next input so the app renders at ~60 FPS while idle.
        /// Returns false when WM_QUIT has been processed.
        /// </summary>
        public static bool PumpMessage()
        {
            NativeMethods.MSG m;
            while (NativeMethods.PeekMessage(out m, IntPtr.Zero, 0, 0))
            {
                if (m.message == NativeMethods.WM_QUIT) return false;
                NativeMethods.TranslateMessage(ref m);
                NativeMethods.DispatchMessage(ref m);
            }
            NativeMethods.MsgWaitForMultipleObjects(0, null, false, 16, NativeMethods.QS_ALLINPUT);
            return true;
        }

        // ------------------------------------------------------------------
        //  Window procedure
        // ------------------------------------------------------------------
        static IntPtr WndProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam)
        {
            Window w = s_instance;
            switch (msg)
            {
                case NativeMethods.WM_CLOSE:
                    NativeMethods.DestroyWindow(hWnd);
                    return IntPtr.Zero;

                case NativeMethods.WM_DESTROY:
                    NativeMethods.PostQuitMessage(0);
                    return IntPtr.Zero;

                case NativeMethods.WM_SIZE:
                    if (w != null && hWnd == w.HWnd)
                    {
                        long lp = (long)lParam;
                        int cw = (int)(lp & 0xFFFF);
                        int ch = (int)((lp >> 16) & 0xFFFF);
                        w.ClientSize = cw;
                        w.ClientHeight = ch;
                        if (GL33.glViewport != null)
                            GL33.glViewport(0, 0, cw, ch);
                        if (w.OnResize != null) w.OnResize(cw, ch);
                    }
                    return IntPtr.Zero;

                case NativeMethods.WM_PAINT:
                    {
                        NativeMethods.PAINTSTRUCT ps;
                        NativeMethods.BeginPaint(hWnd, out ps);
                        NativeMethods.EndPaint(hWnd, ref ps);
                        if (w != null && w.OnPaintRequested != null) w.OnPaintRequested();
                        return IntPtr.Zero;
                    }

                case NativeMethods.WM_KEYDOWN:
                    if (w != null && w.OnKeyDown != null) w.OnKeyDown((uint)(long)wParam, true);
                    break;

                case NativeMethods.WM_CHAR:
                    if (w != null && w.OnChar != null && (long)wParam >= 32 && (long)wParam <= 126)
                        w.OnChar((char)(long)wParam);
                    break;

                case NativeMethods.WM_LBUTTONDOWN:
                case NativeMethods.WM_MBUTTONDOWN:
                case NativeMethods.WM_RBUTTONDOWN:
                    if (w != null && w.OnMouseDown != null)
                    {
                        w.UpdateMousePos(lParam);
                        w.OnMouseDown((msg == NativeMethods.WM_LBUTTONDOWN) ? 0 : (msg == NativeMethods.WM_MBUTTONDOWN) ? 1 : 2);
                    }
                    break;

                case NativeMethods.WM_LBUTTONUP:
                case NativeMethods.WM_MBUTTONUP:
                case NativeMethods.WM_RBUTTONUP:
                    if (w != null && w.OnMouseUp != null)
                        w.OnMouseUp((msg == NativeMethods.WM_LBUTTONUP) ? 0 : (msg == NativeMethods.WM_MBUTTONUP) ? 1 : 2);
                    break;

                case NativeMethods.WM_MOUSEMOVE:
                    if (w != null && w.OnMouseMove != null)
                    {
                        int x = (int)((long)lParam & 0xFFFF);
                        int y = (int)(((long)lParam >> 16) & 0xFFFF);
                        w.OnMouseMove(x, y, x - w.lastMouseX_, y - w.lastMouseY_);
                        w.lastMouseX_ = x;
                        w.lastMouseY_ = y;
                    }
                    break;

                case NativeMethods.WM_MOUSEWHEEL:
                    if (w != null && w.OnWheel != null)
                        w.OnWheel((int)((long)wParam >> 16));
                    break;

                case NativeMethods.WM_KILLFOCUS:
                    if (w != null && w.OnFocusLost != null) w.OnFocusLost();
                    break;
            }
            return NativeMethods.DefWindowProc(hWnd, msg, wParam, lParam);
        }

        void UpdateMousePos(IntPtr lParam)
        {
            int x = (int)((long)lParam & 0xFFFF);
            int y = (int)(((long)lParam >> 16) & 0xFFFF);
            lastMouseX_ = x;
            lastMouseY_ = y;
        }

        void SetError(string message)
        {
            Error = message;
        }
    }
}
