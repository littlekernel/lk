/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/backtrace.h>

#include <kernel/thread.h>
#include <lk/compiler.h>
#include <lk/console_cmd.h>
#include <stdio.h>

#if WITH_LIB_SYMTAB
#include <lib/symtab.h>
#endif

/* Deep enough to be useful, shallow enough that a corrupt stack cannot spew
 * for long. The loop stops at the first frame that fails a check anyway.
 */
#define MAX_FRAMES 32

/* A frame pointer that cannot be bounds checked against a thread stack is
 * only trusted this far above where the walk started.
 */
#define FALLBACK_STACK_WINDOW (64 * 1024)

/* The two word frame record is {saved frame pointer, return address} on every
 * architecture here, but not in the same place: AArch64 and x86 point the
 * frame pointer at the record, while the RISC-V psABI points it at the
 * canonical frame address, just past the end of the record.
 */
static inline uintptr_t frame_record(uintptr_t fp) {
#if ARCH_RISCV
    return fp - 2 * sizeof(uintptr_t);
#else
    return fp;
#endif
}

/* The stack the walk is allowed to read from.
 *
 * On the architectures this module supports, kernel exceptions run on the
 * stack of the thread they interrupted, so the current thread's stack bounds
 * the frames of a fault as well as those of an ordinary call.
 */
static void stack_bounds(uintptr_t fp, uintptr_t *lo, uintptr_t *hi) {
    const thread_t *t = get_current_thread();

    /* Early in boot, and on a stack that a corrupt thread pointer no longer
     * describes, fall back to trusting a window above where we started.
     */
    if (t && t->magic == THREAD_MAGIC && t->stack && t->stack_size > 0) {
        *lo = (uintptr_t)t->stack;
        *hi = *lo + t->stack_size;
        return;
    }

    *lo = fp;
    *hi = fp + FALLBACK_STACK_WINDOW;
}

static bool frame_valid(uintptr_t fp, uintptr_t prev_fp, uintptr_t lo, uintptr_t hi) {
    if (fp == 0) {
        return false;
    }
    /* A misaligned frame pointer is garbage, and dereferencing it may fault
     * on architectures that care.
     */
    if (fp & (sizeof(uintptr_t) - 1)) {
        return false;
    }
    /* Stacks grow down, so each caller's frame is above its callee's. This is
     * what stops a cycle in a corrupt stack from looping forever.
     */
    if (prev_fp != 0 && fp <= prev_fp) {
        return false;
    }

    const uintptr_t record = frame_record(fp);
    const uintptr_t record_end = record + 2 * sizeof(uintptr_t);
    if (record < lo || record_end > hi || record_end < record) {
        return false;
    }
    return true;
}

size_t backtrace_capture(uintptr_t pc, uintptr_t fp, uintptr_t *pcs, size_t max) {
    size_t count = 0;

    if (max == 0) {
        return 0;
    }

    /* The interrupted or current pc is frame zero: it has no frame record of
     * its own, and a leaf function may not have set up a frame pointer at all.
     */
    if (pc != 0) {
        pcs[count++] = pc;
    }

    uintptr_t lo, hi;
    stack_bounds(fp, &lo, &hi);

    uintptr_t prev_fp = 0;
    while (count < max && frame_valid(fp, prev_fp, lo, hi)) {
        const uintptr_t *record = (const uintptr_t *)frame_record(fp);
        const uintptr_t next_fp = record[0];
        const uintptr_t ret = record[1];

        /* A record with no parent frame belongs to the outermost function,
         * whose saved return address was never a call site: a thread starts
         * in initial_thread_func with the link register pointing at it.
         */
        if (next_fp == 0 || ret == 0) {
            break;
        }
        pcs[count++] = ret;

        prev_fp = fp;
        fp = next_fp;
    }

    return count;
}

static void print_frame(size_t index, uintptr_t pc, bool is_return_address) {
#if WITH_LIB_SYMTAB
    /* A return address is the instruction after the call, which lands in the
     * next function when the call was the last thing the caller did. Name the
     * call itself rather than whatever follows it.
     */
    const uintptr_t lookup = is_return_address ? pc - 1 : pc;

    uintptr_t base;
    const char *name = symtab_lookup(lookup, &base);
    if (name) {
        printf("#%02zu %p %s+0x%lx\n", index, (void *)pc, name,
               (unsigned long)(lookup - base));
        return;
    }
#endif
    printf("#%02zu %p\n", index, (void *)pc);
}

void backtrace_print(uintptr_t pc, uintptr_t fp) {
    /* If printing a frame faults, the fault handler will try to print a
     * backtrace of its own. Report the first one and let the second fail
     * quietly rather than recursing until the stack runs out.
     */
    static bool printing = false;
    if (printing) {
        printf("backtrace: recursed, skipping\n");
        return;
    }
    printing = true;

    uintptr_t pcs[MAX_FRAMES];
    const size_t count = backtrace_capture(pc, fp, pcs, countof(pcs));

    /* Without a pc to start from, every entry came out of a frame record. */
    const bool have_pc = (pc != 0);

    printf("backtrace:\n");
    for (size_t i = 0; i < count; i++) {
        print_frame(i, pcs[i], !have_pc || i > 0);
    }
    if (count == 0) {
        printf("  (no frames)\n");
    } else if (count == countof(pcs)) {
        printf("  (truncated at %zu frames)\n", count);
    }

    printing = false;
}

void backtrace_print_current(void) {
    /* No pc: this frame's own record supplies the return address into the
     * caller, which is the first frame worth reporting.
     */
    backtrace_print(0, (uintptr_t)__GET_FRAME());
}

#if LK_DEBUGLEVEL > 1
static int cmd_bt(int argc, const console_cmd_args *argv) {
    backtrace_print_current();
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND_MASKED("bt", "print a backtrace of the current stack", &cmd_bt, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_END(backtrace);
#endif
