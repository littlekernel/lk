# FAT Work Handoff

This file summarizes the FAT read/write/restructure work currently on `wip/fat`, what is validated, and what still appears incomplete.

## Current branch state

- Branch: `fat`
- Current HEAD: `30b7de6c7` - `[fs][fat] update plan.md, not much left`
- Recent WIP commits in this series:
  - `30b7de6c` - `[fs][fat] update plan.md, not much left`
  - `1a37f9ad` - `[fs][fat] add some more unit tests for some internal routines`
  - `d652790a` - `[fs][fat] handle adding LFN file names`
  - `2a8c600a` - `[fs][fat] handle malformed LFNs better and make sure it matches the SFN checksum`
  - `17b3dd2b` - `[lib][fs] format all of the lib/fs files with clang format`
  - `6a0fd86c` - `[lib][fat] format all of the code and update plan.md`
  - `65f32f15` - `WIP [fs][fat] intrinsic rootfs live iterator; live-iter unit test`
  - `b500c13d` - `WIP [fs][fat] fix memory leaks in shell.c (tmppath, cmd_rm, cmd_rmdir)`
  - `c711e581` - `WIP [fs][fat] add intrinsic rootfs to enumerate mount points via ls and opendir`
  - `65a3321a` - `WIP [fs][fat] quiet LOCAL_TRACE; implement ClnShutBit on mount/unmount`
  - `b4a187a4` - `WIP [fs][fat] add rmdir support and tests`
  - `1fa87d9b` - `[scripts] add --disk/-d switch to run-qemu-boot-tests.py; add run-fat-tests.sh wrapper`
  - `2654d18b` - `WIP [fs][fat] add file remove`
  - `3ebc8db6` - `WIP [fs][fat] add mkdir support and tests`
  - `87309bf7` - `WIP [fs][fat] add file write support and zero-fill extend semantics`
  - `524a8962` - `WIP [fs][fat] implement truncate shrink and cluster chain free`
  - `a57f3b93` - `WIP [fs][fat] update FAT32 FSInfo during allocation`
  - `2e80c806` — `WIP [fs][fat] implement FAT12 entry writes and mirror FAT updates`
  - `d36dc022` — `WIP [fs][fat]: refactor fat_sector_for_cluster to use 0xffffffff as error`
  - `c43ce9b7` — `WIP [fs][fat]: implement directory growth and fix cluster boundary issues`

## What was completed

### FAT entry updates and metadata

- Implemented FAT12 entry writes.
- Updated FAT writes to mirror across all FAT copies, not just one table.
- Added FAT32 FSInfo tracking/writeback so allocation and free-cluster accounting stays consistent.

### Cluster management and truncate

- Added cluster-chain free helper support.
- Implemented truncate shrink path instead of leaving shrink unsupported/panicking.
- Added support for shrinking to zero and shrinking in place.

### File write support

- Wired FAT `.write` support into the fs hooks.
- Implemented file writes through `fat_file::write_file` / `write_file_priv`.
- Added zero-fill semantics when extending into newly exposed regions so reads from gaps behave correctly.

### mkdir support

- Wired FAT `.mkdir` into the fs hooks in `fs.cpp`.
- Implemented `fat_dir::mkdir` in `dir.cpp`.
- New directories now allocate a cluster and initialize `.` and `..` entries.
- Fixed FAT32 root-child handling so `..` is written as cluster `0` for directories created directly under the FAT32 root. This was required for `fsck.fat -vn` to come back clean.

### File remove support

- Wired FAT `.remove` into the fs hooks in `fs.cpp`.
- Implemented `fat_dir::remove` for file deletion by path.
- Remove now:
  - rejects directory deletion with `ERR_NOT_FILE`
  - frees the file cluster chain
  - marks all entries in the record as deleted (LFN entries plus SFN entry)
  - rejects deletion when the target is actively open (`ERR_BUSY`)
- Lookup robustness:
  - for 8.3-compatible names, remove prefers exact short-name matching first
  - for non-8.3 names, remove falls back to long-name lookup

### Directory remove (rmdir) support

- Added `fs_remove_dir()` to the public FS API (`lib/fs/include/lib/fs.h`, `lib/fs/fs.c`).
- Added `rmdir` hook to `struct fs_api` and wired shell command `rmdir` in `lib/fs/shell.c`.
- Implemented `fat_dir::rmdir` in `dir.cpp` / `dir.h`:
  - Validates the target exists and is a directory (not a file).
  - Rejects removal of root directory.
  - Rejects removal of non-empty directories.
  - Frees the directory's cluster chain.
  - Marks all directory-entry records (LFN + SFN) as deleted.

### Unlink offset correctness

- Fixed directory-entry offset accounting in unlink/remove/rmdir lookup so offsets are
  directory-relative (not sector-relative) when propagated to delete logic.
- Tightened record-span start selection for long-name records to avoid deleting leading
  already-deleted entries in front of the actual matched LFN/SFN record.
