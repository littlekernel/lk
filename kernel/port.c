/*
 * Copyright (c) 2015 Carlos Pizano-Uribe  cpu@chromium.org
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/**
 * @file
 * @brief  Port object functions
 * @defgroup event Events
 *
 */

#include <kernel/port.h>

#include <kernel/event.h>
#include <kernel/init.h>
#include <kernel/preempt.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/pow2.h>
#include <malloc.h>
#include <string.h>

// Ports have their own lock. It covers every field of every port object -- the
// named write port list, group membership, the magics, and the circular buffers
// -- and nothing else.
//
// Readers must not block holding it. This used to work with the thread lock
// only because that lock is handed off across a context switch, so the incoming
// thread drops it; a private lock has no such escape and a reader that blocked
// while holding it would wedge every writer. So blocking is done on an event_t
// with the port lock dropped, and the read is retried on wakeup. An event's
// signaled state is sticky, which is what closes the window between dropping
// the lock and blocking.
//
// Waking is done with the port lock *held*, so that a concurrent port_close()
// cannot free the object out from under a signaler that has already sampled the
// pointer. That makes the lock order port_lock -> wait queue, which is
// consistent: nothing ever takes the port lock while holding a wait queue lock.
// Waking under the lock does require preemption to be disabled around the
// region, since an inline reschedule would switch away still holding a lock
// nobody else is going to release.
static spin_lock_t port_lock = SPIN_LOCK_INITIAL_VALUE;

#define PORT_LOCK(state) arch_interrupt_saved_state_t state = spin_lock_irqsave(&port_lock)
#define PORT_UNLOCK(state) spin_unlock_irqrestore(&port_lock, state)

// write ports can be in two states, open and closed, which have a
// different magic number.

#define WRITEPORT_MAGIC_W (0x70727477) // 'prtw'
#define WRITEPORT_MAGIC_X (0x70727478) // 'prtx'

#define READPORT_MAGIC  (0x70727472)  // 'prtr'
#define PORTGROUP_MAGIC (0x70727467)  // 'prtg'
#define PORTHOLD_MAGIC (0x70727467)   // 'prth'

#define PORT_BUFF_SIZE      8
#define PORT_BUFF_SIZE_BIG 64

#define MAX_PORT_GROUP_COUNT 256

typedef struct {
    uint log2;
    uint avail;
    uint head;
    uint tail;
    port_packet_t packet[1];
} port_buf_t;

typedef struct {
    int magic;
    struct list_node node;
    port_buf_t *buf;
    struct list_node rp_list;
    port_mode_t mode;
    char name[PORT_NAME_LEN];
} write_port_t;

typedef struct {
    int magic;
    event_t event;
    struct list_node rp_list;
} port_group_t;

typedef struct {
    int magic;
    struct list_node w_node;
    struct list_node g_node;
    port_buf_t *buf;
    void *ctx;
    event_t event;
    write_port_t *wport;
    port_group_t *gport;
} read_port_t;


static struct list_node write_port_list;


static port_buf_t *make_buf(bool big) {
    uint pk_count = big ? PORT_BUFF_SIZE_BIG : PORT_BUFF_SIZE;
    uint size = sizeof(port_buf_t) + ((pk_count - 1) * sizeof(port_packet_t));
    port_buf_t *buf = malloc(size);
    if (!buf)
        return NULL;
    buf->log2 = log2_uint(pk_count);
    buf->head = buf->tail = 0;
    buf->avail = pk_count;
    return buf;
}

static inline bool buf_is_empty(port_buf_t *buf) {
    return buf->avail == valpow2(buf->log2);
}

static status_t buf_write(port_buf_t *buf, const port_packet_t *packets, size_t count) {
    if (buf->avail < count)
        return ERR_NOT_ENOUGH_BUFFER;

    for (size_t ix = 0; ix != count; ix++) {
        buf->packet[buf->tail] = packets[ix];
        buf->tail = modpow2(++buf->tail, buf->log2);
    }
    buf->avail -= count;
    return NO_ERROR;
}

static status_t buf_read(port_buf_t *buf, port_result_t *pr) {
    if (buf_is_empty(buf))
        return ERR_NO_MSG;
    pr->packet = buf->packet[buf->head];
    buf->head = modpow2(++buf->head, buf->log2);
    ++buf->avail;
    return NO_ERROR;
}

