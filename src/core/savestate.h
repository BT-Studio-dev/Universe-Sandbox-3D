// ============================================================================
//  Universe Sandbox 3D - save / load (plain text, versioned)
//
//  Format (US3D v1):
//    US3D SAVE 1
//    time <years>
//    reversed <0|1>
//    body <name> <type> <x y z> <vx vy vz> <mass> <radius> <r g b> <bandFreq>
//  Names must not contain spaces (the generator guarantees this).
// ============================================================================
#pragma once

#include "core/system.h"
#include <string>

namespace us3d {

struct SaveResult {
    bool ok = false;
    std::string message;
};

SaveResult saveSystem(const System& s, const std::string& path);
SaveResult loadSystem(System& s, const std::string& path);

} // namespace us3d
