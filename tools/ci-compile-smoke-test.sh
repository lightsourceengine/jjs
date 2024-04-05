#!/usr/bin/env bash

# ci-compile-smoke-test.sh [debug|release]
#
# JJS has many target platforms and configurations that it is difficult to determine
# if a checkin will even compile on all platforms. This script uses zig to do a local
# so-called smoke test to see if the current tree will compile for known platforms.
#
# No unittests are run due to the emulators being way too slow (minutes on native
# platform vs tens of minutes to hours on emulators) and the emulators not being
# available on all platforms.

set -e
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "${SCRIPT_DIR}/.."

if [[ "$1" == 'debug' ]]; then
    BUILD_TYPE=--debug
elif [[ "$1" == 'release' ]]; then
    BUILD_TYPE=
else
    echo "Invalid build type of '$1'"
    exit 1
fi

# - turn on as many flags to maximize compiler coverage
# - turn off lto, as it can be slow to link the unittests and executables in general
# - always clean, as cmake can cache old settings
BUILDOPTIONS="${BUILD_TYPE} --jjs-pack on --unittests on --jjs-cmdline-snapshot on --jjs-debugger on --lto off --clean"

# primary platforms
./tools/build.py --toolchain=cmake/zig-linux-x86_64.cmake ${BUILDOPTIONS}
# note: BaseTsd.h used by debugger is in MSVC, but not zig/mingw
./tools/build.py --toolchain=cmake/zig-windows-x86_64.cmake ${BUILDOPTIONS} --jjs-debugger off
./tools/build.py --toolchain=cmake/zig-macos-x86_64.cmake ${BUILDOPTIONS}

# various platform/arch combinations
./tools/build.py --toolchain=cmake/zig-linux-x86.cmake ${BUILDOPTIONS}
./tools/build.py --toolchain=cmake/zig-linux-aarch64.cmake ${BUILDOPTIONS}
./tools/build.py --toolchain=cmake/zig-linux-armv7l.cmake ${BUILDOPTIONS}
./tools/build.py --toolchain=cmake/zig-macos-aarch64.cmake ${BUILDOPTIONS}
