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

set(JJS_CORE_NAME jjs-core)

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

include(CheckLibraryExists)

# Optional features
set(JJS_CPOINTER_32_BIT           ON           CACHE BOOL   "Enable 32 bit compressed pointers?")
set(JJS_DEBUGGER                  OFF          CACHE BOOL   "Enable JJS debugger?")
set(JJS_ERROR_MESSAGES            ON           CACHE BOOL   "Enable error messages?")
set(JJS_PARSER                    ON           CACHE BOOL   "Enable javascript-parser?")
set(JJS_FUNCTION_TO_STRING        OFF          CACHE BOOL   "Enable function toString operation?")
set(JJS_LINE_INFO                 ON           CACHE BOOL   "Enable line info?")
set(JJS_LOGGING                   OFF          CACHE BOOL   "Enable logging?")
set(JJS_MEM_STATS                 OFF          CACHE BOOL   "Enable memory statistics?")
set(JJS_MEM_GC_BEFORE_EACH_ALLOC  OFF          CACHE BOOL   "Enable mem-stress test?")
set(JJS_PARSER_DUMP_BYTE_CODE     OFF          CACHE BOOL   "Enable parser byte-code dumps?")
set(JJS_PROMISE_CALLBACK          OFF          CACHE BOOL   "Enable Promise callbacks?")
set(JJS_REGEXP_STRICT_MODE        OFF          CACHE BOOL   "Enable regexp strict mode?")
set(JJS_REGEXP_DUMP_BYTE_CODE     OFF          CACHE BOOL   "Enable regexp byte-code dumps?")
set(JJS_SNAPSHOT_EXEC             ON           CACHE BOOL   "Enable executing snapshot files?")
set(JJS_SNAPSHOT_SAVE             OFF          CACHE BOOL   "Enable saving snapshot files?")
set(JJS_VALGRIND                  OFF          CACHE BOOL   "Enable Valgrind support?")
set(JJS_VM_HALT                   OFF          CACHE BOOL   "Enable VM execution stop callback?")
set(JJS_VM_THROW                  OFF          CACHE BOOL   "Enable VM throw callback?")
set(JJS_DEFAULT_SCRATCH_ARENA_KB  "(32)"       CACHE STRING "Size of scratch arena buffer in kilobytes?")
set(JJS_VM_STACK_LIMIT            OFF          CACHE BOOL   "Enable vm stack limit checks?")
set(JJS_DEFAULT_VM_HEAP_SIZE_KB   "(1024)"     CACHE STRING "Size of vm memory heap in kilobytes")
set(JJS_DEFAULT_VM_STACK_LIMIT_KB "(0)"        CACHE STRING "Default maximum stack usage size, in kilobytes.")

set(JJS_PLATFORM_API_FS_READ_FILE   ON         CACHE BOOL   "Enable default platform.fs.read_file implementation?")
set(JJS_PLATFORM_API_IO_WRITE       ON         CACHE BOOL   "Enable default platform.io.write implementation?")
set(JJS_PLATFORM_API_IO_FLUSH       ON         CACHE BOOL   "Enable default platform.io.flush implementation?")
set(JJS_PLATFORM_API_PATH_CWD       ON         CACHE BOOL   "Enable default platform.path.cwd implementation?")
set(JJS_PLATFORM_API_PATH_REALPATH  ON         CACHE BOOL   "Enable default platform.path.realpath implementation?")
set(JJS_PLATFORM_API_TIME_LOCAL_TZA ON         CACHE BOOL   "Enable default platform.time.local_tza implementation?")
set(JJS_PLATFORM_API_TIME_NOW_MS    ON         CACHE BOOL   "Enable default platform.time.now_ms implementation?")
set(JJS_PLATFORM_API_TIME_SLEEP     ON         CACHE BOOL   "Enable default platform.time.sleep implementation?")

# Option overrides
if (JJS_VM_HEAP_SIZE_KB GREATER 512)
  set(JJS_CPOINTER_32_BIT ON)

  set(JJS_CPOINTER_32_BIT_MESSAGE " (FORCED BY NOT HEAP STATIC OR HEAP SIZE > 512)")
endif()

if(NOT JJS_PARSER)
  set(JJS_SNAPSHOT_EXEC ON)
  set(JJS_PARSER_DUMP   OFF)

  set(JJS_SNAPSHOT_EXEC_MESSAGE " (FORCED BY DISABLED JS PARSER)")
  set(JJS_PARSER_DUMP_MESSAGE   " (FORCED BY DISABLED JS PARSER)")
endif()

if(JJS_PACK AND NOT JJS_SNAPSHOT_EXEC)
  set(JJS_SNAPSHOT_EXEC ON)
  set(JJS_SNAPSHOT_EXEC_MESSAGE " (FORCED BY JJS_PACK)")
endif()

if(JJS_CMDLINE_SNAPSHOT)
  set(JJS_SNAPSHOT_SAVE ON)

  set(JJS_SNAPSHOT_SAVE_MESSAGE " (FORCED BY SNAPSHOT TOOL)")
endif()

if(JJS_MEM_STATS OR JJS_PARSER_DUMP_BYTE_CODE OR JJS_REGEXP_DUMP_BYTE_CODE)
  set(JJS_LOGGING ON)

  set(JJS_LOGGING_MESSAGE " (FORCED BY STATS OR DUMP)")
endif()