- Added range sanity checks in delete-marking path to reject malformed offset ranges.

### ClnShutBit (volume dirty/clean bit in FAT entry 1)

- Added `mark_volume_dirty_locked()` / `mark_volume_clean_locked()` public methods to `fat_fs`.
- Added private helper `set_volume_clean_bit_locked(bool)` (implemented in `fs.cpp`).
- On **mount**: ClnShutBit is cleared (volume marked dirty/in-use) across all FAT copies, then flushed
  to disk immediately, so fsck will see an unclean marker if we crash before unmounting.
- On **unmount**: ClnShutBit is set (volume marked clean) across all FAT copies before
  `write_fsinfo_locked()` + `bcache_flush()`, so a clean unmount leaves a cleanly-marked volume.
- FAT16: bit 15 of FAT entry 1 (`0x8000`). FAT32: bit 27 of FAT entry 1 (`0x08000000`).
- Validated: `fsck.fat -vn` remains clean for all three image types after implementing this.
- FAT12 has no such bit; the methods are a no-op for FAT12.

### Directory Growth and Robustness

- **Implemented Directory Growth**: Directories now automatically expand beyond their initial allocation by allocating new clusters when no free slots are found in the existing chain.
- **Corrected Cluster Count Logic**: Fixed `total_clusters` to be an upper bound (exclusive) and updated all cluster boundary checks to prevent off-by-one errors and assertions at the filesystem edge.
- **Unified EOF Handling**: Standardized EOF marker extension for FAT12/16/32 so that `is_eof_cluster()` works consistently across all formats.
- **Improved Sector Addressing**: Refactored `fat_sector_for_cluster` to return `0xffffffff` on error and added safety checks to prevent accidental reads from sector 0 on invalid metadata.
- **Deduplicated entry generation**: Refactored SFN entry creation into a shared `fill_short_dirent` helper.

### LFN read-path Unicode handling

- **Fixed LFN Unicode truncation on read**: `fat_find_next_entry` no longer truncates UCS-2
  code units to low bytes.
- **Implemented UCS-2 → UTF-8 conversion for LFN parsing**:
  - Added `fat_ucs2_to_utf8` and a single-codepoint helper (`ucs2_char_to_utf8`) for
    shared conversion logic.
  - Updated LFN parsing to emit valid UTF-8 for BMP code points.
- **Removed heavyweight allocation from hot path**:
  - Dropped per-iteration `malloc/free` buffering in directory walk logic.
  - LFN parse now builds UTF-8 bytes backward in `filename_buffer` and then
    `memmove`s to offset 0 on successful SFN checksum match.
- **Kept malformed LFN safeguards**:
  - Sequence and checksum validation are still enforced.
  - Parsing aborts safely if reverse writes would underflow the output buffer.

### Test coverage added/updated

- Resize/shrink/regrow coverage.
- Write-path coverage, including sparse-extension zero-fill behavior.
- mkdir coverage, including:
  - create directory
  - duplicate mkdir rejection
  - nested mkdir
  - open created directory
  - create file inside created directory
  - missing-parent failure
- remove-file coverage, including:
  - remove success + not-found on reopen
  - removing long-name file
  - busy file rejection
  - removing a directory returns `ERR_NOT_FILE`
- **Directory growth stress test**:
  - Added `test_fat_dir_growth` which creates 1000 files in a single directory.
  - Verifies multi-cluster traversal and correct entry allocation after growth.
  - Confirmed content integrity and clean `fsck` across FAT12, FAT16, and FAT32.
- Added helper-level unit tests:
  - `test_fat_split_path`
  - `test_fat_name_to_short_file_name`
  - `test_fat_ucs2_to_utf8`
  - `test_fat_utf8_ucs2_roundtrip`
  - These required exporting helper declarations in `lib/fs/fat/dir.h`.

## Files changed for the mkdir/remove/rmdir phase

- `lib/fs/fat/dir.cpp` — `fat_dir::rmdir` implementation
- `lib/fs/fat/dir.h` — `rmdir` declaration
- `lib/fs/fat/fat_priv.h`
- `lib/fs/fat/fs.cpp` — ClnShutBit methods (`set_volume_clean_bit_locked`, call sites in mount/unmount)
- `lib/fs/fat/fat_fs.h` — `mark_volume_dirty_locked`, `mark_volume_clean_locked`, `set_volume_clean_bit_locked` declarations
- `lib/fs/fat/test/test.cpp` — rmdir tests
- `lib/fs/fs.c` — `fs_remove_dir()` implementation
- `lib/fs/include/lib/fs.h` — `fs_remove_dir` declaration and `rmdir` hook in `fs_api`
- `lib/fs/shell.c` — `rmdir` shell command

## Files changed for test tooling / trace cleanup

- `lib/fs/fat/fat.cpp` — `LOCAL_TRACE` lowered from 1 to 0 (quiets cluster-chain debug noise)
- `lib/fs/fat/file.cpp` — `LOCAL_TRACE` lowered from 1 to 0
- `scripts/run-qemu-boot-tests.py` — added `--disk / -d` flag
- `scripts/run-fat-tests.sh` — new wrapper: mkblk + run-qemu-boot-tests + fsck

