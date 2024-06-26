#!/usr/bin/env bash

set -e
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "${SCRIPT_DIR}/.."

# parse arguments
TAG=$1

if [ -z "${TAG}" ]; then
  echo "First argument must be a tag: OS-ARCH"
  exit 1
fi

shift
ARCHIVE=$1

case "${ARCHIVE}" in
  tgz|zip) ;;
  *)
    echo "Second argument must be tgz or zip"
    exit 1
  ;;
esac

shift

TAG="jjs-$(./tools/version.py)-${TAG}"

# standard jjs configuration
./tools/build.py --builddir build/cmdline \
  --default-vm-heap-size 8192 \
  --show-opcodes ON \
  --show-regexp-opcodes ON \
  --vm-heap-static ON \
  --mem-stats ON \
  --jjs-pack ON \
  --jjs-cmdline ON \
  --logging ON \
  --snapshot-exec ON \
  --clean \
  "$@"

# jjs-snapshot must be a separate build because jjs-pack, mem-stats and opcodes are not needed
./tools/build.py --builddir build/cmdline-snapshot \
  --default-vm-heap-size 8192 \
  --vm-heap-static ON \
  --jjs-cmdline OFF \
  --jjs-cmdline-snapshot ON \
  --clean \
  "$@"

cd build

# prepare staging directory in build
if [ -d "${TAG}" ]; then
  rm -rf "${TAG}"
fi

mkdir -p "${TAG}/bin"

# copy jjs commandline programs
if [ -d "cmdline/bin/MinSizeRel" ]; then
  cp cmdline/bin/MinSizeRel/jjs.exe cmdline-snapshot/bin/MinSizeRel/jjs-snapshot.exe "${TAG}/bin"
else
  cp cmdline/bin/jjs cmdline-snapshot/bin/jjs-snapshot "${TAG}/bin"
fi

# archive
JJS_ARCHIVE="${TAG}.${ARCHIVE}"
rm -f "${JJS_ARCHIVE}"

if [ "${ARCHIVE}" = "tgz" ]; then
  tar -czf "${JJS_ARCHIVE}" "${TAG}"
else
  # zip is not available on windows github actions runner. use 7z instead.
  7z a -r -tzip "${JJS_ARCHIVE}" "${TAG}"
fi

# add git hash to a file
git rev-parse HEAD > "${TAG}.rev"

# cleanup staging directory
rm -rf "${TAG}"