# Status messages
message(STATUS "JJS_CPOINTER_32_BIT             " ${JJS_CPOINTER_32_BIT} ${JJS_CPOINTER_32_BIT_MESSAGE})
message(STATUS "JJS_DEBUGGER                    " ${JJS_DEBUGGER})
message(STATUS "JJS_ERROR_MESSAGES              " ${JJS_ERROR_MESSAGES})
message(STATUS "JJS_PARSER                      " ${JJS_PARSER})
message(STATUS "JJS_FUNCTION_TO_STRING          " ${JJS_FUNCTION_TO_STRING})
message(STATUS "JJS_LINE_INFO                   " ${JJS_LINE_INFO})
message(STATUS "JJS_LOGGING                     " ${JJS_LOGGING} ${JJS_LOGGING_MESSAGE})
message(STATUS "JJS_MEM_STATS                   " ${JJS_MEM_STATS})
message(STATUS "JJS_MEM_GC_BEFORE_EACH_ALLOC    " ${JJS_MEM_GC_BEFORE_EACH_ALLOC})
message(STATUS "JJS_PARSER_DUMP_BYTE_CODE       " ${JJS_PARSER_DUMP_BYTE_CODE} ${JJS_PARSER_DUMP_MESSAGE})
message(STATUS "JJS_PROMISE_CALLBACK            " ${JJS_PROMISE_CALLBACK})
message(STATUS "JJS_REGEXP_STRICT_MODE          " ${JJS_REGEXP_STRICT_MODE})
message(STATUS "JJS_REGEXP_DUMP_BYTE_CODE       " ${JJS_REGEXP_DUMP_BYTE_CODE})
message(STATUS "JJS_SNAPSHOT_EXEC               " ${JJS_SNAPSHOT_EXEC} ${JJS_SNAPSHOT_EXEC_MESSAGE})
message(STATUS "JJS_SNAPSHOT_SAVE               " ${JJS_SNAPSHOT_SAVE} ${JJS_SNAPSHOT_SAVE_MESSAGE})
message(STATUS "JJS_VALGRIND                    " ${JJS_VALGRIND})
message(STATUS "JJS_VM_HALT                     " ${JJS_VM_HALT})
message(STATUS "JJS_VM_THROW                    " ${JJS_VM_THROW})
message(STATUS "JJS_VM_STACK_LIMIT              " ${JJS_VM_STACK_LIMIT})
message(STATUS "JJS_DEFAULT_SCRATCH_ARENA_KB    " ${JJS_DEFAULT_SCRATCH_ARENA_KB})
message(STATUS "JJS_DEFAULT_VM_HEAP_SIZE_KB     " ${JJS_DEFAULT_VM_HEAP_SIZE_KB})
message(STATUS "JJS_DEFAULT_VM_STACK_LIMIT_KB   " ${JJS_DEFAULT_VM_STACK_LIMIT_KB})
message(STATUS "JJS_PLATFORM_API_FS_READ_FILE   " ${JJS_PLATFORM_API_FS_READ_FILE})
message(STATUS "JJS_PLATFORM_API_IO_WRITE       " ${JJS_PLATFORM_API_IO_WRITE})
message(STATUS "JJS_PLATFORM_API_IO_FLUSH       " ${JJS_PLATFORM_API_IO_FLUSH})
message(STATUS "JJS_PLATFORM_API_PATH_CWD       " ${JJS_PLATFORM_API_PATH_CWD})
message(STATUS "JJS_PLATFORM_API_PATH_REALPATH  " ${JJS_PLATFORM_API_PATH_REALPATH})
message(STATUS "JJS_PLATFORM_API_TIME_LOCAL_TZA " ${JJS_PLATFORM_API_TIME_LOCAL_TZA})
message(STATUS "JJS_PLATFORM_API_TIME_NOW_MS    " ${JJS_PLATFORM_API_TIME_NOW_MS})
message(STATUS "JJS_PLATFORM_API_TIME_SLEEP     " ${JJS_PLATFORM_API_TIME_SLEEP})
message(STATUS "HAVE_TM_GMTOFF                  " ${HAVE_TM_GMTOFF})

