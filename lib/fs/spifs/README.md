# spifs

A small flat filesystem for SPI NOR flash. It is aimed at the case an embedded
target usually has: a handful of long lived blobs — a bootloader image, a
calibration table, a config file — on a few megabytes of flash, where the code
size of the filesystem matters more than its features.

It is deliberately simple. Files are contiguous, the directory is a fixed size
table written in two copies, and there is no allocator to speak of. Read the
limitations below before designing around it; several of them are the kind that
are cheap to work around up front and expensive to discover later.

## On-disk layout

    page 0                     ... front ToC (toc_pages pages)
    page toc_pages             ... file data, contiguous runs
    page count - toc_pages     ... back ToC (toc_pages pages)

The table of contents is an array of 32 byte entries:

| entry | contents |
| --- | --- |
| header | magic `SPFS` (0x53504653), version, entry count, generation |
| file × N | first page index, length, capacity, name[20] |
| footer | reserved, CRC32 over the header and all entries |

Two complete copies of the ToC exist, at the start and the end of the device.
Every metadata change writes the copy that was *not* written last, with the
generation counter bumped by one. Mount validates magic, version and CRC32 on
both and takes the higher valid generation. So a metadata update is atomic: the
previous copy stays intact and valid until the new one is fully written, and an
interrupted commit falls back to it.

## Device requirements

spifs uses a "page" as its unit of allocation and of rewriting, and it derives
that from the block device's erase geometry:

| `bdev_t.geometry_count` | page size | behavior |
| --- | --- | --- |
| 0 | `block_size` | overwrites a page in place, no erase |
| 1 | `geometry->erase_size` | erases each page before rewriting it |
| 2+ | — | `ERR_NOT_SUPPORTED`, non-uniform geometry is not handled |

Note `erase_size` is a **byte count** and `erase_shift` is its log2, per
`lib/bio.h`. If you are writing a driver, set both, and set `erase_byte` *after*
`bio_initialize_bdev()`, which zeroes it. `platform/zynq/spiflash.c` is the
reference; two of the STM32 drivers get this wrong.

Format rejects a device that does not satisfy:

- `page_size` is a power of two and a multiple of 32 (the ToC entry size)
- `erase_size % block_size == 0`
- `total_size % page_size == 0` — no partial page at the end
- more than 4 ToC entries, i.e. room for at least one file

## What it can do

- create, open, read, write, truncate, remove, stat
- `opendir`/`readdir` on the mount root
- `fs_stat_fs()` for free space and free inodes
- grow a file in place up to its capacity
- survive a power cut during a metadata update, and mount from the surviving ToC
  copy if one is corrupt
- `FS_IOCTL_GET_FILE_ADDR` / `FS_IOCTL_IS_LINEAR` on a memory mapped device, so
  a file can be executed or read in place without copying

## What it cannot do

- **No directories.** The namespace is flat. A `/` in a name returns
  `ERR_NOT_SUPPORTED`; there is no `mkdir` or `rmdir` in the op table.
- **Filenames are at most 19 characters** plus the terminator, not the 64 that
  `FS_MAX_FILE_LEN` suggests.
- **A file's capacity is fixed when it is created.** `fs_create_file()` takes a
  length that is rounded up to a page and becomes the permanent capacity.
  Writing past it returns `ERR_OUT_OF_RANGE`. There is no way to grow a file
  beyond it — you must remove it and create a larger one.
- **`truncate` only shrinks.** Growing returns `ERR_INVALID_ARGS`, and it changes
  the recorded length only; the capacity and the pages stay allocated.
- **Files are contiguous, and there is no compaction.** Allocation is first fit
  over the gaps between existing files. Free space fragments as files are
  created and removed, so a create can fail even when `fs_stat_fs()` reports
  plenty of free space — what matters is the largest single gap.
- **The ToC is a fixed size table.** The file count ceiling is set at format
  time and cannot change without reformatting.
- **Read-only mount is not supported.** `spifs_mount()` rejects any non-zero
  option, so `FS_MOUNT_OPTION_READ_ONLY` returns `ERR_INVALID_ARGS`.
- **File data is not power-fail safe.** Only metadata gets the dual-ToC
  treatment. A partial write leaves partial data, and on a device with erase
  geometry the page is erased before it is rewritten, so an interrupted write
  can lose data that was already there.
- **No wear leveling.** The two ToC regions are rewritten on every create,
  remove, truncate, and every write that extends a file, so they wear far faster
  than the data area. If your workload appends in small increments, consider
  creating the file at its full size once and writing into it, rather than
  letting each write extend the length.
- **No rename, no sync, no timestamps, no permissions.**
- Maximum file size is 4GB (`ERR_TOO_BIG` above that).

## Sizing

With a ToC entry of 32 bytes:

    entries_per_page = page_size / 32
    total_entries    = toc_pages * entries_per_page - 2   (header, footer)
    usable files     = total_entries - 2                  (the two ToC entries)

A 4KB erase page with the default `toc_pages = 1` gives 124 files. Raise
`toc_pages` at format time if you need more; each extra page costs two pages of
flash, one at each end.

Every mount holds one page-sized buffer on the heap, plus a small struct per
open file.

## Using it

Format once, then mount. `args` may be NULL for the default of one ToC page.

```c
#include <lib/fs.h>
#include <lib/fs/spifs.h>

spifs_format_args_t args = { .toc_pages = 1 };

status_t err = fs_format_device("spifs", "qspi-flash", &args);
if (err != NO_ERROR) { /* ... */ }

err = fs_mount("/spifs", "spifs", "qspi-flash", FS_MOUNT_OPTION_NONE);
```

From the shell that is `fs format spifs <device>` followed by
`mount /spifs spifs <device>`. Note the shell's `format` passes NULL args, so it
always uses one ToC page — set `toc_pages` from code if you need more. The
`mount` command's trailing `ro` flag will be rejected, since spifs does not
implement read-only mounts.

`target/dartuinoP0/init.c` mounts at boot and prints the format command if the
mount fails, which is a reasonable pattern to copy.

Add `lib/fs/spifs` to your project's `MODULES` (or take
`project/virtual/fs.mk`), and note the misspelled but public
`DEAULT_SPIFS_MOUNT_POINT` / `DEAULT_SPIFS_NAME` macros in
`lib/fs/include/lib/fs/spifs.h`.

## Error codes worth knowing

| code | means |
| --- | --- |
| `ERR_ALREADY_EXISTS` | create on a name already in the ToC |
| `ERR_TOO_BIG` | ToC full, **or** no contiguous run large enough, **or** file over 4GB — the three are indistinguishable from the return value |
| `ERR_OUT_OF_RANGE` | write past the file's fixed capacity |
| `ERR_BAD_PATH` | name is 20 characters or longer |
| `ERR_NOT_SUPPORTED` | name contains `/`, or the device geometry is unusable |
| `ERR_CRC_FAIL` | both ToC copies failed validation |
| `ERR_BAD_STATE` | mount found overlapping file page ranges |

## Tests

`lib/fs/spifs/test/` runs under `ut all` on every architecture, against RAM
backed devices in both geometry flavors — `spifs_tests_nogeom` over
`create_membdev()` and `spifs_tests_norflash` over `create_nor_membdev()`, which
publishes erase geometry and erases to 0xff. The same suite can be pointed at
real hardware with `spifs test <device>`, and `spifs bench <path>` times the
common operations.
