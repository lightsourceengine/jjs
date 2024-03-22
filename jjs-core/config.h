/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JJS_CONFIG_H
#define JJS_CONFIG_H

// @JJS_BUILD_CFG@

/**
 * Built-in configurations
 *
 * Allowed values for built-in defines:
 *  0: Disable the given built-in.
 *  1: Enable the given built-in.
 */
/*
 * By default, all built-ins are enabled if they are not defined.
 */
#ifndef JJS_BUILTINS
#define JJS_BUILTINS 1
#endif /* !defined (JJS_BUILTINS) */

#ifndef JJS_BUILTIN_ANNEXB
#define JJS_BUILTIN_ANNEXB JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_ANNEXB) */

#ifndef JJS_BUILTIN_ARRAY
#define JJS_BUILTIN_ARRAY JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_ARRAY) */

#ifndef JJS_BUILTIN_BOOLEAN
#define JJS_BUILTIN_BOOLEAN JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_BOOLEAN) */

#ifndef JJS_BUILTIN_DATE
#define JJS_BUILTIN_DATE JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_DATE) */

#ifndef JJS_BUILTIN_ERRORS
#define JJS_BUILTIN_ERRORS JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_ERRORS) */

#ifndef JJS_BUILTIN_JSON
#define JJS_BUILTIN_JSON JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_JSON) */

#ifndef JJS_BUILTIN_MATH
#define JJS_BUILTIN_MATH JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_MATH) */

#ifndef JJS_BUILTIN_NUMBER
#define JJS_BUILTIN_NUMBER JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_NUMBER) */

#ifndef JJS_BUILTIN_REGEXP
#define JJS_BUILTIN_REGEXP JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_REGEXP) */

#ifndef JJS_BUILTIN_STRING
#define JJS_BUILTIN_STRING JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_STRING) */

#ifndef JJS_BUILTIN_BIGINT
#define JJS_BUILTIN_BIGINT JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_BIGINT) */

#ifndef JJS_BUILTIN_CONTAINER
#define JJS_BUILTIN_CONTAINER JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_CONTAINER) */

#ifndef JJS_BUILTIN_DATAVIEW
#define JJS_BUILTIN_DATAVIEW JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_DATAVIEW) */

#ifndef JJS_BUILTIN_GLOBAL_THIS
#define JJS_BUILTIN_GLOBAL_THIS JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_GLOBAL_THIS) */

#ifndef JJS_BUILTIN_PROXY
#define JJS_BUILTIN_PROXY JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_PROXY) */

#ifndef JJS_BUILTIN_REALMS
#define JJS_BUILTIN_REALMS JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_REALMS) */

#ifndef JJS_BUILTIN_REFLECT
#define JJS_BUILTIN_REFLECT JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_REFLECT) */

#ifndef JJS_BUILTIN_TYPEDARRAY
#define JJS_BUILTIN_TYPEDARRAY JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_TYPEDARRAY) */

#ifndef JJS_BUILTIN_SHAREDARRAYBUFFER
#define JJS_BUILTIN_SHAREDARRAYBUFFER JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_SHAREDARRAYBUFFER) */

#ifndef JJS_BUILTIN_ATOMICS
#define JJS_BUILTIN_ATOMICS JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_ATOMICS) */

#ifndef JJS_BUILTIN_WEAKREF
#define JJS_BUILTIN_WEAKREF JJS_BUILTINS
#endif /* !defined (JJS_BUILTIN_WEAKREF) */

#ifndef JJS_MODULE_SYSTEM
#define JJS_MODULE_SYSTEM JJS_BUILTINS
#endif /* !defined (JJS_MODULE_SYSTEM) */

/*
 * By default, all annex built-ins are enabled if they are not defined.
 */
#ifndef JJS_ANNEX
#define JJS_ANNEX 1
#endif /* !defined (JJS_ANNEX) */

#ifndef JJS_ANNEX_QUEUE_MICROTASK
#define JJS_ANNEX_QUEUE_MICROTASK JJS_ANNEX
#endif /* !defined (JJS_ANNEX_QUEUE_MICROTASK) */

#ifndef JJS_ANNEX_COMMONJS
#define JJS_ANNEX_COMMONJS JJS_ANNEX
#endif /* !defined (JJS_ANNEX_COMMONJS) */

#ifndef JJS_ANNEX_ESM
#define JJS_ANNEX_ESM JJS_ANNEX
#endif /* !defined (JJS_ANNEX_ESM) */

