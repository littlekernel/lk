/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <debug.h>
#include <bits.h>
#include <arch/arch_ops.h>
#include <arch/arm64.h>

#define SHUTDOWN_ON_FATAL 1

struct fault_handler_table_entry {
    uint64_t pc;
    uint64_t fault_handler;
};

extern struct fault_handler_table_entry __fault_handler_table_start[];
extern struct fault_handler_table_entry __fault_handler_table_end[];

static void dump_iframe(const struct arm64_iframe_long *iframe)
{
    printf("iframe %p:\n", iframe);
    printf("x0  0x%16llx x1  0x%16llx x2  0x%16llx x3  0x%16llx\n", iframe->r[0], iframe->r[1], iframe->r[2], iframe->r[3]);
    printf("x4  0x%16llx x5  0x%16llx x6  0x%16llx x7  0x%16llx\n", iframe->r[4], iframe->r[5], iframe->r[6], iframe->r[7]);
    printf("x8  0x%16llx x9  0x%16llx x10 0x%16llx x11 0x%16llx\n", iframe->r[8], iframe->r[9], iframe->r[10], iframe->r[11]);
    printf("x12 0x%16llx x13 0x%16llx x14 0x%16llx x15 0x%16llx\n", iframe->r[12], iframe->r[13], iframe->r[14], iframe->r[15]);
    printf("x16 0x%16llx x17 0x%16llx x18 0x%16llx x19 0x%16llx\n", iframe->r[16], iframe->r[17], iframe->r[18], iframe->r[19]);
    printf("x20 0x%16llx x21 0x%16llx x22 0x%16llx x23 0x%16llx\n", iframe->r[20], iframe->r[21], iframe->r[22], iframe->r[23]);
    printf("x24 0x%16llx x25 0x%16llx x26 0x%16llx x27 0x%16llx\n", iframe->r[24], iframe->r[25], iframe->r[26], iframe->r[27]);
    printf("x28 0x%16llx x29 0x%16llx lr  0x%16llx usp 0x%16llx\n", iframe->r[28], iframe->r[29], iframe->lr, iframe->usp);
    printf("elr 0x%16llx\n", iframe->elr);
    printf("spsr 0x%16llx\n", iframe->spsr);
}

__WEAK void arm64_syscall(struct arm64_iframe_long *iframe, bool is_64bit)
{
    panic("unhandled syscall vector\n");
}

void arm64_sync_exception(struct arm64_iframe_long *iframe)
{
    struct fault_handler_table_entry *fault_handler;
    uint32_t esr = ARM64_READ_SYSREG(esr_el1);
    uint32_t ec = BITS_SHIFT(esr, 31, 26);
    uint32_t il = BIT(esr, 25);
    uint32_t iss = BITS(esr, 24, 0);

    switch (ec) {
        case 0b000111: /* floating point */
            arm64_fpu_exception(iframe);
            return;
        case 0b010001: /* syscall from arm32 */
        case 0b010101: /* syscall from arm64 */
#ifdef WITH_LIB_SYSCALL
            void arm64_syscall(struct arm64_iframe_long *iframe);
            arch_enable_fiqs();
            arm64_syscall(iframe);
            arch_disable_fiqs();
            return;
#else
            arm64_syscall(iframe, (ec == 0x15) ? true : false);
            return;
#endif
        case 0b100000: /* instruction abort from lower level */
        case 0b100001: /* instruction abort from same level */
            printf("instruction abort: PC at 0x%llx\n", iframe->elr);
            break;
        case 0b100100: /* data abort from lower level */
        case 0b100101: { /* data abort from same level */
            for (fault_handler = __fault_handler_table_start;
                    fault_handler < __fault_handler_table_end;
                    fault_handler++) {
                if (fault_handler->pc == iframe->elr) {
                    iframe->elr = fault_handler->fault_handler;
                    return;
                }
            }

            /* read the FAR register */
            uint64_t far = ARM64_READ_SYSREG(far_el1);

            /* decode the iss */
            if (BIT(iss, 24)) { /* ISV bit */
                printf("data fault: PC at 0x%llx, FAR 0x%llx, iss 0x%x (DFSC 0x%lx)\n",
                       iframe->elr, far, iss, BITS(iss, 5, 0));
            } else {
                printf("data fault: PC at 0x%llx, FAR 0x%llx, iss 0x%x\n", iframe->elr, far, iss);
            }

            break;
        }
        default:
            printf("unhandled synchronous exception\n");
    }

    /* unhandled exception, die here */
    printf("ESR 0x%x: ec 0x%x, il 0x%x, iss 0x%x\n", esr, ec, il, iss);
    dump_iframe(iframe);

    panic("die\n");
}

void arm64_invalid_exception(struct arm64_iframe_long *iframe, unsigned int which)
{
    printf("invalid exception, which 0x%x\n", which);
    dump_iframe(iframe);

    panic("die\n");
}



