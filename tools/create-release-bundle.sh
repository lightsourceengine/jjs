#!/usr/bin/env bash

# Create a release tgz or zip bundle of jjs commandline programs.
#
# usage: create-release-bundle.sh TAG tgz|zip
#        TAG format: jjs-SEMVER-OS-ARCH, ex: jjs-4.0.0-linux-x64

set -e

# cd to source root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
cd "${SCRIPT_DIR}/.."

# parse arguments
TAG=$1

if [ -z "${TAG}" ]; then
  echo "First argument must be a tag: jjs-SEMVER-OS-ARCH"
  exit 1
fi

ARCHIVE=$2

case "${ARCHIVE}" in
  tgz|zip) ;;
  *) echo "Second argument must be tgz or zip";;
esac

cd build

# prepare staging directory in build
if [ -d "${TAG}" ]; then
  rm -rf "${TAG}"
fi

mkdir -p "${TAG}/bin"

# copy jjs commandline programs
if [ -d "bin/MinSizeRel" ]; then
  cp bin/MinSizeRel/jjs* "${TAG}/bin"
else
  cp bin/jjs* "${TAG}/bin"
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
echo "$(git rev-parse HEAD)" > "${TAG}.rev"

# cleanup staging directory
rm -rf "${TAG}"