#ifndef JJS_ANNEX_PMAP
#define JJS_ANNEX_PMAP JJS_ANNEX
#endif /* !defined (JJS_ANNEX_PMAP) */

#ifndef JJS_ANNEX_VMOD
#define JJS_ANNEX_VMOD JJS_ANNEX
#endif /* !defined (JJS_ANNEX_VMOD) */

/**
 * Engine internal and misc configurations.
 */

/**
 * Specifies the compressed pointer representation
 *
 * Allowed values:
 *  0: use 16 bit representation
 *  1: use 32 bit representation
 *
 * Default value: 1
 * For more details see: jmem/jmem.h
 */
#ifndef JJS_CPOINTER_32_BIT
#define JJS_CPOINTER_32_BIT 1
#endif /* !defined (JJS_CPOINTER_32_BIT) */

/**
 * Enable/Disable the engine's JavaScript debugger interface
 *
 * Allowed values:
 *  0: Disable the debugger parts.
 *  1: Enable the debugger.
 */
#ifndef JJS_DEBUGGER
#define JJS_DEBUGGER 0
#endif /* !defined (JJS_DEBUGGER) */

/**
 * Enable/Disable built-in error messages for error objects.
 *
 * Allowed values:
 *  0: Disable error messages.
 *  1: Enable error message.
 *
 * Default value: 1
 */
#ifndef JJS_ERROR_MESSAGES
#define JJS_ERROR_MESSAGES 1
#endif /* !defined (JJS_ERROR_MESSAGES) */

/**
 * Enable/Disable external context.
 *
 * Allowed values:
 *  0: Disable external context.
 *  1: Enable external context support.
 *
 * Default value: 0
 */
#ifndef JJS_EXTERNAL_CONTEXT
#define JJS_EXTERNAL_CONTEXT 0
#endif /* !defined (JJS_EXTERNAL_CONTEXT) */

/**
 * Maximum size of heap in kilobytes
 *
 * Default value: 1024 KiB
 */
#ifndef JJS_GLOBAL_HEAP_SIZE
#define JJS_GLOBAL_HEAP_SIZE (1024)
#endif /* !defined (JJS_GLOBAL_HEAP_SIZE) */

/**
 * The allowed heap usage limit until next garbage collection, in bytes.
 *
 * If value is 0, the default is 1/32 of JJS_HEAP_SIZE
 */
#ifndef JJS_GC_LIMIT
#define JJS_GC_LIMIT (0)
#endif /* !defined (JJS_GC_LIMIT) */

/**
 * Maximum stack usage size in kilobytes
 *
 * Note: This feature cannot be used when 'detect_stack_use_after_return=1' ASAN option is enabled.
 * For more detailed description:
 *   - https://github.com/google/sanitizers/wiki/AddressSanitizerUseAfterReturn#compatibility
 *
 * Default value: 0, unlimited
 */
#ifndef JJS_STACK_LIMIT
#define JJS_STACK_LIMIT (0)
#endif /* !defined (JJS_STACK_LIMIT) */

/**
 * Maximum depth of recursion during GC mark phase
 *
 * Default value: 8
 */
#ifndef JJS_GC_MARK_LIMIT
#define JJS_GC_MARK_LIMIT (8)
#endif /* !defined (JJS_GC_MARK_LIMIT) */

/**
 * Enable/Disable property lookup cache.
 *
 * Allowed values:
 *  0: Disable lookup cache.
 *  1: Enable lookup cache.
 *
 * Default value: 1
 */
#ifndef JJS_LCACHE
#define JJS_LCACHE 1
#endif /* !defined (JJS_LCACHE) */

/**
 * Enable/Disable function toString operation.
 *
 * Allowed values:
 *  0: Disable function toString operation.
 *  1: Enable function toString operation.
 *
 * Default value: 0
 */
#ifndef JJS_FUNCTION_TO_STRING
#define JJS_FUNCTION_TO_STRING 0
#endif /* !defined (JJS_FUNCTION_TO_STRING) */

/**
 * Enable/Disable line-info management inside the engine.
 *
 * Allowed values:
 *  0: Disable line-info in the engine.
 *  1: Enable line-info management.
 *
 * Default value: 1
 */
#ifndef JJS_LINE_INFO
#define JJS_LINE_INFO 1
#endif /* !defined (JJS_LINE_INFO) */

/**
 * Enable/Disable logging inside the engine.
 *
 * Allowed values:
 *  0: Disable internal logging.
 *  1: Enable internal logging.
 *
 * Default value: 0
 */
