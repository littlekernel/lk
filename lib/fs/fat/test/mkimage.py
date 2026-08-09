#!/usr/bin/env python3
"""Build the FAT test disk images.

Replaces the old mkblk shell script. The image geometries and the content tree
are data, not twenty unrolled mcopy lines, and the generated payloads carry a
position dependent pattern instead of zeroes so that a block written to the
wrong place is detectable.

Writes a JSON manifest next to each image describing what it should contain,
which run-fat-tests.py uses to verify the image after the guest has run.

Requires mkfs.fat (dosfstools) and mmd/mcopy (mtools).

  ./mkimage.py                 build every image that is out of date
  ./mkimage.py --type fat16    just one
  ./mkimage.py --force         rebuild even if up to date
  ./mkimage.py --huge          also build the 4GB FAT32 image
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# Image geometries. size_kb is what mkfs.fat -C takes (blocks of 1024 bytes).
#
# The FAT32 default is deliberately far smaller than the 4GB the old script
# built: nothing in the test suite depends on the size, and a 4GB file is a real
# cost for a CI runner even when sparse. --huge restores it.
# FAT32 needs at least 65525 data clusters to be legal, so the cluster size has
# to come down as the volume shrinks: at 4KB clusters a 256MB volume falls just
# short (65404) and mkfs.fat will still label it FAT32, producing an image that a
# spec-conformant driver mounts as FAT16. 2KB clusters give ~130k clusters and
# plenty of margin. check_geometry() below enforces this.
IMAGES = {
    'fat12': dict(fat_bits=12, size_kb=2000,       sector=512, cluster_sectors=2,  label='FAT12'),
    'fat16': dict(fat_bits=16, size_kb=16384,      sector=512, cluster_sectors=4,  label='FAT16'),
    'fat32': dict(fat_bits=32, size_kb=256 * 1024, sector=512, cluster_sectors=4,  label='FAT32'),
}
HUGE_IMAGES = {
    'fat32-huge': dict(fat_bits=32, size_kb=4 * 1024 * 1024, sector=512,
                       cluster_sectors=8, label='FAT32HUGE'),
}

# Files checked into this directory that the in-guest tests compare against
# byte for byte (they are pulled into rodata with INCFILE).
SOURCE_FILES = ['hello.txt', 'LICENSE', 'test_4kb.bin', 'test_8kb.bin']

# Generated payloads: name -> size in bytes. Filled with a seeded position
# dependent pattern, never zeroes -- a misplaced cluster usually reads back as
# zeroes, so a zero filled file cannot detect one.
GENERATED = {
    'largefile': 512 * 1024,
}

# The content tree, as (source, destination) pairs plus directories to create.
# 'source' is either a file in this directory or a generated payload.
DIRECTORIES = ['dir.a', 'dir.b', 'dir.c']

# Files that the in-guest tests deliberately delete. The verifier asserts these
# are *absent* afterwards, which proves the test that consumes them really ran.
# See test_fat_remove_file() in test.cpp.
CONSUMED = {'removable_long_filename_victim.txt'}

CONTENT = [
    ('hello.txt', '::hello.txt'),
    ('LICENSE', '::license'),
    ('hello.txt', '::long_filename_hello.txt'),
    ('hello.txt', '::removable_long_filename_victim.txt'),
    ('hello.txt', '::a_very_long_filename_hello_that_uses_at_least_a_few_entries.txt'),
    ('hello.txt', '::dir.a/long_filename_hello.txt'),
    ('largefile', '::largefile'),
    ('test_4kb.bin', '::test_4kb.bin'),
    ('test_8kb.bin', '::test_8kb.bin'),
]
# Enough long names in the root to push it past a single sector.
CONTENT += [('hello.txt', f'::01234longfilename{i}.txt') for i in range(20)]


def pattern_bytes(size, seed):
    """A position dependent byte pattern, matching pattern_byte() in the guest
    tests closely enough in spirit: every byte depends on its offset."""
    out = bytearray(size)
    for i in range(size):
        x = (seed ^ ((i * 2654435761) & 0xFFFFFFFF)) & 0xFFFFFFFF
        x ^= x >> 15
        x = (x * 2246822519) & 0xFFFFFFFF
        x ^= x >> 13
        out[i] = x & 0xFF
    return bytes(out)


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def require_tools():
    missing = [t for t in ('mkfs.fat', 'mmd', 'mcopy') if shutil.which(t) is None]
    if missing:
        sys.exit(f"missing required tools: {', '.join(missing)}\n"
                 f"install dosfstools and mtools")


def run(cmd, quiet):
    if not quiet:
        print('  ' + ' '.join(str(c) for c in cmd))
    subprocess.run(cmd, check=True, capture_output=quiet)


def ensure_generated(out_dir, quiet):
    """Create the generated payloads if missing or the wrong size."""
    for name, size in GENERATED.items():
        path = os.path.join(out_dir, name)
        if os.path.exists(path) and os.path.getsize(path) == size:
            continue
        if not quiet:
            print(f"generating {name} ({size} bytes of pattern)")
        with open(path, 'wb') as f:
            f.write(pattern_bytes(size, seed=0xF00DBEEF))


def read_layout(path):
    """Pull the geometry back out of an image's BPB and compute the data cluster
    count the same way a driver does."""
    import struct
    with open(path, 'rb') as f:
        b = f.read(512)
    u16 = lambda o: struct.unpack_from('<H', b, o)[0]
    u32 = lambda o: struct.unpack_from('<I', b, o)[0]
    bps, spc, rsvd, nfat, rent = u16(0x0b), b[0x0d], u16(0x0e), b[0x10], u16(0x11)
    total = u16(0x13) or u32(0x20)
    spf = u16(0x16) or u32(0x24)
    root_sectors = (rent * 32 + bps - 1) // bps
    clusters = (total - (rsvd + nfat * spf + root_sectors)) // spc
    return dict(bytes_per_sector=bps, sectors_per_cluster=spc, total_sectors=total,
                sectors_per_fat=spf, root_entries=rent, data_clusters=clusters)


def check_geometry(name, spec, img_path):
    """Make sure the image really is the FAT width we asked for.

    mkfs.fat honours -F even when the resulting cluster count is out of range for
    that width, which yields an image a conformant driver mounts at a different
    width. Catch that here rather than as a pile of confusing test failures.
    """
    layout = read_layout(img_path)
    clusters = layout['data_clusters']

    if clusters < 4085:
        actual = 12
    elif clusters < 65525:
        actual = 16
    else:
        actual = 32

    if actual != spec['fat_bits']:
        sys.exit(f"{name}: asked for FAT{spec['fat_bits']} but the layout has "
                 f"{clusters} data clusters, which is FAT{actual}.\n"
                 f"adjust size_kb or cluster_sectors: FAT12 needs <4085 clusters, "
                 f"FAT16 4085..65524, FAT32 >=65525.")
    return layout


def source_path(out_dir, name):
    """Generated payloads live in out_dir, checked in sources live here."""
    if name in GENERATED:
        return os.path.join(out_dir, name)
    return os.path.join(HERE, name)


def image_is_current(img_path, manifest_path, out_dir):
    """An image is current if it and its manifest exist and are newer than every
    input that goes into them."""
    if not os.path.exists(img_path) or not os.path.exists(manifest_path):
        return False
    img_mtime = os.path.getmtime(img_path)
    inputs = [os.path.abspath(__file__)]
    inputs += [source_path(out_dir, n) for n in SOURCE_FILES]
    inputs += [source_path(out_dir, n) for n in GENERATED]
    for dep in inputs:
        if os.path.exists(dep) and os.path.getmtime(dep) > img_mtime:
            return False
    return True


def build_image(name, spec, out_dir, quiet, force):
    img_path = os.path.join(out_dir, f'blk.bin.{name}')
    manifest_path = img_path + '.manifest.json'

    if not force and image_is_current(img_path, manifest_path, out_dir):
        if not quiet:
            print(f"{name}: up to date")
        return manifest_path

    print(f"building {name} ({spec['size_kb']} KB, FAT{spec['fat_bits']}, "
          f"{spec['sector']} byte sectors, {spec['cluster_sectors']} sectors/cluster)")

    if os.path.exists(img_path):
        os.unlink(img_path)

    run(['mkfs.fat',
         '-C', img_path,
         '-F', str(spec['fat_bits']),
         '-S', str(spec['sector']),
         '-s', str(spec['cluster_sectors']),
         '-n', spec['label'],
         str(spec['size_kb'])], quiet)

    layout = check_geometry(name, spec, img_path)
    if not quiet:
        print(f"  {layout['data_clusters']} data clusters, "
              f"{layout['sectors_per_fat']} sectors per FAT")

    for d in DIRECTORIES:
        run(['mmd', '-i', img_path, d], quiet)

    entries = []
    for src, dst in CONTENT:
        src_path = source_path(out_dir, src)
        run(['mcopy', '-i', img_path, src_path, dst], quiet)
        path = dst[2:]  # strip the mtools "::" prefix
        entries.append({
            'path': path,
            'source': src,
            'size': os.path.getsize(src_path),
            'sha256': sha256_of(src_path),
            'consumed': path in CONSUMED,
        })

    manifest = {
        'image': os.path.basename(img_path),
        'geometry': spec,
        'layout': layout,
        'directories': DIRECTORIES,
        'files': entries,
    }
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)

    return manifest_path


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--type', action='append', dest='types',
                   help='image type to build (default: all). May be repeated.')
    p.add_argument('--out-dir', default=HERE,
                   help='where to write images (default: this directory)')
    p.add_argument('--force', action='store_true', help='rebuild even if up to date')
    p.add_argument('--huge', action='store_true',
                   help='also build the 4GB FAT32 image')
    p.add_argument('--quiet', '-q', action='store_true')
    args = p.parse_args()

    require_tools()

    available = dict(IMAGES)
    if args.huge:
        available.update(HUGE_IMAGES)

    types = args.types or list(available)
    for t in types:
        if t not in available:
            sys.exit(f"unknown image type '{t}' (choose from {', '.join(available)})")

    os.makedirs(args.out_dir, exist_ok=True)
    ensure_generated(args.out_dir, args.quiet)

    for t in types:
        build_image(t, available[t], args.out_dir, args.quiet, args.force)

    return 0


if __name__ == '__main__':
    sys.exit(main())
