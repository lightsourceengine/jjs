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

cmake_minimum_required(VERSION 3.10)
project (JJS C)

if(NOT DEFINED PYTHON)
  set(PYTHON "python")
endif()
# Determining version
execute_process(COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/version.py
                OUTPUT_VARIABLE JJS_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Determining platform
set(PLATFORM "${CMAKE_SYSTEM_NAME}")
string(TOUPPER "${PLATFORM}" PLATFORM)

# Determining compiler
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(USING_GCC 1)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(USING_CLANG 1)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "TI")
  set(USING_TI 1)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  set(USING_MSVC 1)
endif()

# Determining build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "MinSizeRel")
endif()

# Optional components
set(JJS_CMDLINE               ON  CACHE BOOL "Build JJS command line tool?")
set(JJS_CMDLINE_TEST          OFF CACHE BOOL "Build JJS test command line tool?")
set(JJS_CMDLINE_SNAPSHOT      OFF CACHE BOOL "Build JJS snapshot command line tool?")
set(JJS_LIBFUZZER             OFF CACHE BOOL "Build JJS with libfuzzer support?")
set(JJS_PORT                  ON  CACHE BOOL "Build default JJS port implementation?")
set(JJS_EXT                   ON  CACHE BOOL "Build jjs-ext?")
set(JJS_EXT_DEBUGGER          ON  CACHE BOOL "Build jjs-ext debugger support?")
set(UNITTESTS                 OFF CACHE BOOL "Build unit tests?")
set(DOCTESTS                  OFF CACHE BOOL "Build doc tests?")

# Optional build settings
set(BUILD_SHARED_LIBS         OFF CACHE BOOL "Build shared libraries?")
set(ENABLE_AMALGAM            OFF CACHE BOOL "Enable amalgamated build?")
set(ENABLE_LTO                ON  CACHE BOOL "Enable LTO build?")
set(ENABLE_STRIP              ON  CACHE BOOL "Enable stripping all symbols from release binary?")
set(ENABLE_COMPILE_COMMANDS   ON  CACHE BOOL "Enable generating compile_commands.json?")

# Option overrides
if(NOT USING_CLANG)
  set(JJS_LIBFUZZER OFF)

  set(JJS_LIBFUZZER_MESSAGE " (FORCED BY COMPILER)")
endif()

if(JJS_CMDLINE OR JJS_CMDLINE_TEST OR JJS_CMDLINE_SNAPSHOT OR JJS_LIBFUZZER OR UNITTESTS OR DOCTESTS)
  set(JJS_PORT ON)

  set(JJS_PORT_MESSAGE " (FORCED BY CMDLINE OR LIBFUZZER OR TESTS)")
endif()

if(JJS_CMDLINE OR DOCTESTS)
  set(JJS_EXT ON)

  set(JJS_EXT_MESSAGE " (FORCED BY CMDLINE OR TESTS)")
endif()

if("${PLATFORM}" STREQUAL "DARWIN")
  set(ENABLE_LTO         OFF)
  set(ENABLE_STRIP       OFF)

  set(ENABLE_LTO_MESSAGE         " (FORCED BY PLATFORM)")
  set(ENABLE_STRIP_MESSAGE       " (FORCED BY PLATFORM)")
endif()

if("${PLATFORM}" STREQUAL "ESP-IDF")
  set(ENABLE_LTO         OFF)
  set(ENABLE_STRIP       OFF)

  set(ENABLE_LTO_MESSAGE         " (FORCED BY PLATFORM)")
  set(ENABLE_STRIP_MESSAGE       " (FORCED BY PLATFORM)")
endif()

if(USING_TI)
  set(ENABLE_STRIP       OFF)

  set(ENABLE_STRIP_MESSAGE       " (FORCED BY COMPILER)")
endif()

if(USING_MSVC)
  set(ENABLE_STRIP       OFF)

  set(ENABLE_STRIP_MESSAGE       " (FORCED BY COMPILER)")
endif()

if(CYGWIN OR MINGW OR MSYS)
  set(ENABLE_LTO         OFF)

  set(ENABLE_LTO_MESSAGE         " (FORCED BY PLATFORM)")
endif()

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ${ENABLE_COMPILE_COMMANDS})

