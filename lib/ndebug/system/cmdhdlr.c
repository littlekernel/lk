/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#include <lib/ndebug/system/cmdhdlr.h>

#include <assert.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lib/ndebug/system/mux.h>
#include <list.h>

typedef struct {
    struct list_node node;
    uint32_t opcode;
    cmdhdlr_callback_t cb;
} cmdhdlr_entry_t;

static mutex_t entries_mutex = MUTEX_INITIAL_VALUE(entries_mutex);
static struct list_node entries = LIST_INITIAL_VALUE(entries);

// NB: Caller must hold entries_mutex
static cmdhdlr_callback_t get_handler_for_opcode_unsafe(const uint32_t opcode)
{
    cmdhdlr_callback_t result = NULL;
    cmdhdlr_entry_t *entry;
    list_for_every_entry(&entries, entry, cmdhdlr_entry_t, node) {
        if (entry->opcode == opcode) {
            result = entry->cb;
        }
    }
    return result;
}


static int cmdhdlr_dispatcher_thread(void *arg)
{
    uint8_t *buf;
    while (true) {
        ssize_t bytes =
            ndebug_read_sys(&buf, NDEBUG_SYS_CHANNEL_COMMAND, INFINITE_TIME);

        // Packet not long enough to contain an opcode.
        if (bytes <= (ssize_t)sizeof(uint32_t)) continue;

        // Extract the opcode from the packet.
        uint32_t opcode = *((uint32_t *)buf);

        // Lookup the handler for the opcode.
        mutex_acquire(&entries_mutex);
        cmdhdlr_callback_t handler = get_handler_for_opcode_unsafe(opcode);
        mutex_release(&entries_mutex);

        // Call the handler if one was found.
        if (handler) {
            dprintf(INFO, "dispatch command, opcode = 0x%x\n", opcode);
            status_t rc = handler(buf + sizeof(opcode), bytes - sizeof(opcode));
            dprintf(INFO, "handler returns %d\n", rc);
            // NB: The contents of buf may be invalid here since the callee may
            //     have modified them. Set buf to NULL to prevent accidental
            //     use.
            buf = NULL;
        } else {
            dprintf(INFO, "no command registered for opcode = 0x%x\n", opcode);
        }
    }

    return 0;
}


void cmdhdlr_register_handler(const uint32_t opcode, cmdhdlr_callback_t cb)
{
    mutex_acquire(&entries_mutex);

    // Make sure two commands aren't trying to register the same opcode.
    cmdhdlr_callback_t result = get_handler_for_opcode_unsafe(opcode);
    DEBUG_ASSERT(result == NULL);

    // Allocate a new entry.
    cmdhdlr_entry_t *new_entry = malloc(sizeof(*new_entry));
    new_entry->cb = cb;
    new_entry->opcode = opcode;
    list_add_tail(&entries, &new_entry->node);

    mutex_release(&entries_mutex);
}

void cmdhdlr_init(void)
{
    thread_resume(
        thread_create("cmdhdlr dispatcher", &cmdhdlr_dispatcher_thread, NULL,
                      DEFAULT_PRIORITY, DEFAULT_STACK_SIZE)
    );
}