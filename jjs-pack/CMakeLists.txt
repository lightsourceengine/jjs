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

set(INCLUDE_PACK_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(INCLUDE_PACK_PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(INCLUDE_PACK_PUBLIC ${INCLUDE_PACK_PUBLIC} PARENT_SCOPE) # for jjs-main

# Source directories
set(PACK_SOURCES
  console/jjs-pack-console.c
  console/jjs-pack-console-js.c
  domexception/jjs-pack-domexception.c
  domexception/jjs-pack-domexception-js.c
  lib/jjs-pack-lib.c
  path/jjs-pack-path.c
  path/jjs-pack-path-js.c
  path-url/jjs-pack-path-url.c
  path-url/jjs-pack-path-url-js.c
  performance/jjs-pack-performance.c
  performance/jjs-pack-performance-js.c
  text/jjs-pack-text.c
  text/jjs-pack-text-api-js.c
  url/jjs-pack-url.c
  url/jjs-pack-url-api-js.c
)

if(ENABLE_AMALGAM)
  set(PACK_HEADERS
    lib/jjs-pack-lib.h
    include/jjs-pack.h
    include/jjs-pack-config.h
  )

  set(AMALGAM_PACK_C "${CMAKE_BINARY_DIR}/amalgam/jjs-pack.c")
  set(AMALGAM_PACK_H "${CMAKE_BINARY_DIR}/amalgam/jjs-pack.h")
  set(AMALGAM_PACK_CONFIG_H "${CMAKE_BINARY_DIR}/amalgam/jjs-pack-config.h")

  add_custom_command(OUTPUT ${AMALGAM_PACK_C} ${AMALGAM_PACK_H}
      COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/amalgam.py
      --jjs-pack
      --output-dir ${CMAKE_BINARY_DIR}/amalgam
      DEPENDS ${PACK_SOURCES}
      ${PACK_HEADERS}
      ${CMAKE_SOURCE_DIR}/tools/amalgam.py
  )

  add_custom_target(amalgam-jjs-pack DEPENDS ${AMALGAM_PACK_C})
  add_dependencies(amalgam amalgam-jjs-pack)

  set(PACK_SOURCES ${AMALGAM_PACK_C})
  set(INCLUDE_PACK_PUBLIC "${CMAKE_BINARY_DIR}/amalgam" PARENT_SCOPE)
endif()

# Default jjs-pack implementation library
add_library(${JJS_PACK_NAME} ${PACK_SOURCES})
add_dependencies(${JJS_PACK_NAME} amalgam)
target_include_directories(${JJS_PACK_NAME} PRIVATE ${INCLUDE_CORE_PUBLIC} ${INCLUDE_PACK_PUBLIC} ${INCLUDE_PACK_PRIVATE})

if (JJS_PACK)
  target_compile_definitions(${JJS_PACK_NAME} PRIVATE -DJJS_PACK=1)
else()
  target_compile_definitions(${JJS_PACK_NAME} PRIVATE -DJJS_PACK=0)
endif()

# Installation
configure_file(libjjs-pack.pc.in libjjs-pack.pc @ONLY)

install(TARGETS ${JJS_PACK_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjjs-pack.pc DESTINATION lib/pkgconfig)