# Include directories
set(INCLUDE_CORE_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(INCLUDE_CORE_PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/annex"
    "${CMAKE_CURRENT_SOURCE_DIR}/api"
    "${CMAKE_CURRENT_SOURCE_DIR}/debugger"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/base"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/builtin-objects"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/builtin-objects/typedarray"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/operations"
    "${CMAKE_CURRENT_SOURCE_DIR}/jcontext"
    "${CMAKE_CURRENT_SOURCE_DIR}/jmem"
    "${CMAKE_CURRENT_SOURCE_DIR}/jrt"
    "${CMAKE_CURRENT_SOURCE_DIR}/lit"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/js"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/regexp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vm")

set(INCLUDE_CORE_PUBLIC ${INCLUDE_CORE_PUBLIC} PARENT_SCOPE) # for jjs-pack
set(INCLUDE_CORE_PRIVATE ${INCLUDE_CORE_PRIVATE} PARENT_SCOPE) # for tests/unit-core

# Sources
# JJS core
set(SOURCE_CORE_FILES
  annex/annex-path.c
  annex/annex-util.c
  api/jjs-annex.c
  api/jjs-annex-commonjs.c
  api/jjs-annex-esm.c
  api/jjs-annex-module-util.c
  api/jjs-annex-pmap.c
  api/jjs-annex-task.c
  api/jjs-annex-vmod.c
  api/jjs-api-object.c
  api/jjs-api-snapshot.c
  api/jjs-context-init.c
  api/jjs-debugger-transport.c
  api/jjs-debugger.c
  api/jjs-module.c
  api/jjs-platform.c
  api/jjs-platform-win.c
  api/jjs-platform-unix.c
  api/jjs-stream.c
  api/jjs-util.c
  api/jjs.c
  debugger/debugger.c
  ecma/base/ecma-alloc.c
  ecma/base/ecma-gc.c
  ecma/base/ecma-errors.c
  ecma/base/ecma-extended-info.c
  ecma/base/ecma-helpers-collection.c
  ecma/base/ecma-helpers-conversion.c
  ecma/base/ecma-helpers-errol.c
  ecma/base/ecma-helpers-external-pointers.c
  ecma/base/ecma-helpers-number.c
  ecma/base/ecma-helpers-string.c
  ecma/base/ecma-helpers-value.c
  ecma/base/ecma-helpers.c
  ecma/base/ecma-init-finalize.c
  ecma/base/ecma-lcache.c
  ecma/base/ecma-line-info.c
  ecma/base/ecma-literal-storage.c
  ecma/base/ecma-module.c
  ecma/base/ecma-property-hashmap.c
  ecma/builtin-objects/ecma-builtin-aggregateerror.c
  ecma/builtin-objects/ecma-builtin-aggregateerror-prototype.c
  ecma/builtin-objects/ecma-builtin-array-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-array-prototype-unscopables.c
  ecma/builtin-objects/ecma-builtin-array-prototype.c
  ecma/builtin-objects/ecma-builtin-array.c
  ecma/builtin-objects/ecma-builtin-arraybuffer-prototype.c
  ecma/builtin-objects/ecma-builtin-arraybuffer.c
  ecma/builtin-objects/ecma-builtin-async-from-sync-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-async-function-prototype.c
  ecma/builtin-objects/ecma-builtin-async-function.c
  ecma/builtin-objects/ecma-builtin-async-generator-function.c
  ecma/builtin-objects/ecma-builtin-async-generator-prototype.c
  ecma/builtin-objects/ecma-builtin-async-generator.c
  ecma/builtin-objects/ecma-builtin-async-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-atomics.c
  ecma/builtin-objects/ecma-builtin-bigint-prototype.c
  ecma/builtin-objects/ecma-builtin-bigint.c
  ecma/builtin-objects/ecma-builtin-boolean-prototype.c
  ecma/builtin-objects/ecma-builtin-boolean.c
  ecma/builtin-objects/ecma-builtin-dataview-prototype.c
  ecma/builtin-objects/ecma-builtin-dataview.c
  ecma/builtin-objects/ecma-builtin-date-prototype.c
  ecma/builtin-objects/ecma-builtin-date.c
  ecma/builtin-objects/ecma-builtin-error-prototype.c
  ecma/builtin-objects/ecma-builtin-error.c
  ecma/builtin-objects/ecma-builtin-evalerror-prototype.c
  ecma/builtin-objects/ecma-builtin-evalerror.c
  ecma/builtin-objects/ecma-builtin-function-prototype.c
  ecma/builtin-objects/ecma-builtin-function.c
  ecma/builtin-objects/ecma-builtin-generator-function.c
  ecma/builtin-objects/ecma-builtin-generator-prototype.c
  ecma/builtin-objects/ecma-builtin-generator.c
  ecma/builtin-objects/ecma-builtin-global.c
  ecma/builtin-objects/ecma-builtin-handlers.c
  ecma/builtin-objects/ecma-builtin-helpers-date.c
  ecma/builtin-objects/ecma-builtin-helpers-error.c
  ecma/builtin-objects/ecma-builtin-helpers-json.c
  ecma/builtin-objects/ecma-builtin-helpers-sort.c
  ecma/builtin-objects/ecma-builtin-helpers.c
  ecma/builtin-objects/ecma-builtin-intrinsic.c
  ecma/builtin-objects/ecma-builtin-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-json.c
  ecma/builtin-objects/ecma-builtin-map-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-map-prototype.c
  ecma/builtin-objects/ecma-builtin-map.c
  ecma/builtin-objects/ecma-builtin-math.c
  ecma/builtin-objects/ecma-builtin-number-prototype.c
  ecma/builtin-objects/ecma-builtin-number.c
  ecma/builtin-objects/ecma-builtin-object-prototype.c
  ecma/builtin-objects/ecma-builtin-object.c
  ecma/builtin-objects/ecma-builtin-promise-prototype.c
  ecma/builtin-objects/ecma-builtin-promise.c
  ecma/builtin-objects/ecma-builtin-proxy.c
  ecma/builtin-objects/ecma-builtin-rangeerror-prototype.c
  ecma/builtin-objects/ecma-builtin-rangeerror.c
  ecma/builtin-objects/ecma-builtin-referenceerror-prototype.c
  ecma/builtin-objects/ecma-builtin-referenceerror.c
  ecma/builtin-objects/ecma-builtin-reflect.c
  ecma/builtin-objects/ecma-builtin-regexp-prototype.c
  ecma/builtin-objects/ecma-builtin-regexp-string-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-regexp.c
  ecma/builtin-objects/ecma-builtin-set-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-set-prototype.c
  ecma/builtin-objects/ecma-builtin-set.c
  ecma/builtin-objects/ecma-builtin-shared-arraybuffer-prototype.c
  ecma/builtin-objects/ecma-builtin-shared-arraybuffer.c
  ecma/builtin-objects/ecma-builtin-string-iterator-prototype.c
  ecma/builtin-objects/ecma-builtin-string-prototype.c
  ecma/builtin-objects/ecma-builtin-string.c
  ecma/builtin-objects/ecma-builtin-symbol-prototype.c
  ecma/builtin-objects/ecma-builtin-symbol.c
  ecma/builtin-objects/ecma-builtin-syntaxerror-prototype.c
  ecma/builtin-objects/ecma-builtin-syntaxerror.c
  ecma/builtin-objects/ecma-builtin-type-error-thrower.c
  ecma/builtin-objects/ecma-builtin-typeerror-prototype.c
  ecma/builtin-objects/ecma-builtin-typeerror.c
  ecma/builtin-objects/ecma-builtin-urierror-prototype.c
  ecma/builtin-objects/ecma-builtin-urierror.c
  ecma/builtin-objects/ecma-builtin-weakmap-prototype.c
  ecma/builtin-objects/ecma-builtin-weakmap.c
  ecma/builtin-objects/ecma-builtin-weakref-prototype.c
  ecma/builtin-objects/ecma-builtin-weakref.c
  ecma/builtin-objects/ecma-builtin-weakset-prototype.c
  ecma/builtin-objects/ecma-builtin-weakset.c
  ecma/builtin-objects/ecma-builtins.c
  ecma/builtin-objects/typedarray/ecma-builtin-bigint64array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-bigint64array.c
  ecma/builtin-objects/typedarray/ecma-builtin-biguint64array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-biguint64array.c
  ecma/builtin-objects/typedarray/ecma-builtin-float32array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-float32array.c
  ecma/builtin-objects/typedarray/ecma-builtin-float64array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-float64array.c
  ecma/builtin-objects/typedarray/ecma-builtin-int16array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-int16array.c
  ecma/builtin-objects/typedarray/ecma-builtin-int32array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-int32array.c
  ecma/builtin-objects/typedarray/ecma-builtin-int8array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-int8array.c
  ecma/builtin-objects/typedarray/ecma-builtin-typedarray-helpers.c
  ecma/builtin-objects/typedarray/ecma-builtin-typedarray-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-typedarray.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint16array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint16array.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint32array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint32array.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint8array-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint8array.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint8clampedarray-prototype.c
  ecma/builtin-objects/typedarray/ecma-builtin-uint8clampedarray.c
  ecma/operations/ecma-arguments-object.c
  ecma/operations/ecma-array-object.c
  ecma/operations/ecma-arraybuffer-object.c
  ecma/operations/ecma-async-generator-object.c
  ecma/operations/ecma-atomics-object.c
  ecma/operations/ecma-big-uint.c
  ecma/operations/ecma-bigint-object.c
  ecma/operations/ecma-bigint.c
  ecma/operations/ecma-boolean-object.c
  ecma/operations/ecma-comparison.c
  ecma/operations/ecma-container-object.c
  ecma/operations/ecma-conversion.c
  ecma/operations/ecma-dataview-object.c
  ecma/operations/ecma-eval.c
  ecma/operations/ecma-exceptions.c
  ecma/operations/ecma-function-object.c
  ecma/operations/ecma-get-put-value.c
  ecma/operations/ecma-iterator-object.c
  ecma/operations/ecma-jobqueue.c
  ecma/operations/ecma-lex-env.c
  ecma/operations/ecma-number-object.c
  ecma/operations/ecma-objects-general.c
  ecma/operations/ecma-objects.c
  ecma/operations/ecma-promise-object.c
  ecma/operations/ecma-proxy-object.c
  ecma/operations/ecma-reference.c
  ecma/operations/ecma-regexp-object.c
  ecma/operations/ecma-shared-arraybuffer-object.c
  ecma/operations/ecma-string-object.c
  ecma/operations/ecma-symbol-object.c
  ecma/operations/ecma-typedarray-object.c
  jcontext/jcontext.c
  jmem/jmem-allocator.c
  jmem/jmem-heap.c
  jmem/jmem-poolman.c
  jrt/jrt-fatals.c
  lit/lit-char-helpers.c
  lit/lit-magic-strings.c
  lit/lit-strings.c
  parser/js/byte-code.c
  parser/js/common.c
  parser/js/js-lexer.c
  parser/js/js-parser-expr.c
  parser/js/js-parser-line-info-create.c
  parser/js/js-parser-mem.c
  parser/js/js-parser-module.c
  parser/js/js-parser-statm.c
  parser/js/js-parser-tagged-template-literal.c
  parser/js/js-parser-util.c
  parser/js/js-parser.c
  parser/js/js-scanner-ops.c
  parser/js/js-scanner-util.c
  parser/js/js-scanner.c
  parser/js/parser-errors.c
  parser/regexp/re-bytecode.c
  parser/regexp/re-compiler.c
  parser/regexp/re-parser.c
  vm/opcodes-ecma-arithmetics.c
  vm/opcodes-ecma-bitwise.c
  vm/opcodes-ecma-relational-equality.c
  vm/opcodes.c
  vm/vm-stack.c
  vm/vm-utils.c
  vm/vm.c
)

# Amalgamated JJS source/header build.
#  The process will create the following files:
#   * jjs.c
#   * jjs.h
#   * jjs-config.h
if(JJS_AMALGAM)

  # Create single C/H file
  set(HEADER_CORE_FILES
    annex/annex.h
    api/jjs-annex.h
    api/jjs-annex-module-util.h
    api/jjs-annex-vmod.h
    api/jjs-api-object.h
    api/jjs-api-snapshot.h
    api/jjs-platform.h
    api/jjs-stream.h
    api/jjs-util.h
    debugger/debugger.h
    ecma/base/ecma-alloc.h
    ecma/base/ecma-error-messages.inc.h
    ecma/base/ecma-errors.h
    ecma/base/ecma-gc.h
    ecma/base/ecma-globals.h
    ecma/base/ecma-helpers.h
    ecma/base/ecma-init-finalize.h
    ecma/base/ecma-lcache.h
    ecma/base/ecma-line-info.h
    ecma/base/ecma-literal-storage.h
    ecma/base/ecma-module.h
    ecma/base/ecma-property-hashmap.h
    ecma/builtin-objects/ecma-builtin-aggregateerror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-aggregateerror.inc.h
    ecma/builtin-objects/ecma-builtin-array-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-array-prototype-unscopables.inc.h
    ecma/builtin-objects/ecma-builtin-array-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-array.inc.h
    ecma/builtin-objects/ecma-builtin-arraybuffer-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-arraybuffer.inc.h
    ecma/builtin-objects/ecma-builtin-async-from-sync-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-async-function-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-async-function.inc.h
    ecma/builtin-objects/ecma-builtin-async-generator-function.inc.h
    ecma/builtin-objects/ecma-builtin-async-generator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-async-generator.inc.h
    ecma/builtin-objects/ecma-builtin-async-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-atomics.inc.h
    ecma/builtin-objects/ecma-builtin-bigint-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-bigint.inc.h
    ecma/builtin-objects/ecma-builtin-boolean-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-boolean.inc.h
    ecma/builtin-objects/ecma-builtin-dataview-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-dataview.inc.h
    ecma/builtin-objects/ecma-builtin-date-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-date.inc.h
    ecma/builtin-objects/ecma-builtin-error-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-error.inc.h
    ecma/builtin-objects/ecma-builtin-evalerror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-evalerror.inc.h
    ecma/builtin-objects/ecma-builtin-function-prototype.h
    ecma/builtin-objects/ecma-builtin-function-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-function.inc.h
    ecma/builtin-objects/ecma-builtin-generator-function.inc.h
    ecma/builtin-objects/ecma-builtin-generator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-generator.inc.h
    ecma/builtin-objects/ecma-builtin-global.inc.h
    ecma/builtin-objects/ecma-builtin-handlers.h
    ecma/builtin-objects/ecma-builtin-handlers.inc.h
    ecma/builtin-objects/ecma-builtin-helpers-macro-defines.inc.h
    ecma/builtin-objects/ecma-builtin-helpers-macro-undefs.inc.h
    ecma/builtin-objects/ecma-builtin-helpers.h
    ecma/builtin-objects/ecma-builtin-internal-routines-template.inc.h
    ecma/builtin-objects/ecma-builtin-intrinsic.inc.h
    ecma/builtin-objects/ecma-builtin-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-json.inc.h
    ecma/builtin-objects/ecma-builtin-map-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-map-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-map.inc.h
    ecma/builtin-objects/ecma-builtin-math.inc.h
    ecma/builtin-objects/ecma-builtin-number-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-number.inc.h
    ecma/builtin-objects/ecma-builtin-object-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-object.h
    ecma/builtin-objects/ecma-builtin-object.inc.h
    ecma/builtin-objects/ecma-builtin-promise-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-promise.inc.h
    ecma/builtin-objects/ecma-builtin-proxy.inc.h
    ecma/builtin-objects/ecma-builtin-rangeerror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-rangeerror.inc.h
    ecma/builtin-objects/ecma-builtin-referenceerror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-referenceerror.inc.h
    ecma/builtin-objects/ecma-builtin-reflect.inc.h
    ecma/builtin-objects/ecma-builtin-regexp-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-regexp-string-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-regexp.inc.h
    ecma/builtin-objects/ecma-builtin-set-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-set-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-set.inc.h
    ecma/builtin-objects/ecma-builtin-shared-arraybuffer-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-shared-arraybuffer.inc.h
    ecma/builtin-objects/ecma-builtin-string-iterator-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-string-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-string.inc.h
    ecma/builtin-objects/ecma-builtin-symbol-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-symbol.inc.h
    ecma/builtin-objects/ecma-builtin-syntaxerror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-syntaxerror.inc.h
    ecma/builtin-objects/ecma-builtin-type-error-thrower.inc.h
    ecma/builtin-objects/ecma-builtin-typeerror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-typeerror.inc.h
    ecma/builtin-objects/ecma-builtin-urierror-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-urierror.inc.h
    ecma/builtin-objects/ecma-builtin-weakmap-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-weakmap.inc.h
    ecma/builtin-objects/ecma-builtin-weakref-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-weakref.inc.h
    ecma/builtin-objects/ecma-builtin-weakset-prototype.inc.h
    ecma/builtin-objects/ecma-builtin-weakset.inc.h
    ecma/builtin-objects/ecma-builtins-internal.h
    ecma/builtin-objects/ecma-builtins.h
    ecma/builtin-objects/ecma-builtins.inc.h
    ecma/operations/ecma-arguments-object.h
    ecma/operations/ecma-array-object.h
    ecma/operations/ecma-arraybuffer-object.h
    ecma/operations/ecma-async-generator-object.h
    ecma/operations/ecma-atomics-object.h
    ecma/operations/ecma-big-uint.h
    ecma/operations/ecma-bigint-object.h
    ecma/operations/ecma-bigint.h
    ecma/operations/ecma-boolean-object.h
    ecma/operations/ecma-comparison.h
    ecma/operations/ecma-container-object.h
    ecma/operations/ecma-conversion.h
    ecma/operations/ecma-dataview-object.h
    ecma/operations/ecma-eval.h
    ecma/operations/ecma-exceptions.h
    ecma/operations/ecma-function-object.h
    ecma/operations/ecma-iterator-object.h
    ecma/operations/ecma-jobqueue.h
    ecma/operations/ecma-lex-env.h
    ecma/operations/ecma-number-object.h
    ecma/operations/ecma-objects-general.h
    ecma/operations/ecma-objects.h
    ecma/operations/ecma-promise-object.h
    ecma/operations/ecma-proxy-object.h
    ecma/operations/ecma-reference.h
    ecma/operations/ecma-regexp-object.h
    ecma/operations/ecma-shared-arraybuffer-object.h
    ecma/operations/ecma-string-object.h
    ecma/operations/ecma-symbol-object.h
    ecma/operations/ecma-typedarray-object.h
    include/jjs-compiler.h
    include/jjs-context-init.h
    include/jjs-core.h
    include/jjs-debugger-transport.h
    include/jjs-debugger.h
    include/jjs-snapshot.h
    jcontext/jcontext.h
    jmem/jmem-allocator-internal.h
    jmem/jmem.h
    jrt/jrt-bit-fields.h
    jrt/jrt-libc-includes.h
    jrt/jrt-types.h
    jrt/jrt.h
    lit/lit-char-helpers.h
    lit/lit-globals.h
    lit/lit-magic-strings.h
    lit/lit-magic-strings.inc.h
    lit/lit-strings.h
    lit/lit-unicode-conversions-sup.inc.h
    lit/lit-unicode-conversions.inc.h
    lit/lit-unicode-folding.inc.h
    lit/lit-unicode-ranges-sup.inc.h
    lit/lit-unicode-ranges.inc.h
    vm/opcodes.h
    vm/vm-defines.h
    vm/vm-stack.h
    vm/vm.h
  )

  # Generated files
  set(AMALGAM_CORE_C "${CMAKE_BINARY_DIR}/amalgam/jjs.c")
  set(AMALGAM_CORE_H "${CMAKE_BINARY_DIR}/amalgam/jjs.h")
  set(AMALGAM_CONFIG_H "${CMAKE_BINARY_DIR}/amalgam/jjs-config.h")

  add_custom_command(OUTPUT ${AMALGAM_CORE_C} ${AMALGAM_CORE_H}
                     COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/amalgam.py
                             --jjs-core
                             --output-dir ${CMAKE_BINARY_DIR}/amalgam
                     DEPENDS ${SOURCE_CORE_FILES}
                             ${HEADER_CORE_FILES}
                             ${PROJECT_SOURCE_DIR}/tools/amalgam.py
  )

  # The "true" jjs-config.h will be generated by the configure_file below,
  # which contains the default options and the ones passed for the CMake.
  # The input for this is the jjs-config.h generated by the command above.
  set(JJS_GEN_CONFIG_H ${CMAKE_CURRENT_BINARY_DIR}/jjs-config.h)
  add_custom_command(OUTPUT ${AMALGAM_CONFIG_H}
                     COMMAND ${CMAKE_COMMAND} -E copy ${JJS_GEN_CONFIG_H} ${AMALGAM_CONFIG_H}
                     DEPENDS ${AMALGAM_CORE_C} ${AMALGAM_CORE_H})
  add_custom_target(amalgam-jjs DEPENDS ${AMALGAM_CORE_C} ${AMALGAM_CORE_H} ${AMALGAM_CONFIG_H})
  add_dependencies(amalgam amalgam-jjs)

  set(SOURCE_CORE_FILES ${AMALGAM_CORE_C} ${AMALGAM_CORE_H} ${AMALGAM_CONFIG_H})
  set(INCLUDE_CORE_PUBLIC PARENT_SCOPE)
  set(INCLUDE_CORE_PRIVATE PARENT_SCOPE)
endif()

# Third-party
# Valgrind
set(INCLUDE_THIRD_PARTY_VALGRIND "${PROJECT_SOURCE_DIR}/third-party/valgrind")

# build mode specific compile/link flags
set(DEFINES_JJS ${DEFINES_JJS} $<$<NOT:$<CONFIG:Debug>>:JJS_NDEBUG>)

# JJS heap-section
if(DEFINED JJS_ATTR_STATIC_HEAP)
  set(DEFINES_JJS ${DEFINES_JJS} JJS_ATTR_STATIC_HEAP=${JJS_ATTR_STATIC_HEAP})
endif()

# Helper macro to set 0/1 switch as JJS Defines
macro(jjs_add_define01 NAME)
  if(${NAME})
    set(DEFINES_JJS ${DEFINES_JJS} ${NAME}=1)
  else()
    set(DEFINES_JJS ${DEFINES_JJS} ${NAME}=0)
  endif()
endmacro(jjs_add_define01)

# Checks the optional features
# Enable 32 bit cpointers
jjs_add_define01(JJS_CPOINTER_32_BIT)

# Fill error messages for builtin error objects
jjs_add_define01(JJS_ERROR_MESSAGES)

# JS-Parser
jjs_add_define01(JJS_PARSER)

# JS function toString
jjs_add_define01(JJS_FUNCTION_TO_STRING)

# JS line info
jjs_add_define01(JJS_LINE_INFO)

# Logging
jjs_add_define01(JJS_LOGGING)

# Memory statistics
jjs_add_define01(JJS_MEM_STATS)

# Enable debugger
jjs_add_define01(JJS_DEBUGGER)

# Memory management stress-test mode
jjs_add_define01(JJS_MEM_GC_BEFORE_EACH_ALLOC)

# Parser byte-code dumps
jjs_add_define01(JJS_PARSER_DUMP_BYTE_CODE)

# Promise callback
jjs_add_define01(JJS_PROMISE_CALLBACK)

# RegExp strict mode
jjs_add_define01(JJS_REGEXP_STRICT_MODE)

# RegExp byte-code dumps
jjs_add_define01(JJS_REGEXP_DUMP_BYTE_CODE)

# Snapshot exec
jjs_add_define01(JJS_SNAPSHOT_EXEC)

# Snapshot save
jjs_add_define01(JJS_SNAPSHOT_SAVE)

# Valgrind
jjs_add_define01(JJS_VALGRIND)
if(JJS_VALGRIND)
  set(INCLUDE_CORE_PRIVATE ${INCLUDE_CORE_PRIVATE} ${INCLUDE_THIRD_PARTY_VALGRIND})
endif()

# Enable VM execution stop callback
jjs_add_define01(JJS_VM_HALT)

# Enable VM throw callback
jjs_add_define01(JJS_VM_THROW)

# Enable VM static stack usage checks flag
jjs_add_define01(JJS_VM_STACK_LIMIT)

# Default scratch allocator size
set(DEFINES_JJS ${DEFINES_JJS} JJS_DEFAULT_SCRATCH_ARENA_KB=${JJS_DEFAULT_SCRATCH_ARENA_KB})

# Default vm heap size
set(DEFINES_JJS ${DEFINES_JJS} JJS_DEFAULT_VM_HEAP_SIZE_KB=${JJS_DEFAULT_VM_HEAP_SIZE_KB})

# Default vm stack limit size
set(DEFINES_JJS ${DEFINES_JJS} JJS_DEFAULT_VM_STACK_LIMIT_KB=${JJS_DEFAULT_VM_STACK_LIMIT_KB})

# platform apis
jjs_add_define01(JJS_PLATFORM_API_FS_READ_FILE)
jjs_add_define01(JJS_PLATFORM_API_IO_WRITE)
jjs_add_define01(JJS_PLATFORM_API_IO_FLUSH)
jjs_add_define01(JJS_PLATFORM_API_PATH_CWD)
jjs_add_define01(JJS_PLATFORM_API_PATH_REALPATH)
jjs_add_define01(JJS_PLATFORM_API_TIME_LOCAL_TZA)
jjs_add_define01(JJS_PLATFORM_API_TIME_NOW_MS)
jjs_add_define01(JJS_PLATFORM_API_TIME_SLEEP)

# localtime_r is is not threadsafe with clang on OSX
if(HAVE_TM_GMTOFF AND NOT "${PLATFORM}" STREQUAL "DARWIN")
  set(DEFINES_JJS ${DEFINES_JJS} HAVE_TM_GMTOFF)
endif()

set(DEFINES_JJS ${DEFINES_JJS} _BSD_SOURCE=)
set(DEFINES_JJS ${DEFINES_JJS} _DEFAULT_SOURCE=)

# Export defines to parent scope (for other modules, like jjs-pack)
set(DEFINES_JJS_CORE ${DEFINES_JJS} PARENT_SCOPE)

## This function is to read "config.h" for default values
function(read_set_defines FILE PREFIX OUTPUTVAR)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}" INPUT_FILE_CONTENTS)

  # match all "#define <PREFIX>\n" lines
  # notes:
  #  * before the "#" there must be a newline and any number of spaces.
  #  * after the "#" there can be any number of spaces.
  string(REGEX MATCHALL "\r?\n[ ]*#[ ]*define ${PREFIX}[^\n]*"
         RAW_DEFINES "${INPUT_FILE_CONTENTS}")

  set(SELECTED_VARS )

  # Transform the defines to a list of (<name>; <value>; <name 2>; <value 2>; ...) list
  foreach(DEFINE_ENTRY ${RAW_DEFINES})
    # by default every define value is empty
    set(DEFINE_VALUE " ")

    # split up the define at the space between the define name and value (if there is any)

    # first remove "#define" part of the string
    string(REGEX REPLACE "\r?\n[ ]*#[ ]*define[ ]+" "" DEFINE_KEY_VALUE "${DEFINE_ENTRY}")
    string(FIND "${DEFINE_KEY_VALUE}" " " DEFINE_KEY_IDX)
    string(LENGTH "${DEFINE_KEY_VALUE}" DEFINE_LENGTH)

    if (DEFINE_KEY_IDX EQUAL "-1")
        set(DEFINE_KEY ${DEFINE_KEY_VALUE})
    else()
        string(SUBSTRING "${DEFINE_KEY_VALUE}" 0 ${DEFINE_KEY_IDX} DEFINE_KEY)
        string(SUBSTRING "${DEFINE_KEY_VALUE}" ${DEFINE_KEY_IDX} -1 DEFINE_VALUE)
        string(STRIP "${DEFINE_VALUE}" DEFINE_VALUE)
    endif()

    list(APPEND SELECTED_VARS ${DEFINE_KEY} ${DEFINE_VALUE})
  endforeach()

  set(${OUTPUTVAR} ${SELECTED_VARS} PARENT_SCOPE)
