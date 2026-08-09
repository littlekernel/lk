#!/usr/bin/env python3
# Copyright (c) 2026 Travis Geiselbrecht
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
#
"""Static stack usage analysis over a built lk.elf disassembly listing.

Parses the per-function stack pointer adjustments and direct call sites out of
the build's lk.elf.lst, then either reports worst-case call chains from a root
function (--root) or lists individual frame sizes (--frames).

Typical use, after building a project:

  # worst-case stack path starting at a function, most useful for sizing
  # a thread's stack
  scripts/stackwalk.py build-qemu-virt-arm64-test/lk.elf.lst --root shell_entry

  # C++ symbols appear demangled in the listing, quote them whole
  scripts/stackwalk.py build-qemu-virt-arm64-test/lk.elf.lst \\
      --root 'fat_dir::opendir(fscookie*, char const*, dircookie**)'

  # frame sizes of functions matching any of the given substrings,
  # sorted largest first
  scripts/stackwalk.py build-qemu-virt-arm64-test/lk.elf.lst \\
      --frames fat_dir fs_open command_loop

Caveats, this is a static lower bound rather than an exact answer:

- Indirect calls (function pointers, vtables) are not followed; functions
  containing any are marked '[has indirect]' in the output so it is obvious
  where the walk may be cut short. Re-run with --root at the far side of an
  indirection to stitch chains together by hand.
- Tail calls and calls into the middle of another function are ignored.
- Recursion cycles are cut at the back edge (counted once).
- Interrupts and exceptions that land on the same stack are not modeled.
- Alloca/VLA-style dynamic adjustments are not counted.

Supported listing flavors (--format, default autodetect): GNU objdump for
arm64 and riscv, and llvm-objdump for riscv (clang/LTO builds). For the
llvm riscv flavor, call targets are recomputed from the auipc/jalr pairs
because the annotations in the listing itself are unreliable.
"""

import argparse
import re
import sys
from bisect import bisect_right
from collections import defaultdict

# functions that terminate execution or are otherwise not interesting for
# worst-path analysis; pruned by default
DEFAULT_PRUNE = 'panic,_panic,assert_fail,platform_halt,panic_shell_start'

SYM_RE = re.compile(r'^([0-9a-f]+) <(.+)>:$')
INSN_ADDR_RE = re.compile(r'^([0-9a-f]+):')


class Flavor:
    """Regexes describing one toolchain/architecture listing format."""

    def __init__(self, name, frame_res, call_re, indirect_re, detect_re):
        self.name = name
        self.frame_res = frame_res      # each match group(1) adds to the frame
        self.call_re = call_re          # group(1) is the callee symbol name
        self.indirect_re = indirect_re  # marks the function as having indirect calls
        self.detect_re = detect_re      # used by autodetection


FLAVORS = [
    Flavor(
        'gnu-arm64',
        # 'sub sp, sp, #N' and pre-index writeback stores 'stp ... [sp, #-N]!'
        [re.compile(r'\bsub\s+sp, sp, #(\d+)'),
         re.compile(r'\bst[rp]\s+.*\[sp, #-(\d+)\]!')],
        re.compile(r'\bbl\s+[0-9a-f]+ <(.+?)(\+0x[0-9a-f]+)?>\s*$'),
        re.compile(r'\bblr\b'),
        re.compile(r'\bstp\s+x29, x30|\bsub\s+sp, sp, #'),
    ),
    Flavor(
        'gnu-riscv',
        [re.compile(r'\baddi\s+sp,sp,-(\d+)')],
        re.compile(r'\bjal\s+[0-9a-f]+ <(.+?)(\+0x[0-9a-f]+)?>\s*$'),
        re.compile(r'\bjalr\b'),
        re.compile(r'\baddi\s+sp,sp,-\d+'),
    ),
    # llvm-riscv is handled separately since it needs numeric call resolution,
    # but keep a Flavor for frame sizes and detection.
    Flavor(
        'llvm-riscv',
        [re.compile(r'\baddi\s+sp, sp, -(0x[0-9a-f]+|\d+)')],
        None,
        re.compile(r'\bjalr\b'),
        re.compile(r'\baddi\s+sp, sp, -0x[0-9a-f]+'),
    ),
]

AUIPC_RE = re.compile(r'\bauipc\s+(\w+), (-?0x[0-9a-f]+|\d+)')
JALR_OFF_RE = re.compile(r'\bjalr\s+(-?0x[0-9a-f]+|-?\d+)?\((\w+)\)')


def is_label(name):
    """Local labels llvm-objdump emits as symbol headers mid-function."""
    return name.startswith('.L') or name.startswith('$')


def detect_flavor(path):
    counts = {f.name: 0 for f in FLAVORS}
    with open(path) as f:
        for i, line in enumerate(f):
            if i > 200000:
                break
            for fl in FLAVORS:
                if fl.detect_re.search(line):
                    counts[fl.name] += 1
    best = max(counts, key=counts.get)
    if counts[best] == 0:
        sys.exit('error: could not autodetect listing format, pass --format')
    return best


