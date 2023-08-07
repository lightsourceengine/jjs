#!/bin/bash

# Copyright JS Foundation and other contributors, http://js.foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if [[ "$OSTYPE" == "linux"* ]]; then
  CPPCHECK_JOBS=${CPPCHECK_JOBS:=$(nproc)}
elif [[ "$OSTYPE" == "darwin"* ]]; then
  CPPCHECK_JOBS=${CPPCHECK_JOBS:=$(sysctl -n hw.ncpu)}
else
  CPPCHECK_JOBS=${CPPCHECK_JOBS:=1}
fi

JJS_CORE_DIRS=`find jjs-core -type d`
JJS_EXT_DIRS=`find jjs-ext -type d`
JJS_PORT_DIRS=`find jjs-port -type d`
JJS_MATH_DIRS=`find jjs-math -type d`


INCLUDE_DIRS=()
for DIR in $JJS_CORE_DIRS $JJS_EXT_DIRS $JJS_PORT_DIRS $JJS_MATH_DIRS
do
  INCLUDE_DIRS=("${INCLUDE_DIRS[@]}" "-I$DIR")
done

cppcheck -j$CPPCHECK_JOBS --force \
  --language=c --std=c99 \
  --quiet \
  -D'JJS_LIKELY(x)=(x)' \
  -D'JJS_UNLIKELY(x)=(x)' \
  --enable=warning,style,performance,portability,information \
  --template="{file}:{line}: {severity}({id}): {message}" \
  --error-exitcode=1 \
  --inline-suppr \
  --exitcode-suppressions=tools/cppcheck/suppressions-list \
  --suppressions-list=tools/cppcheck/suppressions-list \
  "${INCLUDE_DIRS[@]}" \
  jjs-core jjs-ext jjs-port jjs-math jjs-main tests/unit-*
