#!/bin/sh
# ============================================================================
#  Universe Sandbox 3D - Windows code compile-check (runs on Linux/macOS)
#
#  Compiles every translation unit of the Windows build against a minimal
#  stub <windows.h> (syntax/types/declaration check only; nothing is linked
#  or executed). Catches most real errors in the Win32 layer without a
#  Windows SDK. For a true build, use CMake on Windows (see README.md).
# ============================================================================
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-g++}"
FLAGS="-fsyntax-only -std=c++17 -Wall -Wextra -D_WIN32 -Itools/compile-check/stub_include -Isrc"

fail=0
for f in $(find src -name '*.cpp' | sort); do
    echo "CHECK $f"
    if ! $CXX $FLAGS "$f" 2> /tmp/us3d_stub_err.txt; then
        echo "  FAIL"
        sed 's/^/    /' /tmp/us3d_stub_err.txt | head -30
        fail=1
    else
        echo "  ok"
    fi
done

if [ "$fail" -eq 0 ]; then
    echo "WIN32 COMPILE CHECK: all translation units OK"
else
    echo "WIN32 COMPILE CHECK: FAILURES"
    exit 1
fi
