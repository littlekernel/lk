#!/usr/bin/env sh
#
# Copyright (c) 2026 Travis Geiselbrecht
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT

set -eu

WORKDIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
IMG="$(mktemp /tmp/disktest-corrupt-XXXXXX.img)"
OUT="$(mktemp /tmp/disktest-verify-XXXXXX.out)"
BLOCK_TMP="$(mktemp /tmp/disktest-block-XXXXXX.bin)"
BLOCK_TMP2="$(mktemp /tmp/disktest-block2-XXXXXX.bin)"
BLOCK_SIZE=512
IMG_SIZE=1M
PASS_COUNT=0

cleanup() {
    rm -f "$IMG" "$OUT" "$BLOCK_TMP" "$BLOCK_TMP2"
}
trap cleanup EXIT INT TERM

reset_image() {
    truncate -s "$IMG_SIZE" "$IMG"
    "$WORKDIR/disktest" fill "$IMG" "$BLOCK_SIZE" >/dev/null
    "$WORKDIR/disktest" verify "$IMG" "$BLOCK_SIZE" >/dev/null
}

run_verify_expect_failure() {
    case_name="$1"
    expected_pattern="$2"
    forbidden_pattern="${3:-}"
    expected_block="$4"
    expected_reason="$5"
    expected_mismatch_offset_in_block="$6"
    expected_header_offset="${7:-}"

    if "$WORKDIR/disktest" verify "$IMG" "$BLOCK_SIZE" >"$OUT" 2>&1; then
        echo "FAIL: $case_name: verify succeeded after corruption"
        cat "$OUT"
        exit 1
    fi

    if ! grep -qi "verify failed at block" "$OUT"; then
        echo "FAIL: $case_name: verify failed but missing block failure details"
        cat "$OUT"
        exit 1
    fi

    if ! grep -qi "$expected_pattern" "$OUT"; then
        echo "FAIL: $case_name: expected failure reason pattern '$expected_pattern' not found"
        cat "$OUT"
        exit 1
    fi

    if [ -n "$forbidden_pattern" ] && grep -Eqi "$forbidden_pattern" "$OUT"; then
        echo "FAIL: $case_name: got unexpected failure reason matching '$forbidden_pattern'"
        cat "$OUT"
        exit 1
    fi

    line="$(grep -m1 "verify failed at block" "$OUT" || true)"
    if [ -z "$line" ]; then
        echo "FAIL: $case_name: could not parse verify failure line"
        cat "$OUT"
        exit 1
    fi

    block_and_offset="$(printf '%s\n' "$line" | sed -n 's/.*verify failed at block \([0-9][0-9]*\) (byte offset \([0-9][0-9]*\)).*/\1 \2/p')"
    if [ -z "$block_and_offset" ]; then
        echo "FAIL: $case_name: output format did not include parseable block/offset"
        echo "$line"
        exit 1
    fi
    actual_block="$(printf '%s\n' "$block_and_offset" | awk '{print $1}')"
    actual_offset="$(printf '%s\n' "$block_and_offset" | awk '{print $2}')"

    actual_reason="$(printf '%s\n' "$line" | sed -n 's/.*): \(.*\); header_offset=.*/\1/p')"
    if [ -z "$actual_reason" ]; then
        echo "FAIL: $case_name: could not parse reason from output"
        echo "$line"
        exit 1
    fi

    actual_mismatch_offset_in_block="$(printf '%s\n' "$line" | sed -n 's/.*mismatch_offset_in_block=\([0-9][0-9]*\).*/\1/p')"
    if [ -z "$actual_mismatch_offset_in_block" ]; then
        echo "FAIL: $case_name: could not parse mismatch_offset_in_block from output"
        echo "$line"
        exit 1
    fi

    actual_header_offset="$(printf '%s\n' "$line" | sed -n 's/.*header_offset=\(0x[0-9a-fA-F][0-9a-fA-F]*\).*/\1/p')"
    if [ -z "$actual_header_offset" ]; then
        echo "FAIL: $case_name: could not parse header_offset from output"
        echo "$line"
        exit 1
    fi

    expected_offset=$((expected_block * BLOCK_SIZE))
    if [ "$actual_block" -ne "$expected_block" ]; then
        echo "FAIL: $case_name: expected block $expected_block, got $actual_block"
        echo "$line"
        exit 1
    fi

    if [ "$actual_offset" -ne "$expected_offset" ]; then
        echo "FAIL: $case_name: expected byte offset $expected_offset, got $actual_offset"
        echo "$line"
        exit 1
    fi

    if [ "$actual_reason" != "$expected_reason" ]; then
        echo "FAIL: $case_name: expected reason '$expected_reason', got '$actual_reason'"
        echo "$line"
        exit 1
    fi

    if [ "$actual_mismatch_offset_in_block" -ne "$expected_mismatch_offset_in_block" ]; then
        echo "FAIL: $case_name: expected mismatch_offset_in_block $expected_mismatch_offset_in_block, got $actual_mismatch_offset_in_block"
        echo "$line"
        exit 1
    fi

    if [ -n "$expected_header_offset" ] && [ "$actual_header_offset" != "$expected_header_offset" ]; then
        echo "FAIL: $case_name: expected header_offset $expected_header_offset, got $actual_header_offset"
        echo "$line"
        exit 1
    fi

    echo "---- disktest output: $case_name ----"
    cat "$OUT"
    echo "--------------------------------------"

    PASS_COUNT=$((PASS_COUNT + 1))
    echo "PASS: $case_name"
}

