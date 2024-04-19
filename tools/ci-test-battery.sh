#!/usr/bin/env bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd ${SCRIPT_DIR}/..

BUILD_TYPE=

if [[ "$1" == 'debug' ]]; then
    BUILD_TYPE=--build-debug
    SANITIZER_BUILDOPTIONS="--buildoptions=--compile-flag=-fsanitize=address,--compile-flag=-fsanitize=undefined,--compile-flag=-fno-omit-frame-pointer,--compile-flag=-fno-common"
elif [[ "$1" == 'release' ]]; then
    BUILD_TYPE=
    SANITIZER_BUILDOPTIONS=""
else
    echo "Invalid build type of '$1'"
    exit 1
fi

EXTRA_BUILDOPTIONS=$2
COMMON_BUILDOPTIONS=--clean,--lto=OFF,--mem-stress-test=ON

DYNAMICVM_CPOINTER16=--buildoptions=${COMMON_BUILDOPTIONS},--vm-heap-static=OFF,--vm-stack-static=OFF,--cpointer-32bit=OFF
STATICVM_CPOINTER16=--buildoptions=${COMMON_BUILDOPTIONS},--vm-heap-static=ON,--vm-stack-static=ON,--cpointer-32bit=OFF
DYNAMICVM_CPOINTER32=--buildoptions=${COMMON_BUILDOPTIONS},--vm-heap-static=OFF,--vm-stack-static=OFF,--cpointer-32bit=ON
STATICVM_CPOINTER32=--buildoptions=${COMMON_BUILDOPTIONS},--vm-heap-static=ON,--vm-stack-static=ON,--cpointer-32bit=ON

./tools/run-tests.py -q --jjs-cli-tests --buildoptions=${COMMON_BUILDOPTIONS},--mem-stress-test=off ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

./tools/run-tests.py -q --unittests ${BUILD_TYPE} ${DYNAMICVM_CPOINTER16} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --unittests ${BUILD_TYPE} ${DYNAMICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --unittests ${BUILD_TYPE} ${STATICVM_CPOINTER16} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --unittests ${BUILD_TYPE} ${STATICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

./tools/run-tests.py -q --jjs-tests ${BUILD_TYPE} ${DYNAMICVM_CPOINTER16} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --jjs-tests ${BUILD_TYPE} ${DYNAMICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --jjs-tests ${BUILD_TYPE} ${STATICVM_CPOINTER16} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --jjs-tests ${BUILD_TYPE} ${STATICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

./tools/run-tests.py -q --jjs-snapshot-tests ${BUILD_TYPE} ${DYNAMICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --jjs-snapshot-tests ${BUILD_TYPE} ${STATICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}

./tools/run-tests.py -q --jjs-pack-tests ${BUILD_TYPE} ${DYNAMICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
./tools/run-tests.py -q --jjs-pack-tests ${BUILD_TYPE} ${STATICVM_CPOINTER32} ${SANITIZER_BUILDOPTIONS} ${EXTRA_BUILDOPTIONS}