def parse(path, flavor):
    """Returns (frames, calls, indirect, syms).

    frames: {func: stack bytes}
    calls: {func: set of callee names or addresses (llvm-riscv)}
    indirect: set of funcs containing indirect calls
    syms: [(addr, func)] sorted by address
    """
    fl = next(f for f in FLAVORS if f.name == flavor)
    numeric = flavor == 'llvm-riscv'

    frames = {}
    calls = defaultdict(set)
    indirect = set()
    syms = []

    cur = None
    regvals = {}
    with open(path) as f:
        for line in f:
            m = SYM_RE.match(line)
            if m:
                name = m.group(2)
                if is_label(name):
                    continue
                cur = name
                frames.setdefault(cur, 0)
                syms.append((int(m.group(1), 16), cur))
                regvals = {}
                continue
            if cur is None:
                continue

            for frame_re in fl.frame_res:
                fm = frame_re.search(line)
                if fm:
                    frames[cur] += int(fm.group(1), 0)

            if numeric:
                # resolve auipc/jalr pairs to absolute addresses
                am = AUIPC_RE.search(line)
                if am:
                    im = INSN_ADDR_RE.match(line)
                    if im:
                        imm = int(am.group(2), 0)
                        if imm >= 0x80000:  # sign-extend the 20 bit immediate
                            imm -= 0x100000
                        regvals[am.group(1)] = int(im.group(1), 16) + (imm << 12)
                    continue
                jm = JALR_OFF_RE.search(line)
                if jm:
                    reg = jm.group(2)
                    if reg in regvals:
                        off = int(jm.group(1), 0) if jm.group(1) else 0
                        calls[cur].add(regvals[reg] + off)
                    else:
                        indirect.add(cur)
                    continue
                if fl.indirect_re.search(line):
                    indirect.add(cur)
            else:
                cm = fl.call_re.search(line)
                if cm:
                    if cm.group(1) != cur:
                        calls[cur].add(cm.group(1))
                elif fl.indirect_re.search(line):
                    indirect.add(cur)

    syms.sort()
    return frames, calls, indirect, syms


def walk(frames, calls, indirect, syms, root, prune):
    """Find the worst-case stack path from root, cutting recursion cycles."""
    addrs = [a for a, _ in syms]
    by_addr = dict(syms)

    def resolve(target):
        if isinstance(target, str):
            return target
        # numeric target: only accept exact function entry addresses, a call
        # into the middle of another function would be a resolution error
        i = bisect_right(addrs, target) - 1
        if i >= 0 and addrs[i] == target:
            return by_addr[addrs[i]]
        return None

    sys.setrecursionlimit(100000)
    memo = {}
    onstack = set()

    def worst(fn):
        if fn in memo:
            return memo[fn]
        if fn in onstack:
            return (0, [])
        onstack.add(fn)
        best = (0, [])
        for target in calls.get(fn, ()):
            callee = resolve(target)
            if callee is None or callee not in frames or callee in prune or callee == fn:
                continue
            w = worst(callee)
            if w[0] > best[0]:
                best = w
        onstack.discard(fn)
        me = frames.get(fn, 0)
        result = (me + best[0], [(fn, me)] + best[1])
        memo[fn] = result
        return result

    return worst(root)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('listing', help='disassembly listing, e.g. build-<project>/lk.elf.lst')
    parser.add_argument('--format', choices=[f.name for f in FLAVORS] + ['auto'],
                        default='auto', help='listing flavor (default: autodetect)')
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--root', metavar='SYMBOL',
                      help='report the worst-case stack path from this function '
                           '(demangled C++ names as they appear in the listing)')
    mode.add_argument('--frames', metavar='PATTERN', nargs='+',
                      help='list frame sizes of functions whose name contains any '
                           'of the given case-insensitive substrings, largest first')
    parser.add_argument('--prune', metavar='SYMS', default=DEFAULT_PRUNE,
                        help='comma separated functions to exclude from --root walks '
                             f'(default: {DEFAULT_PRUNE}; pass an empty string to keep all)')
    parser.add_argument('--all', action='store_true',
                        help='with --frames, include zero sized frames')
    args = parser.parse_args()

    flavor = args.format
    if flavor == 'auto':
        flavor = detect_flavor(args.listing)
        print(f'detected listing format: {flavor}', file=sys.stderr)

    frames, calls, indirect, syms = parse(args.listing, flavor)

    if args.frames:
        pats = [p.lower() for p in args.frames]
        matched = [(sz, name) for name, sz in frames.items()
                   if any(p in name.lower() for p in pats) and (sz or args.all)]
        if not matched:
            sys.exit('no matching functions')
        for sz, name in sorted(matched, reverse=True):
            mark = ' [has indirect]' if name in indirect else ''
            print(f'{sz:6d}  {name}{mark}')
        return

    if args.root not in frames:
        candidates = [n for n in frames if args.root in n]
        if len(candidates) == 1:
            print(f"note: using '{candidates[0]}' for --root {args.root}", file=sys.stderr)
            args.root = candidates[0]
        elif candidates:
            print(f"error: --root '{args.root}' is ambiguous, candidates:", file=sys.stderr)
            for n in sorted(candidates):
                print(f'  {n}', file=sys.stderr)
            sys.exit(1)
        else:
            sys.exit(f"error: no function named '{args.root}' in the listing")

    prune = set(p for p in args.prune.split(',') if p)
    total, path = walk(frames, calls, indirect, syms, args.root, prune)
    print(f'worst static stack from {args.root}: {total} bytes')
    for fn, sz in path:
        mark = ' [has indirect]' if fn in indirect else ''
        print(f'  {sz:6d}  {fn}{mark}')


if __name__ == '__main__':
    main()