// Wake every thread blocked on an event, rather than the single thread
// event_signal() releases for an autounsignal event. The last call finds nobody
// waiting and leaves the event signaled, so a reader that arrives afterwards
// also gets to run and observe whatever changed.
static void event_signal_all(event_t *ev) {
    while (event_signal(ev) > 0)
        ;
}

// must be called before any use of ports.
void port_init(void) {
    list_initialize(&write_port_list);
}

status_t port_create(const char *name, port_mode_t mode, port_t *port) {
    if (!name || !port)
        return ERR_INVALID_ARGS;

    // only unicast ports can have a large buffer.
    if (mode & PORT_MODE_BROADCAST) {
        if (mode & PORT_MODE_BIG_BUFFER)
            return ERR_INVALID_ARGS;
    }

    if (strnlen(name, PORT_NAME_LEN) >= PORT_NAME_LEN)
        return ERR_INVALID_ARGS;


    // Add a stack-allocated port to the list until we can
    // replace it with a heap-allocated port.
    write_port_t stack_wp = { .magic = PORTHOLD_MAGIC };
    // We waste a few cycles here with a throwaway copy.
    strlcpy(stack_wp.name, name, sizeof(stack_wp.name));

    // lookup for existing port, return that if found.
    write_port_t *wp = NULL;
    PORT_LOCK(state1);
    list_for_every_entry(&write_port_list, wp, write_port_t, node) {
        if (strcmp(wp->name, name) == 0) {
            // can't return closed or partial ports.
            if (wp->magic == WRITEPORT_MAGIC_X ||
                wp->magic == PORTHOLD_MAGIC)
                wp = NULL;
            PORT_UNLOCK(state1);
            if (wp) {
                *port = (void *) wp;
                return ERR_ALREADY_EXISTS;
            } else {
                return ERR_BUSY;
            }
        }
    }
    list_add_tail(&write_port_list, &stack_wp.node);
    PORT_UNLOCK(state1);

    // not found, create the write port and the circular buffer.
    wp = calloc(1, sizeof(write_port_t));
    if (!wp) {
        PORT_LOCK(state2);
        list_delete(&stack_wp.node);
        PORT_UNLOCK(state2);
        return ERR_NO_MEMORY;
    }

    wp->magic = WRITEPORT_MAGIC_W;
    wp->mode = mode;
    strlcpy(wp->name, name, sizeof(wp->name));
    list_initialize(&wp->rp_list);

    wp->buf = make_buf(mode & PORT_MODE_BIG_BUFFER);
    if (!wp->buf) {
        free(wp);
        PORT_LOCK(state2);
        list_delete(&stack_wp.node);
        PORT_UNLOCK(state2);
        return ERR_NO_MEMORY;
    }

    // Avoid a name collision by swapping the temporary placeholder out of the
    // list for the actual port.
    PORT_LOCK(state2);
    // Let's reserve a stack allocated entry then swap it for the allocated one.
    list_add_tail(&write_port_list, &wp->node);
    list_delete(&stack_wp.node);
    PORT_UNLOCK(state2);

    *port = (void *)wp;
    return NO_ERROR;
}

status_t port_open(const char *name, void *ctx, port_t *port) {
    if (!name || !port)
        return ERR_INVALID_ARGS;

    // assume success; create the read port and buffer now.
    read_port_t *rp = calloc(1, sizeof(read_port_t));
    if (!rp)
        return ERR_NO_MEMORY;

    rp->magic = READPORT_MAGIC;
    event_init(&rp->event, false, EVENT_FLAG_AUTOUNSIGNAL);
    rp->ctx = ctx;

    // |buf| might not be needed, but we always allocate outside the lock.
    // this buffer is only needed for broadcast ports, but we don't know
    // that here.
    port_buf_t *buf = make_buf(false);  // Small is enough.
    if (!buf) {
        free(rp);
        return ERR_NO_MEMORY;
    }

    // find the named write port and associate it with read port.
    status_t rc = ERR_NOT_FOUND;

    PORT_LOCK(state);
    write_port_t *wp = NULL;
    list_for_every_entry(&write_port_list, wp, write_port_t, node) {
        if (strcmp(wp->name, name) == 0 &&
            wp->magic != PORTHOLD_MAGIC) {
            // found; add read port to write port list.
            rp->wport = wp;
            if (wp->buf) {
                // this is the first read port; transfer the circular buffer.
                list_add_tail(&wp->rp_list, &rp->w_node);
                rp->buf = wp->buf;
                wp->buf = NULL;
                rc = NO_ERROR;
            } else if (buf) {
                // not first read port.
                if (wp->mode & PORT_MODE_UNICAST) {
                    // cannot add a second listener.
                    rc = ERR_NOT_ALLOWED;
                    break;
                }
                // use the new (small) circular buffer.
                list_add_tail(&wp->rp_list, &rp->w_node);
                rp->buf = buf;
                buf = NULL;
                rc = NO_ERROR;
            } else {
                // |buf| allocation failed and the buffer was needed.
                rc = ERR_NO_MEMORY;
            }
            break;
        }
    }
    PORT_UNLOCK(state);

    free(buf);

    if (rc == NO_ERROR) {
        *port = (void *)rp;
    } else {
        free(rp);
    }
    return rc;
}

