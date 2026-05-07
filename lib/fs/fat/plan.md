# FAT Work - To Do

This file tracks the remaining tasks and improvements for the FAT filesystem implementation.

## Functional Gaps

- **Timestamps** (`fat_priv.h`, `dir.cpp`): Implement proper creation, access, and modification time handling. Currently, these are written as zero and ignored during `stat`.
- **Device Size Validation on Mount** (`fs.cpp`): Verify the BPB `total_sectors` field against the actual block device size to prevent out-of-bounds reads on malformed images.
- **Unmount Active File Check** (`fs.cpp`): Ensure `fs_unmount` handles open files or directories correctly (either rejecting unmount or ensuring all buffers are flushed).
- **Attribute Validation** (`file.cpp`): Tighten `open_file_priv` to reject entries with `volume_id` or other special attributes that shouldn't be opened as regular files.

## Performance Improvements

- **FSInfo Free Cluster Hint** (`fat.cpp`): Use the `fsinfo_next_free` hint during cluster allocation to avoid linear scans from the beginning of the FAT on large volumes.

## Code Quality & Refactoring

- **Cluster Extension Consolidation**: Share the cluster allocation logic between directory growth (`fat_dir_allocate`) and file growth/truncate (`file.cpp`).

## Testing & Validation

- **Edge Case Tests**:
  - mkdir/remove in non-root directories on FAT12/16 and FAT32.
  - Invalid name handling.
  - Verification of `.` and `..` content via direct directory reads.
  - LFN record boundary conditions and reuse of deleted slots.

## Next Steps

1. Add mount-time device size validation.
2. Implement FSInfo free-cluster hint usage during allocation.
3. Fix unmount to properly handle or reject open files/dirs.
4. Tighten special-attribute validation in open/stat paths.
