import os
import sys
import signal
import subprocess
from subprocess import PIPE, STDOUT
import io
import time
import select


def wait_for_output(fp: io.TextIOBase, predicate, timeout: float):
    start_time = time.time()
    end_time = start_time + timeout
    poll_obj = select.poll()
    poll_obj.register(fp, select.POLLIN)
    output = []
    cur_line = ""
    while time.time() < end_time:
        poll_result = poll_obj.poll(max(end_time-time.time(), 0.001))
        if poll_result:
            data = fp.read()
            if "\n" in data:
                output.append(cur_line + data[:data.index("\n") + 1])
                cur_line = data[data.index("\n") + 1:]
                if predicate(output[-1]):
                    return True, output
            else:
                cur_line += data

    return False, output


def shutdown_little_kernel(p: subprocess.Popen):
    try:
        ret = p.poll()
        if ret:
            print("LittleKernel already exited with code", ret)
            return
        status_path = "/proc/{}/status".format(p.pid)
        if os.path.exists(status_path):
            with open(status_path) as fp:
                lines = fp.readlines()
                state_line = [l for l in lines if "State:" in l]
                if state_line:
                    print("LittleKernel process state after test:",state_line[0].rstrip())
        else:
            print(status_path, "does not exists")
        p.stdin.write("poweroff\n")
        p.stdin.flush()
        p.wait(0.3)
        p.send_signal(signal.SIGINT)
        p.wait(1)
    except subprocess.TimeoutExpired:
        pass
    finally:
        p.kill()
        p.wait()


def main():
    # Test relies on reading subprocess output, so set bufsize=0
    # to ensure that we get real-time output.
    p = subprocess.Popen(['qemu-system-aarch64',
                          '-cpu',
                          'max',
                          '-m',
                          '512',
                          '-smp',
                          '1',
                          '-machine',
                          'virt,highmem=off',
                          '-kernel',
                          'build-qemu-virt-arm64-test/lk.elf',
                          '-net',
                          'none',
                          '-nographic',
                          '-drive',
                          'if=none,file=lib/uefi/helloworld_aa64.efi,id=blk,format=raw',
                          '-device',
                          'virtio-blk-device,drive=blk'], stdout=PIPE, stdin=PIPE, stderr=STDOUT, text=True, bufsize=0)
    try:
        os.set_blocking(p.stdout.fileno(), False)
        condition_met, output = wait_for_output(
            p.stdout, lambda l: "starting app shell" in l, 5)
        assert condition_met, "Did not see 'starting app shell', stdout: {}".format(
            "".join(output))
        p.stdin.write("uefi_load virtio0\n")
        p.stdin.flush()
        condition_met, output = wait_for_output(
            p.stdout, lambda l: "Hello World!" in l, 0.5)
        print("".join(output))
        if condition_met:
            return

    finally:
        shutdown_little_kernel(p)


if __name__ == "__main__":
    main()