## Validation performed

Validation is now fully automated via `scripts/run-fat-tests.sh`:

```bash
./scripts/run-fat-tests.sh              # test fat12, fat16, fat32
./scripts/run-fat-tests.sh fat32        # single type
```

The script:

1. Calls `lib/fs/fat/test/mkblk` to regenerate fresh disk images.
2. For each image type, runs `scripts/run-qemu-boot-tests.py --arch arm64 -d <image>`
   (uses `RUN_UNITTESTS_AT_BOOT=1`; FAT tests run automatically at boot).
3. Runs `fsck.fat -vn <image>` on each image after QEMU exits.

`run-qemu-boot-tests.py` also gained a `--disk / -d` flag (repeatable) so any
disk image can be passed through to the underlying `do-qemuarm` `-d` switch.

Latest validated result (as of commit `30b7de6c7` on 2026-05-03):

- FAT12: all 16 `CASE fat` subtests passed, `fsck.fat -vn` clean
- FAT16: all 16 `CASE fat` subtests passed, `fsck.fat -vn` clean
- FAT32: all 16 `CASE fat` subtests passed, `fsck.fat -vn` clean

`run-qemu-boot-tests.py --arch arm64` (no disk) still passes 10/10 cases;
FAT subtests are skipped cleanly when no `virtio0` device is present.

Latest validation after LFN read-path fix:

- `scripts/run-fat-tests.sh` passes for FAT12/FAT16/FAT32 with clean `fsck.fat -vn`
  on all three images.
- `CASE fat` now reports 16/16 subtests passed.

## Important caveats / current limitations

- LFN create path is implemented for supported UTF-8 input that maps to UCS-2 BMP code points.
- UTF-8 that is malformed, surrogate-range, or non-BMP remains intentionally rejected.
- Remove supports file deletion only; there is no directory-delete behavior in this path — `rmdir` is now a separate API (see above).

## What still needs to be done

### Functional gaps

- **No Timestamps** (`fat_priv.h:43`, `dir.cpp`): The `dir_entry` struct has `// TODO time`
  and all time fields (creation, access, modification) are written as zero. The FAT
  format stores these, but LK ignores them entirely. `stat_file_priv` returns zeroed
  timestamps.

- **No Device Size Validation on Mount** (`fs.cpp:306`): The mount code trusts the BPB
  `total_sectors` field without verifying it doesn't exceed the actual block device size.
  A corrupted or malformed BPB could cause reads past the device end.

- **Unmount Doesn't Check Active Files** (`fs.cpp:426`): `fs_unmount` will silently
  proceed even if files or directories are still open, which could leave the filesystem
  in an inconsistent state (dirty buffers never flushed).

- **Weak Attribute Validation** (`file.cpp:115`): `open_file_priv` only checks whether
  an entry is a directory or not. It doesn't reject entries with `volume_id` or other
  special attributes that shouldn't be opened as regular files.

### Performance

- **FSInfo Free Cluster Hint Unused** (`fat.cpp:257`): When allocating new clusters, the
  free-cluster search always starts from cluster 2 instead of using the FSInfo hint
  (`fsinfo_next_free`). On larger filesystems this causes unnecessary linear scans.

### Code quality

- **Cluster Extension Code Duplication** (`file.cpp:392`): The directory growth code in
  `fat_dir_allocate` allocates new clusters for directory expansion, but the logic isn't
  shared with the file truncate/growth code in `file.cpp`. A shared helper would reduce
  duplication.

### Likely next FAT work (tests)

- Add more targeted mkdir/remove/rmdir tests:
  - mkdir/remove in non-root FAT12/16 and FAT32 directories with more edge cases
  - invalid-name cases
  - dot/dotdot content verification through direct directory reads if needed
  - direct edge cases around LFN record boundaries and pre-existing deleted slots

### Cleanup / polish

- If these WIP commits are going to be turned into a final series, they will probably
  need squashing/rewording into a cleaner history.
- If desired, update FAT documentation or test notes to reflect write/mkdir/remove coverage.
- Remove temporary verbose deletion tracing (`LTRACEF_LEVEL(2)`) in
  `mark_entry_record_deleted` once active debugging is complete.
- **Code Style**: Ensure `clang-format -i lib/fs/fat/*` is run after making changes to
  maintain consistency.

## Recommended resume point

If resuming on a new machine, start with:

1. `git checkout fat`
2. `git log --oneline --decorate -8`
3. `./scripts/run-fat-tests.sh` — runs mkblk, QEMU boot tests, and fsck for all three image types

After that, the most natural next items are:

1. Add mount-time device size validation
2. Add FSInfo free-cluster hint usage during allocation
3. Fix unmount to reject or flush open files/dirs
4. Tighten special-attribute validation in open/stat paths
