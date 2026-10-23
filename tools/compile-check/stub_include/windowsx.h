// ============================================================================
//  Stub <windowsx.h> for the Linux compile-check only.
//  Provides the windowing helper macros the project uses.
// ============================================================================
#pragma once

#ifndef GET_X_LPARAM
#  define GET_X_LPARAM(lp)       ((int)(short)(((LPARAM)(lp)) & 0xFFFF))
#endif
#ifndef GET_Y_LPARAM
#  define GET_Y_LPARAM(lp)       ((int)(short)(((LPARAM)(lp) >> 16) & 0xFFFF))
#endif
