// ============================================================================
//  Universe Sandbox 3D - Windows 7 compatibility shim (MinGW/zig builds)
//
//  The MinGW/zig C++ runtime's <chrono> implementation (pulled in by
//  std::atomic / std::thread) calls GetSystemTimePreciseAsFileTime -
//  an API that only exists on Windows 8 and newer - through the classic
//  MinGW import pointer __imp_GetSystemTimePreciseAsFileTime. That puts
//  a kernel32 import in the exe which the Windows 7 loader cannot
//  resolve, so the cross-built exe fails at startup with:
//
//    "The procedure entry point GetSystemTimePreciseAsFileTime could not
//     be located in the dynamic link library KERNEL32.dll."
//
//  Defining the __imp_ pointer in this program makes the linker resolve
//  the reference to our data symbol instead of generating the kernel32
//  import; we point it at a Win7-safe implementation
//  (GetSystemTimeAsFileTime, available since Windows 2000). The cost is
//  a coarser time source for the few runtime clock paths that use it
//  (~15 ms vs ~1 us) - irrelevant for this app, whose own frame timing
//  uses QueryPerformanceCounter directly.
//
//  Compiled into MinGW/zig builds only. MSVC builds link the
//  Windows 7-safe UCRT and never import the Win8+ API.
// ============================================================================
#if defined(_WIN32) && defined(__MINGW32__)

#include <windows.h>

extern "C" {

static void us3d_get_system_time_precise_as_file_time(LPFILETIME ft) {
    GetSystemTimeAsFileTime(ft);
}

// Satisfies the runtime's import-pointer reference with a Win7-safe
// function pointer.
void* __imp_GetSystemTimePreciseAsFileTime =
    reinterpret_cast<void*>(&us3d_get_system_time_precise_as_file_time);

} // extern "C"

#endif // _WIN32 && __MINGW32__
