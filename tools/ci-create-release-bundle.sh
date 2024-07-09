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
ARCHIVE_TYPE=$1

case "${ARCHIVE_TYPE}" in
  tgz|zip) ;;
  *)
    echo "Second argument must be tgz or zip"
    exit 1
  ;;
esac

shift

ARCHIVE_NAME="jjs-$(./tools/version.py)-${TAG}"

# standard jjs configuration
./tools/build.py \
  --default-vm-heap-size-kb 8192 \
  --show-opcodes ON \
  --show-regexp-opcodes ON \
  --jjs-pack ON \
  --jjs-cmdline ON \
  --logging ON \
  --snapshot-exec ON \
  --snapshot-save ON \
  --clean \
  "$@"

cd build

# prepare staging directory in build
if [ -d "${ARCHIVE_NAME}" ]; then
  rm -rf "${ARCHIVE_NAME}"
fi

mkdir "${ARCHIVE_NAME}"

# copy jjs commandline program
if [ -d "bin/MinSizeRel" ]; then
  JJS_EXE="jjs-${TAG}.exe"
  cp bin/MinSizeRel/jjs.exe "${ARCHIVE_NAME}/${JJS_EXE}"
elif [ -d "bin" ]; then
  JJS_EXE="jjs-${TAG}"
  cp bin/jjs "${ARCHIVE_NAME}/${JJS_EXE}"
else
  echo "Could not find jjs executable in build/"
  exit 1
fi

# archive
JJS_ARCHIVE_FILENAME="${ARCHIVE_NAME}.${ARCHIVE_TYPE}"
rm -f "${JJS_ARCHIVE_FILENAME}"

cd "${ARCHIVE_NAME}"

if [ "${ARCHIVE_TYPE}" = "tgz" ]; then
  tar -czf "../${JJS_ARCHIVE_FILENAME}" "${JJS_EXE}"
else
  # zip is not available on windows github actions runner. use 7z instead.
  7z a -r -tzip "../${JJS_ARCHIVE_FILENAME}" "${JJS_EXE}"
fi

cd ..

# add git hash to a file
git rev-parse HEAD > "${ARCHIVE_NAME}.rev"

echo "archive:      ${JJS_ARCHIVE_FILENAME}"
echo "git revision: ${ARCHIVE_NAME}.rev"

# cleanup staging directory
rm -rf "${ARCHIVE_NAME}"
