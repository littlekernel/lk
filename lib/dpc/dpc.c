/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <stddef.h>
#include <lk/list.h>
#include <malloc.h>
#include <lk/err.h>
#include <lib/dpc.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <lk/init.h>

struct dpc {
    struct list_node node;

    dpc_callback cb;
    void *arg;
};

static struct list_node dpc_list = LIST_INITIAL_VALUE(dpc_list);
static event_t dpc_event;

static int dpc_thread_routine(void *arg);

status_t dpc_queue(dpc_callback cb, void *arg, uint flags) {
    struct dpc *dpc;

    dpc = malloc(sizeof(struct dpc));

    if (dpc == NULL)
        return ERR_NO_MEMORY;

    dpc->cb = cb;
    dpc->arg = arg;
    enter_critical_section();
    list_add_tail(&dpc_list, &dpc->node);
    event_signal(&dpc_event, (flags & DPC_FLAG_NORESCHED) ? false : true);
    exit_critical_section();

    return NO_ERROR;
}

static int dpc_thread_routine(void *arg) {
    for (;;) {
        event_wait(&dpc_event);

        enter_critical_section();
        struct dpc *dpc = list_remove_head_type(&dpc_list, struct dpc, node);
        if (!dpc)
            event_unsignal(&dpc_event);
        exit_critical_section();

        if (dpc) {
//          dprintf("dpc calling %p, arg %p\n", dpc->cb, dpc->arg);
            dpc->cb(dpc->arg);

            free(dpc);
        }
    }

    return 0;
}

static void dpc_init(uint level) {
    event_init(&dpc_event, false, 0);

    thread_detach_and_resume(thread_create("dpc", &dpc_thread_routine, NULL, DPC_PRIORITY, DEFAULT_STACK_SIZE));
}

LK_INIT_HOOK(libdpc, &dpc_init, LK_INIT_LEVEL_THREADING);


