# FAT Work Handoff

This file summarizes the FAT read/write/restructure work currently on `wip/fat`, what is validated, and what still appears incomplete.

## Current branch state

- Branch: `fat`
- Current HEAD: `17b3dd2b4` - `[lib][fs] format all of the lib/fs files with clang format`
- Recent WIP commits in this series:
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

Latest validated result (as of commit `17b3dd2b4` on 2026-05-01):

- FAT12: all 10 `CASE fat` subtests passed, `fsck.fat -vn` clean
- FAT16: all 10 `CASE fat` subtests passed, `fsck.fat -vn` clean
- FAT32: all 10 `CASE fat` subtests passed, `fsck.fat -vn` clean

`run-qemu-boot-tests.py --arch arm64` (no disk) still passes 10/10 cases;
FAT subtests are skipped cleanly when no `virtio0` device is present.

## Important caveats / current limitations

- LFN create path is implemented for supported UTF-8 input that maps to UCS-2 BMP code points.
- UTF-8 that is malformed, surrogate-range, or non-BMP remains intentionally rejected.
- Remove supports file deletion only; there is no directory-delete behavior in this path — `rmdir` is now a separate API (see above).

## What may still need to be done

### Likely next FAT work

- Add more targeted mkdir/remove/rmdir tests if desired:
  - mkdir/remove in non-root FAT12/16 and FAT32 directories with more edge cases
  - invalid-name cases
  - dot/dotdot content verification through direct directory reads if needed
  - direct edge cases around LFN record boundaries and pre-existing deleted slots

### Cleanup / polish

- If these WIP commits are going to be turned into a final series, they will probably need squashing/rewording into a cleaner history.
- If desired, update FAT documentation or test notes to reflect write/mkdir/remove coverage.
- Remove temporary verbose deletion tracing (`printf`) in `mark_entry_record_deleted` once active debugging is complete.
- **Code Style**: Ensure `clang-format -i lib/fs/fat/*` is run after making changes to maintain consistency.

## Recommended resume point

If resuming on a new machine, start with:

1. `git checkout fat`
2. `git log --oneline --decorate -8`
3. `./scripts/run-fat-tests.sh` — runs mkblk, QEMU boot tests, and fsck for all three image types

After that, the most natural next feature is long filename creation support for create/mkdir.
