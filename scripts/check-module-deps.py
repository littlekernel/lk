#!/usr/bin/env python3
"""Report modules that include headers of modules they do not declare in MODULE_DEPS.

The build system adds every module's include/ directory to one global -I list, so a
module can #include a header from any other module in the build without declaring a
dependency on it. That works until the module is used in a project where nothing else
happens to pull the header's owner in. This script finds those undeclared edges.

For each build directory given (build-<project>/ as produced by make), it:

  1. takes the set of modules in that build from the module_config.h files make writes,
     and reads each module's MODULE_DEPS and MODULE_WEAK_DEPS from the same file,
  2. scans the module's own sources for #include lines and resolves each one to the
     module whose include/ directory provides it,
  3. reports an include as undeclared when its owner is not the module itself, not part
     of the always-visible substrate (top, kernel, libc, arch, platform, ...), and not
     in the transitive closure of the module's declared dependencies.

Only direct includes are checked; a header a module gets transitively through one of its
declared dependencies' headers is that dependency's problem, and shows up under it.

An include that is only compiled when the other module is in the build (guarded by
WITH_<module>) should be declared with MODULE_WEAK_DEPS rather than MODULE_DEPS, so the
dependency is on record without forcing the module into every build.

Usage:
    scripts/check-module-deps.py                      # every build-* dir under the LK root
    scripts/check-module-deps.py build-qemu-virt-arm64-test build-pc-x86-test
    scripts/check-module-deps.py --edges | sort       # one 'module -> dep' line per edge
    scripts/check-module-deps.py --strict             # exit 1 if anything is reported
"""

import argparse
import collections
import os
import re
import sys

# Modules whose headers every module may use without declaring a dependency. These are
# the implicit substrate of the system: nothing lists MODULE_DEPS += top or lib/libc, and
# the arch/platform/target for the build are chosen by the project, not by the modules
# that use their headers. Anything under arch/, platform/ or target/ is treated the same.
ALWAYS_VISIBLE = {
    'top', 'kernel', 'lib/libc', 'lib/heap', 'app', 'dev', 'arch', 'platform', 'target',
}
ALWAYS_VISIBLE_PREFIXES = ('arch/', 'platform/', 'target/')

SOURCE_SUFFIXES = ('.c', '.cpp', '.cc', '.h', '.hpp', '.S')
INCLUDE_RE = re.compile(r'\s*#\s*include\s*[<"]([^>"]+)[>"]')
DEPS_RE = re.compile(r'#define MODULE_(?:WEAK_)?DEPS "([^"]*)"')
GLOBAL_INCLUDES_RE = re.compile(r'^\s*GLOBAL_INCLUDES\s*\+=', re.M)

# Directories make searches for modules and for the top level include/ dir (LKINC in
# the makefile). A dep declared as lib/cksum is the module that calls itself
# external/lib/cksum, so dependency names have to be resolved through the same list.
SEARCH_ROOTS = ('', 'external/')


def is_always_visible(module):
    return module in ALWAYS_VISIBLE or module.startswith(ALWAYS_VISIBLE_PREFIXES)


class Build:
    """The set of modules in one build directory and their declared dependencies."""

    def __init__(self, root, builddir):
        self.root = root
        self.builddir = builddir
        self.name = os.path.basename(os.path.normpath(builddir))
        self.modules = {}   # module name -> path to module_config.h
        for dirpath, _, filenames in os.walk(builddir):
            if 'module_config.h' in filenames:
                rel = os.path.relpath(dirpath, builddir)
                self.modules[rel] = os.path.join(dirpath, 'module_config.h')
        self.deps = {m: self._parse_deps(p) for m, p in self.modules.items()}
        # module_config.h records MODULE_DEPS as of the last build. It is only rewritten
        # when its content changes, so compare rules.mk against lk.elf, which every build
        # relinks; a rules.mk newer than that means the report may be out of date.
        built_at = self._mtime(os.path.join(builddir, 'lk.elf'))
        self.stale = sorted(
            m for m in self.modules
            if self._mtime(os.path.join(root, m, 'rules.mk')) > built_at
        )
        self._closure_cache = {}
        # include dir -> owning module, for every module in this build that has one
        self.include_dirs = []
        for m in self.modules:
            d = os.path.join(root, m, 'include')
            if os.path.isdir(d):
                self.include_dirs.append((d, m))

    @staticmethod
    def _mtime(path):
        try:
            return os.stat(path).st_mtime
        except OSError:
            return 0

    def _parse_deps(self, config_path):
        """MODULE_DEPS plus MODULE_WEAK_DEPS, resolved to module names in this build.

        Weak deps only matter for modules that are in the build, which is exactly what
        the resolution filters for, so both lists are treated alike here.
        """
        with open(config_path, errors='replace') as f:
            text = f.read()
        recorded = ' '.join(m.group(1) for m in DEPS_RE.finditer(text))
        if not recorded.strip():
            return []
        # make/module.mk stores the lists with spaces replaced by '_', so a module name
        # that itself contains an underscore (lib/rust_support) is ambiguous. Rejoin
        # tokens greedily, longest match first, against the modules known to this build.
        tokens = recorded.replace(' ', '_').strip('_').split('_')
        deps = []
        i = 0
        while i < len(tokens):
            for j in range(len(tokens), i, -1):
                candidate = self._resolve_module('_'.join(tokens[i:j]))
                if candidate:
                    deps.append(candidate)
                    i = j
                    break
            else:
                deps.append(tokens[i])
                i += 1
        return deps

    def _resolve_module(self, name):
        """Map a MODULE_DEPS entry to the name a module in this build gives itself."""
        for prefix in SEARCH_ROOTS:
            if prefix + name in self.modules:
                return prefix + name
        return None

    def closure(self, module):
        """Transitive closure of MODULE_DEPS starting at (and including) module."""
        if module in self._closure_cache:
            return self._closure_cache[module]
        seen = set()
        worklist = [module]
        while worklist:
            m = worklist.pop()
            if m in seen:
                continue
            seen.add(m)
            worklist.extend(self.deps.get(m, ()))
        self._closure_cache[module] = seen
        return seen

    def owner_of(self, include, from_module):
        """Which module provides an include path, or None if it is local/global/toolchain."""
        if os.path.exists(os.path.join(self.root, from_module, include)):
            return None     # relative to the including module's own directory
        for prefix in SEARCH_ROOTS:
            if os.path.exists(os.path.join(self.root, prefix, 'include', include)):
                return None     # top level include/ of a search root, always on the path
        hits = [m for d, m in self.include_dirs if os.path.exists(os.path.join(d, include))]
        if not hits:
            return None     # toolchain header, or provided by a GLOBAL_INCLUDES += dir
        # arch/include and arch/<arch>/include can both provide the same path; the more
        # specific module wins the same way the compiler's search order does in practice.
        return max(hits, key=len)