#ifndef JJS_LOGGING
#define JJS_LOGGING 0
#endif /* !defined (JJS_LOGGING) */

/**
 * Enable/Disable gc call before every allocation.
 *
 * Allowed values:
 *  0: Disable gc call before each allocation.
 *  1: Enable and force gc call before each allocation.
 *
 * Default value: 0
 * Warning!: This is an advanced option and will slow down the engine!
 *           Only enable it for debugging purposes.
 */
#ifndef JJS_MEM_GC_BEFORE_EACH_ALLOC
#define JJS_MEM_GC_BEFORE_EACH_ALLOC 0
#endif /* !defined (JJS_MEM_GC_BEFORE_EACH_ALLOC) */

/**
 * Enable/Disable the collection if run-time memory statistics.
 *
 * Allowed values:
 *  0: Disable run-time memory information collection.
 *  1: Enable run-time memory statistics collection.
 *
 * Default value: 0
 */
#ifndef JJS_MEM_STATS
#define JJS_MEM_STATS 0
#endif /* !defined (JJS_MEM_STATS) */

/**
 * Use 32-bit/64-bit float for ecma-numbers
 * This option is for expert use only!
 *
 * Allowed values:
 *  1: use 64-bit floating point number mode
 *  0: use 32-bit floating point number mode
 *
 * Default value: 1
 */
#ifndef JJS_NUMBER_TYPE_FLOAT64
#define JJS_NUMBER_TYPE_FLOAT64 1
#endif /* !defined (JJS_NUMBER_TYPE_FLOAT64 */

/**
 * Enable/Disable the JavaScript parser.
 *
 * Allowed values:
 *  0: Disable the JavaScript parser and all related functionallity.
 *  1: Enable the JavaScript parser.
 *
 * Default value: 1
 */
#ifndef JJS_PARSER
#define JJS_PARSER 1
#endif /* !defined (JJS_PARSER) */

/**
 * Enable/Disable JJS byte code dump functions during parsing.
 * To dump the JJS byte code the engine must be initialized with opcodes
 * display flag. This option does not influence RegExp byte code dumps.
 *
 * Allowed values:
 *  0: Disable all bytecode dump functions.
 *  1: Enable bytecode dump functions.
 *
 * Default value: 0
 */
#ifndef JJS_PARSER_DUMP_BYTE_CODE
#define JJS_PARSER_DUMP_BYTE_CODE 0
#endif /* defined (JJS_PARSER_DUMP_BYTE_CODE) */

/**
 * Enable/Disable ECMA property hashmap.
 *
 * Allowed values:
 *  0: Disable property hasmap.
 *  1: Enable property hashmap.
 *
 * Default value: 1
 */
#ifndef JJS_PROPERTY_HASHMAP
#define JJS_PROPERTY_HASHMAP 1
#endif /* !defined (JJS_PROPERTY_HASHMAP) */

/**
 * Enables/disables the Promise event callbacks
 *
 * Default value: 0
 */
#ifndef JJS_PROMISE_CALLBACK
#define JJS_PROMISE_CALLBACK 0
#endif /* !defined (JJS_PROMISE_CALLBACK) */

/**
 * Enable/Disable byte code dump functions for RegExp objects.
 * To dump the RegExp byte code the engine must be initialized with
 * regexp opcodes display flag. This option does not influence the
 * JJS byte code dumps.
 *
 * Allowed values:
 *  0: Disable all bytecode dump functions.
 *  1: Enable bytecode dump functions.
 *
 * Default value: 0
 */
#ifndef JJS_REGEXP_DUMP_BYTE_CODE
#define JJS_REGEXP_DUMP_BYTE_CODE 0
#endif /* !defined (JJS_REGEXP_DUMP_BYTE_CODE) */

/**
 * Enables/disables the RegExp strict mode
 *
 * Default value: 0
 */
#ifndef JJS_REGEXP_STRICT_MODE
#define JJS_REGEXP_STRICT_MODE 0
#endif /* !defined (JJS_REGEXP_STRICT_MODE) */

/**
 * Enable/Disable the snapshot execution functions.
 *
 * Allowed values:
 *  0: Disable snapshot execution.
 *  1: Enable snapshot execution.
 *
 * Default value: 1
 */
#ifndef JJS_SNAPSHOT_EXEC
#define JJS_SNAPSHOT_EXEC 1
#endif /* !defined (JJS_SNAPSHOT_EXEC) */

