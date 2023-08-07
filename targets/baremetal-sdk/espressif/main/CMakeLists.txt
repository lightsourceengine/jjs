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
cmake_minimum_required(VERSION 3.5)

# Set JJS variables
set(JJS_ROOT_DIR ${PROJECT_DIR}/../../..)
set(JJS_BUILD_DIR ${JJS_ROOT_DIR}/build/jjs)
set(JJS_INCLUDE_DIRS "${JJS_ROOT_DIR}/jjs-ext/include" "${JJS_ROOT_DIR}/jjs-core/include")
set(JJS_HEAP_SIZE 20)

# Make compile flags separated by spaces
string(REPLACE ";" " " COMPILE_FLAGS "${COMPILE_OPTIONS};${C_COMPILE_OPTIONS}")

# Build JJS
execute_process(COMMAND ${JJS_ROOT_DIR}/tools/build.py
                            --clean
		                    --builddir=${JJS_BUILD_DIR}
                            --lto=OFF
                            --jjs-cmdline=OFF
                            --jjs-math=OFF
                            --amalgam=ON
                            --mem-heap=${JJS_HEAP_SIZE}
                            --profile=es.next
                            --compile-flag=${COMPILE_FLAGS}
                            --toolchain=${CMAKE_TOOLCHAIN_FILE})

# Register JJS target application
idf_component_register(SRCS "jjs-main.c" "jjs-port.c" INCLUDE_DIRS ${JJS_INCLUDE_DIRS})

add_library(jjs-core STATIC IMPORTED)
add_library(jjs-ext STATIC IMPORTED)
add_library(jjs-port STATIC IMPORTED)

set_target_properties(jjs-core PROPERTIES IMPORTED_LOCATION "${JJS_BUILD_DIR}/lib/libjjs-core.a")
set_target_properties(jjs-ext PROPERTIES IMPORTED_LOCATION "${JJS_BUILD_DIR}/lib/libjjs-ext.a")
set_target_properties(jjs-port PROPERTIES IMPORTED_LOCATION "${JJS_BUILD_DIR}/lib/libjjs-port.a")

# jjs-core has espressif/main/jjs-port.c as dependency
target_link_libraries(jjs-core INTERFACE ${COMPONENT_LIB})
# jjs-main has jjs-core, jjs-ext, jjs-port as dependencies
target_link_libraries(${COMPONENT_LIB} INTERFACE jjs-core jjs-ext jjs-port)
