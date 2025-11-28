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

    def run_qemu_test(self, arch, arch_config, quiet=False, log_dir=None):
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
                "panic (caller",
                "CRASH: starting debug shell"
            ]

            start_time = time.time()
            output_lines = []
            buffer = ''
            test_passed = False
            test_failed = False

            # Use select + nonblocking fd to avoid blocking on partial lines
            import select, fcntl, os as _os
            fd = stdout.fileno()
            flags = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

            while True:
                # Check timeout
                if time.time() - start_time > arch_config['timeout']:
                    print(f"Timeout reached for {arch}")
                    process.terminate()
                    break

                # If process exited and buffer empty, stop
                if process.poll() is not None and not buffer:
                    break

                # Wait up to 1s for data to arrive
                rlist, _, _ = select.select([fd], [], [], 1)
                if rlist:
                    try:
                        chunk = _os.read(fd, 4096).decode(errors='replace')
                    except BlockingIOError:
                        chunk = ''
                    if chunk == '':
                        # EOF
                        if process.poll() is not None:
                            break
                    else:
                        buffer += chunk

                # Emit full lines from buffer
                while True:
                    if '\n' not in buffer:
                        break
                    line, buffer = buffer.split('\n', 1)
                    output_lines.append(line)
                    if not quiet:
                        print(f"[{arch}] {line}")
                        sys.stdout.flush()

                    # Check for success indicators
                    for indicator in success_indicators:
                        if indicator.lower() in line.lower():
                            test_passed = True
                            print(f"✓ Test success detected for {arch}")
                            sys.stdout.flush()
                            break

                    # Check for failure indicators
                    for indicator in failure_indicators:
                        if indicator.lower() in line.lower():
                            test_failed = True
                            print(f"✗ Test failure detected for {arch}")
                            sys.stdout.flush()
                            break

                    if test_passed or test_failed:
                        break

                if test_passed or test_failed:
                    break

            # Emit any trailing partial line for visibility
            if buffer:
                tail = buffer.rstrip('\n')
                if tail:
                    output_lines.append(tail)
                    if not quiet:
                        print(f"[{arch}] {tail}")
                        sys.stdout.flush()

            # Clean up process
            if not quiet:
                print("Terminating QEMU process...")
                sys.stdout.flush()
            if process.poll() is None:
                process.terminate()
                time.sleep(1)
                if process.poll() is None:
                    process.kill()

            # If requested, write captured output to a log file
            if log_dir:
                try:
                    log_path = Path(log_dir) / f"{arch}.log"
                    log_path.parent.mkdir(parents=True, exist_ok=True)
                    with open(log_path, 'w', encoding='utf-8') as f:
                        f.write("\n".join(output_lines))
                    if not quiet:
                        print(f"[{arch}] wrote log to {log_path}")
                        sys.stdout.flush()
                except Exception as le:
                    print(f"[{arch}] failed to write log: {le}")
                    sys.stdout.flush()

            return test_passed and not test_failed

        except Exception as e:
            print(f"Error running QEMU for {arch}: {e}")
            sys.stdout.flush()
            return False

    def run_all_tests(self, selected_archs=None, quiet=False, log_dir=None):
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
            results[arch] = self.run_qemu_test(arch, arch_config, quiet, log_dir)

        return results

    def print_summary(self, results, log_dir=None):
        """Print a summary of test results and optionally write to summary.txt"""
        summary_lines = []
        summary_lines.append("" + "="*50)
        summary_lines.append("TEST SUMMARY")
        summary_lines.append("="*50)

        total_tests = len(results)
        passed_tests = sum(1 for result in results.values() if result)

        for arch, passed in results.items():
            status = "PASSED" if passed else "FAILED"
            symbol = "✓" if passed else "✗"
            line = f"{symbol} {arch:10} {status}"
            print(line)
            summary_lines.append(line)

        print("-"*50)
        summary_lines.append("-"*50)
        print(f"Total: {passed_tests}/{total_tests} architectures passed")
        summary_lines.append(f"Total: {passed_tests}/{total_tests} architectures passed")

        if passed_tests == total_tests:
            print("🎉 All architectures passed!")
            summary_lines.append("All architectures passed!")
            exit_code = 0
        else:
            print("❌ Some architectures failed!")
            summary_lines.append("Some architectures failed!")
            exit_code = 1

        # Optionally write summary.txt
        if log_dir:
            try:
                log_path = Path(log_dir)
                log_path.mkdir(parents=True, exist_ok=True)
                summary_file = log_path / "summary.txt"
                with open(summary_file, 'w', encoding='utf-8') as f:
                    f.write("\n".join(summary_lines) + "\n")
                print(f"Summary written to {summary_file}")
            except Exception as e:
                print(f"Failed to write summary: {e}")

        return exit_code

def main():
    parser = argparse.ArgumentParser(description='Run LK QEMU tests for multiple architectures')
    parser.add_argument('--arch', choices=['arm', 'arm64', 'm68k', 'riscv32', 'riscv64', 'x86', 'x86-64'], action='append',
                       help='Architecture to test (can be specified multiple times)')
    parser.add_argument('--lk-root', default='.',
                       help='Path to LK root directory (default: current directory)')
    parser.add_argument('--quiet', action='store_true',
                       help='Run tests in quiet mode (suppress output)')
    parser.add_argument('--log-dir', default=None,
                       help='Directory to write per-architecture logs (optional)')

    args = parser.parse_args()

    # Change to LK root directory
    lk_root = os.path.abspath(args.lk_root)
    if not os.path.exists(os.path.join(lk_root, 'makefile')) and not os.path.exists(os.path.join(lk_root, 'Makefile')):
        print(f"Error: {lk_root} doesn't appear to be the LK root directory")
        return 1

    runner = QEMUTestRunner(lk_root)

    # Run tests
    results = runner.run_all_tests(args.arch, args.quiet, args.log_dir)

    # Print summary and return appropriate exit code
    return runner.print_summary(results, args.log_dir)

if __name__ == '__main__':
    sys.exit(main())