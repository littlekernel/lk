# LK Filesystem Layer

## Overview

`lib/fs` provides a single mount namespace and a small set of POSIX-shaped calls
(`fs_open_file`, `fs_read_dir`, `fs_make_dir`, ...) over any number of mounted
filesystems. Six filesystems ship in the tree:

| module         | media                         | writable | notes |
|----------------|-------------------------------|----------|-------|
| `lib/fs/memfs` | none (RAM)                    | yes      | mounts with no device; the reference implementation and the main test vehicle |
| `lib/fs/spifs` | a `lib/bio` device (SPI flash)| yes      | flat, no directories; `FS_IOCTL_GET_FILE_ADDR` for execute-in-place |
| `lib/fs/fat`   | a `lib/bio` device            | yes      | FAT12/16/32, long names; C++ |
| `lib/fs/ext2`  | a `lib/bio` device            | no       | read only, including symlinks |
| `lib/fs/rofs`  | a `lib/bio` device            | no       | an image built ahead of time by `scripts/mkrofs.py`; `FS_IOCTL_GET_FILE_ADDR` on a mapped device |
| `lib/fs/9p`    | a virtio-9p device            | yes      | 9P2000.L to a host directory under QEMU |

An rofs image can come with the kernel: `lib/fs/rofs/embed` links `ROFS_EMBED_IMAGE` into the
binary at `ROFS_EMBED_ALIGN` (default 4096) and registers it as the memory block device
`ROFS_EMBED_NAME` (default `rofs0`) before the platform's own devices appear, so it mounts like
any other device, and every file on it is reachable in place at an address that keeps the
image's alignment. Any project can take it on the command line:
`make <project> EXTERNAL_MODULES=lib/fs/rofs/embed ROFS_EMBED_IMAGE=<image>`.

The layer owns path resolution. A filesystem never sees a path: it is asked to
resolve **one component at a time** relative to a directory it previously handed
back. That is what makes mount stitching, `..`, symlink following and the
unlink-while-open rule uniform across all six, rather than six slightly
different implementations of each.

`lib/fs/plan.md` is the design document for this arrangement and records the
decisions and the places the implementation deviated from the original proposal.
This file describes the result.

## The object model

Two structures, both owned by the layer (`lib/fs/fs.c`):

- **`struct fs_node`** — one *name*. It has a parent, a single path component, a
  child list, and either a vnode or nothing. A node with no vnode is
  *scaffolding*: an intermediate directory that exists only because a mount lives
  somewhere below it. Mounting at `/mnt/foo` creates nodes `mnt` and `foo` whether
  or not anything is mounted at `/mnt`.
- **`struct fs_vnode`** — one *object* (file, directory or symlink), declared in
  `lib/fs/include/lib/fs.h`. It carries the mount it belongs to, a reference
  count, a type, an optional identity `id`, and the filesystem's own `priv`
  pointer.

Names and objects are separate because they have different lifetimes: a node is
cheap and disposable, while an open handle must keep the object alive even after
every name for it is gone.

Because scaffold nodes are real directories to the layer, `ls /` works with
nothing mounted at `/`, and listing a scaffold node enumerates the mount points
beneath it. There is no special-cased root filesystem.

## Path resolution

Every entry point normalizes its path with `fs_normalize_path()` into a private
buffer, then hands it to `fs_walk()`, which walks it component by component from
the namespace root:

- **Mount stitching.** Reaching a node with a filesystem mounted at it continues
  the walk in that filesystem's root vnode. Mount points are nodes, not string
  prefixes.
- **`.` and `..` are lexical**, flattened by `fs_normalize_path()` before the walk
  starts. `..` therefore crosses a mount point back out to the parent namespace,
  and never asks a filesystem for a `..` entry.
- **Symlinks are followed by the layer.** The target is spliced into the path
  buffer and the walk **restarts from the namespace root**, so an absolute target
  resolves against the namespace, not against the filesystem that contains the
  link — a symlink to `/etc/foo` on a filesystem mounted at `/mnt` names
  `/etc/foo`. At most `FS_MAX_SYMLINK_DEPTH` (8) links are followed per walk,
  after which the walk returns `ERR_RECURSE_TOO_DEEP`. A spliced path must still
  fit `FS_MAX_PATH_LEN`; over that it is `ERR_NOT_ENOUGH_BUFFER`.
- **Creation walks stop at the parent** (`FS_WALK_PARENT`) and hand the
  filesystem the final component, so `create`/`mkdir`/`unlink`/`rmdir` are all
  single-component operations.