# Status messages
message(STATUS "CMAKE_BUILD_TYPE               " ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_C_COMPILER_ID            " ${CMAKE_C_COMPILER_ID})
message(STATUS "CMAKE_SYSTEM_NAME              " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_SYSTEM_PROCESSOR         " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "BUILD_SHARED_LIBS              " ${BUILD_SHARED_LIBS})
message(STATUS "ENABLE_AMALGAM                 " ${ENABLE_AMALGAM} ${ENABLE_AMALGAM_MESSAGE})
message(STATUS "ENABLE_LTO                     " ${ENABLE_LTO} ${ENABLE_LTO_MESSAGE})
message(STATUS "ENABLE_STRIP                   " ${ENABLE_STRIP} ${ENABLE_STRIP_MESSAGE})
message(STATUS "ENABLE_COMPILE_COMMANDS        " ${ENABLE_COMPILE_COMMANDS})
message(STATUS "JJS_VERSION                    " ${JJS_VERSION})
message(STATUS "JJS_CMDLINE                    " ${JJS_CMDLINE} ${JJS_CMDLINE_MESSAGE})
message(STATUS "JJS_CMDLINE_TEST               " ${JJS_CMDLINE_TEST} ${JJS_CMDLINE_TEST_MESSAGE})
message(STATUS "JJS_CMDLINE_SNAPSHOT           " ${JJS_CMDLINE_SNAPSHOT} ${JJS_CMDLINE_SNAPSHOT_MESSAGE})
message(STATUS "JJS_LIBFUZZER                  " ${JJS_LIBFUZZER} ${JJS_LIBFUZZER_MESSAGE})
message(STATUS "JJS_PORT                       " ${JJS_PORT} ${JJS_PORT_MESSAGE})
message(STATUS "JJS_EXT                        " ${JJS_EXT} ${JJS_EXT_MESSAGE})
message(STATUS "JJS_MATH                       " ${JJS_MATH} ${JJS_MATH_MESSAGE})
message(STATUS "UNITTESTS                      " ${UNITTESTS})
message(STATUS "DOCTESTS                       " ${DOCTESTS})

# Setup directories
# Note: This mimics a conventional file system layout in the build directory for
# the sake of convenient location of build artefacts. Proper installation to
# traditional locations is also supported, e.g., to /usr/local.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")

# Remove rdynamic option
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS )

# Compile/link flags
# Helper macros
macro(jjs_add_flags VAR)
  foreach(_flag ${ARGN})
    set(${VAR} "${${VAR}} ${_flag}")
  endforeach()
endmacro()

macro(jjs_add_compile_flags)
  jjs_add_flags(CMAKE_C_FLAGS ${ARGV})
endmacro()

macro(jjs_add_compile_warnings)
  foreach(_warning ${ARGV})
    jjs_add_compile_flags(-W${_warning})
    jjs_add_compile_flags(-Werror=${_warning})
  endforeach()
endmacro()

macro(jjs_add_link_flags)
  jjs_add_flags(LINKER_FLAGS_COMMON ${ARGV})
endmacro()

# Architecture-specific compile/link flags
jjs_add_compile_flags(${FLAGS_COMMON_ARCH})
jjs_add_flags(CMAKE_EXE_LINKER_FLAGS ${FLAGS_COMMON_ARCH})

# LTO
if(ENABLE_LTO)
  if(USING_GCC OR USING_CLANG)
    jjs_add_compile_flags(-flto)
    jjs_add_link_flags(-flto)
  endif()
  if(USING_GCC)
    jjs_add_compile_flags(-fno-fat-lto-objects)
    # Use gcc-ar and gcc-ranlib to support LTO
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  endif()
  if(USING_TI)
    jjs_add_link_flags(-lto)
  endif()
endif()

# Compiler / Linker flags
if("${PLATFORM}" STREQUAL "DARWIN")
  jjs_add_link_flags(-lSystem)
  set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Sqc <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
  set(CMAKE_SHARED_LINKER_FLAGS  "-undefined dynamic_lookup")
elseif((NOT CYGWIN AND NOT MINGW AND NOT MSYS) AND (USING_GCC OR USING_CLANG))
  jjs_add_link_flags(-Wl,-z,noexecstack)
endif()

if(USING_GCC OR USING_CLANG)
  jjs_add_compile_flags(-std=c99 -pedantic)
  if(JJS_MATH)
    jjs_add_compile_flags(-fno-builtin)
  endif()
  jjs_add_compile_warnings(all extra format-nonliteral init-self sign-conversion format-security missing-declarations shadow strict-prototypes undef old-style-definition)
  if(NOT "${PLATFORM}" STREQUAL "WINDOWS")
    jjs_add_compile_warnings(conversion)
  endif()
  jjs_add_compile_flags(-Wno-stack-protector -Wno-attributes -Werror)
endif()

if(USING_GCC)
  jjs_add_compile_warnings(logical-op)
  # TODO: Remove workaround for gcc 7 bug if the fallthrough comment detection is fixed.
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER 7.0)
    jjs_add_compile_flags(-Wno-implicit-fallthrough)
  endif()
endif()

if(USING_CLANG)
  jjs_add_compile_flags(-Wno-nested-anon-types -Wno-static-in-inline)
endif()

if(USING_TI)
  jjs_add_compile_flags(--c99)
endif()

if(USING_MSVC)
  jjs_add_link_flags(/OPT:NOREF)
  # Disable MSVC warning 4996 globally because it stops us from using standard C functions.
  jjs_add_compile_flags(/wd4996)
endif()

if(JJS_LIBFUZZER)
  jjs_add_compile_flags(-fsanitize=fuzzer-no-link)
endif()

# Strip binary
if(ENABLE_STRIP AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  jjs_add_link_flags(-s)
endif()

# External compiler & linker flags
if(DEFINED EXTERNAL_COMPILE_FLAGS)
  jjs_add_compile_flags(${EXTERNAL_COMPILE_FLAGS})
endif()

if(DEFINED EXTERNAL_LINKER_FLAGS)
  jjs_add_link_flags(${EXTERNAL_LINKER_FLAGS})
endif()

# Used as placeholder to attach amalgamated build targets to
add_custom_target(amalgam)

# JJS's core
add_subdirectory(jjs-core)

# JJS's extension tools
if(JJS_EXT)
  add_subdirectory(jjs-ext)
endif()

# JJS's default port implementation
if(JJS_PORT)
  add_subdirectory(jjs-port)
endif()

# JJS command line tool
if(JJS_CMDLINE OR JJS_CMDLINE_TEST OR JJS_CMDLINE_SNAPSHOT OR JJS_LIBFUZZER)
  add_subdirectory(jjs-main)
endif()

# Unittests
if(UNITTESTS)
  add_subdirectory(tests/unit-core)
  if(JJS_EXT)
    add_subdirectory(tests/unit-ext)
  endif()
endif()

# Doctests
if(DOCTESTS)
  add_subdirectory(tests/unit-doc)
endif()
