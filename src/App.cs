// ============================================================================
//  Universe Sandbox 3D (C#) - application controller
//
//  Phase 1 (foundation): window + WGL 3.3 context bootstrap + GL version
//  verification + a clear/render loop. The simulation, renderer, HUD and
//  panel GUI are added in the following phases on top of this foundation.
// ============================================================================
using System;
using System.Text;
using UniverseSandbox3D.Win32;

namespace UniverseSandbox3D
{
    public class App
    {
        Window window_ = new Window();
        bool inited_;

        public int Run()
        {
            if (!Init()) return 1;
            while (inited_)
            {
                if (!Window.PumpMessage()) break;
                RenderFrame();
            }
            window_.Destroy();
            return 0;
        }

        bool Init()
        {
            if (!window_.Create("Universe Sandbox 3D (C#)", 1280, 720))
            {
                MessageBoxFail(window_.Error);
                return false;
            }
            if (!GL33.IsCompatible())
            {
                StringBuilder msg = new StringBuilder();
                msg.AppendLine("This computer's graphics driver reports an OpenGL version below 3.3.");
                msg.AppendLine();
                msg.Append(window_.GlReport);
                msg.AppendLine();
                msg.Append("Universe Sandbox 3D needs OpenGL 3.3 (core or compatibility profile).");
                msg.AppendLine("Options:");
                msg.AppendLine(" - On a physical PC: update the graphics driver and try again.");
                msg.AppendLine(" - On a VM or a PC without a working 3D driver: put the");
                msg.AppendLine("   Mesa-for-Windows DLLs (opengl32.dll + libgallium_wgl.dll) next");
                msg.AppendLine("   to the exe and start it with the included run_mesa.bat.");
                msg.AppendLine(" - The 'Context bootstrap' lines above show which 3.3-context");
                msg.AppendLine("   attempt succeeded or failed - useful when reporting the issue.");
                MessageBoxFail(msg.ToString());
                return false;
            }

            // Phase 1: show the detected GL version in the title bar so a
            // working context is visible without a screenshot.
            window_.SetTitle("Universe Sandbox 3D (C#) - "
                + GL33.VersionString() + " | " + GL33.RendererString());
            inited_ = true;
            return true;
        }

        void RenderFrame()
        {
            // Phase 1: clear only. The N-body scene replaces this in the
            // renderer phase.
            GL33.glClearColor(0.016f, 0.02f, 0.05f, 1f);
            GL33.glClear(GL33.GL_COLOR_BUFFER_BIT | GL33.GL_DEPTH_BUFFER_BIT);
            window_.SwapBuffers();
        }

        static void MessageBoxFail(string text)
        {
            NativeMethods.MessageBox(IntPtr.Zero, text,
                "Universe Sandbox 3D (C#) - OpenGL 3.3 not available",
                NativeMethods.MB_OK | NativeMethods.MB_ICONERROR);
        }
    }
}