/**
 * Enable/Disable the snapshot save functions.
 *
 * Allowed values:
 *  0: Disable snapshot save functions.
 *  1: Enable snapshot save functions.
 */
#ifndef JJS_SNAPSHOT_SAVE
#define JJS_SNAPSHOT_SAVE 0
#endif /* !defined (JJS_SNAPSHOT_SAVE) */

/**
 * Enable/Disable usage of system allocator.
 *
 * Allowed values:
 *  0: Disable usage of system allocator.
 *  1: Enable usage of system allocator.
 *
 * Default value: 0
 */
#ifndef JJS_SYSTEM_ALLOCATOR
#define JJS_SYSTEM_ALLOCATOR 0
#endif /* !defined (JJS_SYSTEM_ALLOCATOR) */

/**
 * Enables/disables the unicode case conversion in the engine.
 * By default Unicode case conversion is enabled.
 */
#ifndef JJS_UNICODE_CASE_CONVERSION
#define JJS_UNICODE_CASE_CONVERSION 1
#endif /* !defined (JJS_UNICODE_CASE_CONVERSION) */

/**
 * Configures if the internal memory allocations are exposed to Valgrind or not.
 *
 * Allowed values:
 *  0: Disable the Valgrind specific memory allocation notifications.
 *  1: Enable the Valgrind specific allocation notifications.
 */
#ifndef JJS_VALGRIND
#define JJS_VALGRIND 0
#endif /* !defined (JJS_VALGRIND) */

/**
 * Enable/Disable the vm execution stop callback function.
 *
 * Allowed values:
 *  0: Disable vm exec stop callback support.
 *  1: Enable vm exec stop callback support.
 */
#ifndef JJS_VM_HALT
#define JJS_VM_HALT 0
#endif /* !defined (JJS_VM_HALT) */

/**
 * Enable/Disable the vm throw callback function.
 *
 * Allowed values:
 *  0: Disable vm throw callback support.
 *  1: Enable vm throw callback support.
 */
#ifndef JJS_VM_THROW
#define JJS_VM_THROW 0
#endif /* !defined (JJS_VM_THROW) */

/**
 * Advanced section configurations.
 */

/**
 * Allow configuring attributes on a few constant data inside the engine.
 *
 * One of the main usages:
 * Normally compilers store const(ant)s in ROM. Thus saving RAM.
 * But if your compiler does not support it then the directive below can force it.
 *
 * For the moment it is mainly meant for the following targets:
 *      - ESP8266
 *
 * Example configuration for moving (some) constatns into a given section:
 *  # define JJS_ATTR_CONST_DATA __attribute__((section(".rodata.const")))
 */
#ifndef JJS_ATTR_CONST_DATA
#define JJS_ATTR_CONST_DATA
#endif /* !defined (JJS_ATTR_CONST_DATA) */

/**
 * The JJS_ATTR_GLOBAL_HEAP allows adding extra attributes for the JJS global heap.
 *
 * Example on how to move the global heap into it's own section:
 *   #define JJS_ATTR_GLOBAL_HEAP __attribute__((section(".text.globalheap")))
 */
#ifndef JJS_ATTR_GLOBAL_HEAP
#define JJS_ATTR_GLOBAL_HEAP
#endif /* !defined (JJS_ATTR_GLOBAL_HEAP) */

/**
 * Sanity check for macros to see if the values are 0 or 1
 *
 * If a new feature is added this should be updated.
 */
/**
 * Check base builtins.
 */
