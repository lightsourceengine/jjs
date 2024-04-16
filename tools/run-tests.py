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
import collections
import hashlib
import os
import platform
import subprocess
import sys
import settings
import shutil

if sys.version_info.major >= 3:
    import runners.util as util  # pylint: disable=import-error
else:
    sys.path.append(os.path.dirname(os.path.realpath(__file__)) + '/runners')
    import util

OUTPUT_DIR = os.path.join(settings.PROJECT_DIR, 'build', 'tests')

Options = collections.namedtuple('Options', ['name', 'build_args', 'test_args', 'skip'])
Options.__new__.__defaults__ = ([], [], False)


def skip_if(condition, desc):
    return desc if condition else False


OPTIONS_COMMON = [
    # vm configuration
    '--default-vm-heap-size=512',
    # stuff the tests need
    '--function-to-string=on',
]
OPTIONS_STACK_LIMIT = ['--default-vm-stack-limit=96']
OPTIONS_MEM_STRESS = ['--mem-stress-test=on']
OPTIONS_DEBUG = ['--debug']
OPTIONS_SNAPSHOT = [
    '--snapshot-save=on',
    '--snapshot-exec=on',
    '--jjs-cmdline-snapshot=on',
]
OPTIONS_UNITTESTS = [
    # enable unittests
    '--unittests=on',
    # vm configuration
    '--default-vm-heap-size=512',
    # stuff the tests need
    '--function-to-string=on',
    '--jjs-cmdline=off',
    '--snapshot-save=on',
    '--snapshot-exec=on',
    '--vm-exec-stop=on',
    '--vm-throw=on',
    '--mem-stats=on',
    '--promise-callback=on',
    '--jjs-ext=on',
    '--jjs-ext-debugger=on',
    '--line-info=on',
]
OPTIONS_PROMISE_CALLBACK = ['--promise-callback=on']
JJS_UNITTESTS_OPTIONS = [
    Options('unittests', OPTIONS_UNITTESTS),
]

# Test options for jjs-tests
JJS_TESTS_OPTIONS = [
    Options('jjs_tests', OPTIONS_COMMON),
]

# Test options for jjs-snapshot-tests
JJS_SNAPSHOT_TESTS_OPTIONS = [
    Options('jjs_tests-snapshot', OPTIONS_COMMON + OPTIONS_SNAPSHOT, ['--snapshot']),
]

# Test options for jjs-pack-tests
JJS_PACK_TESTS_OPTIONS = [
    Options('jjs_pack_tests', OPTIONS_COMMON + ['--jjs-pack=on']),
]

# Test options for test262
TEST262_TEST_SUITE_OPTIONS = [
    Options('test262', ['--default-vm-heap-size=20480', '--function-to-string=on', '--jjs-cmdline-test262=on']),
]

# Test options for jjs-debugger
DEBUGGER_TEST_OPTIONS = [
    Options('jjs_debugger_tests',
            ['--jjs-debugger=on'])
]


JJS_BUILDOPTIONS_COMMON = ['--lto=off']

