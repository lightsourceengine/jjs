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

set(JJS_EXT_NAME jjs-ext)

# Optional features
set(FEATURE_INIT_FINI OFF CACHE BOOL "Enable library constructor/destructor support?")

# Status messages
message(STATUS "FEATURE_INIT_FINI              " ${FEATURE_INIT_FINI})

# Include directories
set(INCLUDE_EXT_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(INCLUDE_EXT_PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/common")

set(INCLUDE_EXT_PUBLIC ${INCLUDE_EXT_PUBLIC} PARENT_SCOPE) # for jjs-port

if(FEATURE_INIT_FINI)
  set(DEFINES_EXT ${DEFINES_EXT} ENABLE_INIT_FINI)
endif()

# Source directories
set(SOURCE_EXT
  arg/arg-js-iterator-helper.c
  arg/arg-transform-functions.c
  arg/arg.c
  handle-scope/handle-scope-allocator.c
  handle-scope/handle-scope.c
  util/handlers.c
  util/print.c
  util/properties.c
  util/repl.c
  util/sources.c
  util/test262.c
)

if (JJS_EXT_DEBUGGER)
  list(APPEND SOURCE_EXT
    debugger/debugger-common.c
    debugger/debugger-rp.c
    debugger/debugger-serial.c
    debugger/debugger-sha1.c
    debugger/debugger-tcp.c
    debugger/debugger-ws.c
  )
endif()

add_library(${JJS_EXT_NAME} ${SOURCE_EXT})

target_include_directories(${JJS_EXT_NAME} PUBLIC ${INCLUDE_EXT_PUBLIC})
target_include_directories(${JJS_EXT_NAME} PRIVATE ${INCLUDE_EXT_PRIVATE})
target_compile_definitions(${JJS_EXT_NAME} PUBLIC ${DEFINES_EXT})
target_link_libraries(${JJS_EXT_NAME} jjs-core)

set(JJS_EXT_PKGCONFIG_LIBS)

if("${PLATFORM}" STREQUAL "WINDOWS" AND JJS_EXT_DEBUGGER)
  target_link_libraries(${JJS_EXT_NAME} ws2_32)
  set(JJS_EXT_PKGCONFIG_LIBS -lws2_32)
endif()

configure_file(libjjs-ext.pc.in libjjs-ext.pc @ONLY)

install(TARGETS ${JJS_EXT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjjs-ext.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_EXT_PUBLIC}/ DESTINATION include)
