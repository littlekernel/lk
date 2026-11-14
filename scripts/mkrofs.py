#!/usr/bin/env python3
"""Build an rofs image: a read-only file system lib/fs/rofs mounts over any block device.

The format is documented in lib/fs/rofs/include/lib/fs/rofs.h. The image holds the files
given and every directory leading to them; directories list their entries sorted bytewise,
and each file starts at a multiple of the alignment. The output depends only on the inputs,
so the same tree always builds the same bytes.

  mkrofs.py -o out.rofs -d tree/               a directory tree, at the image's root
  mkrofs.py -o out.rofs build/app:bin/app ...  files as src:dst pairs
  mkrofs.py -o out.rofs -m manifest            the same pairs, one per line ('#' comments)
  mkrofs.py -o out.rofs -a 4096 ...            page aligned files, to map them in place
  mkrofs.py --list out.rofs                    check an image and print what it holds

The sources combine; a destination named twice, or both a file and a directory, is an error.
The image is padded to a whole number of 512 byte blocks.
"""

import argparse
import os
import struct
import sys

MAGIC = 0x53464F52
VERSION = 1
NAME_MAX = 63
TYPE_FILE = 1
TYPE_DIR = 2
HEADER = struct.Struct('<IIIIIIQQQ')
ENTRY = struct.Struct('<IHBBQQ')
BLOCK = 512

assert HEADER.size == 48 and ENTRY.size == 24


class Error(Exception):
    pass


def split_path(dst):
    """The components of an image path, checked the way the driver checks names."""
    parts = [p for p in dst.split('/') if p]
    if not parts:
        raise Error(f"'{dst}': no file name")
    for p in parts:
        b = p.encode()
        if p in ('.', '..') or b'\0' in b:
            raise Error(f"'{dst}': '{p}' cannot be a name")
        if len(b) > NAME_MAX:
            raise Error(f"'{dst}': '{p}' is longer than {NAME_MAX} bytes")
    return parts


class Tree:
    def __init__(self):
        self.root = {}  # name -> dict (a directory) or str (a file's source path)

    def add_file(self, src, dst):
        if not os.path.isfile(src):
            raise Error(f"'{src}' is not a file")
        parts = split_path(dst)
        d = self.root
        for i, p in enumerate(parts[:-1]):
            d = d.setdefault(p, {})
            if not isinstance(d, dict):
                raise Error(f"'{dst}': '{'/'.join(parts[:i + 1])}' is a file")
        name = parts[-1]
        if name in d:
            raise Error(f"'{dst}' is named twice")
        d[name] = src

    def add_dir(self, top):
        if not os.path.isdir(top):
            raise Error(f"'{top}' is not a directory")
        for dirpath, dirnames, filenames in os.walk(top):
            dirnames.sort()
            rel = os.path.relpath(dirpath, top)
            prefix = '' if rel == '.' else rel.replace(os.sep, '/') + '/'
            # empty directories too
            if prefix:
                d = self.root
                for p in split_path(prefix):
                    d = d.setdefault(p, {})
                    if not isinstance(d, dict):
                        raise Error(f"'{prefix}' is both a file and a directory")
            for f in sorted(filenames):
                self.add_file(os.path.join(dirpath, f), prefix + f)


def build(tree, align):
    """The image's bytes."""
    # the index in breadth first order: each directory's run follows every entry before it
    entries = []  # [name bytes, type, src or child dict]
    runs = []     # (index of the directory entry or None for the root, its dict)

    def add_run(d):
        first = len(entries)
        for name in sorted(d, key=lambda n: n.encode()):
            v = d[name]
            entries.append([name.encode(), TYPE_DIR if isinstance(v, dict) else TYPE_FILE, v])
        return first, len(d)

    root_first, root_count = add_run(tree.root)
    assert root_first == 0
    dir_runs = {}
    i = 0
    while i < len(entries):
        name, typ, v = entries[i]
        if typ == TYPE_DIR:
            dir_runs[i] = add_run(v)
        i += 1

    names = bytearray()
    name_offsets = []
    names_base = len(entries) * ENTRY.size
    for name, _, _ in entries:
        name_offsets.append(names_base + len(names))
        names += name + b'\0'
    index_offset = HEADER.size
    index_size = names_base + len(names) if entries else 0

    def roundup(x, a):
        return (x + a - 1) // a * a

    data = bytearray()
    data_start = roundup(index_offset + index_size, align)
    file_at = {}
    for i, (_, typ, src) in enumerate(entries):
        if typ != TYPE_FILE:
            continue
        with open(src, 'rb') as f:
            contents = f.read()
        at = data_start + roundup(len(data), align)
        data += bytes(at - data_start - len(data))
        file_at[i] = (at, len(contents))
        data += contents
    image_size = data_start + len(data)

    out = bytearray(HEADER.pack(MAGIC, VERSION, HEADER.size, align, len(entries), root_count,
                                index_offset, index_size, image_size))
    for i, (name, typ, _) in enumerate(entries):
        offset, size = file_at[i] if typ == TYPE_FILE else dir_runs[i]
        out += ENTRY.pack(name_offsets[i], len(name), typ, 0, offset, size)
    out += names
    out += bytes(data_start - len(out))
    out += data
    out += bytes(roundup(len(out), BLOCK) - len(out))
    return bytes(out)


