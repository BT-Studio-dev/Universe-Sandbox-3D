@echo off
rem ============================================================================
rem  run_mesa.bat - software-rendering fallback launcher
rem
rem  For VMs or computers without a working GPU/3D driver: run the
rem  simulation with Mesa's llvmpipe software renderer instead of a
rem  hardware OpenGL driver.
rem
rem  Requirements (all in the SAME folder as UniverseSandbox3D.exe):
rem    UniverseSandbox3D.exe
rem    opengl32.dll       <- "Mesa for Windows" release build
rem    libgallium_wgl.dll <- "Mesa for Windows" release build
rem
rem  Get the DLLs:  https://www.mesa3d.org/download.html
rem  (take the Windows x64 build; both DLLs go next to the exe)
rem
rem  The MESA_* variables force llvmpipe to advertise an OpenGL 3.3
rem  Core / 330 GLSL context even if this Mesa build under-reports
rem  its version. They are harmless on Mesa builds that already
rem  report 3.3 or newer.
rem ============================================================================
cd /d "%~dp0"
set MESA_GL_VERSION_OVERRIDE=3.3FC
set MESA_GLSL_VERSION_OVERRIDE=330
UniverseSandbox3D.exe %*
