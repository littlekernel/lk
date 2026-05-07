#!/usr/bin/env bash
# Run FAT filesystem tests: build disk images, run QEMU boot tests, then fsck.
# Usage: run-fat-tests.sh [fat12|fat16|fat32] ...
#   With no arguments, tests all three image types.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LK_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FAT_TEST_DIR="$LK_ROOT/lib/fs/fat/test"
BOOT_TEST_SCRIPT="$SCRIPT_DIR/run-qemu-boot-tests.py"

# Map type names to image filenames
declare -A IMAGES=(
    [fat12]="$FAT_TEST_DIR/blk.bin.fat12"
    [fat16]="$FAT_TEST_DIR/blk.bin.fat16"
    [fat32]="$FAT_TEST_DIR/blk.bin.fat32"
)

# Determine which types to test
if [[ $# -gt 0 ]]; then
    TYPES=("$@")
else
    TYPES=(fat12 fat16 fat32)
fi

# Validate requested types
for t in "${TYPES[@]}"; do
    if [[ -z "${IMAGES[$t]}" ]]; then
        echo "Unknown FAT type: $t (choose from fat12, fat16, fat32)" >&2
        exit 1
    fi
done

echo "=== Building disk images ==="
(cd "$FAT_TEST_DIR" && ./mkblk)

OVERALL=0

for t in "${TYPES[@]}"; do
    img="${IMAGES[$t]}"
    echo ""
    echo "=== Running QEMU boot tests with $t image ==="
    if "$BOOT_TEST_SCRIPT" --arch arm64 -d "$img"; then
        qemu_ok=0
    else
        qemu_ok=1
        OVERALL=1
    fi

    echo ""
    echo "=== fsck $t ($img) ==="
    if fsck.fat -vn "$img"; then
        echo "✓ $t fsck: OK"
    else
        echo "✗ $t fsck: ERRORS FOUND"
        OVERALL=1
    fi
done

echo ""
if [[ $OVERALL -eq 0 ]]; then
    echo "✓ All FAT tests and image checks passed."
else
    echo "✗ One or more FAT tests or image checks failed."
fi

exit $OVERALL