# Test options for buildoption-test
JJS_BUILDOPTIONS = [
    Options('buildoption_test-lto', JJS_BUILDOPTIONS_COMMON + ['--lto=on']),
    Options('buildoption_test-logging', JJS_BUILDOPTIONS_COMMON + ['--logging=on']),
    Options('buildoption_test-amalgam', JJS_BUILDOPTIONS_COMMON + ['--amalgam=on']),
    Options('buildoption_test-valgrind', JJS_BUILDOPTIONS_COMMON + ['--valgrind=on'],
            skip=skip_if((sys.platform == 'win32'), 'valgrind not supported on msvc (mingw is ok)')),
    Options('buildoption_test-init_flag',
            JJS_BUILDOPTIONS_COMMON + ['--mem-stats=on', '--show-opcodes=on', '--show-regexp-opcodes=on']),
    Options('buildoption_test-no_lcache_prophashmap',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_LCACHE=0', '--compile-flag=-DJJS_PROPERTY_HASHMAP=0']),
    Options('buildoption_test-shared_libs',
            JJS_BUILDOPTIONS_COMMON + ['--shared-libs=on'],
            skip=skip_if((sys.platform == 'win32'), 'Not yet supported, link failure on Windows')),
    # cmdline options
    Options('buildoption_test-cmdline_test',
            JJS_BUILDOPTIONS_COMMON + ['--jjs-cmdline-test=on'],
            skip=skip_if((sys.platform == 'win32'), 'rand() can\'t be overridden on Windows (benchmarking.c)')),
    Options('buildoption_test-cmdline_snapshot',
            JJS_BUILDOPTIONS_COMMON + ['--jjs-cmdline-snapshot=on']),
    Options('buildoption_test-recursion_limit',
            JJS_BUILDOPTIONS_COMMON + OPTIONS_STACK_LIMIT),
    Options('buildoption_test-jjs-debugger',
            JJS_BUILDOPTIONS_COMMON + ['--jjs-debugger=on']),
    # annex (or just esm) needs to be off to disable the module system api
    Options('buildoption_test-module-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_MODULE_SYSTEM=0', '--compile-flag=-DJJS_ANNEX=0']),
    Options('buildoption_test-commonjs-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_ANNEX_COMMONJS=0']),
    Options('buildoption_test-esm-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_ANNEX_ESM=0']),
    Options('buildoption_test-commonjs-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_ANNEX_PMAP=0']),
    Options('buildoption_test-vmod-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_ANNEX_VMOD=0']),
    Options('buildoption_test-queuemicrotask-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_ANNEX_QUEUE_MICROTASK=0']),
    Options('buildoption_test-pack-esm-off',
            JJS_BUILDOPTIONS_COMMON + ['--jjs-pack=on', '--compile-flag=-DJJS_ANNEX_ESM=0', '--compile-flag=-DJJS_ANNEX_COMMONJS=0']),
    Options('buildoption_test-builtin-proxy-off',
            JJS_BUILDOPTIONS_COMMON + ['--compile-flag=-DJJS_BUILTIN_PROXY=0']),
    # platform api flags
    Options('buildoption_test-no-platform_api_io_write',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-io-write=off']),
    Options('buildoption_test-no-platform_api_io_flush',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-io-flush=off']),
    Options('buildoption_test-no-platform_api_fs_read_file',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-fs-read-file=off']),
    Options('buildoption_test-no-platform_api_path_cwd',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-path-cwd=off']),
    Options('buildoption_test-no-platform_api_path_realpath',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-path-realpath=off']),
    Options('buildoption_test-no-platform_api_time_now_ms',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-time-now-ms=off']),
    Options('buildoption_test-no-platform_api_time_sleep',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-time-sleep=off']),
    Options('buildoption_test-no-platform_api_time_local_tza',
            JJS_BUILDOPTIONS_COMMON + ['--platform-api-time-local-tza=off']),
]

def get_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--toolchain', metavar='FILE',
                        help='Add toolchain file')
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Only print out failing tests')
    parser.add_argument('--buildoptions', metavar='LIST',
                        help='Add a comma separated list of extra build options to each test')
    parser.add_argument('--skip-list', metavar='LIST',
                        help='Add a comma separated list of patterns of the excluded JS-tests')
    parser.add_argument('--outdir', metavar='DIR', default=OUTPUT_DIR,
                        help='Specify output directory (default: %(default)s)')
    parser.add_argument('--check-signed-off', metavar='TYPE', nargs='?',
                        choices=['strict', 'tolerant', 'gh-actions'], const='strict',
                        help='Run signed-off check (%(choices)s; default type if not given: %(const)s)')
    parser.add_argument('--check-cppcheck', action='store_true',
                        help='Run cppcheck')
    parser.add_argument('--check-doxygen', action='store_true',
                        help='Run doxygen')
    parser.add_argument('--check-pylint', action='store_true',
                        help='Run pylint')
    parser.add_argument('--check-format', action='store_true',
                        help='Run format check')
    parser.add_argument('--check-license', action='store_true',
                        help='Run license check')
    parser.add_argument('--check-strings', action='store_true',
                        help='Run "magic string source code generator should be executed" check')
    parser.add_argument('--build-debug', action='store_true',
                        help='Build debug version JJS')
    parser.add_argument('--jjs-debugger', action='store_true',
                        help='Run jjs-debugger tests')
    parser.add_argument('--jjs-tests', action='store_true',
                        help='Run jjs-tests')
    parser.add_argument('--jjs-snapshot-tests', action='store_true',
                        help='Run jjs-snapshot-tests')
    parser.add_argument('--jjs-pack-tests', action='store_true',
                        help='Run jjs-pack-tests')
    parser.add_argument('--test262', default=False, const='default',
                        nargs='?', choices=['default', 'all', 'update'],
                        help='Run test262 - default: all tests except excludelist, ' +
                        'all: all tests, update: all tests and update excludelist')
    parser.add_argument('--test262-job-count', metavar='COUNT', type=int, default=0,
                        help='Set the number of jobs to run test262 tests')
    parser.add_argument('--test262-test-list', metavar='LIST',
                        help='Add a comma separated list of tests or directories to run in test262 test suite')
    parser.add_argument('--unittests', action='store_true',
                        help='Run unittests')
    parser.add_argument('--buildoption-test', action='store_true',
                        help='Run buildoption-test')
    parser.add_argument('--all', '--precommit', action='store_true',
                        help='Run all tests')
    parser.add_argument('--no-snapshot-tests', action='store_true', default=False,
                        help='Disable snapshot tests for jjs-tests runs')

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    script_args = parser.parse_args()

    return script_args

BINARY_CACHE = {}

TERM_NORMAL = '\033[0m'
TERM_YELLOW = '\033[1;33m'
TERM_BLUE = '\033[1;34m'
TERM_RED = '\033[1;31m'

def report_command(cmd_type, cmd, env=None):
    sys.stderr.write('%s%s%s\n' % (TERM_BLUE, cmd_type, TERM_NORMAL))
    if env is not None:
        sys.stderr.write(''.join('%s%s=%r \\%s\n' % (TERM_BLUE, var, val, TERM_NORMAL)
                                 for var, val in sorted(env.items())))
    sys.stderr.write('%s%s%s\n' % (TERM_BLUE, (' \\%s\n\t%s' % (TERM_NORMAL, TERM_BLUE)).join(cmd), TERM_NORMAL))

def report_skip(job):
    sys.stderr.write('%sSkipping: %s' % (TERM_YELLOW, job.name))
    if job.skip:
        sys.stderr.write(' (%s)' % job.skip)
    sys.stderr.write('%s\n' % TERM_NORMAL)

def create_binary(job, options):
    build_args = job.build_args[:]
    build_dir_path = os.path.join(options.outdir, job.name)
    if options.build_debug:
        build_args.extend(OPTIONS_DEBUG)
        build_dir_path = os.path.join(options.outdir, job.name + '-debug')
    if options.buildoptions:
        for option in options.buildoptions.split(','):
            if option not in build_args:
                build_args.append(option)

    build_cmd = util.get_python_cmd_prefix()
    build_cmd.append(settings.BUILD_SCRIPT)
    build_cmd.extend(build_args)

    build_cmd.append('--builddir=%s' % build_dir_path)

    install_dir_path = os.path.join(build_dir_path, 'local')
    build_cmd.append('--install=%s' % install_dir_path)

    if options.toolchain:
        build_cmd.append('--toolchain=%s' % options.toolchain)

    report_command('Build command:', build_cmd)

    binary_key = tuple(sorted(build_args))
    if binary_key in BINARY_CACHE:
        ret, build_dir_path = BINARY_CACHE[binary_key]
        sys.stderr.write('(skipping: already built at %s with returncode %d)\n' % (build_dir_path, ret))
        return ret, build_dir_path

    try:
        subprocess.check_output(build_cmd)
        ret = 0
    except subprocess.CalledProcessError as err:
        print(err.output.decode(errors="ignore"))
        ret = err.returncode

    BINARY_CACHE[binary_key] = (ret, build_dir_path)
    return ret, build_dir_path

def get_binary_path(build_dir_path):
    executable_extension = '.exe' if sys.platform == 'win32' else ''
    return os.path.join(build_dir_path, 'local', 'bin', 'jjs' + executable_extension)

def get_test262_binary_path(build_dir_path):
    executable_extension = '.exe' if sys.platform == 'win32' else ''
    return os.path.join(build_dir_path, 'local', 'bin', 'jjs-test262' + executable_extension)

def hash_binary(bin_path):
    blocksize = 65536
    hasher = hashlib.sha1()
    with open(bin_path, 'rb') as bin_file:
        buf = bin_file.read(blocksize)
        while buf:
            hasher.update(buf)
            buf = bin_file.read(blocksize)
    return hasher.hexdigest()

def iterate_test_runner_jobs(jobs, options):
    tested_paths = set()
    tested_hashes = {}

    for job in jobs:
        ret_build, build_dir_path = create_binary(job, options)
        if ret_build:
            yield job, ret_build, None

        if build_dir_path in tested_paths:
            sys.stderr.write('(skipping: already tested with %s)\n' % build_dir_path)
            continue
        else:
            tested_paths.add(build_dir_path)

        bin_path = get_binary_path(build_dir_path)
        bin_hash = hash_binary(bin_path)

        if bin_hash in tested_hashes:
            sys.stderr.write('(skipping: already tested with equivalent %s)\n' % tested_hashes[bin_hash])
            continue
        else:
            tested_hashes[bin_hash] = build_dir_path

        test_cmd = util.get_python_cmd_prefix()
        test_cmd.extend([settings.TEST_RUNNER_SCRIPT, '--engine', bin_path])
        test_cmd.extend(['--pmap', settings.JJS_TESTS_PMAP_FILE])

        yield job, ret_build, test_cmd

def run_check(runnable, env=None, cwd=None):
    report_command('Test command:', runnable, env=env)

    if env is not None:
        full_env = dict(os.environ)
        full_env.update(env)
        env = full_env

    kwargs = {'errors': 'replace', 'encoding': 'utf-8'}
    proc = subprocess.Popen(runnable, cwd=cwd, env=env, **kwargs)
    proc.communicate()
    return proc.returncode

def run_jjs_debugger_tests(options):
    ret_build = ret_test = 0
    for job in DEBUGGER_TEST_OPTIONS:
        ret_build, build_dir_path = create_binary(job, options)
        if ret_build:
            print("\n%sBuild failed%s\n" % (TERM_RED, TERM_NORMAL))
            break

        for channel in ["websocket", "rawpacket"]:
            for test_file in os.listdir(settings.DEBUGGER_TESTS_DIR):
                if test_file.endswith(".cmd"):
                    test_case, _ = os.path.splitext(test_file)
                    test_case_path = os.path.join(settings.DEBUGGER_TESTS_DIR, test_case)
                    test_cmd = [
                        settings.DEBUGGER_TEST_RUNNER_SCRIPT,
                        get_binary_path(build_dir_path),
                        channel,
                        settings.DEBUGGER_CLIENT_SCRIPT,
                        os.path.relpath(test_case_path, settings.PROJECT_DIR)
                    ]

                    if job.test_args:
                        test_cmd.extend(job.test_args)

                    ret_test |= run_check(test_cmd)

    return ret_build | ret_test

def run_jjs_tests_internal(options, test_set):
    ret_build = ret_test = 0
    for job, ret_build, test_cmd in iterate_test_runner_jobs(test_set, options):
        if ret_build:
            break

        test_cmd.append('--test-dir')
        test_cmd.append(settings.JJS_TESTS_DIR)

        if options.quiet:
            test_cmd.append("-q")

        skip_list = []

        if job.name == 'jjs_tests-snapshot':
            if options.no_snapshot_tests:
                continue
            else:
                with open(settings.SNAPSHOT_TESTS_SKIPLIST, 'r') as snapshot_skip_list:
                    for line in snapshot_skip_list:
                        skip_list.append(line.rstrip())

        if options.skip_list:
            skip_list.append(options.skip_list)

        if skip_list:
            test_cmd.append("--skip-list=" + ",".join(skip_list))

        if job.test_args:
            test_cmd.extend(job.test_args)

        ret_test |= run_check(test_cmd, env=dict(TZ='UTC'))

    return ret_build | ret_test

def run_jjs_tests(options):
    return run_jjs_tests_internal(options, JJS_TESTS_OPTIONS)

def run_jjs_snapshot_tests(options):
    return run_jjs_tests_internal(options, JJS_SNAPSHOT_TESTS_OPTIONS)

def run_jjs_pack_tests(options):
    ret_build = ret_test = 0
    for job, ret_build, test_cmd in iterate_test_runner_jobs(JJS_PACK_TESTS_OPTIONS, options):
        if ret_build:
            break

        test_cmd.append('--test-dir')
        test_cmd.append(settings.JJS_PACK_TESTS_DIR)

        if options.quiet:
            test_cmd.append("-q")

        skip_list = []

        if options.skip_list:
            skip_list.append(options.skip_list)

        if skip_list:
            test_cmd.append("--skip-list=" + ",".join(skip_list))

        if job.test_args:
            test_cmd.extend(job.test_args)

        ret_test |= run_check(test_cmd, env=dict(TZ='UTC'))

    return ret_build | ret_test


def run_test262_test_suite(options):
    ret_build = ret_test = 0

    jobs = TEST262_TEST_SUITE_OPTIONS

    for job in jobs:
        ret_build, build_dir_path = create_binary(job, options)
        if ret_build:
            print("\n%sBuild failed%s\n" % (TERM_RED, TERM_NORMAL))
            break

        test_cmd = util.get_python_cmd_prefix() + [
            settings.TEST262_RUNNER_SCRIPT,
            '--engine', get_test262_binary_path(build_dir_path),
            '--test-dir', settings.TEST262_TEST_SUITE_DIR,
            '--harness-patch-dir', settings.TEST262_HARNESS_PATCH_DIR,
            '--mode', options.test262,
            '--job-count', str(options.test262_job_count),
        ]

        if job.test_args:
            test_cmd.extend(job.test_args)

        if options.test262_test_list:
            test_cmd.append('--test262-test-list')
            test_cmd.append(options.test262_test_list)

        ret_test |= run_check(test_cmd, env=dict(TZ='America/Los_Angeles'))

    return ret_build | ret_test

def run_unittests(options):
    ret_build = ret_test = 0
    for job in JJS_UNITTESTS_OPTIONS:
        if job.skip:
            report_skip(job)
            continue
        ret_build, build_dir_path = create_binary(job, options)
        if ret_build:
            print("\n%sBuild failed%s\n" % (TERM_RED, TERM_NORMAL))
            break

        if sys.platform == 'win32':
            if options.build_debug:
                build_config = "Debug"
            else:
                build_config = "MinSizeRel"
        else:
            build_config = ""

        ret_test |= run_check(
            util.get_python_cmd_prefix() +
            [settings.UNITTEST_RUNNER_SCRIPT] +
            (["--skip-list=" + options.skip_list] if options.skip_list else []) +
            [os.path.join(build_dir_path, 'tests', build_config)] +
            (["-q"] if options.quiet else []),
            cwd=settings.TESTS_DIR
        )

    return ret_build | ret_test

def run_buildoption_test(options):
    for job in JJS_BUILDOPTIONS:
        if job.skip:
            report_skip(job)
            continue

        ret, _ = create_binary(job, options)
        if ret:
            print("\n%sBuild failed%s\n" % (TERM_RED, TERM_NORMAL))
            break

    return ret

Check = collections.namedtuple('Check', ['enabled', 'runner', 'arg'])

def main(options):
    checks = [
        Check(options.check_signed_off, run_check, [settings.SIGNED_OFF_SCRIPT]
              + {'tolerant': ['--tolerant'], 'gh-actions': ['--gh-actions']}.get(options.check_signed_off, [])),
        Check(options.check_cppcheck, run_check, [settings.CPPCHECK_SCRIPT]),
        Check(options.check_doxygen, run_check, [settings.DOXYGEN_SCRIPT]),
        Check(options.check_pylint, run_check, [settings.PYLINT_SCRIPT]),
        Check(options.check_format, run_check, [settings.FORMAT_SCRIPT]),
        Check(options.check_license, run_check, [settings.LICENSE_SCRIPT]),
        Check(options.check_strings, run_check, [settings.STRINGS_SCRIPT]),
        Check(options.jjs_debugger, run_jjs_debugger_tests, options),
        Check(options.jjs_tests, run_jjs_tests, options),
        Check(options.jjs_snapshot_tests, run_jjs_snapshot_tests, options),
        Check(options.jjs_pack_tests, run_jjs_pack_tests, options),
        Check(options.test262, run_test262_test_suite, options),
        Check(options.unittests, run_unittests, options),
        Check(options.buildoption_test, run_buildoption_test, options),
    ]

    for check in checks:
        if check.enabled or options.all:
            ret = check.runner(check.arg)
            if ret:
                sys.exit(ret)

if __name__ == "__main__":
    main(get_arguments())
