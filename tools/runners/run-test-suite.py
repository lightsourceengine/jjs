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
import os
import subprocess
import sys
import util

import multiprocessing
import signal
from itertools import count

def get_arguments():
    execution_runtime = os.environ.get('RUNTIME')
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Only print out failing tests')
    parser.add_argument('--runtime', metavar='FILE', default=execution_runtime,
                        help='Execution runtime (e.g. qemu)')
    parser.add_argument('--engine', metavar='FILE',
                        help='JJS binary to run tests with')
    parser.add_argument('--test-list', metavar='FILE',
                        help='File contains test paths to run')
    parser.add_argument('--skip-list', metavar='LIST',
                        help='Add a comma separated list of patterns of the excluded JS-tests')
    parser.add_argument('--test-dir', metavar='DIR',
                        help='Directory contains tests to run')
    parser.add_argument('--snapshot', action='store_true',
                        help='Snapshot test')
    parser.add_argument('--pmap', metavar='FILE', help='JJS engine pmap file')

    script_args = parser.parse_args()
    if script_args.skip_list:
        script_args.skip_list = script_args.skip_list.split(',')
    else:
        script_args.skip_list = []

    return script_args


def get_tests(test_dir, test_list, skip_list):
    tests = []
    if test_dir:
        tests = []
        for root, _, files in os.walk(test_dir):
            for test_file in files:
                if test_file.endswith('.js') or test_file.endswith('.mjs'):
                    tests.extend([os.path.normpath(os.path.join(root, test_file))])

    if test_list:
        dirname = os.path.dirname(test_list)
        with open(test_list, "r") as test_list_fd:
            for test in test_list_fd:
                tests.append(os.path.normpath(os.path.join(dirname, test.rstrip())))

    tests.sort()

    def filter_tests(test):
        for skipped in skip_list:
            if skipped in test:
                return False
        container = os.path.dirname(test)
        if container.endswith('lib') or container.endswith('exclude') or os.path.normpath('tests/jjs/fixtures') in container:
            return False
        return True

    return [test for test in tests if filter_tests(test)]


def execute_test_command(test_cmd, cwd):
    kwargs = {'encoding': 'unicode_escape', 'errors': 'replace'}
    process = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               universal_newlines=True, cwd=cwd, **kwargs)
    stdout = process.communicate()[0]
    return (process.returncode, stdout)


def main(args):
    tests = get_tests(args.test_dir, args.test_list, args.skip_list)

    total = len(tests)
    if total == 0:
        print("No test to execute.")
        return 1

    if sys.platform == 'win32':
        original_timezone = util.get_timezone()
        util.set_sighdl_to_reset_timezone(original_timezone)
        util.set_timezone('UTC')

    if args.snapshot:
        passed = run_snapshot_tests(args, tests)
    else:
        passed = run_normal_tests(args, tests)

    if sys.platform == 'win32':
        util.set_timezone(original_timezone)

    failed = total - passed

    summary_list = [os.path.relpath(args.engine)]
    if args.snapshot:
        summary_list.append('--snapshot')
    if args.test_dir:
        summary_list.append(os.path.relpath(args.test_dir))
    if args.test_list:
        summary_list.append(os.path.relpath(args.test_list))
    util.print_test_summary(' '.join(summary_list), total, passed, failed)

    return bool(failed)


class TestCase:
    def __init__(self, test, cmd, test_dir):
        self.test = test
        self.cmd = cmd
        self.test_dir = test_dir
        self.code = -1
        self.stdout = ''

    def run(self):
        test_argument = ['--loader', 'esm' if self.test.endswith('.mjs') else 'sloppy']

        (returncode, stdout) = execute_test_command(self.cmd + test_argument + [self.test], self.test_dir)

        self.code = returncode
        self.stdout = stdout

        return self


snapshot_counter = count(start=1)


