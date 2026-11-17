#!/usr/bin/env python3
#
# Copyright (c) 2026 Travis Geiselbrecht
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
"""Write a disk image whose every 512 byte block names itself.

The bio_pattern unit test (lib/bio/test/bio_pattern_tests.c) finds any block device
whose block 0 carries the stamp and checks that every block it reads is the one it
asked for, which catches addressing bugs (CHS translation, multi sector batching,
LBA48 setup) that a plain read/write round trip hides. It also writes to a few
blocks and puts them back, so the image is a scratch disk: don't stamp a disk whose
contents matter.

Block layout, little endian:
  0..7    b'LKPATTRN'
  8..11   block number
  12..15  total blocks in the image
  16..511 (block * 7 + offset) & 0xff

Examples:
  scripts/mkpattern-image.py pattern.img --size 20M
  scripts/mkpattern-image.py fd.img --floppy 1.44M
  scripts/do-qemux86 -l -d pattern.img -A 'lk.autorun=ut+bio_pattern'
"""

import argparse
import struct
import sys

BLOCK = 512
MAGIC = b'LKPATTRN'

FLOPPY_SIZES = {
    '360K': 360 * 1024,
    '720K': 720 * 1024,
    '1.2M': 1200 * 1024,
    '1.44M': 1440 * 1024,
    '2.88M': 2880 * 1024,
}


def parse_size(text):
    mult = 1
    suffix = text[-1].upper()
    if suffix in 'KMG':
        mult = {'K': 1024, 'M': 1024 * 1024, 'G': 1024 * 1024 * 1024}[suffix]
        text = text[:-1]
    return int(text) * mult


def block_bytes(block, total):
    fill = bytes(((block * 7 + i) & 0xff) for i in range(16, BLOCK))
    return MAGIC + struct.pack('<II', block, total) + fill


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('image', help='output file (overwritten)')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--size', help='image size, e.g. 20M or 40960K')
    group.add_argument('--floppy', choices=sorted(FLOPPY_SIZES), help='a floppy sized image')
    args = parser.parse_args()

    size = FLOPPY_SIZES[args.floppy] if args.floppy else parse_size(args.size)
    if size % BLOCK or size == 0:
        sys.exit(f'size {size} is not a positive multiple of {BLOCK}')
    total = size // BLOCK

    with open(args.image, 'wb') as f:
        for block in range(total):
            f.write(block_bytes(block, total))

    print(f'{args.image}: {total} blocks of {BLOCK} bytes')


if __name__ == '__main__':
    main()