endfunction(read_set_defines)

# CONFIG_DEFAULTS contains define name and values which have the JJS_ prefix
# as a list of (<name>; <value>; <name 2>; <value 2>; ...)
read_set_defines("config.h" JJS_ CONFIG_DEFAULTS)


## Process the default values and build options to generate build config defines
list(LENGTH CONFIG_DEFAULTS CONFIG_DEFAULT_LENGTH)
math(EXPR CONFIG_DEFAULT_LENGTH "${CONFIG_DEFAULT_LENGTH} - 1")

set(JJS_MODIFIED_OPTIONS)
foreach(CONFIG_IDX RANGE 0 ${CONFIG_DEFAULT_LENGTH} 2)
  list(GET CONFIG_DEFAULTS ${CONFIG_IDX} KEY)
  math(EXPR VALUE_IDX "${CONFIG_IDX} + 1")
  list(GET CONFIG_DEFAULTS ${VALUE_IDX} VALUE)

  # ${KEY} is the value for the given variable (aka define)
  # normalize ON/OFF cmake values to 1/0 for easier processing.
  if(${KEY} STREQUAL "ON")
    set(${KEY} 1)
  elseif(${KEY} STREQUAL "OFF")
    set(${KEY} 0)
  endif()

  # Generate "#define JJS_<CONFIG> <CONFIG_VALUE>" entries if it is different from
  # the config default.

  # If the define loaded from the config file have a different value than the
  # relevant option passed for the CMake means that it does not have a default value.
  if(DEFINED ${KEY} AND NOT (${KEY} STREQUAL ${VALUE}))
    set(JJS_MODIFIED_OPTIONS "${JJS_MODIFIED_OPTIONS}#define ${KEY} ${${KEY}}\n")
  endif()