class SnapshotTestCase(TestCase):
    def __init__(self, test, generate_snapshot_cmd, execute_snapshot_cmd, test_dir):
        super().__init__(test, '', test_dir)
        self.unique = next(snapshot_counter)
        self.snapshot_failed = False
        self.generate_snapshot_cmd = generate_snapshot_cmd
        self.execute_snapshot_cmd = execute_snapshot_cmd

    def run(self):
        # TODO: should not go in source root
        snapshot_filename = os.path.join(self.test_dir, 'js-{}.snapshot'.format(self.unique))
        (returncode, stdout) = execute_test_command(
            self.generate_snapshot_cmd + ['-o', snapshot_filename] + [self.test], self.test_dir)

        if returncode != 0:
            self.code = returncode
            self.stdout = stdout
            self.snapshot_failed = True
            return self

        (returncode, stdout) = execute_test_command(self.execute_snapshot_cmd + [snapshot_filename], self.test_dir)
        os.remove(snapshot_filename)

        self.code = returncode
        self.stdout = stdout

        return self


def run_normal_tests(args, tests):
    test_dir = args.test_dir
    test_cmd = util.get_platform_cmd_prefix()
    if args.runtime:
        test_cmd.append(args.runtime)

    test_cmd.extend([args.engine, 'test'])

    if args.pmap:
        test_cmd.extend(['--pmap', args.pmap])

    total = len(tests)
    tested = 0
    passed = 0

    job_count = multiprocessing.cpu_count()
    pool = multiprocessing.Pool(processes=job_count, initializer=pool_init)

    try:
        for case in pool.imap(test_case_run, map(lambda x: TestCase(x, test_cmd, test_dir), tests)):
            tested += 1
            test_path = os.path.relpath(case.test)
            is_expected_to_fail = os.path.join(os.path.sep, 'fail', '') in case.test
            code = case.code

            if (code == 0 and not is_expected_to_fail) or (code == 1 and is_expected_to_fail):
                passed += 1
                if not args.quiet:
                    passed_string = 'PASS' + (' (XFAIL)' if is_expected_to_fail else '')
                    util.print_test_result(tested, total, True, passed_string, test_path)
            else:
                passed_string = 'FAIL%s (%d)' % (' (XPASS)' if code == 0 and is_expected_to_fail else '', code)
                util.print_test_result(tested, total, False, passed_string, test_path)
                print("================================================")
                print(case.stdout)
                print("================================================")
    except KeyboardInterrupt:
        pool.terminate()
        pool.join()

    return passed


def run_snapshot_tests(args, tests):
    execute_snapshot_cmd = util.get_platform_cmd_prefix()
    generate_snapshot_cmd = util.get_platform_cmd_prefix()
    if args.runtime:
        execute_snapshot_cmd.append(args.runtime)
        generate_snapshot_cmd.append(args.runtime)

    execute_snapshot_cmd = [args.engine, 'test']

    if args.pmap:
        execute_snapshot_cmd.extend(['--pmap', args.pmap])

    # engine: jjs[.exe] -> snapshot generator: jjs-snapshot[.exe]
    engine = os.path.splitext(args.engine)
    generate_snapshot_cmd.append(engine[0] + '-snapshot' + engine[1])
    generate_snapshot_cmd.append('generate')

    total = len(tests)
    tested = 0
    passed = 0

    job_count = multiprocessing.cpu_count()
    pool = multiprocessing.Pool(processes=job_count, initializer=pool_init)

    def gen(test):
        return SnapshotTestCase(test, generate_snapshot_cmd, execute_snapshot_cmd, args.test_dir)

    try:
        for case in pool.imap(test_case_run, map(gen, tests)):
            tested += 1
            test_path = os.path.relpath(case.test)
            is_expected_to_fail = os.path.join(os.path.sep, 'fail', '') in case.test

            if (case.code == 0 and not is_expected_to_fail) or (case.code == 1 and is_expected_to_fail):
                passed += 1
                if not args.quiet:
                    passed_string = 'PASS' + (' (XFAIL)' if is_expected_to_fail else '')
                    util.print_test_result(tested, total, True, passed_string, test_path, False)
            else:
                passed_string = 'FAIL%s (%d)' % (' (XPASS)' if case.code == 0 and is_expected_to_fail else '', case.code)
                util.print_test_result(tested, total, False, passed_string, test_path, False)
                print("================================================")
                print(case.stdout)
                print("================================================")
    except KeyboardInterrupt:
        pool.terminate()
        pool.join()

    return passed


def pool_init():
    """Ignore CTRL+C in the worker process."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def test_case_run(case):
    return case.run()


if __name__ == "__main__":
    sys.exit(main(get_arguments()))
