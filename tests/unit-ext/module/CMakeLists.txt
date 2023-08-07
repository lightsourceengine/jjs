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
set(JJS_MODULE_UNITTEST_NAME unit-test-jjs-module)
project (${JJS_MODULE_UNITTEST_NAME} C)

set(JJS_MODULE_UNIT_TEST_SOURCES
  jjs-module-test.c
  my-broken-module.c
  my-custom-module.c
)

if (FEATURE_INIT_FINI)
  set(DEFINES_JJS_MODULE_UNITTEST ${DEFINES_JJS_MODULE_UNITTEST} ENABLE_INIT_FINI)
endif()

add_executable(${JJS_MODULE_UNITTEST_NAME} ${JJS_MODULE_UNIT_TEST_SOURCES})
set_property(TARGET ${JJS_MODULE_UNITTEST_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
set_property(TARGET ${JJS_MODULE_UNITTEST_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(${JJS_MODULE_UNITTEST_NAME} jjs-ext jjs-core jjs-port)
target_include_directories(${JJS_MODULE_UNITTEST_NAME} PRIVATE ${INCLUDE_UNIT_EXT})
