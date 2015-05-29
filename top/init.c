/*
 * Copyright (c) 2013 Travis Geiselbrecht
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

/*
 * Global init hook mechanism. Allows code anywhere in the system to define
 * a init hook that is called at increasing init levels as the system is
 * initialized.
 */
#include <arch/ops.h>
#include <lk/init.h>

#include <assert.h>
#include <compiler.h>
#include <debug.h>
#include <trace.h>

#define LOCAL_TRACE 0
#define TRACE_INIT (LK_DEBUGLEVEL >= 2)
#ifndef EARLIEST_TRACE_LEVEL
#define EARLIEST_TRACE_LEVEL LK_INIT_LEVEL_ARCH_EARLY
#endif

extern const struct lk_init_struct __lk_init[];
extern const struct lk_init_struct __lk_init_end[];

void lk_init_level(enum lk_init_flags required_flag, uint start_level, uint stop_level)
{
    LTRACEF("flags %#x, start_level %#x, stop_level %#x\n",
            required_flag, start_level, stop_level);

    ASSERT(start_level > 0);
    uint last_called_level = start_level - 1;
    const struct lk_init_struct *last = NULL;
    for (;;) {
        /* search for the lowest uncalled hook to call */
        LTRACEF("last %p, last_called_level %#x\n", last, last_called_level);

        const struct lk_init_struct *found = NULL;
        bool seen_last = false;
        for (const struct lk_init_struct *ptr = __lk_init; ptr != __lk_init_end; ptr++) {
            LTRACEF("looking at %p (%s) level %#x, flags %#x, seen_last %d\n", ptr, ptr->name, ptr->level, ptr->flags, seen_last);

            if (ptr == last)
                seen_last = true;

            /* reject the easy ones */
            if (!(ptr->flags & required_flag))
                continue;
            if (ptr->level > stop_level)
                continue;
            if (ptr->level < last_called_level)
                continue;
            if (found && found->level <= ptr->level)
                continue;

            /* keep the lowest one we haven't called yet */
            if (ptr->level >= start_level && ptr->level > last_called_level) {
                found = ptr;
                continue;
            }

            /* if we're at the same level as the last one we called and we've
             * already passed over it this time around, we can mark this one
             * and early terminate the loop.
             */
            if (ptr->level == last_called_level && ptr != last && seen_last) {
                found = ptr;
                break;
            }
        }

        if (!found)
            break;

#if TRACE_INIT
        if (found->level >= EARLIEST_TRACE_LEVEL) {
            printf("INIT: cpu %d, calling hook %p (%s) at level %#x, flags %#x\n",
                   arch_curr_cpu_num(), found->hook, found->name, found->level, found->flags);
        }
#endif
        found->hook(found->level);
        last_called_level = found->level;
        last = found;
    }
}

#if 0
void test_hook(uint level)
{
    LTRACEF("level %#x\n", level);
}
void test_hook1(uint level)
{
    LTRACEF("level %#x\n", level);
}
void test_hook1a(uint level)
{
    LTRACEF("level %#x\n", level);
}
void test_hook1b(uint level)
{
    LTRACEF("level %#x\n", level);
}
void test_hook2(uint level)
{
    LTRACEF("level %#x\n", level);
}

LK_INIT_HOOK(test, test_hook, 1);
LK_INIT_HOOK(test1, test_hook1, 1);
LK_INIT_HOOK(test2, test_hook2, 2);
LK_INIT_HOOK(test1a, test_hook1a, 1);
LK_INIT_HOOK(test1b, test_hook1b, 1);
#endif

// vim: set ts=4 sw=4 expandtab:
