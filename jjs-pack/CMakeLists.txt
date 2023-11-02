# Copyright Light Source Software, LLC and other contributors.
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

set(JJS_PACK_NAME jjs-pack)
project (${JJS_PACK_NAME} C)

# Source directories
set(PACK_SOURCES
  console/jjs-pack-console.c
  console/jjs-pack-console-js.c
  domexception/jjs-pack-domexception.c
  domexception/jjs-pack-domexception-js.c
  lib/jjs-pack-lib.c
  path/jjs-pack-path.c
  path/jjs-pack-path-js.c
  performance/jjs-pack-performance.c
  performance/jjs-pack-performance-js.c
  url/jjs-pack-url.c
  url/jjs-pack-url-js.c
  url/jjs-pack-url-search-params-js.c
)

# Default jjs-pack implementation library
add_library(${JJS_PACK_NAME} ${PACK_SOURCES})
target_include_directories(${JJS_PACK_NAME} PRIVATE ${INCLUDE_CORE_PUBLIC} include lib)
target_compile_definitions(${JJS_PACK_NAME} PUBLIC ${DEFINES_JJS_CORE})

# Installation
configure_file(libjjs-pack.pc.in libjjs-pack.pc @ONLY)

install(TARGETS ${JJS_PACK_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjjs-pack.pc DESTINATION lib/pkgconfig)
