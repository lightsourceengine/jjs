#!/usr/bin/env python

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

from __future__ import print_function

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys
import settings

def default_toolchain():
    # We don't have default toolchain on Windows and os.uname() isn't supported.
    if sys.platform == 'win32':
        return None

    (sysname, _, _, _, machine) = os.uname()
    toolchain = os.path.join(settings.PROJECT_DIR,
                             'cmake',
                             'toolchain_%s_%s.cmake' % (sysname.lower(), machine.lower()))
    return toolchain if os.path.isfile(toolchain) else None

def get_arguments():
    devhelp_preparser = argparse.ArgumentParser(add_help=False)
    devhelp_preparser.add_argument('--devhelp', action='store_true', default=False,
                                   help='show help with all options '
                                        '(including those, which are useful for developers only)')

    devhelp_arguments, args = devhelp_preparser.parse_known_args()
    if devhelp_arguments.devhelp:
        args.append('--devhelp')

    def devhelp(helpstring):
        return helpstring if devhelp_arguments.devhelp else argparse.SUPPRESS

    parser = argparse.ArgumentParser(parents=[devhelp_preparser], epilog="""
        This tool is a thin wrapper around cmake and make to help build the
        project easily. All the real build logic is in the CMakeLists.txt files.
        For most of the options, the defaults are also defined there.
        """)

    buildgrp = parser.add_argument_group('general build options')
    buildgrp.add_argument('--builddir', metavar='DIR', default=os.path.join(settings.PROJECT_DIR, 'build'),
                          help='specify build directory (default: %(default)s)')
    buildgrp.add_argument('--clean', action='store_true', default=False,
                          help='clean build')
    buildgrp.add_argument('--cmake-param', metavar='OPT', action='append', default=[],
                          help='add custom argument to CMake')
    buildgrp.add_argument('--cmake-g', metavar='GENERATOR',
                          help='set the CMake build file generator (-G option in CMake, see cmake --help for supported names)')
    buildgrp.add_argument('--compile-flag', metavar='OPT', action='append', default=[],
                          help='add custom compile flag')
    buildgrp.add_argument('--build-type', metavar='TYPE', default='MinSizeRel',
                          help='set build type (default: %(default)s)')
    buildgrp.add_argument('--debug', dest='build_type', action='store_const', const='Debug', default=argparse.SUPPRESS,
                          help='debug build (alias for --build-type %(const)s)')
    buildgrp.add_argument('--install', metavar='DIR', nargs='?', default=None, const=False,
                          help='install after build (default: don\'t install; '
                               'default directory if install: OS-specific)')
    buildgrp.add_argument('-j', '--jobs', metavar='N', type=int, default=multiprocessing.cpu_count() + 1,
                          help='number of parallel build jobs (default: %(default)s)')
    buildgrp.add_argument('--link-lib', metavar='OPT', action='append', default=[],
                          help='add custom library to be linked')
    buildgrp.add_argument('--linker-flag', metavar='OPT', action='append', default=[],
                          help='add custom linker flag')
    buildgrp.add_argument('--amalgam', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                          help='enable amalgamated build (%(choices)s)')
    buildgrp.add_argument('--lto', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                          help='enable link-time optimizations (%(choices)s)')
    buildgrp.add_argument('--shared-libs', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                          help='enable building of shared libraries (%(choices)s)')
    buildgrp.add_argument('--strip', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                          help='strip release binaries (%(choices)s)')
    buildgrp.add_argument('--toolchain', metavar='FILE', default=default_toolchain(),
                          help='specify toolchain file (default: %(default)s)')
    buildgrp.add_argument('-v', '--verbose', action='store_const', const='ON',
                          help='increase verbosity')

    compgrp = parser.add_argument_group('optional components')
    compgrp.add_argument('--jjs-cmdline', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='build jjs command line tool (%(choices)s)')
    compgrp.add_argument('--jjs-cmdline-snapshot', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='build snapshot command line tool (%(choices)s)')
    compgrp.add_argument('--jjs-cmdline-test', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('build test version of the jjs command line tool (%(choices)s)'))
    compgrp.add_argument('--jjs-cmdline-test262', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('build a JJS engine compatible with the python test262 harness (%(choices)s)'))
    compgrp.add_argument('--libfuzzer', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('build JJS with libfuzzer support (%(choices)s)'))
    compgrp.add_argument('--jjs-pack', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='build jjs-pack(age) (%(choices)s)')
    compgrp.add_argument('--unittests', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('build unittests (%(choices)s)'))

    coregrp = parser.add_argument_group('jjs-core options')
    coregrp.add_argument('--cpointer-32bit', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable 32 bit compressed pointers (%(choices)s)')
    coregrp.add_argument('--error-messages', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable error messages (%(choices)s)')
    coregrp.add_argument('--jjs-debugger', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable the JJS debugger (%(choices)s)')
    coregrp.add_argument('--js-parser', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable js-parser (%(choices)s)')
    coregrp.add_argument('--function-to-string', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable function toString (%(choices)s)')
    coregrp.add_argument('--line-info', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='provide line info (%(choices)s)')
    coregrp.add_argument('--logging', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable logging (%(choices)s)')
    coregrp.add_argument('--default-vm-heap-size-kb', metavar='SIZE', type=int,
                         help='default size of memory heap (in kilobytes)')
    coregrp.add_argument('--default-vm-stack-limit-kb', metavar='SIZE', type=int,
                         help='default maximum stack usage (in kilobytes)')
    coregrp.add_argument('--default-scratch-arena-kb', metavar='SIZE', type=int,
                         help='default size of scratch arena buffer (in kilobytes)')
    coregrp.add_argument('--vm-stack-limit', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable stack usage limit checks')
    coregrp.add_argument('--mem-stats', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable memory statistics (%(choices)s)'))
    coregrp.add_argument('--mem-stress-test', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable mem-stress test (%(choices)s)'))
    coregrp.add_argument('--profile', metavar='FILE',
                         help='specify profile file')
    coregrp.add_argument('--promise-callback', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable promise callback (%(choices)s)')
    coregrp.add_argument('--regexp-strict-mode', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable regexp strict mode (%(choices)s)'))
    coregrp.add_argument('--show-opcodes', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable parser byte-code dumps (%(choices)s)'))
    coregrp.add_argument('--show-regexp-opcodes', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable regexp byte-code dumps (%(choices)s)'))
    coregrp.add_argument('--snapshot-exec', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable executing snapshot files (%(choices)s)')
    coregrp.add_argument('--snapshot-save', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable saving snapshot files (%(choices)s)')
    coregrp.add_argument('--valgrind', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable Valgrind support (%(choices)s)'))
    coregrp.add_argument('--vm-exec-stop', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable VM execution stop callback (%(choices)s)')
    coregrp.add_argument('--vm-throw', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable VM throw callback (%(choices)s)')

    coregrp.add_argument('--platform-api-io-write', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.io.write (%(choices)s)')
    coregrp.add_argument('--platform-api-io-flush', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.io.flush (%(choices)s)')
    coregrp.add_argument('--platform-api-fs-read-file', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.fs.read_file (%(choices)s)')
    coregrp.add_argument('--platform-api-path-cwd', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.path.cwd (%(choices)s)')
    coregrp.add_argument('--platform-api-path-realpath', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.path.realpath (%(choices)s)')
    coregrp.add_argument('--platform-api-time-now-ms', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.time.now_ms (%(choices)s)')
    coregrp.add_argument('--platform-api-time-local-tza', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.time.local_tza (%(choices)s)')
    coregrp.add_argument('--platform-api-time-sleep', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help='enable default implementation of platform.time.sleep (%(choices)s)')

    maingrp = parser.add_argument_group('jjs-main options')
    maingrp.add_argument('--link-map', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable the generation of link map for jjs command line tool (%(choices)s)'))
    maingrp.add_argument('--compile-commands', metavar='X', choices=['ON', 'OFF'], type=str.upper,
                         help=devhelp('enable the generation of compile_commands.json (%(choices)s)'))

    arguments = parser.parse_args(args)
    if arguments.devhelp:
        parser.print_help()
        sys.exit(0)

    return arguments

def generate_build_options(arguments):
    build_options = []

    def build_options_append(cmakeopt, cliarg):
        if cliarg:
            build_options.append('-D%s=%s' % (cmakeopt, cliarg))

    # general build options
    build_options_append('CMAKE_BUILD_TYPE', arguments.build_type)
    build_options_append('JJS_EXTERNAL_COMPILE_FLAGS', ' '.join(arguments.compile_flag))
    build_options_append('JJS_EXTERNAL_LINK_LIBS', ' '.join(arguments.link_lib))
    build_options_append('JJS_EXTERNAL_LINKER_FLAGS', ' '.join(arguments.linker_flag))
    build_options_append('JJS_AMALGAM', arguments.amalgam)
    build_options_append('JJS_LTO', arguments.lto)
    build_options_append('JJS_BUILD_SHARED_LIBS', arguments.shared_libs)
    build_options_append('JJS_STRIP', arguments.strip)
    build_options_append('CMAKE_TOOLCHAIN_FILE', arguments.toolchain)
    build_options_append('CMAKE_VERBOSE_MAKEFILE', arguments.verbose)

    # optional components
    build_options_append('JJS_CMDLINE', arguments.jjs_cmdline)
    build_options_append('JJS_CMDLINE_SNAPSHOT', arguments.jjs_cmdline_snapshot)
    build_options_append('JJS_CMDLINE_TEST', arguments.jjs_cmdline_test)
    build_options_append('JJS_CMDLINE_TEST262', arguments.jjs_cmdline_test262)
    build_options_append('JJS_LIBFUZZER', arguments.libfuzzer)
    build_options_append('JJS_PACK', arguments.jjs_pack)
    build_options_append('JJS_UNITTESTS', arguments.unittests)

    # jjs-core options
    build_options_append('JJS_CPOINTER_32_BIT', arguments.cpointer_32bit)
    build_options_append('JJS_ERROR_MESSAGES', arguments.error_messages)
    build_options_append('JJS_DEBUGGER', arguments.jjs_debugger)
    build_options_append('JJS_PARSER', arguments.js_parser)
    build_options_append('JJS_FUNCTION_TO_STRING', arguments.function_to_string)
    build_options_append('JJS_LINE_INFO', arguments.line_info)
    build_options_append('JJS_LOGGING', arguments.logging)
    build_options_append('JJS_DEFAULT_VM_HEAP_SIZE_KB', arguments.default_vm_heap_size_kb)
    build_options_append('JJS_DEFAULT_VM_STACK_LIMIT_KB', arguments.default_vm_stack_limit_kb)
    build_options_append('JJS_DEFAULT_SCRATCH_ARENA_KB', arguments.default_scratch_arena_kb)
    build_options_append('JJS_MEM_STATS', arguments.mem_stats)
    build_options_append('JJS_MEM_GC_BEFORE_EACH_ALLOC', arguments.mem_stress_test)
    build_options_append('JJS_PROFILE', arguments.profile)
    build_options_append('JJS_PROMISE_CALLBACK', arguments.promise_callback)
    build_options_append('JJS_REGEXP_STRICT_MODE', arguments.regexp_strict_mode)
    build_options_append('JJS_PARSER_DUMP_BYTE_CODE', arguments.show_opcodes)
    build_options_append('JJS_REGEXP_DUMP_BYTE_CODE', arguments.show_regexp_opcodes)
    build_options_append('JJS_SNAPSHOT_EXEC', arguments.snapshot_exec)
    build_options_append('JJS_SNAPSHOT_SAVE', arguments.snapshot_save)
    build_options_append('JJS_VALGRIND', arguments.valgrind)
    build_options_append('JJS_VM_HALT', arguments.vm_exec_stop)
    build_options_append('JJS_VM_THROW', arguments.vm_throw)
    build_options_append('JJS_VM_STACK_LIMIT', arguments.vm_stack_limit)

    # platform api options
    build_options_append('JJS_PLATFORM_API_IO_WRITE', arguments.platform_api_io_write)
    build_options_append('JJS_PLATFORM_API_IO_FLUSH', arguments.platform_api_io_flush)
    build_options_append('JJS_PLATFORM_API_FS_READ_FILE', arguments.platform_api_fs_read_file)
    build_options_append('JJS_PLATFORM_API_PATH_CWD', arguments.platform_api_path_cwd)
    build_options_append('JJS_PLATFORM_API_PATH_REALPATH', arguments.platform_api_path_realpath)
    build_options_append('JJS_PLATFORM_API_TIME_NOW_MS', arguments.platform_api_time_now_ms)
    build_options_append('JJS_PLATFORM_API_TIME_LOCAL_TZA', arguments.platform_api_time_local_tza)
    build_options_append('JJS_PLATFORM_API_TIME_SLEEP', arguments.platform_api_time_sleep)

    # jjs-main options
    build_options_append('JJS_CMDLINE_LINK_MAP', arguments.link_map)
    build_options_append('JJS_COMPILE_COMMANDS', arguments.compile_commands)

    # general build options (final step)
    if arguments.cmake_param:
        build_options.extend(arguments.cmake_param)

    # set the cmake generator
    if arguments.cmake_g:
        build_options.extend(["-G %s" % arguments.cmake_g])

    return build_options

def configure_output_dir(arguments):
    if not os.path.isabs(arguments.builddir):
        arguments.builddir = os.path.join(settings.PROJECT_DIR, arguments.builddir)

    if arguments.clean and os.path.exists(arguments.builddir):
        shutil.rmtree(arguments.builddir)

    if not os.path.exists(arguments.builddir):
        os.makedirs(arguments.builddir)

def configure_jjs(arguments):
    configure_output_dir(arguments)

    build_options = generate_build_options(arguments)

    cmake_cmd = ['cmake', '-B' + arguments.builddir, '-H' + settings.PROJECT_DIR]

    if arguments.install:
        cmake_cmd.append('-DCMAKE_INSTALL_PREFIX=%s' % arguments.install)

    cmake_cmd.extend(build_options)

    return subprocess.call(cmake_cmd)

def make_jjs(arguments):
    make_cmd = ['cmake', '--build', arguments.builddir, '--config', arguments.build_type]
    env = dict(os.environ)
    env['CMAKE_BUILD_PARALLEL_LEVEL'] = str(arguments.jobs)
    env['MAKEFLAGS'] = '-j%d' % (arguments.jobs) # Workaround for CMake < 3.12
    proc = subprocess.Popen(make_cmd, env=env)
    proc.communicate()

    return proc.returncode

def install_jjs(arguments):
    install_target = 'INSTALL' if os.path.exists(os.path.join(arguments.builddir, 'jjs.sln')) else 'install'
    make_cmd = ['cmake', '--build', arguments.builddir, '--config', arguments.build_type, '--target', install_target]
    return subprocess.call(make_cmd)

def print_result(ret):
    print('=' * 30)
    if ret:
        print('Build failed with exit code: %s' % (ret))
    else:
        print('Build succeeded!')
    print('=' * 30)

def main():
    arguments = get_arguments()

    ret = configure_jjs(arguments)

    if not ret:
        ret = make_jjs(arguments)

    if not ret and arguments.install is not None:
        ret = install_jjs(arguments)

    print_result(ret)
    sys.exit(ret)


if __name__ == "__main__":
    main()