Sizes are fixed: `FS_MAX_PATH_LEN` is 128 bytes for a whole path and
`FS_MAX_FILE_LEN` is 64 for one component (which is also the size of
`struct dirent`'s name).

## Lifetime rules

These are the parts most likely to bite someone writing or debugging a
filesystem.

**Vnodes are deduplicated by `id`, and `id` 0 means "no identity".** A filesystem
that can produce a stable per-object number should return it, and the layer then
guarantees that every name for one object resolves to the same `fs_vnode`. That
is what makes a shared file length and the busy check below work. If a
filesystem's natural identity can legitimately *be* zero, it must shift it into a
nonzero encoding — FAT identifies an object by the location of its short name
directory entry, and `0:0` is a legal location (the first entry of a FAT12/16
root directory), so it sets the high bit; 9p uses `qid.path + 1`. Getting this
wrong is quiet: that one object gets a fresh vnode per open, and the busy check
below stops working for it. A filesystem whose objects are owned elsewhere and
never move (memfs, spifs) can just use the object's address, and rofs uses its index
entry's number plus one.

**Unlink and rmdir are refused with `ERR_BUSY` while a handle is open.** The
layer checks this before calling the filesystem, so no filesystem implements
POSIX unlink-while-open semantics and none of them has to defend against having
an object freed underneath a handle. (`FS_CAP_UNLINK_OPEN`, an opt-in for
filesystems that could support the POSIX behaviour, was considered and not built;
add it when something needs it.)

**`release()` is deferred.** The last reference dropping does not necessarily
mean the last *user* went away — an unreferenced name is parked in the node cache
below and its vnode with it. A filesystem must not treat `release()` as a flush
point for anything the caller expects to be durable, because a reopen may be
served from the cached vnode without the filesystem seeing anything at all. This
is why the 9p driver writes through rather than keeping a per-file page buffer.

**Handles reference the vnode, not the node.** An open file survives its name
being pruned from the tree.

## The node cache

An unreferenced node with a vnode is parked on an LRU rather than freed, so
rewalking a recently used path skips the filesystem lookups — which matters most
for a filesystem where a lookup is I/O (ext2, FAT) or a round trip (9p).

- `FS_NODE_CACHE_SIZE` is the build variable holding the capacity, plumbed
  through `lib/fs/rules.mk`:

  ```bash
  make qemu-virt-arm64-test FS_NODE_CACHE_SIZE=8
  ```

  The default is **0 on `LK_EMBEDDED` targets and 64 elsewhere**. Zero disables
  caching entirely: a node is pruned the moment nothing references it, and the
  resident tree is the scaffolding plus whatever a walk is standing on.
- `fs_set_node_cache_size()` / `fs_get_node_cache_size()` change it at runtime,
  primarily for tests and bringup. Shrinking evicts immediately.

Cached names are evicted, not merely invalidated, when the name they hold could
go stale: unlinking a name evicts it first, and unmounting evicts every cached
name belonging to that mount.

## Locking

One global mutex, `fs_lock`, protects the node tree, every refcount, the mount
list and the per-mount vnode lists. It is held across the namespace ops
(`lookup`, `create`, `mkdir`, `unlink`, `rmdir`, and `readlink` when the walk
follows a symlink) and **not** across `read`, `write`, `truncate`, `stat` or
`readdir`, which go straight to the vnode.

Holding it across `lookup` means a lookup that does I/O serializes the whole
layer — ext2 holds it across bcache reads, spifs across a flash erase and ToC
commit. memfs, spifs, ext2, FAT and 9p each serialize on a per-mount mutex of
their own (and the 9p transport allows one outstanding RPC besides), and rofs
looks names up in an index it read whole at mount, so for those this costs
nothing today. If a real target ever shows contention, the upgrade
path is a per-node busy flag: mark the node, drop the lock around the filesystem
call, wake waiters afterwards.

A per-mount lock is what makes the ops that run outside `fs_lock` — `read`,
`readdir`, `readlink` — safe on a filesystem that reads through `lib/bcache`,
which has no locking of its own. Two threads in the cache at once can have a
block evicted and refilled underneath them, so a pointer from
`bcache_get_block()` ends up naming another block's contents; and because an
instance is only a handful of blocks and asserts when every one is referenced,
concurrent readers of one mount can exhaust it outright. ext2 takes its lock in
exactly the ops that reach the cache and asserts on it in the routines that touch
the cache; `stat`, `opendir` and `closedir` need no lock because the inode is
held by value in the vnode.

## Writing a filesystem

Implement `struct fs_api` (`lib/fs/include/lib/fs.h`) and register it:

```c
static const struct fs_api myfs_api = {
    .mount = myfs_mount,
    .unmount = myfs_unmount,
    .lookup = myfs_lookup,
    .read = myfs_read,
    /* ... */
};

STATIC_FS_IMPL(myfs, &myfs_api);
```

`STATIC_FS_IMPL` places the registration in the `fs_impl` linker section, where
the layer finds it; the name is what `fs_mount()` and the `mount` shell command
match on. A C++ implementation must spell out every optional member as `nullptr`,
or `-Werror=missing-field-initializers` fails the build (see `lib/fs/fat/fs.cpp`).

### Which ops are required

Most members are optional: a NULL one makes the corresponding call return
`ERR_NOT_SUPPORTED`, so a read-only flat filesystem is a short list.

| op | required? |
|----|-----------|
| `mount`, `unmount` | **yes** |
| `lookup` | **yes** — the walk calls it for every component |
| `read` | **yes** |
| `readdir`, `closedir` | **yes if `opendir` is provided** — called without a NULL check once a handle exists |
| `format`, `fs_stat` | optional |
| `create`, `mkdir`, `unlink`, `rmdir`, `rename` | optional |
| `readlink` | optional; required to resolve symlinks the filesystem returns |
| `release` | optional; omit it when the objects are owned elsewhere |
| `write`, `truncate`, `stat`, `ioctl` | optional |
| `opendir` | optional |

`rename` is currently unreachable: there is no public `fs_rename()` yet.

### Contracts to hold to

- **Return a fresh vnode from every call.** `mount`, `lookup`, `create` and
  `mkdir` allocate with `fs_vnode_create(id, type, priv, &vn)`; the layer adopts
  it and deduplicates by `id`, freeing the duplicate if it already had one.
  `fs_vnode_destroy()` is for unwinding an error *before* the layer has seen the
  vnode, and only then.
- **`mount` must produce a directory** as its root vnode, or the mount is
  rejected with `ERR_NOT_VALID`.
- **`vn->cookie` is filled by the layer** with the mount's `fscookie`, so an op
  that has a vnode can reach the filesystem instance without storing it in
  `priv`.
- **A missing name is `ERR_NOT_FOUND`.** The walk fails on it before the
  filesystem is asked anything further, so a path *through* a nonexistent
  component on a flat filesystem is `ERR_NOT_FOUND`, not `ERR_NOT_SUPPORTED`.
- **`readdir` reports end of directory as `ERR_NOT_FOUND`**, and does **not**
  list `.` or `..`. The layer flattens both lexically, so listing them would only
  make every caller filter them out; none of the six filesystems does. They
  remain resolvable as path components.
- **The mount root is openable.** `fs_open_file()` on a mount point succeeds and
  `stat` on it reports `is_dir` with a zero size, rather than failing. Every
  other file op on it should return `ERR_NOT_FILE`.
- **`unlink`/`rmdir` receive the child vnode** as well as the name, so the
  filesystem can refuse or invalidate it. The layer has already refused if the
  child has open handles.
- **Do not allocate on hot paths.** `read`, `write`, `stat` and `readdir` should
  allocate nothing; a cursor for `opendir` and the vnode for
  `lookup`/`create`/`mkdir` are the allocations the interface expects. Stacks are
  small (4KB on arm64, 1KB on cortex-m), so large stack buffers are as much of a
  problem as a malloc — put a scratch buffer in the vnode or the cursor.

### Tests

`lib/fs/test` holds the layer's own suite — the walk, mount stitching, scaffold
directories, the node cache, the busy rule — and runs it against memfs under
`ut all` on every architecture, memfs being the filesystem with no hardware
behind it. Each filesystem then has its own `<fs>/test/`, declared with
`MODULE_OPTIONS := test` on the parent module so it is only built when
`WITH_TESTS` is set, following the pattern in `AGENTS.md`. Filesystems that need
real media also have host-side runners that build an image, boot QEMU with it
attached and check the result afterwards:

```bash
./scripts/run-fat-tests.py          # FAT12/16/32, verified with fsck.fat and mtools
./scripts/run-ext2-tests.py         # 1K and 4K block ext2 images
./scripts/run-rofs-tests.py         # mkrofs.py images at three alignments
scripts/do-qemuarm -6 -f <dir>      # 9p: exports <dir> to the guest
```

The 9p share must be on a filesystem that supports extended attributes: QEMU's
`security_model=mapped` keeps each file's mode in a `user.virtfs.*` xattr, so a
share on NFS fails every create with an I/O error that looks like a driver bug.

## Shell commands

With `lib/fs` and `app/shell` in the build: `mount`, `unmount`, `df`, `ls`, `cd`,
`pwd`, `stat`, `cat`, `mkdir`, `mkfile`, `rm`, `rmdir`, plus `fs` for debug
subcommands. The shell's working directory is a string, not a held reference, so
`cd` does not pin a mount.

```text
] mount <path> <type> [device] [ro]
] mount /mnt fat virtio0
] ls /mnt
] cat /mnt/hello.txt
] unmount /mnt
```

A trailing `ro` passes `FS_MOUNT_OPTION_READ_ONLY`; the device argument is
omitted for a filesystem that has no backing store, such as memfs.

`fs_unmount()` matches the mount point exactly — a path inside the filesystem is
`ERR_NOT_FOUND`, not an unmount of its mount. Unmounting with handles still open
defers the teardown until the last one closes.
