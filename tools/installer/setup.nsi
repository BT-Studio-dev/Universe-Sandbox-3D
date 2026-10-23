; ============================================================================
;  Universe Sandbox 3D - NSIS installer (Windows 7 SP1 / 10 / 11, x64)
;
;  Built by CI:  makensis tools\installer\setup.nsi
;  (expects the app at ..\..\build\us3d.exe, i.e. the CMake MSVC build)
; ============================================================================
!include "MUI2.nsh"

Name "Universe Sandbox 3D"
OutFile "UniverseSandbox3D-Setup-1.0.0-win64.exe"
InstallDir "$PROGRAMFILES64\Universe Sandbox 3D"
InstallDirRegKey HKLM "Software\UniverseSandbox3D" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\us3d.exe"

Section "Install"
  SetOutPath "$INSTDIR"
  File "..\..\build\us3d.exe"
  WriteRegStr HKLM "Software\UniverseSandbox3D" "InstallDir" "$INSTDIR"
  CreateShortcut "$SMPROGRAMS\Universe Sandbox 3D.lnk" "$INSTDIR\us3d.exe"
  CreateShortcut "$DESKTOP\Universe Sandbox 3D.lnk" "$INSTDIR\us3d.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\us3d.exe"
  Delete "$SMPROGRAMS\Universe Sandbox 3D.lnk"
  Delete "$DESKTOP\Universe Sandbox 3D.lnk"
  DeleteRegKey HKLM "Software\UniverseSandbox3D"
  RMDir "$INSTDIR"
  RMDir "$SMPROGRAMS"
SectionEnd
