#!/usr/bin/env python3
"""Run the FAT filesystem tests against real disk images.

Replaces run-fat-tests.sh. What it does differently:

  * Works on a scratch copy of each image, so the pristine image is reusable and
    a run does not have to rebuild a large image every time. Combined with the
    in-guest tests cleaning up after themselves, runs are repeatable.
  * Passes test.fat.required=1 so a misconfigured run fails instead of silently
    skipping every device backed test.
  * Verifies content, not just structure. fsck.fat checks the metadata graph;
    this also compares the files mkimage.py put there and the witness tree the
    guest wrote, byte for byte, using mtools.
  * Is not hardcoded to arm64.

  ./scripts/run-fat-tests.py                      all three images on arm64
  ./scripts/run-fat-tests.py --type fat16         just one
  ./scripts/run-fat-tests.py --arch x86-64        a different architecture
  ./scripts/run-fat-tests.py --stress             include the stress suite
  ./scripts/run-fat-tests.py --keep               keep the scratch images
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LK_ROOT = os.path.dirname(SCRIPT_DIR)
FAT_TEST_DIR = os.path.join(LK_ROOT, 'lib', 'fs', 'fat', 'test')
MKIMAGE = os.path.join(FAT_TEST_DIR, 'mkimage.py')
BOOT_TESTS = os.path.join(SCRIPT_DIR, 'run-qemu-boot-tests.py')

# What the first attached disk is called inside the guest. The virtio backed
# machines enumerate it as virtio0; the x86 machines default to the q35 AHCI
# controller, where dev/block/ahci names it <controller>.<port>.
ARCH_DEVICE = {
    'arm': 'virtio0',
    'arm64': 'virtio0',
    'm68k': 'virtio0',
    'riscv32': 'virtio0',
    'riscv64': 'virtio0',
    'x86': 'ahci0.0',
    'x86-64': 'ahci0.0',
}

# The tree test_fat_witness() in lib/fs/fat/test/test.cpp leaves behind. Keep in
# step with the comment above that function.
WITNESS_DIR = 'witness'
WITNESS_FILES = {
    'witness/small.txt': b'witness small',
    'witness/a_long_witness_file_name_that_needs_lfn.txt': b'hello',
    'witness/nested/deep.txt': b'deep',
    'witness/pattern.bin': bytes((i * 7 + 11) & 0xff for i in range(9000)),
}


def require_tools(names):
    missing = [t for t in names if shutil.which(t) is None]
    if missing:
        sys.exit(f"missing required tools: {', '.join(missing)}\n"
                 f"install dosfstools and mtools")


def build_images(types, force, quiet):
    cmd = [sys.executable, MKIMAGE]
    for t in types:
        cmd += ['--type', t]
    if force:
        cmd.append('--force')
    if quiet:
        cmd.append('--quiet')
    subprocess.run(cmd, check=True)


def mtools_read(image, path):
    """Read a file out of the image with mtools. Returns bytes, or None."""
    r = subprocess.run(['mcopy', '-i', image, f'::{path}', '-'],
                       capture_output=True)
    if r.returncode != 0:
        return None
    return r.stdout


def verify_manifest(image, manifest_path, quiet):
    """Every file mkimage.py put in the image must still be byte for byte
    correct after the guest has had its way with the volume."""
    with open(manifest_path) as f:
        manifest = json.load(f)

    problems = []
    checked = consumed = 0
    for entry in manifest['files']:
        data = mtools_read(image, entry['path'])

        if entry.get('consumed'):
            # the guest is supposed to have deleted this one; its absence is
            # evidence that the test which removes it actually ran
            if data is not None:
                problems.append(f"{entry['path']}: still present, but a test "
                                f"should have removed it")
            consumed += 1
            continue

        if data is None:
            problems.append(f"{entry['path']}: could not be read back")
            continue
        if len(data) != entry['size']:
            problems.append(f"{entry['path']}: size {len(data)}, expected {entry['size']}")
            continue
        got = hashlib.sha256(data).hexdigest()
        if got != entry['sha256']:
            problems.append(f"{entry['path']}: content differs from the original")
        checked += 1

    if not quiet and not problems:
        print(f"  content: {checked} pre-existing files verified, "
              f"{consumed} correctly consumed")
    return problems


def list_image(image):
    """Every path in the image, as mtools sees it. Directories keep a trailing
    slash, which is how we tell them apart."""
    r = subprocess.run(['mdir', '-b', '-/', '-i', image, '::'],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return None
    paths = set()
    for line in r.stdout.splitlines():
        line = line.strip()
        if not line.startswith('::/'):
            continue
        # A name that fits 8.3 is stored as an upper case short name with no long
        # name entry to preserve case, and FAT lookup is case insensitive, so
        # compare that way. Two names differing only in case cannot coexist.
        paths.add(line[len('::/'):].lower())
    return paths


def verify_nothing_leaked(image, manifest_path, quiet):
    """The guest must leave the volume exactly as it found it, apart from the
    witness tree. Anything else is a test that failed to clean up, or worse, a
    directory entry the driver created and lost track of.

    verify_manifest() only checks that the expected files are still right; this
    is the other half, and it is the half that catches leaks.
    """
    with open(manifest_path) as f:
        manifest = json.load(f)

    expected = set()
    for d in manifest['directories']:
        expected.add((d + '/').lower())
    for entry in manifest['files']:
        if not entry.get('consumed'):
            expected.add(entry['path'].lower())
    expected.add((WITNESS_DIR + '/').lower())
    expected.add(f'{WITNESS_DIR}/nested/'.lower())
    expected.update(p.lower() for p in WITNESS_FILES)

    actual = list_image(image)
    if actual is None:
        return ["could not list the image"]

    problems = []
    for extra in sorted(actual - expected):
        problems.append(f"unexpected leftover on the volume: {extra}")
    for missing in sorted(expected - actual):
        problems.append(f"expected path is gone: {missing}")

    if not quiet and not problems:
        print(f"  layout: {len(actual)} paths, exactly as expected")
    return problems


def verify_witness(image, quiet):
    """The tree the guest wrote must be readable from the host and match byte
    for byte. This is what proves the guest's writes are really correct, which
    fsck alone cannot tell you."""
    problems = []
    for path, expected in WITNESS_FILES.items():
        data = mtools_read(image, path)
        if data is None:
            problems.append(f"{path}: missing (test_fat_witness did not run?)")
            continue
        if data != expected:
            if len(data) != len(expected):
                problems.append(f"{path}: size {len(data)}, expected {len(expected)}")
            else:
                bad = next(i for i in range(len(data)) if data[i] != expected[i])
                problems.append(f"{path}: first difference at byte {bad}: "
                                f"got {data[bad]:#04x}, expected {expected[bad]:#04x}")

    if not quiet and not problems:
        print(f"  witness: {len(WITNESS_FILES)} guest written files verified")
    return problems


def run_fsck(image, quiet):
    r = subprocess.run(['fsck.fat', '-vn', image], capture_output=True, text=True)
    out = r.stdout + r.stderr
    problems = []
    if r.returncode != 0:
        problems.append(f"fsck.fat exited {r.returncode}")
    # fsck exits 0 while still reporting recoverable damage, so look at what it
    # actually said. A leaked cluster is exactly the symptom of a driver that
    # allocates without recording the allocation.
    for marker in ('Reclaimed', 'Cluster chain', 'Contains a free cluster',
                   'has invalid', 'Bad file name', 'Broken directory'):
        if marker in out:
            problems.append(f"fsck.fat reported: {marker}")
    if not quiet and not problems:
        last = [l for l in out.splitlines() if l.strip()][-1]
        print(f"  fsck: {last.split(':')[-1].strip()}")
    return problems, out


def run_one(image_type, arch, timeout, stress, keep, quiet, scratch_dir):
    src = os.path.join(FAT_TEST_DIR, f'blk.bin.{image_type}')
    manifest = src + '.manifest.json'
    if not os.path.exists(src):
        print(f"{image_type}: image {src} does not exist")
        return False

    # Work on a copy so the pristine image survives and the run is repeatable.
    # --sparse=always matters: the huge FAT32 image is mostly holes.
    scratch = os.path.join(scratch_dir, f'{image_type}.img')
    subprocess.run(['cp', '--sparse=always', src, scratch], check=True)

    device = ARCH_DEVICE[arch]
    cmd = [BOOT_TESTS, '--arch', arch, '--timeout', str(timeout),
           '-A', f'test.fat.device={device}',
           '-A', 'test.fat.required=1',
           '-d', scratch]
    if stress:
        cmd += ['-A', 'test.fat.stress=1']
    if quiet:
        cmd.append('--quiet')

    print(f"\n=== {image_type} on {arch} (device {device}) ===")
    boot_ok = subprocess.run(cmd).returncode == 0

    problems = []
    fsck_problems, fsck_out = run_fsck(scratch, quiet)
    problems += fsck_problems
    problems += verify_manifest(scratch, manifest, quiet)
    problems += verify_witness(scratch, quiet)
    problems += verify_nothing_leaked(scratch, manifest, quiet)

    if problems:
        print(f"  {image_type}: image verification FAILED")
        for p in problems:
            print(f"    - {p}")
        if not quiet:
            print(fsck_out)

    ok = boot_ok and not problems
    print(f"  {image_type}: {'PASS' if ok else 'FAIL'}"
          f"{'' if boot_ok else ' (guest tests failed)'}")

    if keep:
        print(f"  scratch image kept at {scratch}")
    return ok


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--type', action='append', dest='types',
                   choices=['fat12', 'fat16', 'fat32'],
                   help='image type to test (default: all). May be repeated.')
    p.add_argument('--arch', default='arm64', choices=sorted(ARCH_DEVICE),
                   help='architecture to run under (default: arm64)')
    p.add_argument('--timeout', type=int, default=120,
                   help='per-image QEMU timeout in seconds (default: 120)')
    p.add_argument('--stress', action='store_true',
                   help='also run the fat_stress suite (much slower)')
    p.add_argument('--force-images', action='store_true',
                   help='rebuild the disk images even if up to date')
    p.add_argument('--keep', action='store_true',
                   help='keep the scratch images instead of deleting them')
    p.add_argument('--quiet', '-q', action='store_true')
    args = p.parse_args()

    require_tools(['mkfs.fat', 'fsck.fat', 'mcopy', 'mmd'])

    types = args.types or ['fat12', 'fat16', 'fat32']
    if args.stress and args.timeout < 600:
        args.timeout = 600

    print("=== building disk images ===")
    build_images(types, args.force_images, args.quiet)

    scratch_dir = tempfile.mkdtemp(prefix='lk-fat-')
    results = {}
    try:
        for t in types:
            results[t] = run_one(t, args.arch, args.timeout, args.stress,
                                 args.keep, args.quiet, scratch_dir)
    finally:
        if not args.keep:
            shutil.rmtree(scratch_dir, ignore_errors=True)

    print("\n" + "=" * 50)
    for t, ok in results.items():
        print(f"{'PASS' if ok else 'FAIL'}  {t}")
    print("=" * 50)

    failed = [t for t, ok in results.items() if not ok]
    if failed:
        print(f"FAILED: {', '.join(failed)}")
        return 1
    print("all FAT image tests passed")
    return 0


if __name__ == '__main__':
    sys.exit(main())
