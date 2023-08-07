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

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

set(JJS_BASE ${CMAKE_SOURCE_DIR}/../../..)

# Build JJS
execute_process(COMMAND ${JJS_BASE}/tools/build.py
                            --clean
                            --lto=OFF
                            --jjs-cmdline=OFF
                            --jjs-math=ON
                            --amalgam=ON
                            --mem-heap=70
                            --profile=es.next
                            --toolchain=${JJS_BASE}/cmake/toolchain_mcu_stm32f4.cmake)

# Define library targets
add_library(jjs-core STATIC IMPORTED)
add_library(jjs-ext STATIC IMPORTED)
add_library(jjs-port STATIC IMPORTED)

# Define include directories and archives
set_target_properties(jjs-core PROPERTIES IMPORTED_LOCATION ${JJS_BASE}/build/lib/libjjs-core.a)
set_target_properties(jjs-core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${JJS_BASE}/jjs-core/include)
set_target_properties(jjs-ext PROPERTIES IMPORTED_LOCATION ${JJS_BASE}/build/lib/libjjs-ext.a)
set_target_properties(jjs-ext PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${JJS_BASE}/jjs-ext/include)
set_target_properties(jjs-port PROPERTIES IMPORTED_LOCATION ${JJS_BASE}/build/lib/libjjs-port.a)

target_sources(app PRIVATE src/jjs-main.c src/getline-zephyr.c src/jjs-port.c)
target_link_libraries(app PUBLIC jjs-core jjs-ext jjs-port)
