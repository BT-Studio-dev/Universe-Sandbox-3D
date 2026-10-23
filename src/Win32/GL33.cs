// ============================================================================
//  Universe Sandbox 3D (C#) - self-contained OpenGL 3.3 loader
//
//  Port of the C++ v1 gl33 loader: no GLEW/GLFW. Function pointers are
//  resolved from opengl32.dll exports first, then from wglGetProcAddress
//  (which is how modern GL entry points are exposed on Windows).
//
//  Phase 1 (this file) declares only the entry points needed to create a
//  context and clear the screen; the renderer phases add the rest through
//  the same LoadEntryPoint mechanism.
// ============================================================================
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UniverseSandbox3D.Win32;

namespace UniverseSandbox3D.Win32
{
    public static class GL33
    {
        // ------------------------------------------------------------------
        //  Constants (subset)
        // ------------------------------------------------------------------
        public const uint GL_COLOR_BUFFER_BIT = 0x4000;
        public const uint GL_DEPTH_BUFFER_BIT = 0x0100;
        public const uint GL_STENCIL_BUFFER_BIT = 0x0400;
        public const uint GL_VENDOR   = 0x1F00;
        public const uint GL_VERSION  = 0x1F01;
        public const uint GL_RENDERER = 0x1F02;
        public const uint GL_SHADING_LANGUAGE_VERSION = 0x8B8C;

        // ------------------------------------------------------------------
        //  Entry-point delegates
        // ------------------------------------------------------------------
        public delegate void GLClearDelegate(uint mask);
        public delegate void GLClearColorDelegate(float red, float green, float blue, float alpha);
        public delegate void GLViewportDelegate(int x, int y, int width, int height);
        public delegate IntPtr GLGetStringDelegate(uint name);

        // ------------------------------------------------------------------
        //  Resolved entry points (phase 1)
        // ------------------------------------------------------------------
        public static GLClearDelegate glClear;
        public static GLClearColorDelegate glClearColor;
        public static GLViewportDelegate glViewport;
        public static GLGetStringDelegate glGetString;

        // Generic resolution cache: name -> function pointer.
        static readonly Dictionary<string, IntPtr> s_ptrs = new Dictionary<string, IntPtr>();
        static IntPtr s_openglLib;
        static NativeMethods.WglGetProcAddressDelegate s_wglGetProcAddress;
        public static string LastError = "";

        // ------------------------------------------------------------------
        //  Loading
        // ------------------------------------------------------------------

        /// <summary>
        /// Resolve the entry points this phase needs. Call after wglMakeCurrent
        /// on a fresh context (some drivers require a current context to
        /// resolve extension functions).
        /// </summary>
        public static bool Load()
        {
            LastError = "";
            if (!EnsureOpengl32())
            {
                LastError = "opengl32.dll not found";
                return false;
            }
            if (!Require("glClear", (IntPtr p) => { glClear = MakeDelegate<GLClearDelegate>(p); return glClear != null; })) return false;
            if (!Require("glClearColor", (IntPtr p) => { glClearColor = MakeDelegate<GLClearColorDelegate>(p); return glClearColor != null; })) return false;
            if (!Require("glViewport", (IntPtr p) => { glViewport = MakeDelegate<GLViewportDelegate>(p); return glViewport != null; })) return false;
            if (!Require("glGetString", (IntPtr p) => { glGetString = MakeDelegate<GLGetStringDelegate>(p); return glGetString != null; })) return false;
            return true;
        }

        /// <summary>Raw pointer for a GL entry point (null if not resolvable).
        /// Only successful resolutions are cached: drivers may expose
        /// extension entry points only once a context is current, so a
        /// failed lookup must be retried on later Load() calls.</summary>
        public static IntPtr Resolve(string name)
        {
            IntPtr cached;
            if (s_ptrs.TryGetValue(name, out cached) && cached != IntPtr.Zero)
                return cached;

            IntPtr p = IntPtr.Zero;
            if (s_openglLib != IntPtr.Zero)
                p = NativeMethods.GetProcAddress(s_openglLib, name);
            if (p == IntPtr.Zero && s_wglGetProcAddress != null)
                p = s_wglGetProcAddress(name);

            if (p != IntPtr.Zero) s_ptrs[name] = p;
            return p;
        }

        static bool Require(string name, Func<IntPtr, bool> finish)
        {
            IntPtr p = Resolve(name);
            if (p == IntPtr.Zero)
            {
                LastError = "could not resolve " + name;
                return false;
            }
            return finish(p);
        }

        static T MakeDelegate<T>(IntPtr p) where T : class
        {
            try
            {
                return (T)Marshal.GetDelegateForFunctionPointer(p, typeof(T));
            }
            catch
            {
                return null;
            }
        }

        static bool EnsureOpengl32()
        {
            if (s_openglLib != IntPtr.Zero) return true;
            s_openglLib = NativeMethods.LoadLibrary("opengl32.dll");
            if (s_openglLib == IntPtr.Zero) return false;
            IntPtr p = NativeMethods.GetProcAddress(s_openglLib, "wglGetProcAddress");
            if (p != IntPtr.Zero)
                s_wglGetProcAddress = MakeDelegate<NativeMethods.WglGetProcAddressDelegate>(p);
            return true;
        }

        /// <summary>Module handle of the loaded opengl32.dll (zero if not loaded).</summary>
        public static IntPtr Opengl32()
        {
            EnsureOpengl32();
            return s_openglLib;
        }

        /// <summary>wglGetProcAddress(name) - for WGL extension entry points.</summary>
        public static IntPtr WglGetProc(string name)
        {
            if (!EnsureOpengl32() || s_wglGetProcAddress == null) return IntPtr.Zero;
            return s_wglGetProcAddress(name);
        }

        // ------------------------------------------------------------------
        //  Version queries
        // ------------------------------------------------------------------
        public static string VersionString()
        {
            return GetString(GL_VERSION);
        }

        public static string VendorString()
        {
            return GetString(GL_VENDOR);
        }

        public static string RendererString()
        {
            return GetString(GL_RENDERER);
        }

        static string GetString(uint name)
        {
            if (glGetString == null) return "<no GL context>";
            IntPtr p = glGetString(name);
            if (p == IntPtr.Zero) return "<null>";
            return Marshal.PtrToStringAnsi(p);
        }

        /// <summary>True when the current context is OpenGL 3.3 or newer.</summary>
        public static bool IsCompatible()
        {
            string v = VersionString();
            int major = 0, minor = 0;
            int i = 0;
            while (i < v.Length && v[i] >= '0' && v[i] <= '9')
            {
                major = major * 10 + (v[i] - '0');
                i++;
            }
            if (i < v.Length && v[i] == '.')
            {
                i++;
                while (i < v.Length && v[i] >= '0' && v[i] <= '9')
                {
                    minor = minor * 10 + (v[i] - '0');
                    i++;
                }
            }
            return major > 3 || (major == 3 && minor >= 3);
        }
    }
}