endforeach()

# Generate the jjs-config.h file into the build directory
# This file will contain the options different from the default (aka it's the build config).
if(JJS_MODIFIED_OPTIONS)
  set(JJS_BUILD_CFG
      "Generated differences from default by CMake based on build options:\n${JJS_MODIFIED_OPTIONS}")
else()
  set(JJS_BUILD_CFG "JJS configuration")
endif()
configure_file(config.h jjs-config.h @ONLY)

add_library(${JJS_CORE_NAME} ${SOURCE_CORE_FILES})

target_compile_definitions(${JJS_CORE_NAME} PUBLIC ${DEFINES_JJS})
target_include_directories(${JJS_CORE_NAME} PUBLIC ${INCLUDE_CORE_PUBLIC})
target_include_directories(${JJS_CORE_NAME} PRIVATE ${INCLUDE_CORE_PRIVATE})
add_dependencies(${JJS_CORE_NAME} amalgam)

set(JJS_CORE_PKGCONFIG_REQUIRES)
set(JJS_CORE_PKGCONFIG_LIBS)
set(JJS_CORE_PKGCONFIG_CFLAGS)

if(JJS_LTO)
  set(JJS_CORE_PKGCONFIG_CFLAGS "${JJS_CORE_PKGCONFIG_CFLAGS} -flto")