#if (JJS_BUILTIN_ANNEXB != 0) && (JJS_BUILTIN_ANNEXB != 1)
#error "Invalid value for JJS_BUILTIN_ANNEXB macro."
#endif /* (JJS_BUILTIN_ANNEXB != 0) && (JJS_BUILTIN_ANNEXB != 1) */
#if (JJS_BUILTIN_ARRAY != 0) && (JJS_BUILTIN_ARRAY != 1)
#error "Invalid value for JJS_BUILTIN_ARRAY macro."
#endif /* (JJS_BUILTIN_ARRAY != 0) && (JJS_BUILTIN_ARRAY != 1) */
#if (JJS_BUILTIN_BOOLEAN != 0) && (JJS_BUILTIN_BOOLEAN != 1)
#error "Invalid value for JJS_BUILTIN_BOOLEAN macro."
#endif /* (JJS_BUILTIN_BOOLEAN != 0) && (JJS_BUILTIN_BOOLEAN != 1) */
#if (JJS_BUILTIN_DATE != 0) && (JJS_BUILTIN_DATE != 1)
#error "Invalid value for JJS_BUILTIN_DATE macro."
#endif /* (JJS_BUILTIN_DATE != 0) && (JJS_BUILTIN_DATE != 1) */
#if (JJS_BUILTIN_ERRORS != 0) && (JJS_BUILTIN_ERRORS != 1)
#error "Invalid value for JJS_BUILTIN_ERRORS macro."
#endif /* (JJS_BUILTIN_ERRORS != 0) && (JJS_BUILTIN_ERRORS != 1) */
#if (JJS_BUILTIN_JSON != 0) && (JJS_BUILTIN_JSON != 1)
#error "Invalid value for JJS_BUILTIN_JSON macro."
#endif /* (JJS_BUILTIN_JSON != 0) && (JJS_BUILTIN_JSON != 1) */
#if (JJS_BUILTIN_MATH != 0) && (JJS_BUILTIN_MATH != 1)
#error "Invalid value for JJS_BUILTIN_MATH macro."
#endif /* (JJS_BUILTIN_MATH != 0) && (JJS_BUILTIN_MATH != 1) */
#if (JJS_BUILTIN_NUMBER != 0) && (JJS_BUILTIN_NUMBER != 1)
#error "Invalid value for JJS_BUILTIN_NUMBER macro."
#endif /* (JJS_BUILTIN_NUMBER != 0) && (JJS_BUILTIN_NUMBER != 1) */
#if (JJS_BUILTIN_REGEXP != 0) && (JJS_BUILTIN_REGEXP != 1)
#error "Invalid value for JJS_BUILTIN_REGEXP macro."
#endif /* (JJS_BUILTIN_REGEXP != 0) && (JJS_BUILTIN_REGEXP != 1) */
#if (JJS_BUILTIN_STRING != 0) && (JJS_BUILTIN_STRING != 1)
#error "Invalid value for JJS_BUILTIN_STRING macro."
#endif /* (JJS_BUILTIN_STRING != 0) && (JJS_BUILTIN_STRING != 1) */
#if (JJS_BUILTINS != 0) && (JJS_BUILTINS != 1)
#error "Invalid value for JJS_BUILTINS macro."
#endif /* (JJS_BUILTINS != 0) && (JJS_BUILTINS != 1) */
#if (JJS_BUILTIN_REALMS != 0) && (JJS_BUILTIN_REALMS != 1)
#error "Invalid value for JJS_BUILTIN_REALMS macro."
#endif /* (JJS_BUILTIN_REALMS != 0) && (JJS_BUILTIN_REALMS != 1) */
#if (JJS_BUILTIN_DATAVIEW != 0) && (JJS_BUILTIN_DATAVIEW != 1)
#error "Invalid value for JJS_BUILTIN_DATAVIEW macro."
#endif /* (JJS_BUILTIN_DATAVIEW != 0) && (JJS_BUILTIN_DATAVIEW != 1) */
#if (JJS_BUILTIN_GLOBAL_THIS != 0) && (JJS_BUILTIN_GLOBAL_THIS != 1)
#error "Invalid value for JJS_BUILTIN_GLOBAL_THIS macro."
#endif /* (JJS_BUILTIN_GLOBAL_THIS != 0) && (JJS_BUILTIN_GLOBAL_THIS != 1) */
#if (JJS_BUILTIN_REFLECT != 0) && (JJS_BUILTIN_REFLECT != 1)
#error "Invalid value for JJS_BUILTIN_REFLECT macro."
#endif /* (JJS_BUILTIN_REFLECT != 0) && (JJS_BUILTIN_REFLECT != 1) */
#if (JJS_BUILTIN_WEAKREF != 0) && (JJS_BUILTIN_WEAKREF != 1)
#error "Invalid value for JJS_BUILTIN_WEAKREF macro."
#endif /* (JJS_BUILTIN_WEAKREF != 0) && (JJS_BUILTIN_WEAKREF != 1) */
#if (JJS_BUILTIN_PROXY != 0) && (JJS_BUILTIN_PROXY != 1)
#error "Invalid value for JJS_BUILTIN_PROXY macro."
#endif /* (JJS_BUILTIN_PROXY != 0) && (JJS_BUILTIN_PROXY != 1) */
#if (JJS_BUILTIN_TYPEDARRAY != 0) && (JJS_BUILTIN_TYPEDARRAY != 1)
#error "Invalid value for JJS_BUILTIN_TYPEDARRAY macro."
#endif /* (JJS_BUILTIN_TYPEDARRAY != 0) && (JJS_BUILTIN_TYPEDARRAY != 1) */
#if (JJS_BUILTIN_SHAREDARRAYBUFFER != 0) && (JJS_BUILTIN_SHAREDARRAYBUFFER != 1)
#error "Invalid value for JJS_BUILTIN_SHAREDARRAYBUFFER macro."
#endif /* (JJS_BUILTIN_SHAREDARRAYBUFFER != 0) && (JJS_BUILTIN_SHAREDARRAYBUFFER != 1) */
#if (JJS_BUILTIN_ATOMICS != 0) && (JJS_BUILTIN_ATOMICS != 1)
#error "Invalid value for JJS_BUILTIN_ATOMICS macro."
#endif /* (JJS_BUILTIN_ATOMICS != 0) && (JJS_BUILTIN_ATOMICS != 1) */
#if (JJS_BUILTIN_BIGINT != 0) && (JJS_BUILTIN_BIGINT != 1)
#error "Invalid value for JJS_BUILTIN_BIGINT macro."
#endif /* (JJS_BUILTIN_BIGINT != 0) && (JJS_BUILTIN_BIGINT != 1) */
#if (JJS_MODULE_SYSTEM != 0) && (JJS_MODULE_SYSTEM != 1)
#error "Invalid value for JJS_MODULE_SYSTEM macro."
#endif /* (JJS_MODULE_SYSTEM != 0) && (JJS_MODULE_SYSTEM != 1) */
#if (JJS_ANNEX_QUEUE_MICROTASK != 0) && (JJS_ANNEX_QUEUE_MICROTASK != 1)
#error "Invalid value for JJS_ANNEX_QUEUE_MICROTASK macro."
#endif /* (JJS_ANNEX_QUEUE_MICROTASK != 0) && (JJS_ANNEX_QUEUE_MICROTASK != 1) */
#if (JJS_ANNEX_COMMONJS != 0) && (JJS_ANNEX_COMMONJS != 1)
#error "Invalid value for JJS_ANNEX_COMMONJS macro."
#endif /* (JJS_ANNEX_COMMONJS != 0) && (JJS_ANNEX_COMMONJS != 1) */
#if (JJS_ANNEX_ESM != 0) && (JJS_ANNEX_ESM != 1)
#error "Invalid value for JJS_ANNEX_ESM macro."
#endif /* (JJS_ANNEX_ESM != 0) && (JJS_ANNEX_ESM != 1) */
#if (JJS_ANNEX_PMAP != 0) && (JJS_ANNEX_PMAP != 1)
#error "Invalid value for JJS_ANNEX_PMAP macro."
#endif /* (JJS_ANNEX_PMAP != 0) && (JJS_ANNEX_PMAP != 1) */
#if (JJS_ANNEX_VMOD != 0) && (JJS_ANNEX_VMOD != 1)
#error "Invalid value for JJS_ANNEX_VMOD macro."
#endif /* (JJS_ANNEX_VMOD != 0) && (JJS_ANNEX_VMOD != 1) */
#if (JJS_BUILTIN_TYPEDARRAY == 0) && (JJS_BUILTIN_SHAREDARRAYBUFFER == 1)
#error "JJS_BUILTIN_TYPEDARRAY should be enabled too to enable JJS_BUILTIN_SHAREDARRAYBUFFER macro."
#endif /* (JJS_BUILTIN_TYPEDARRAY == 0) && (JJS_BUILTIN_SHAREDARRAYBUFFER == 1) */
#if (JJS_BUILTIN_SHAREDARRAYBUFFER == 0) && (JJS_BUILTIN_ATOMICS == 1)
#error "JJS_BUILTIN_SHAREDARRAYBUFFER should be enabled too to enable JJS_BUILTIN_ATOMICS macro."
#endif /* (JJS_BUILTIN_SHAREDARRAYBUFFER == 0) && (JJS_BUILTIN_ATOMICS == 1) */

