// ============================================================================
//  Universe Sandbox 3D (C#) - entry point
//
//  .NET Framework 4.5, x64, Windows 7 SP1 / 10 / 11.
//  Raw Win32 + WGL + self-contained OpenGL 3.3 loader - no WinForms,
//  no external packages.
// ============================================================================
using System;

namespace UniverseSandbox3D
{
    static class Program
    {
        [STAThread]
        static int Main()
        {
            try
            {
                App app = new App();
                return app.Run();
            }
            catch (Exception e)
            {
                Win32.NativeMethods.MessageBox(IntPtr.Zero,
                    "Universe Sandbox 3D failed to start:\n\n" + e.Message,
                    "Universe Sandbox 3D (C#)",
                    Win32.NativeMethods.MB_OK | Win32.NativeMethods.MB_ICONERROR);
                return 1;
            }
        }
    }
}