endif()

if(JJS_MATH)
  target_link_libraries(${JJS_CORE_NAME} jjs-math)
  set(JJS_CORE_PKGCONFIG_REQUIRES libjjs-math)
else()
  if(USING_GCC OR USING_CLANG)
    set(CMAKE_REQUIRED_FLAGS "-Wno-error -Wno-error=strict-prototypes")
  endif()
  check_library_exists(m sin "" HAVE_M_LIB)
  if(HAVE_M_LIB)
    target_link_libraries(${JJS_CORE_NAME} m)
    set(JJS_CORE_PKGCONFIG_LIBS "${JJS_CORE_PKGCONFIG_LIBS} -lm")
  endif()
endif()

separate_arguments(JJS_EXTERNAL_LINK_LIBS)
foreach(EXT_LIB ${JJS_EXTERNAL_LINK_LIBS})
  target_link_libraries(${JJS_CORE_NAME} ${EXT_LIB})
  set(JJS_CORE_PKGCONFIG_LIBS "${JJS_CORE_PKGCONFIG_LIBS} -l${EXT_LIB}")
endforeach()

configure_file(libjjs-core.pc.in libjjs-core.pc @ONLY)

install(TARGETS ${JJS_CORE_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjjs-core.pc DESTINATION lib/pkgconfig)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jjs-config.h DESTINATION include)
install(DIRECTORY ${INCLUDE_CORE_PUBLIC}/ DESTINATION include)
