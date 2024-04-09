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

#ifndef JJS_COMPILER_H
#define JJS_COMPILER_H

#ifdef __cplusplus
#define JJS_C_API_BEGIN extern "C" {
#define JJS_C_API_END   }
#else /* !__cplusplus */
#define JJS_C_API_BEGIN
#define JJS_C_API_END
#endif /* __cplusplus */

JJS_C_API_BEGIN

/** \addtogroup jjs-compiler JJS compiler compatibility components
 * @{
 */

#ifdef __GNUC__

/*
 * Compiler-specific macros relevant for GCC.
 */
#define JJS_ATTR_ALIGNED(ALIGNMENT) __attribute__ ((aligned (ALIGNMENT)))
#define JJS_ATTR_ALWAYS_INLINE      __attribute__ ((always_inline))
#define JJS_ATTR_CONST              __attribute__ ((const))
#define JJS_ATTR_DEPRECATED         __attribute__ ((deprecated))
#define JJS_ATTR_FORMAT(...)        __attribute__ ((format (__VA_ARGS__)))
#define JJS_ATTR_HOT                __attribute__ ((hot))
#define JJS_ATTR_NOINLINE           __attribute__ ((noinline))
#define JJS_ATTR_NORETURN           __attribute__ ((noreturn))
#define JJS_ATTR_PURE               __attribute__ ((pure))
#define JJS_ATTR_WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
#define JJS_ATTR_WEAK               __attribute__ ((weak))

#define JJS_WEAK_SYMBOL_SUPPORT

#ifndef JJS_LIKELY
#define JJS_LIKELY(x) __builtin_expect (!!(x), 1)
#endif /* !JJS_LIKELY */

#ifndef JJS_UNLIKELY
#define JJS_UNLIKELY(x) __builtin_expect (!!(x), 0)
#endif /* !JJS_UNLIKELY */

#endif /* __GNUC__ */

#ifdef _MSC_VER

/*
 * Compiler-specific macros relevant for Microsoft Visual C/C++ Compiler.
 */
#define JJS_ATTR_DEPRECATED __declspec(deprecated)
#define JJS_ATTR_NOINLINE   __declspec(noinline)
#define JJS_ATTR_NORETURN   __declspec(noreturn)

/*
 * Microsoft Visual C/C++ Compiler doesn't support for VLA, using _alloca
 * instead.
 */
#define JJS_VLA(type, name, size) type *name = (type *) (_alloca (sizeof (type) * (size)))

#endif /* _MSC_VER */

/*
 * Default empty definitions for all compiler-specific macros. Define any of
 * these in a guarded block above (e.g., as for GCC) to fine tune compilation
 * for your own compiler. */

/**
 * Function attribute to align function to given number of bytes.
 */
#ifndef JJS_ATTR_ALIGNED
#define JJS_ATTR_ALIGNED(ALIGNMENT)
#endif /* !JJS_ATTR_ALIGNED */

/**
 * Function attribute to inline function to all call sites.
 */
#ifndef JJS_ATTR_ALWAYS_INLINE
#define JJS_ATTR_ALWAYS_INLINE
#endif /* !JJS_ATTR_ALWAYS_INLINE */

/**
 * Function attribute to declare that function has no effect except the return
 * value and it only depends on parameters.
 */
#ifndef JJS_ATTR_CONST
#define JJS_ATTR_CONST
#endif /* !JJS_ATTR_CONST */

/**
 * Function attribute to trigger warning if deprecated function is called.
 */
#ifndef JJS_ATTR_DEPRECATED
#define JJS_ATTR_DEPRECATED
#endif /* !JJS_ATTR_DEPRECATED */

/**
 * Function attribute to declare that function is variadic and takes a format
 * string and some arguments as parameters.
 */
#ifndef JJS_ATTR_FORMAT
#define JJS_ATTR_FORMAT(...)
#endif /* !JJS_ATTR_FORMAT */

/**
 * Function attribute to predict that function is a hot spot, and therefore
 * should be optimized aggressively.
 */
