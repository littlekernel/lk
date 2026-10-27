#!/usr/bin/env python3
"""Run the rofs tests against images scripts/mkrofs.py builds.

Generates a tree (nested and empty directories, a wide one, names that sort awkwardly, the
longest name rofs allows, files from empty to a few hundred KB), writes a MANIFEST into it
listing every directory and every file with its size and CRC-32, builds an image of it with
mkrofs.py at each alignment, checks it with mkrofs.py --list, then boots the qemu test project
with the image attached. The in-guest rofs_tests suite reads the manifest and checks every file
and every directory listing against it; test.rofs.required=1 makes a missing device a
failure rather than a skip. rofs is read-only, so the image must come back byte for byte as
it went in.

  ./scripts/run-rofs-tests.py                  arm64, every alignment
  ./scripts/run-rofs-tests.py --arch riscv64   a different architecture
  ./scripts/run-rofs-tests.py --align 4096     one alignment
"""

import argparse
import hashlib
import os
import random
import shutil
import subprocess
import sys
import tempfile
import zlib

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MKROFS = os.path.join(SCRIPT_DIR, 'mkrofs.py')
BOOT_TESTS = os.path.join(SCRIPT_DIR, 'run-qemu-boot-tests.py')

# what the first attached disk is called inside the guest; same table as run-fat-tests.py
ARCH_DEVICE = {
    'arm': 'virtio0',
    'arm64': 'virtio0',
    'm68k': 'virtio0',
    'riscv32': 'virtio0',
    'riscv64': 'virtio0',
    'x86': 'ahci0.0',
    'x86-64': 'ahci0.0',
}

ALIGNS = [1, 16, 4096]


def make_tree(top):
    """The test tree under top, the same every run."""
    rng = random.Random(0x524f4653)
    files = {
        'a.txt': b'hello, rofs\n',
        'empty-file': b'',
        'one': b'1',
        'block': bytes(rng.getrandbits(8) for _ in range(512)),
        'block+1': bytes(rng.getrandbits(8) for _ in range(513)),
        'bin/big': bytes(rng.getrandbits(8) for _ in range(300 * 1024 + 17)),
        'bin/small': b'small\n',
        'a/b/c/d/deep.txt': b'deep\n',
        'n' * 63: b'the longest name\n',
    }
    # bytewise order: 'B' < 'Z' < 'a' < 'a-b' < 'a.b' < 'a_b' < '~'
    for name in ['B', 'Z', 'a-b', 'a.b', 'a_b', '~']:
        files[f'sort/{name}'] = name.encode()
    for i in range(50):
        files[f'wide/w{i:03d}'] = bytes(rng.getrandbits(8) for _ in range(i * 37))
    for path, data in files.items():
        full = os.path.join(top, path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, 'wb') as f:
            f.write(data)
    os.makedirs(os.path.join(top, 'empty-dir'))
    os.makedirs(os.path.join(top, 'a/empty'))

    lines = ['d .']
    for dirpath, dirnames, filenames in os.walk(top):
        dirnames.sort()
        rel = os.path.relpath(dirpath, top).replace(os.sep, '/')
        if rel != '.':
            lines.append(f'd {rel}')
        for name in sorted(filenames):
            path = name if rel == '.' else f'{rel}/{name}'
            with open(os.path.join(dirpath, name), 'rb') as f:
                data = f.read()
            lines.append(f'f {len(data)} {zlib.crc32(data):08x} {path}')
    with open(os.path.join(top, 'MANIFEST'), 'w') as f:
        f.write('\n'.join(lines) + '\n')
    return len(lines)


def sha256_of(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def run_one(tree, align, arch, timeout, quiet, scratch_dir):
    image = os.path.join(scratch_dir, f'align{align}.rofs')
    subprocess.run([sys.executable, MKROFS, '-o', image, '-a', str(align), '-d', tree],
                   check=True)
    listing = subprocess.run([sys.executable, MKROFS, '--list', image], check=True,
                             capture_output=True, text=True)
    if not quiet:
        print(listing.stdout.splitlines()[0])
    before = sha256_of(image)

    device = ARCH_DEVICE[arch]
    cmd = [BOOT_TESTS, '--arch', arch, '--timeout', str(timeout),
           '-A', f'test.rofs.device={device}',
           '-A', 'test.rofs.required=1',
           '-d', image]
    if quiet:
        cmd.append('--quiet')

    print(f"\n=== align {align} on {arch} (device {device}) ===")
    boot_ok = subprocess.run(cmd).returncode == 0

    unchanged = sha256_of(image) == before
    if not unchanged:
        print(f"  align {align}: the image CHANGED during the run")
    ok = boot_ok and unchanged
    print(f"  align {align}: {'PASS' if ok else 'FAIL'}"
          f"{'' if boot_ok else ' (guest tests failed)'}")
    return ok


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--align', type=int, action='append', dest='aligns',
                   help=f'file alignment of the image (default: {ALIGNS}). May be repeated.')
    p.add_argument('--arch', default='arm64', choices=sorted(ARCH_DEVICE),
                   help='architecture to run under (default: arm64)')
    p.add_argument('--timeout', type=int, default=120,
                   help='per-image QEMU timeout in seconds (default: 120)')
    p.add_argument('--quiet', '-q', action='store_true')
    args = p.parse_args()

    scratch_dir = tempfile.mkdtemp(prefix='lk-rofs-')
    results = {}
    try:
        tree = os.path.join(scratch_dir, 'tree')
        os.makedirs(tree)
        n = make_tree(tree)
        print(f"=== test tree: {n} manifest lines ===")
        for align in args.aligns or ALIGNS:
            results[f'align {align}'] = run_one(tree, align, args.arch, args.timeout,
                                                 args.quiet, scratch_dir)
    finally:
        shutil.rmtree(scratch_dir, ignore_errors=True)

    print("\n" + "=" * 50)
    for run, ok in results.items():
        print(f"{'PASS' if ok else 'FAIL'}  {run}")
    print("=" * 50)

    failed = [r for r, ok in results.items() if not ok]
    if failed:
        print(f"FAILED: {', '.join(failed)}")
        return 1
    print("all rofs image tests passed")
    return 0


if __name__ == '__main__':
    sys.exit(main())