status_t port_group(port_t *ports, size_t count, port_t *group) {
    if (count > MAX_PORT_GROUP_COUNT)
        return ERR_TOO_BIG;

    // Allow empty port groups.
    if (count && !ports)
        return ERR_INVALID_ARGS;

    if (!group)
        return ERR_INVALID_ARGS;

    // assume success; create port group now.
    port_group_t *pg = calloc(1, sizeof(port_group_t));
    if (!pg)
        return ERR_NO_MEMORY;

    pg->magic = PORTGROUP_MAGIC;
    event_init(&pg->event, false, EVENT_FLAG_AUTOUNSIGNAL);
    list_initialize(&pg->rp_list);

    status_t rc = NO_ERROR;

    PORT_LOCK(state);
    for (size_t ix = 0; ix != count; ix++) {
        read_port_t *rp = (read_port_t *)ports[ix];
        if ((rp->magic != READPORT_MAGIC) || rp->gport) {
            // wrong type of port, or port already part of a group,
            // in any case, undo the changes to the previous read ports.
            for (size_t jx = 0; jx != ix; jx++) {
                ((read_port_t *)ports[jx])->gport = NULL;
            }
            rc = ERR_BAD_HANDLE;
            break;
        }
        // link port group and read port.
        rp->gport = pg;
        list_add_tail(&pg->rp_list, &rp->g_node);
    }
    PORT_UNLOCK(state);

    if (rc == NO_ERROR) {
        *group = (port_t *)pg;
    } else {
        free(pg);
    }
    return rc;
}

status_t port_group_add(port_t group, port_t port) {
    if (!port || !group)
        return ERR_INVALID_ARGS;

    // Make sure the user has actually passed in a port group and a read-port.
    port_group_t *pg = (port_group_t *)group;
    if (pg->magic != PORTGROUP_MAGIC)
        return ERR_INVALID_ARGS;

    read_port_t *rp = (read_port_t *)port;
    if (rp->magic != READPORT_MAGIC || rp->gport)
        return ERR_BAD_HANDLE;

    status_t rc = NO_ERROR;

    // the signal below happens under the port lock, so preemption has to be off
    preempt_disable();
    PORT_LOCK(state);

    if (list_length(&pg->rp_list) == MAX_PORT_GROUP_COUNT) {
        rc = ERR_TOO_BIG;
    } else {
        rp->gport = pg;
        list_add_tail(&pg->rp_list, &rp->g_node);

        // If the new read port being added has messages available, try to wake
        // any readers that might be present.
        if (!buf_is_empty(rp->buf)) {
            event_signal(&pg->event);
        }
    }

    PORT_UNLOCK(state);
    preempt_enable();

    return rc;
}

status_t port_group_remove(port_t group, port_t port) {
    if (!port || !group)
        return ERR_INVALID_ARGS;

    // Make sure the user has actually passed in a port group and a read-port.
    port_group_t *pg = (port_group_t *)group;
    if (pg->magic != PORTGROUP_MAGIC)
        return ERR_INVALID_ARGS;

    read_port_t *rp = (read_port_t *)port;
    if (rp->magic != READPORT_MAGIC || rp->gport != pg)
        return ERR_BAD_HANDLE;

    PORT_LOCK(state);

    bool found = false;
    read_port_t *current_rp;
    list_for_every_entry(&pg->rp_list, current_rp, read_port_t, g_node) {
        if (current_rp == rp) {
            found = true;
        }
    }

    if (!found) {
        PORT_UNLOCK(state);
        return ERR_BAD_HANDLE;
    }

    list_delete(&rp->g_node);
    // Drop the back pointer as well, otherwise the read port keeps a dangling
    // reference to a group it is no longer a member of. port_destroy() would
    // wake that group's wait queue (possibly after the group was freed) and
    // port_group_add() would refuse to ever add this port to a group again.
    rp->gport = NULL;

    PORT_UNLOCK(state);

    return NO_ERROR;
}

