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

#include <compiler.h>
#include <debug.h>
#include <trace.h>

#define LOCAL_TRACE 0
#define TRACE_INIT 0

extern const struct lk_init_struct __lk_init[];
extern const struct lk_init_struct __lk_init_end[];

enum init_level_type {
    INIT_LEVEL_TYPE_PRIMARY_CPU,
    INIT_LEVEL_TYPE_SECONDARY_CPUS,
    INIT_LEVEL_TYPE_COUNT
};
static uint last_init_level[INIT_LEVEL_TYPE_COUNT];

int lk_init_level_common(uint level, enum init_level_type type)
{
    LTRACEF("level %#x, last_init_level %#x\n", level, last_init_level[type]);

    uint required_flag = (type == INIT_LEVEL_TYPE_PRIMARY_CPU) ?
        LK_INIT_FLAG_PRIMARY_CPU : LK_INIT_FLAG_SECONDARY_CPUS;
    uint last_called_level = last_init_level[type];
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
            if (ptr->level > level)
                continue;
            if (ptr->level < last_called_level)
                continue;
            if (found && found->level <= ptr->level)
                continue;

            /* keep the lowest one we haven't called yet */
            if (ptr->level > last_init_level[type] && ptr->level > last_called_level) {
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
        printf("INIT: cpu %d, calling hook %p (%s) at level %#x, flags %#x\n",
               arch_curr_cpu_num(), found->hook, found->name, found->level, found->flags);
#endif
        found->hook(found->level);
        last_called_level = found->level;
        last = found;
    }

    last_init_level[type] = level;

    return 0;
}

int lk_init_level(uint level)
{
    return lk_init_level_common(level, INIT_LEVEL_TYPE_PRIMARY_CPU);
}

/* Since there may be multiple secondary CPUs, we allow reseting the
 * last_init_level for secondary CPUs so that we can call the same handlers
 * on all secondary CPUs without having a separate last_init_level for each CPU.
 */
void lk_secondary_cpu_reset_init_level(void)
{
    last_init_level[INIT_LEVEL_TYPE_SECONDARY_CPUS] = 0;
}

int lk_secondary_cpu_init_level(uint level)
{
    return lk_init_level_common(level, INIT_LEVEL_TYPE_SECONDARY_CPUS);
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