class Tree:
    """Facts about the source tree that do not depend on which build is being checked."""

    def __init__(self, root):
        self.root = root
        self._rules_cache = {}

    def is_module_dir(self, path):
        return os.path.exists(os.path.join(path, 'rules.mk'))

    def exports_globally(self, module):
        """True if the module's rules.mk adds to GLOBAL_INCLUDES itself."""
        if module not in self._rules_cache:
            rules = os.path.join(self.root, module, 'rules.mk')
            try:
                with open(rules, errors='replace') as f:
                    self._rules_cache[module] = bool(GLOBAL_INCLUDES_RE.search(f.read()))
            except OSError:
                self._rules_cache[module] = False
        return self._rules_cache[module]

    def sources_of(self, module):
        """Yield source files belonging to the module, skipping nested modules.

        Subdirectories that are modules in their own right (they have a rules.mk) belong
        to those modules, whether or not they are part of this particular build.
        """
        top = os.path.join(self.root, module)
        for dirpath, dirnames, filenames in os.walk(top):
            dirnames[:] = sorted(
                d for d in dirnames
                if not self.is_module_dir(os.path.join(dirpath, d))
            )
            for f in filenames:
                if f.endswith(SOURCE_SUFFIXES):
                    yield os.path.join(dirpath, f)


def includes_in(path):
    try:
        with open(path, errors='replace') as f:
            for line in f:
                m = INCLUDE_RE.match(line)
                if m:
                    yield m.group(1)
    except OSError:
        return


def check_build(tree, build):
    """Return {module: {owner: {(source, include), ...}}} of undeclared includes."""
    findings = collections.defaultdict(lambda: collections.defaultdict(set))
    for module in build.modules:
        closure = build.closure(module)
        for src in tree.sources_of(module):
            for inc in includes_in(src):
                owner = build.owner_of(inc, module)
                if owner is None or owner == module:
                    continue
                if is_always_visible(owner) or tree.exports_globally(owner):
                    continue
                if owner in closure:
                    continue
                findings[module][owner].add((os.path.relpath(src, tree.root), inc))
    return findings


def find_build_dirs(root):
    return sorted(
        os.path.join(root, d) for d in os.listdir(root)
        if d.startswith('build-') and os.path.isdir(os.path.join(root, d, 'top'))
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0],
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     epilog='\n\n'.join(__doc__.split('\n\n')[1:]))
    parser.add_argument('builddirs', nargs='*',
                        help='build-<project> directories to check (default: all under --root)')
    parser.add_argument('--root', default=None,
                        help='LK source root (default: parent of the scripts/ directory)')
    parser.add_argument('--edges', action='store_true',
                        help='print one "module -> missing dependency" line per edge')
    parser.add_argument('--strict', action='store_true',
                        help='exit with status 1 if any undeclared dependency is found')
    args = parser.parse_args()

    root = os.path.abspath(args.root or os.path.join(os.path.dirname(__file__), '..'))
    builddirs = [os.path.abspath(b) for b in args.builddirs] or find_build_dirs(root)
    if not builddirs:
        print(f'no build-* directories found under {root}', file=sys.stderr)
        return 2

    tree = Tree(root)
    # (module, owner) -> {'projects': set, 'sites': set}
    merged = collections.defaultdict(lambda: {'projects': set(), 'sites': set()})
    for b in builddirs:
        build = Build(root, b)
        if not build.modules:
            print(f'{build.name}: no module_config.h files, is it built?', file=sys.stderr)
            continue
        if build.stale:
            print(f'{build.name}: rules.mk newer than the build for {", ".join(build.stale)}; '
                  f'rebuild before trusting this report', file=sys.stderr)
        for module, owners in check_build(tree, build).items():
            for owner, sites in owners.items():
                entry = merged[(module, owner)]
                entry['projects'].add(build.name)
                entry['sites'] |= sites

    if args.edges:
        for module, owner in sorted(merged):
            print(f'{module} -> {owner}')
    else:
        by_module = collections.defaultdict(list)
        for (module, owner), entry in merged.items():
            by_module[module].append((owner, entry))
        for module in sorted(by_module):
            print(f'{module}: undeclared dependency on')
            for owner, entry in sorted(by_module[module]):
                projects = ', '.join(sorted(entry['projects']))
                print(f'    {owner}  [{projects}]')
                for src, inc in sorted(entry['sites']):
                    print(f'        {src}: #include <{inc}>')
        print(f'\n{len(by_module)} module(s) with {len(merged)} undeclared dependency edge(s) '
              f'across {len(builddirs)} build dir(s)')

    return 1 if (args.strict and merged) else 0


if __name__ == '__main__':
    sys.exit(main())
