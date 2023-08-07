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

cmake_minimum_required (VERSION 2.8.12)
set(JJS_PORT_NAME jjs-port)
project (${JJS_PORT_NAME} C)

# Source directories
set(PORT_SOURCES
  common/jjs-port-context.c
  common/jjs-port-fs.c
  common/jjs-port-io.c
  common/jjs-port-process.c
  unix/jjs-port-unix-date.c
  unix/jjs-port-unix-fs.c
  unix/jjs-port-unix-process.c
  win/jjs-port-win-date.c
  win/jjs-port-win-fs.c
  win/jjs-port-win-process.c
)

# Define _BSD_SOURCE and _DEFAULT_SOURCE
# (should only be necessary if we used compiler default libc but not checking that)
set(PORT_DEFINES
  _BSD_SOURCE
  _DEFAULT_SOURCE
  JJS_GLOBAL_HEAP_SIZE=${JJS_GLOBAL_HEAP_SIZE}
)

# Amalgamated JJS source/header build.
#  The process will create the following files:
#   * jjs-port.c
#   * jjs-port.h
if(ENABLE_AMALGAM)
  set(AMALGAM_PORT_C "${CMAKE_BINARY_DIR}/amalgam/jjs-port.c")

  add_custom_command(OUTPUT ${AMALGAM_PORT_C}
                     COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/amalgam.py
                             --jjs-port
                             --output-dir ${CMAKE_BINARY_DIR}/amalgam
                     DEPENDS ${PORT_SOURCES}
                             ${CMAKE_SOURCE_DIR}/tools/amalgam.py
  )
  add_custom_target(amalgam-port DEPENDS ${AMALGAM_PORT_C})
  add_dependencies(amalgam amalgam-port)

  set(PORT_SOURCES ${AMALGAM_PORT_C})
endif()

INCLUDE (CheckStructHasMember)
# CHECK_STRUCT_HAS_MEMBER works by trying to compile some C code that accesses the
# given field of the given struct. However, our default compiler options break this
# C code, so turn a couple of them off for this.
if(USING_GCC OR USING_CLANG)
  set(CMAKE_REQUIRED_FLAGS "-Wno-error=strict-prototypes -Wno-error=old-style-definition -Wno-error=unused-value")
endif()

# tm.tm_gmtoff is non-standard, so glibc doesn't expose it in c99 mode
# (our default). Define some macros to expose it anyway.
set(CMAKE_REQUIRED_DEFINITIONS "-D_BSD_SOURCE -D_DEFAULT_SOURCE")
CHECK_STRUCT_HAS_MEMBER ("struct tm" tm_gmtoff time.h HAVE_TM_GMTOFF)

# localtime_r is is not threadsafe with clang on OSX
if(HAVE_TM_GMTOFF AND NOT "${PLATFORM}" STREQUAL "DARWIN" AND NOT USING_CLANG)
  set(PORT_DEFINES ${PORT_DEFINES} HAVE_TM_GMTOFF)
endif()

# Default JJS port implementation library
add_library(${JJS_PORT_NAME} ${PORT_SOURCES})
add_dependencies(${JJS_PORT_NAME} amalgam)
target_include_directories(${JJS_PORT_NAME} PRIVATE ${INCLUDE_CORE_PUBLIC})
target_compile_definitions(${JJS_PORT_NAME} PRIVATE ${PORT_DEFINES})

# Installation
configure_file(libjjs-port.pc.in libjjs-port.pc @ONLY)

install(TARGETS ${JJS_PORT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjjs-port.pc DESTINATION lib/pkgconfig)
