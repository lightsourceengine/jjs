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
project (jjs-main C)

# Optional build settings
set(ENABLE_LINK_MAP           OFF CACHE BOOL "Enable generating a link map file?")
set(JJS_TEST_STACK_MEASURE    OFF CACHE BOOL "Enable stack measurement for the jjs-test binary?")

# Status messages
message(STATUS "ENABLE_LINK_MAP                " ${ENABLE_LINK_MAP})
message(STATUS "JJS_TEST_STACK_MEASURE         " ${JJS_TEST_STACK_MEASURE})

# Generate map file
if(ENABLE_LINK_MAP)
  if("${PLATFORM}" STREQUAL "DARWIN")
    set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -Xlinker -map -Xlinker jjs.map")
  else()
    set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -Xlinker -Map -Xlinker jjs.map")
  endif()
endif()

# Get version information from git
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND git rev-parse --short HEAD
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE JJS_COMMIT_HASH
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(JJS_COMMIT_HASH " (${JJS_COMMIT_HASH})")
else()
  set(JJS_COMMIT_HASH "")
endif()

set(DEFINES_JJS ${DEFINES_JJS} JJS_COMMIT_HASH="${JJS_COMMIT_HASH}")

macro(jjs_create_executable JJS_NAME)
  add_executable(${JJS_NAME} ${ARGN})
  set_property(TARGET ${JJS_NAME}
               PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
  target_compile_definitions(${JJS_NAME} PRIVATE ${DEFINES_JJS})

  target_link_libraries(${JJS_NAME} jjs-core)

  install(TARGETS ${JJS_NAME} DESTINATION bin)
endmacro()

# JJS with libfuzzer support
if(JJS_LIBFUZZER)
  jjs_create_executable("jjs-libfuzzer" "main-libfuzzer.c")
  target_link_libraries("jjs-libfuzzer" jjs-port-fsanitize=fuzzer)
endif()

# JJS standalones
if(JJS_CMDLINE)
  jjs_create_executable("jjs" "main-desktop.c" "arguments/options.c" "arguments/cli.c")
  target_link_libraries("jjs" jjs-ext jjs-port)
endif()

if(JJS_CMDLINE_TEST)
  jjs_create_executable("jjs-test" "benchmark/main-benchmark.c" "benchmark/stubs.c")
  target_link_libraries("jjs-test" jjs-port)
  if (JJS_TEST_STACK_MEASURE)
    target_compile_definitions("jjs-test" PRIVATE -DJJS_TEST_STACK_MEASURE=1)
  endif()
endif()

if(JJS_CMDLINE_SNAPSHOT)
  jjs_create_executable("jjs-snapshot" "main-snapshot.c" "arguments/cli.c")
  target_link_libraries("jjs-snapshot" jjs-port)
endif()
