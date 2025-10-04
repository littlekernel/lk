#!/usr/bin/env python3
import subprocess
import sys
import os
import time
import argparse
from pathlib import Path

class QEMUTestRunner:
    def __init__(self, lk_root):
        self.lk_root = Path(lk_root)
        self.architectures = {
            'arm': {
                'script': 'do-qemuarm',
                'args': '',
                'timeout': 30
            },
            'arm64': {
                'script': 'do-qemuarm',
                'args': '-6s4',
                'timeout': 30
            },
            'm68k': {
                'script': 'do-qemum68k',
                'args': '',
                'timeout': 30
            },
            'riscv32': {
                'script': 'do-qemuriscv',
                'args': '',
                'timeout': 30
            },
            'riscv64': {
                'script': 'do-qemuriscv',
                'args': '-6Ss4',
                'timeout': 30
            },
            'x86': {
                'script': 'do-qemux86',
                'args': '-s4',
                'timeout': 30
            },
            'x86-64': {
                'script': 'do-qemux86',
                'args': '-6s4',
                'timeout': 30
            }
        }

    def run_qemu_test(self, arch, arch_config, quiet=False):
        """Run QEMU for the specified architecture and monitor for test completion"""
        print(f"\nRunning QEMU test for {arch}...")

        script_path = self.lk_root / 'scripts' / arch_config['script']
        if not script_path.exists():
            print(f"Script {script_path} not found")
            return False

        # Create the QEMU commandline
        qemu_cmdline = [str(script_path)]
        if arch_config['args']:
            qemu_cmdline.append(str(arch_config['args']))

        if not quiet:
            print(f"Executing command: {' '.join(qemu_cmdline)}")

        # Run the QEMU script
        try:
            process = subprocess.Popen(
                qemu_cmdline,
                cwd=self.lk_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                universal_newlines=True,
                env={**os.environ, 'LK_ROOT': str(self.lk_root), 'RUN_UNITTESTS_AT_BOOT': '1'},
            )

            stdout = process.stdout
            if stdout is None:
                print("Failed to capture stdout")
                return False

            # Monitor output for success/failure indicators
            success_indicators = [
                "SUCCESS! All test cases passed",
            ]

            failure_indicators = [
                "FAILURE! Some test cases failed",
                "panic (caller"
                "CRASH: starting debug shell"
            ]

            start_time = time.time()
            output_lines = []
            test_passed = False
            test_failed = False

            while True:
                # Check timeout
                if time.time() - start_time > arch_config['timeout']:
                    print(f"Timeout reached for {arch}")
                    process.terminate()
                    break

                # Read a line of output
                # TODO: fix the situation where readline() blocks indefinitely because the process
                # has printed a partial line without a newline.
                line = stdout.readline()
                if not line and process.poll() is not None:
                    break

                if line:
                    line = line.removesuffix('\n')
                    output_lines.append(line)
                    if not quiet:
                        print(f"[{arch}] {line}")

                    # Check for success indicators
                    for indicator in success_indicators:
                        if indicator.lower() in line.lower():
                            test_passed = True
                            print(f"✓ Test success detected for {arch}")
                            break

                    # Check for failure indicators
                    for indicator in failure_indicators:
                        if indicator.lower() in line.lower():
                            test_failed = True
                            print(f"✗ Test failure detected for {arch}")
                            break

                    if test_passed or test_failed:
                        break

            # Clean up process
            if not quiet:
                print("Terminating QEMU process...")
            if process.poll() is None:
                process.terminate()
                time.sleep(1)
                if process.poll() is None:
                    process.kill()

            return test_passed and not test_failed

        except Exception as e:
            print(f"Error running QEMU for {arch}: {e}")
            return False

    def run_all_tests(self, selected_archs=None, quiet=False):
        """Run tests for all or selected architectures"""
        if selected_archs is None:
            selected_archs = list(self.architectures.keys())

        results = {}

        for arch in selected_archs:
            if arch not in self.architectures:
                print(f"Unknown architecture: {arch}")
                continue

            arch_config = self.architectures[arch]

            # Run the test
            results[arch] = self.run_qemu_test(arch, arch_config, quiet)

        return results

    def print_summary(self, results):
        """Print a summary of test results"""
        print("\n" + "="*50)
        print("TEST SUMMARY")
        print("="*50)

        total_tests = len(results)
        passed_tests = sum(1 for result in results.values() if result)

        for arch, passed in results.items():
            status = "PASSED" if passed else "FAILED"
            symbol = "✓" if passed else "✗"
            print(f"{symbol} {arch:10} {status}")

        print("-"*50)
        print(f"Total: {passed_tests}/{total_tests} architectures passed")

        if passed_tests == total_tests:
            print("🎉 All architectures passed!")
            return 0
        else:
            print("❌ Some architectures failed!")
            return 1

def main():
    parser = argparse.ArgumentParser(description='Run LK QEMU tests for multiple architectures')
    parser.add_argument('--arch', choices=['arm', 'arm64', 'm68k', 'riscv32', 'riscv64', 'x86', 'x86-64'], action='append',
                       help='Architecture to test (can be specified multiple times)')
    parser.add_argument('--lk-root', default='.',
                       help='Path to LK root directory (default: current directory)')
    parser.add_argument('--quiet', action='store_true',
                       help='Run tests in quiet mode (suppress output)')

    args = parser.parse_args()

    # Change to LK root directory
    lk_root = os.path.abspath(args.lk_root)
    if not os.path.exists(os.path.join(lk_root, 'makefile')) and not os.path.exists(os.path.join(lk_root, 'Makefile')):
        print(f"Error: {lk_root} doesn't appear to be the LK root directory")
        return 1

    runner = QEMUTestRunner(lk_root)

    # Run tests
    results = runner.run_all_tests(args.arch, args.quiet)

    # Print summary and return appropriate exit code
    return runner.print_summary(results)

if __name__ == '__main__':
    sys.exit(main())