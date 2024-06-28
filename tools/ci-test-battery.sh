#!/usr/bin/env bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd "${SCRIPT_DIR}/.."

BUILD_TYPE=

if [[ "$1" == 'debug' ]]; then
    BUILD_TYPE=--build-debug
    SANITIZER_BUILDOPTIONS="--compile-flag=-fsanitize=address,--compile-flag=-fsanitize=undefined,--compile-flag=-fno-omit-frame-pointer,--compile-flag=-fno-common"
elif [[ "$1" == 'release' ]]; then
    BUILD_TYPE=
    SANITIZER_BUILDOPTIONS=","
else
    echo "Invalid build type of '$1'"
    exit 1
fi

EXTRA_BUILDOPTIONS=$2
COMMON_BUILDOPTIONS=--clean,--lto=OFF,--mem-stress-test=ON

CPOINTER16=${COMMON_BUILDOPTIONS},--cpointer-32bit=OFF
CPOINTER32=${COMMON_BUILDOPTIONS},--cpointer-32bit=ON

./tools/run-tests.py -q --jjs-cli-tests ${BUILD_TYPE} --buildoptions=${COMMON_BUILDOPTIONS},--mem-stress-test=OFF,${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

#./tools/run-tests.py -q --unittests ${BUILD_TYPE} --buildoptions=${CPOINTER16},${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --unittests ${BUILD_TYPE} --buildoptions=${CPOINTER32},${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

#./tools/run-tests.py -q --jjs-tests ${BUILD_TYPE} --buildoptions=${CPOINTER16},${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --jjs-tests ${BUILD_TYPE} --buildoptions=${CPOINTER32},${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

./tools/run-tests.py -q --jjs-snapshot-tests ${BUILD_TYPE} --buildoptions=${CPOINTER32},${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

./tools/run-tests.py -q --jjs-pack-tests ${BUILD_TYPE} --buildoptions=${CPOINTER32},${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