status_t port_write(port_t port, const port_packet_t *pk, size_t count) {
    if (!port || !pk)
        return ERR_INVALID_ARGS;

    write_port_t *wp = (write_port_t *)port;

    /* A single write can wake a thread on every attached read port, and the
     * signalling is done under the port lock. Preemption must be off for the
     * whole region: an inline reschedule would switch away holding the port
     * lock, which -- unlike the thread lock -- nobody else is going to release.
     * It also batches the wakeups into a single reschedule at the end.
     *
     * That reschedule is a thread_yield() rather than letting preempt_enable()
     * do it, because ports deliberately hand the cpu to the reader: a preempt
     * puts the writer back at the head of the run queue, ahead of the reader it
     * just woke, so the reader would not run until this thread's quantum ran
     * out. Yielding puts the writer at the tail instead.
     */
    preempt_disable();

    PORT_LOCK(state);
    if (wp->magic != WRITEPORT_MAGIC_W) {
        // wrong port type.
        PORT_UNLOCK(state);
        (void)preempt_enable_no_resched();
        return ERR_BAD_HANDLE;
    }

    status_t status = NO_ERROR;

    if (wp->buf) {
        // there are no read ports, just write to the buffer.
        status = buf_write(wp->buf, pk, count);
    } else {
        // there are read ports. for each, write and attempt to wake a thread
        // from the port group or from the read port itself.
        read_port_t *rp;
        list_for_every_entry(&wp->rp_list, rp, read_port_t, w_node) {
            if (buf_write(rp->buf, pk, count) < 0) {
                // buffer full.
                status = ERR_PARTIAL_WRITE;
                continue;
            }

            // Prefer to wake a reader blocked on the group; only fall back to
            // the port's own event if the group had nobody waiting.
            int awaken = 0;
            if (rp->gport) {
                awaken = event_signal(&rp->gport->event);
            }
            if (!awaken) {
                event_signal(&rp->event);
            }
        }
    }

    PORT_UNLOCK(state);

    if (preempt_enable_no_resched()) {
        thread_yield();
    }

    return status;
}

// Try to take one packet from a read port. Must be called with the port lock
// held and preemption disabled; returns ERR_NO_MSG if the port is empty.
static status_t try_read(read_port_t *rp, port_result_t *result) {
    status_t status = buf_read(rp->buf, result);
    result->ctx = rp->ctx;

    if (status != NO_ERROR)
        return status;

    // The event carries a single wakeup token no matter how many packets are
    // queued, so if there is more left, hand a token to the next reader. Without
    // this a second reader can sit blocked with data sitting in the buffer.
    if (!buf_is_empty(rp->buf)) {
        if (rp->gport) {
            event_signal(&rp->gport->event);
        }
        event_signal(&rp->event);
    }

    return NO_ERROR;
}

status_t port_read(port_t port, lk_time_t timeout, port_result_t *result) {
    if (!port || !result)
        return ERR_INVALID_ARGS;

    read_port_t *rp = (read_port_t *)port;

    for (;;) {
        status_t rc;
        event_t *ev;

        preempt_disable();
        PORT_LOCK(state);

        if (rp->magic == READPORT_MAGIC) {
            // dealing with a single port.
            rc = try_read(rp, result);
            // a destroyed write port cancels its readers, but only once they
            // have drained whatever was already buffered.
            if (rc == ERR_NO_MSG && !rp->wport) {
                rc = ERR_CANCELLED;
            }
            ev = &rp->event;
        } else if (rp->magic == PORTGROUP_MAGIC) {
            // dealing with a port group. read each member in turn.
            // todo: this order is fixed, probably a bad thing.
            port_group_t *pg = (port_group_t *)port;
            bool cancelled = false;
            read_port_t *crp;

            rc = ERR_NO_MSG;
            list_for_every_entry(&pg->rp_list, crp, read_port_t, g_node) {
                rc = try_read(crp, result);
                if (rc != ERR_NO_MSG)
                    break;
                if (!crp->wport)
                    cancelled = true;
            }
            if (rc == ERR_NO_MSG && cancelled) {
                rc = ERR_CANCELLED;
            }
            ev = &pg->event;
        } else {
            // wrong port type.
            rc = ERR_BAD_HANDLE;
            ev = NULL;
        }

        PORT_UNLOCK(state);
        preempt_enable();

        if (rc != ERR_NO_MSG)
            return rc;
        if (!timeout)
            return ERR_TIMED_OUT;

        // Nothing to read. Block outside the port lock -- the event's signaled
        // state is sticky, so a write that lands between the unlock above and
        // the wait below is not lost, it just makes the wait return at once and
        // the read is retried.
        //
        // Note the caller's full timeout is used on every pass, so a stream of
        // spurious wakeups can extend the total wait. That matches what the
        // wait_queue version did.
        status_t wr = event_wait_timeout(ev, timeout);
        if (wr != NO_ERROR)
            return wr;
    }
}