# Case 1: Wrong-placement corruption.
# Place a complete valid block 1 into block 0; payload and trailer stay valid for block 1,
# but the offset header should fail for block 0 verification.
reset_image
dd if="$IMG" of="$BLOCK_TMP" bs="$BLOCK_SIZE" skip=1 count=1 status=none
dd if="$BLOCK_TMP" of="$IMG" bs="$BLOCK_SIZE" seek=0 count=1 conv=notrunc status=none
run_verify_expect_failure "wrong block placement" "offset header mismatch" "payload mismatch|offset trailer mismatch" \
    "0" "offset header mismatch" "0" "0x00000001"

# Case 2: Header corruption.
reset_image
printf '\177' | dd of="$IMG" bs=1 seek=0 conv=notrunc status=none
run_verify_expect_failure "header byte flip" "offset header mismatch" "" \
    "0" "offset header mismatch" "0" "0x0000007f"

# Case 3: Payload corruption.
reset_image
printf '\125' | dd of="$IMG" bs=1 seek=16 conv=notrunc status=none
run_verify_expect_failure "payload byte flip" "payload mismatch" "" \
    "0" "payload mismatch" "16" "0x00000000"

# Case 4: Trailer corruption (stored ~offset).
reset_image
printf '\000' | dd of="$IMG" bs=1 seek=$((BLOCK_SIZE - 1)) conv=notrunc status=none
run_verify_expect_failure "trailer byte flip" "offset trailer mismatch" "" \
    "0" "offset trailer mismatch" "508" "0x00000000"

# Case 5: Swap two blocks in place.
# Both blocks remain internally valid, but each is now at the wrong offset.
reset_image
dd if="$IMG" of="$BLOCK_TMP" bs="$BLOCK_SIZE" skip=7 count=1 status=none
dd if="$IMG" of="$BLOCK_TMP2" bs="$BLOCK_SIZE" skip=123 count=1 status=none
dd if="$BLOCK_TMP2" of="$IMG" bs="$BLOCK_SIZE" seek=7 count=1 conv=notrunc status=none
dd if="$BLOCK_TMP" of="$IMG" bs="$BLOCK_SIZE" seek=123 count=1 conv=notrunc status=none
run_verify_expect_failure "swap two blocks" "offset header mismatch" "payload mismatch|offset trailer mismatch" \
    "7" "offset header mismatch" "0" "0x0000007b"

echo "PASS: corruption suite complete ($PASS_COUNT cases)"