#ifndef JJS_ATTR_HOT
#define JJS_ATTR_HOT
#endif /* !JJS_ATTR_HOT */

/**
 * Function attribute not to inline function ever.
 */
#ifndef JJS_ATTR_NOINLINE
#define JJS_ATTR_NOINLINE
#endif /* !JJS_ATTR_NOINLINE */

/**
 * Function attribute to declare that function never returns.
 */
#ifndef JJS_ATTR_NORETURN
#define JJS_ATTR_NORETURN
#endif /* !JJS_ATTR_NORETURN */

/**
 * Function attribute to declare that function has no effect except the return
 * value and it only depends on parameters and global variables.
 */
#ifndef JJS_ATTR_PURE
#define JJS_ATTR_PURE
#endif /* !JJS_ATTR_PURE */

/**
 * Function attribute to trigger warning if function's caller doesn't use (e.g.,
 * check) the return value.
 */
#ifndef JJS_ATTR_WARN_UNUSED_RESULT
#define JJS_ATTR_WARN_UNUSED_RESULT
#endif /* !JJS_ATTR_WARN_UNUSED_RESULT */

/**
 * Function attribute to declare a function a weak symbol
 */
#ifndef JJS_ATTR_WEAK
#define JJS_ATTR_WEAK
#endif /* !JJS_ATTR_WEAK */

/**
 * Helper to predict that a condition is likely.
 */
#ifndef JJS_LIKELY
#define JJS_LIKELY(x) (x)
#endif /* !JJS_LIKELY */

/**
 * Helper to predict that a condition is unlikely.
 */
#ifndef JJS_UNLIKELY
#define JJS_UNLIKELY(x) (x)
#endif /* !JJS_UNLIKELY */

/**
 * Helper to declare (or mimic) a C99 variable-length array.
 */
#ifndef JJS_VLA
#define JJS_VLA(type, name, size) type name[size]
#endif /* !JJS_VLA */

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#define JJS_OS_IS_WINDOWS
#define JJS_PLATFORM_OS_TYPE JJS_PLATFORM_OS_WIN32
#elif defined(_AIX) || defined(__TOS_AIX__)
#define JJS_OS_IS_AIX
#define JJS_PLATFORM_OS_TYPE JJS_PLATFORM_OS_AIX
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define JJS_OS_IS_LINUX
#define JJS_PLATFORM_OS_TYPE JJS_PLATFORM_OS_LINUX
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define JJS_OS_IS_MACOS
#define JJS_PLATFORM_OS_TYPE JJS_PLATFORM_OS_DARWIN
#else
#define JJS_PLATFORM_OS_TYPE JJS_PLATFORM_OS_UNKNOWN
#endif

#if defined (JJS_OS_IS_LINUX) || defined (JJS_OS_IS_MACOS) || defined (JJS_OS_IS_AIX)
#define JJS_OS_IS_UNIX
#endif

#if defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)    \
  || defined(__i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) \
  || defined(__INTEL__)
#define JJS_ARCH_IS_X32
#define JJS_PLATFORM_ARCH_TYPE JJS_PLATFORM_ARCH_IA32
#elif defined(__ARM_ARCH) || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARM) \
  || defined(__arm__) || defined(__thumb__)
#define JJS_ARCH_IS_ARM
#define JJS_PLATFORM_ARCH_TYPE JJS_PLATFORM_ARCH_ARM
#elif defined(__arm64) || defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__)
#define JJS_ARCH_IS_ARM64
#define JJS_PLATFORM_ARCH_TYPE JJS_PLATFORM_ARCH_ARM64
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
#define JJS_ARCH_IS_X64
#define JJS_PLATFORM_ARCH_TYPE JJS_PLATFORM_ARCH_X64
#else
#define JJS_PLATFORM_ARCH_TYPE JJS_PLATFORM_ARCH_UNKNOWN
#endif

/**
 * @}
 */

JJS_C_API_END

#endif /* !JJS_COMPILER_H */