def check(image):
    """The tree in an image, as (path, type, offset, size) in index order, after the checks
    lib/fs/rofs makes when it mounts one."""
    if len(image) < HEADER.size:
        raise Error('shorter than a header')
    (magic, version, header_size, align, entry_count, root_count, index_offset, index_size,
     image_size) = HEADER.unpack_from(image)
    if magic != MAGIC or version != VERSION or header_size != HEADER.size:
        raise Error('not an rofs image of this version')
    if align == 0 or align & (align - 1):
        raise Error(f'alignment {align} is not a power of two')
    if image_size > len(image) or index_offset < HEADER.size or index_offset % 8 or \
            index_offset + index_size > image_size:
        raise Error('the index or the image runs past the end')
    if entry_count == 0:
        if index_size or root_count:
            raise Error('an empty index with a size or a root')
    elif index_size <= entry_count * ENTRY.size or root_count > entry_count:
        raise Error('the entries do not fit the index')
    index = image[index_offset:index_offset + index_size]
    entries = [ENTRY.unpack_from(index, i * ENTRY.size) for i in range(entry_count)]

    def name(i):
        name_offset, name_len = entries[i][0], entries[i][1]
        if not 0 < name_len <= NAME_MAX or name_offset < entry_count * ENTRY.size or \
                name_offset + name_len >= index_size:
            raise Error(f'entry {i}: the name is out of place')
        b = bytes(index[name_offset:name_offset + name_len + 1])
        if b[-1] != 0 or b'\0' in b[:-1] or b'/' in b or b[:-1] in (b'.', b'..'):
            raise Error(f'entry {i}: not a name')
        return b[:-1]

    def run(first, count):
        names = [name(i) for i in range(first, first + count)]
        if any(a >= b for a, b in zip(names, names[1:])):
            raise Error(f'entries {first}..{first + count}: not sorted')

    run(0, root_count)
    out = []
    paths = {}
    next_run = root_count
    for i in range(root_count):
        paths[i] = name(i).decode()
    for i, (_, _, typ, reserved, offset, size) in enumerate(entries):
        name(i)
        if reserved:
            raise Error(f'entry {i}: reserved is set')
        if typ == TYPE_FILE:
            if offset % align or offset + size > image_size:
                raise Error(f'entry {i}: the file is out of place')
        elif typ == TYPE_DIR:
            if offset != next_run or offset <= i or offset + size > entry_count:
                raise Error(f'entry {i}: the directory is out of place')
            next_run += size
            run(offset, size)
            for c in range(offset, offset + size):
                paths.setdefault(c, f'{paths.get(i, "?")}/{name(c).decode()}')
        else:
            raise Error(f'entry {i}: unknown type {typ}')
        out.append((paths.get(i, '?'), typ, offset, size))
    if next_run != entry_count:
        raise Error('entries outside every directory')
    return align, out


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('-o', '--output', help='the image to write')
    p.add_argument('-a', '--align', type=int, default=16,
                   help='where each file starts: a power of two (default 16)')
    p.add_argument('-d', '--dir', action='append', default=[],
                   help='a directory whose contents go at the root; may be repeated')
    p.add_argument('-m', '--manifest', action='append', default=[],
                   help='a file of src:dst lines; may be repeated')
    p.add_argument('--list', metavar='IMAGE', help='check IMAGE and print its tree')
    p.add_argument('pairs', nargs='*', metavar='SRC:DST', help='a file and its path in the image')
    args = p.parse_args()

    try:
        if args.list:
            with open(args.list, 'rb') as f:
                align, entries = check(f.read())
            print(f'align {align}, {len(entries)} entries')
            for path, typ, offset, size in entries:
                kind = 'file' if typ == TYPE_FILE else 'dir '
                print(f'{kind} {offset:#10x} {size:10} {path}')
            return 0
        if not args.output:
            p.error('an output image (-o) or --list')
        if args.align <= 0 or args.align & (args.align - 1) or args.align >= 1 << 32:
            raise Error(f'alignment {args.align} is not a power of two')
        tree = Tree()
        for d in args.dir:
            tree.add_dir(d)
        pairs = list(args.pairs)
        for m in args.manifest:
            with open(m) as f:
                pairs += [l.strip() for l in f if l.strip() and not l.lstrip().startswith('#')]
        for pair in pairs:
            src, sep, dst = pair.rpartition(':')
            if not sep or not src:
                raise Error(f"'{pair}' is not src:dst")
            tree.add_file(src, dst)
        image = build(tree, args.align)
        check(image)
        tmp = args.output + '.tmp'
        with open(tmp, 'wb') as f:
            f.write(image)
        os.replace(tmp, args.output)
        return 0
    except (Error, OSError) as e:
        print(f'{p.prog}: {e}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
