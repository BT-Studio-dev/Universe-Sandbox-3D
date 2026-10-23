@echo off
rem ============================================================================
rem  build_csc.bat - build Universe Sandbox 3D (C#) without Visual Studio
rem
rem  Uses the stock .NET Framework C# compiler (csc.exe), which is
rem  preinstalled with .NET Framework 4.x on Windows 7 SP1 / 8 / 10 / 11.
rem  Output: UniverseSandbox3D.exe (next to this file).
rem ============================================================================
setlocal
set "CSC="
where csc >nul 2>nul && set "CSC=csc"
if "%CSC%"=="" if exist "%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" set "CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if "%CSC%"=="" (
  echo C# compiler (csc) not found.
  echo Install .NET Framework 4.5 or newer, or build with Visual Studio.
  exit /b 1
)
"%CSC%" /nologo /target:winexe /platform:x64 /optimize+ /out:UniverseSandbox3D.exe ^
  src\Program.cs ^
  src\App.cs ^
  src\Win32\NativeMethods.cs ^
  src\Win32\GL33.cs ^
  src\Win32\Window.cs
if errorlevel 1 (
  echo Build FAILED
  exit /b 1
)
echo Built: UniverseSandbox3D.exe