status_t port_destroy(port_t port) {
    if (!port)
        return ERR_INVALID_ARGS;

    write_port_t *wp = (write_port_t *) port;
    port_buf_t *buf = NULL;

    /* The wakes below happen under the port lock and in the middle of walking
     * wp->rp_list, so preemption has to be off across the whole region.
     */
    preempt_disable();

    PORT_LOCK(state);
    if (wp->magic != WRITEPORT_MAGIC_X) {
        // wrong port type.
        PORT_UNLOCK(state);
        preempt_enable();
        return ERR_BAD_HANDLE;
    }
    // remove self from global named ports list.
    list_delete(&wp->node);

    if (wp->buf) {
        // we have no readers.
        buf = wp->buf;
    } else {
        // for each reader:
        read_port_t *rp;
        list_for_every_entry(&wp->rp_list, rp, read_port_t, w_node) {
            // Detach the reader first, then wake it. A reader that finds its
            // write port gone and nothing left to read returns ERR_CANCELLED.
            rp->wport = NULL;
            event_signal_all(&rp->event);
            if (rp->gport) {
                event_signal_all(&rp->gport->event);
            }
        }
    }

    wp->magic = 0;
    PORT_UNLOCK(state);

    preempt_enable();

    free(buf);
    free(wp);
    return NO_ERROR;
}

status_t port_close(port_t port) {
    if (!port)
        return ERR_INVALID_ARGS;

    read_port_t *rp = (read_port_t *) port;
    port_buf_t *buf = NULL;

    /* event_destroy() wakes waiters from under the port lock, so preemption has
     * to be off for the same reason as everywhere else in this file. */
    preempt_disable();

    PORT_LOCK(state);
    if (rp->magic == READPORT_MAGIC) {
        // dealing with a read port.
        if (rp->wport) {
            // remove self from write port list and reassign the bufer if last.
            list_delete(&rp->w_node);
            if (list_is_empty(&rp->wport->rp_list)) {
                rp->wport->buf = rp->buf;
                rp->buf = NULL;
            } else {
                buf = rp->buf;
            }
        }
        if (rp->gport) {
            // remove self from port group list.
            list_delete(&rp->g_node);
        }
        // wake up waiters, the return code is ERR_OBJECT_DESTROYED.
        event_destroy(&rp->event);
        rp->magic = 0;

    } else if (rp->magic == PORTGROUP_MAGIC) {
        // dealing with a port group.
        port_group_t *pg = (port_group_t *) port;
        // wake up waiters.
        event_destroy(&pg->event);
        // remove self from reader ports.
        rp = NULL;
        list_for_every_entry(&pg->rp_list, rp, read_port_t, g_node) {
            rp->gport = NULL;
        }
        pg->magic = 0;

    } else if (rp->magic == WRITEPORT_MAGIC_W) {
        // dealing with a write port.
        write_port_t *wp = (write_port_t *) port;
        // mark it as closed. Now it can be read but not written to.
        wp->magic = WRITEPORT_MAGIC_X;
        PORT_UNLOCK(state);
        preempt_enable();
        return NO_ERROR;

    } else {
        PORT_UNLOCK(state);
        preempt_enable();
        return ERR_BAD_HANDLE;
    }

    PORT_UNLOCK(state);
    preempt_enable();

    free(buf);
    free(port);
    return NO_ERROR;
}

