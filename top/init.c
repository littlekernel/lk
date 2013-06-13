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
#include <lk/init.h>

#include <compiler.h>
#include <debug.h>
#include <trace.h>

#define LOCAL_TRACE 0
#define TRACE_INIT 0

extern const struct lk_init_struct __lk_init[];
extern const struct lk_init_struct __lk_init_end[];

static uint last_init_level = 0;

int lk_init_level(uint level)
{
    LTRACEF("level %#x, last_init_level %#x\n", level, last_init_level);

    uint last_called_level = last_init_level;
    const struct lk_init_struct *last = NULL;
    for (;;) {
        /* search for the lowest uncalled hook to call */
        LTRACEF("last %p, last_called_level %#x\n", last, last_called_level);

        const struct lk_init_struct *found = NULL;
        bool seen_last = false;
        for (const struct lk_init_struct *ptr = __lk_init; ptr != __lk_init_end; ptr++) {
            LTRACEF("looking at %p (%s) level %#x, seen_last %d\n", ptr, ptr->name, ptr->level, seen_last);

            if (ptr == last)
                seen_last = true;

            /* reject the easy ones */
            if (ptr->level > level)
                continue;
            if (ptr->level < last_called_level)
                continue;
            if (found && found->level <= ptr->level)
                continue;

            /* keep the lowest one we haven't called yet */
            if (ptr->level > last_init_level && ptr->level > last_called_level) {
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
        printf("INIT: calling hook %p (%s) at level %#x\n", found->hook, found->name, found->level);
#endif
        found->hook(found->level);
        last_called_level = found->level;
        last = found;
    }

    last_init_level = level;

    return 0;
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