/**
 * Internal options.
 */
#if (JJS_CPOINTER_32_BIT != 0) && (JJS_CPOINTER_32_BIT != 1)
#error "Invalid value for 'JJS_CPOINTER_32_BIT' macro."
#endif /* (JJS_CPOINTER_32_BIT != 0) && (JJS_CPOINTER_32_BIT != 1) */
#if (JJS_DEBUGGER != 0) && (JJS_DEBUGGER != 1)
#error "Invalid value for 'JJS_DEBUGGER' macro."
#endif /* (JJS_DEBUGGER != 0) && (JJS_DEBUGGER != 1) */
#if (JJS_ERROR_MESSAGES != 0) && (JJS_ERROR_MESSAGES != 1)
#error "Invalid value for 'JJS_ERROR_MESSAGES' macro."
#endif /* (JJS_ERROR_MESSAGES != 0) && (JJS_ERROR_MESSAGES != 1) */
#if (JJS_EXTERNAL_CONTEXT != 0) && (JJS_EXTERNAL_CONTEXT != 1)
#error "Invalid value for 'JJS_EXTERNAL_CONTEXT' macro."
#endif /* (JJS_EXTERNAL_CONTEXT != 0) && (JJS_EXTERNAL_CONTEXT != 1) */
#if JJS_GLOBAL_HEAP_SIZE <= 0
#error "Invalid value for 'JJS_GLOBAL_HEAP_SIZE' macro."
#endif /* JJS_GLOBAL_HEAP_SIZE <= 0 */
#if JJS_GC_LIMIT < 0
#error "Invalid value for 'JJS_GC_LIMIT' macro."
#endif /* JJS_GC_LIMIT < 0 */
#if JJS_STACK_LIMIT < 0
#error "Invalid value for 'JJS_STACK_LIMIT' macro."
#endif /* JJS_STACK_LIMIT < 0 */
#if JJS_GC_MARK_LIMIT < 0
#error "Invalid value for 'JJS_GC_MARK_LIMIT' macro."
#endif /* JJS_GC_MARK_LIMIT < 0 */
#if (JJS_LCACHE != 0) && (JJS_LCACHE != 1)
#error "Invalid value for 'JJS_LCACHE' macro."
#endif /* (JJS_LCACHE != 0) && (JJS_LCACHE != 1) */
#if (JJS_FUNCTION_TO_STRING != 0) && (JJS_FUNCTION_TO_STRING != 1)
#error "Invalid value for 'JJS_FUNCTION_TO_STRING' macro."
#endif /* (JJS_FUNCTION_TO_STRING != 0) && (JJS_FUNCTION_TO_STRING != 1) */
#if (JJS_LINE_INFO != 0) && (JJS_LINE_INFO != 1)
#error "Invalid value for 'JJS_LINE_INFO' macro."
#endif /* (JJS_LINE_INFO != 0) && (JJS_LINE_INFO != 1) */
#if (JJS_LOGGING != 0) && (JJS_LOGGING != 1)
#error "Invalid value for 'JJS_LOGGING' macro."
#endif /* (JJS_LOGGING != 0) && (JJS_LOGGING != 1) */
#if (JJS_MEM_GC_BEFORE_EACH_ALLOC != 0) && (JJS_MEM_GC_BEFORE_EACH_ALLOC != 1)
#error "Invalid value for 'JJS_MEM_GC_BEFORE_EACH_ALLOC' macro."
#endif /* (JJS_MEM_GC_BEFORE_EACH_ALLOC != 0) && (JJS_MEM_GC_BEFORE_EACH_ALLOC != 1) */
#if (JJS_MEM_STATS != 0) && (JJS_MEM_STATS != 1)
#error "Invalid value for 'JJS_MEM_STATS' macro."
#endif /* (JJS_MEM_STATS != 0) && (JJS_MEM_STATS != 1) */
#if (JJS_NUMBER_TYPE_FLOAT64 != 0) && (JJS_NUMBER_TYPE_FLOAT64 != 1)
#error "Invalid value for 'JJS_NUMBER_TYPE_FLOAT64' macro."
#endif /* (JJS_NUMBER_TYPE_FLOAT64 != 0) && (JJS_NUMBER_TYPE_FLOAT64 != 1) */
#if (JJS_PARSER != 0) && (JJS_PARSER != 1)
#error "Invalid value for 'JJS_PARSER' macro."
#endif /* (JJS_PARSER != 0) && (JJS_PARSER != 1) */
#if (JJS_PARSER_DUMP_BYTE_CODE != 0) && (JJS_PARSER_DUMP_BYTE_CODE != 1)
#error "Invalid value for 'JJS_PARSER_DUMP_BYTE_CODE' macro."
#endif /* (JJS_PARSER_DUMP_BYTE_CODE != 0) && (JJS_PARSER_DUMP_BYTE_CODE != 1) */
#if (JJS_PROPERTY_HASHMAP != 0) && (JJS_PROPERTY_HASHMAP != 1)
#error "Invalid value for 'JJS_PROPERTY_HASHMAP' macro."
#endif /* (JJS_PROPERTY_HASHMAP != 0) && (JJS_PROPERTY_HASHMAP != 1) */
#if (JJS_PROMISE_CALLBACK != 0) && (JJS_PROMISE_CALLBACK != 1)
#error "Invalid value for 'JJS_PROMISE_CALLBACK' macro."
#endif /* (JJS_PROMISE_CALLBACK != 0) && (JJS_PROMISE_CALLBACK != 1) */
#if (JJS_REGEXP_DUMP_BYTE_CODE != 0) && (JJS_REGEXP_DUMP_BYTE_CODE != 1)
#error "Invalid value for 'JJS_REGEXP_DUMP_BYTE_CODE' macro."
#endif /* (JJS_REGEXP_DUMP_BYTE_CODE != 0) && (JJS_REGEXP_DUMP_BYTE_CODE != 1) */
#if (JJS_REGEXP_STRICT_MODE != 0) && (JJS_REGEXP_STRICT_MODE != 1)
#error "Invalid value for 'JJS_REGEXP_STRICT_MODE' macro."
#endif /* (JJS_REGEXP_STRICT_MODE != 0) && (JJS_REGEXP_STRICT_MODE != 1) */
#if (JJS_SNAPSHOT_EXEC != 0) && (JJS_SNAPSHOT_EXEC != 1)
#error "Invalid value for 'JJS_SNAPSHOT_EXEC' macro."
#endif /* (JJS_SNAPSHOT_EXEC != 0) && (JJS_SNAPSHOT_EXEC != 1) */
#if (JJS_SNAPSHOT_SAVE != 0) && (JJS_SNAPSHOT_SAVE != 1)
#error "Invalid value for 'JJS_SNAPSHOT_SAVE' macro."
#endif /* (JJS_SNAPSHOT_SAVE != 0) && (JJS_SNAPSHOT_SAVE != 1) */
#if (JJS_SYSTEM_ALLOCATOR != 0) && (JJS_SYSTEM_ALLOCATOR != 1)
#error "Invalid value for 'JJS_SYSTEM_ALLOCATOR' macro."
#endif /* (JJS_SYSTEM_ALLOCATOR != 0) && (JJS_SYSTEM_ALLOCATOR != 1) */
#if (JJS_UNICODE_CASE_CONVERSION != 0) && (JJS_UNICODE_CASE_CONVERSION != 1)
#error "Invalid value for 'JJS_UNICODE_CASE_CONVERSION' macro."
#endif /* (JJS_UNICODE_CASE_CONVERSION != 0) && (JJS_UNICODE_CASE_CONVERSION != 1) */
#if (JJS_VALGRIND != 0) && (JJS_VALGRIND != 1)
#error "Invalid value for 'JJS_VALGRIND' macro."
#endif /* (JJS_VALGRIND != 0) && (JJS_VALGRIND != 1) */
#if (JJS_VM_HALT != 0) && (JJS_VM_HALT != 1)
#error "Invalid value for 'JJS_VM_HALT' macro."
#endif /* (JJS_VM_HALT != 0) && (JJS_VM_HALT != 1) */
#if (JJS_VM_THROW != 0) && (JJS_VM_THROW != 1)
#error "Invalid value for 'JJS_VM_THROW' macro."
#endif /* (JJS_VM_THROW != 0) && (JJS_VM_THROW != 1) */

/**
 * Cross component requirements check.
 */

/**
 * The date module can only use the float 64 number types.
 */
#if JJS_BUILTIN_DATE && !JJS_NUMBER_TYPE_FLOAT64
#error "Date does not support float32"
#endif /* JJS_BUILTIN_DATE && !JJS_NUMBER_TYPE_FLOAT64 */

/**
 * Source name related types into a single guard
 */
#if JJS_LINE_INFO || JJS_ERROR_MESSAGES || JJS_MODULE_SYSTEM
#define JJS_SOURCE_NAME 1
#else /* !(JJS_LINE_INFO || JJS_ERROR_MESSAGES || JJS_MODULE_SYSTEM) */
#define JJS_SOURCE_NAME 0
#endif /* JJS_LINE_INFO || JJS_ERROR_MESSAGES || JJS_MODULE_SYSTEM */

#if JJS_ANNEX_ESM && !JJS_MODULE_SYSTEM
#error "JJS_ANNEX_ESM depends on JJS_MODULE_SYSTEM"
#endif /* JJS_ANNEX_ESM && !JJS_MODULE_SYSTEM */

#endif /* !JJS_CONFIG_H */
