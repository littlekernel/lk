/*
 * Copyright (c) 2015 Carlos Pizano-Uribe  cpu@chromium.org
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/unittest.h>

#include <lk/debug.h>
#include <lk/err.h>
#include <rand.h>
#include <string.h>
#include <lk/trace.h>

#include <kernel/event.h>
#include <kernel/port.h>
#include <kernel/thread.h>

#include <platform.h>

#define LOCAL_TRACE 0

static void *context1 = (void *) 0x53;

static void dump_port_result(const port_result_t *result) {
    const port_packet_t *p = &result->packet;
    LTRACEF("[%02x %02x %02x %02x %02x %02x %02x %02x]\n",
            p->value[0], p->value[1], p->value[2], p->value[3],
            p->value[4], p->value[5], p->value[6], p->value[7]);
}

static bool single_thread_basic(void) {
    BEGIN_TEST;

    port_t w_port;
    status_t st = port_create("sh_prt1", PORT_MODE_UNICAST, &w_port);
    ASSERT_GE(st, 0, "could not create port");

    port_t r_port;
    st = port_open("sh_prt0", context1, &r_port);
    ASSERT_EQ(ERR_NOT_FOUND, st, "expected not to find port");

    st = port_open("sh_prt1", context1, &r_port);
    ASSERT_GE(st, 0, "could not open port");

    port_packet_t packet[3] = {
        {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}},
        {{0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11}},
        {{0x33, 0x66, 0x99, 0xcc, 0x33, 0x66, 0x99, 0xcc}},
    };

    st = port_write(w_port, &packet[0], 1);
    ASSERT_GE(st, 0, "could not write port");

    port_result_t res = {0};

    st = port_read(r_port, 0, &res);
    ASSERT_GE(st, 0, "could not read port");
    ASSERT_EQ(context1, res.ctx, "bad context");

    st = port_read(r_port, 0, &res);
    ASSERT_EQ(ERR_TIMED_OUT, st, "expected timeout on empty port");

    st = port_write(w_port, &packet[1], 1);
    ASSERT_GE(st, 0, "could not write port");

    st = port_write(w_port, &packet[0], 1);
    ASSERT_GE(st, 0, "could not write port");

    st = port_write(w_port, &packet[2], 1);
    ASSERT_GE(st, 0, "could not write port");

    int expected_count = 3;
    while (true) {
        st = port_read(r_port, 0, &res);
        if (st < 0)
            break;
        dump_port_result(&res);
        --expected_count;
    }
    ASSERT_EQ(0, expected_count, "invalid read count");

    // port should be empty. should be able to write 8 packets.
    expected_count = 8;
    while (true) {
        st = port_write(w_port, &packet[1], 1);
        if (st < 0)
            break;
        --expected_count;
        st = port_write(w_port, &packet[2], 1);
        if (st < 0)
            break;
        --expected_count;
    }
    ASSERT_EQ(0, expected_count, "invalid write count");

    // tod(cpu) fix this possibly wrong error.
    ASSERT_EQ(ERR_PARTIAL_WRITE, st, "expected buffer full error");

    // read 3 packets.
    for (int ix = 0; ix != 3; ++ix) {
        st = port_read(r_port, 0, &res);
        ASSERT_GE(st, 0, "could not read port");
    }

    // there are 5 packets, now we add another 3.
    st = port_write(w_port, packet, 3);
    ASSERT_GE(st, 0, "could not write port");

    expected_count = 8;
    while (true) {
        st = port_read(r_port, 0, &res);
        if (st < 0)
            break;
        dump_port_result(&res);
        --expected_count;
    }
    ASSERT_EQ(0, expected_count, "invalid read count");

    // attempt to use the wrong port.
    st = port_write(r_port, &packet[1], 1);
    ASSERT_EQ(ERR_BAD_HANDLE, st, "expected bad handle writing to a read port");

    st = port_read(w_port, 0, &res);
    ASSERT_EQ(ERR_BAD_HANDLE, st, "expected bad handle reading from a write port");

    st = port_close(r_port);
    ASSERT_GE(st, 0, "could not close read port");

    st = port_close(w_port);
    ASSERT_GE(st, 0, "could not close write port");

    st = port_close(r_port);
    ASSERT_EQ(ERR_BAD_HANDLE, st, "expected bad handle on double close");

    st = port_close(w_port);
    ASSERT_EQ(ERR_BAD_HANDLE, st, "expected bad handle on double close");

    st = port_destroy(w_port);
    ASSERT_GE(st, 0, "could not destroy port");

    END_TEST;
}

static int ping_pong_thread(void *arg) {
    port_t r_port;
    status_t st = port_open("ping_port", NULL, &r_port);
    if (st < 0) {
        printf("thread: could not open ping port, status = %d\n", st);
        return __LINE__;
    }

    // The two workers race to create the pong port. The loser normally sees
    // ERR_ALREADY_EXISTS, but if it lands inside the winner's creation window
    // it sees ERR_BUSY for as long as the winner's placeholder entry is in
    // the port list, so retry promptly. Bounded so that a stuck placeholder
    // fails the test instead of hanging it (and the parent's join) forever.
    bool should_dispose_pong_port = true;
    port_t w_port;
    for (int tries = 0; ; ++tries) {
        st = port_create("pong_port", PORT_MODE_UNICAST, &w_port);
        if (st != ERR_BUSY)
            break;
        if (tries >= 1000) {
            printf("thread: pong port stuck busy\n");
            port_close(r_port);
            return __LINE__;
        }
        thread_sleep(1);
    }
    if (st == ERR_ALREADY_EXISTS) {
        // lost the race to create the port.
        should_dispose_pong_port = false;
    } else if (st < 0) {
        printf("thread: could not create pong port, status = %d\n", st);
        port_close(r_port);
        return __LINE__;
    }

    port_result_t pr;

    // the loop is read-mutate-write until the write port
    // is closed by the master thread.
    while (true) {
        st = port_read(r_port, INFINITE_TIME, &pr);

        if (st == ERR_CANCELLED) {
            break;
        } else if (st < 0) {
            printf("thread: could not read port, status = %d\n", st);
            return __LINE__;
        }

        pr.packet.value[0]++;
        pr.packet.value[5]--;

        st = port_write(w_port, &pr.packet, 1);
        if (st < 0) {
            printf("thread: could not write port, status = %d\n", st);
            return __LINE__;
        }
    }

    port_close(r_port);

    if (should_dispose_pong_port) {
        port_close(w_port);
        port_destroy(w_port);
    }

    return 0;
}

static const char *kStatusPortNames[] = {
  "status0",
  "status1",
};
static const char *kRacePortName = "racer_port";

static const port_packet_t kRepeat =
        {{'R', 'E', 'P', 'E', 'A', 'T', 0, 0}};
static const port_packet_t kQuit =
        {{'Q', 'U', 'I', 'T', 0, 0, 0, 0}};

static event_t race_evt;

/* How long the thread that wins the port_create() race keeps the port alive
 * before tearing it down, giving the losing thread time to observe it. */
#define RACE_HOLD_MS 25

static int race_thread(void *arg)
{
    port_t r_port;
    int tid = (int)(intptr_t)arg;

    LTRACEF("thread %d: connecting to control port\n", tid);
    status_t st = port_open("race_ctl", NULL, &r_port);
    if (st < 0) {
        printf("thread %d: could not open control port, status = %d\n", tid, st);
        return __LINE__;
    }

    LTRACEF("thread %d: creating status port\n", tid);
    port_t w_port;
    st = port_create(kStatusPortNames[tid], PORT_MODE_UNICAST, &w_port);
    if (st < 0) {
        printf("thread %d: could not create status port, status = %d\n", tid, st);
        port_close(r_port);
        return __LINE__;
    }

    // Loop is meant to coordinate a port_create() race.
    // The event triggers the race.
    // The thread sleeps briefly then cleans up
    // The thread reports its claim to its status port.
    // It then waits for a repeat or quit message.
    int ret = -1;
    while (ret < 0) {
        LTRACEF_LEVEL(1, "thread %d: waiting at the starting line\n", tid);
        if (event_wait_timeout(&race_evt, INFINITE_TIME) != NO_ERROR) {
            ret = __LINE__;
            break;
        }

        port_t race_port;
        int tries = 0;
        while(true) {
            st = port_create(kRacePortName, PORT_MODE_UNICAST, &race_port);
            if (st != ERR_BUSY)
                break;
            // The loser only sees ERR_BUSY for as long as the winner's
            // placeholder entry is in the port list, so retry promptly. This
            // has to be well under RACE_HOLD_MS: the winner tears the port down
            // after that long, and if we retry any later we create a second
            // port of our own instead of finding the winner's, which looks
            // exactly like both threads having won.
            if (++tries >= 1000) // a stuck placeholder must not hang the join
                break;
            thread_sleep(1);
        } // EINTR all over again . . .
        if (st == ERR_BUSY) {
            printf("thread %d: race port stuck busy\n", tid);
            ret = __LINE__;
            break;
        }
        LTRACEF_LEVEL(1, "thread %d: sampling chronochip (%p)\n", tid, race_port);
        if (st == ERR_ALREADY_EXISTS) {
            // lost the race to create the port.
        } else if (st < 0) {
            LTRACEF_LEVEL(1, "thread %d: could not open port, status = %d\n", tid, st);
            ret = __LINE__;
            break;
        } else { // Dispose of it now.
            // Hold the port long enough for the losing thread to observe it.
            thread_sleep(RACE_HOLD_MS);
            port_close(race_port);
            port_destroy(race_port);
        }

        // Now send the stale pointer address as a status.
        port_packet_t claimed_port = {{0}};
        int len = sizeof(claimed_port.value);
        if (sizeof(race_port) < (size_t)len)
            len = sizeof(race_port);
        for (int i = 0; i < len; ++i) {
            claimed_port.value[i] = 0xff & ((uintptr_t)race_port >> (i * 8));
        }
        LTRACEF_LEVEL(1, "thread %d: reporting status\n", tid);
        st = port_write(w_port, &claimed_port, 1);
        if (st < 0) {
            printf("thread %d: could not write port, status = %d\n", tid, st);
            ret = __LINE__;
            break;
        }

        LTRACEF_LEVEL(1, "thread %d: awaiting instructions\n", tid);
        port_result_t pr;
        st = port_read(r_port, INFINITE_TIME, &pr);
        if (st == ERR_CANCELLED) {
            printf("thread %d: could not read port, status = %d (CANCELLED)\n", tid, st);
            ret = __LINE__;
            break;
        } else if (st < 0) {
            printf("thread %d: could not read port, status = %d\n", tid, st);
            ret = __LINE__;
            break;
        }
        if (memcmp(pr.packet.value, kQuit.value, sizeof(pr.packet.value)) == 0) {
            ret = 0;
            break;
        }
        if (memcmp(pr.packet.value, kRepeat.value, sizeof(pr.packet.value)) == 0) {
            continue;
        }
        printf("thread %d: got a weird message from the control port\n", tid);
        ret = __LINE__;
    }
    thread_sleep((1+tid) * 5); // Make console output orderly.
    LTRACEF("thread %d: shutting down (ret=%d)\n", tid, ret);

    port_close(r_port);
    port_close(w_port);
    port_destroy(w_port);
    return ret;
}


/* Number of times the two threads race to create the same named port. Each pass
 * costs at least a couple of 25ms sleeps in race_thread, so keep this modest --
 * this test runs as part of `ut all` at boot, which has a 60 second budget on
 * the slowest emulated targets. */
#define RACE_PASSES 16

/* Bounds every wait on a test worker thread so that one dying unexpectedly
 * fails the test instead of hanging it until the CI harness times the whole
 * boot out. */
#define WORKER_WAIT_TIMEOUT_MS 10000

typedef struct {
    port_t r_port[2];
    bool opened[2];
} race_state_t;

/* The fallible middle of two_threads_race. This runs while the two race
 * threads are alive, so it must not leave anything behind that the caller's
 * shutdown path does not know about: every status port it opens is recorded
 * in the state before use, and a failed ASSERT simply returns through here
 * back into the caller, which always drives the threads to exit. */
static bool two_threads_race_body(race_state_t *rs, port_t w_port) {
    BEGIN_TEST;

    // wait for each status port to be created so we can
    // track behavior.
    status_t st;
    for (int tid = 0; tid < 2; ++tid) {
        LTRACEF("control: connecting to thread %d . . .\n", tid);
        st = ERR_NOT_FOUND;
        for (int tries = 0; tries < 500 && st == ERR_NOT_FOUND; ++tries) {
            st = port_open(kStatusPortNames[tid], NULL, &rs->r_port[tid]);
            if (st == ERR_NOT_FOUND)
                thread_sleep(10);
        }
        ASSERT_EQ(NO_ERROR, st, "could not open status port");
        rs->opened[tid] = true;
    }

    // control port says: "REPEAT" or "QUIT"
    // Both threads race to create the same named port; exactly one must win, so
    // both must report the same port handle back to us every pass.
    bool raced = false;
    int count = 0;
    while (true) {
        LTRACEF_LEVEL(1, "Go!\n");
        event_signal(&race_evt, false);
        port_result_t pr0, pr1;
        LTRACEF_LEVEL(1, "Collecting status from thread 0 . . .\n");
        st = port_read(rs->r_port[0], WORKER_WAIT_TIMEOUT_MS, &pr0);
        ASSERT_GE(st, 0, "could not read status port 0");
        LTRACEF_LEVEL(1, "Collecting status from thread 1 . . .\n");
        st = port_read(rs->r_port[1], WORKER_WAIT_TIMEOUT_MS, &pr1);
        ASSERT_GE(st, 0, "could not read status port 1");
        LTRACEF_LEVEL(1, "Checking responses . . .\n");
        if (memcmp(pr0.packet.value, pr1.packet.value, sizeof(pr0.packet.value)) != 0) {
            UNITTEST_FAIL_TRACEF("both threads claimed the port on iteration %d\n", count);
            raced = true;
            all_ok = false;
        }
        event_unsignal(&race_evt);
        if (raced || ++count >= RACE_PASSES)
            break;
        LTRACEF_LEVEL(1, "Telling threads to repeat\n");
        st = port_write(w_port, &kRepeat, 1);
        ASSERT_GE(st, 0, "could not write control port");
    }

    END_TEST;
}

static bool two_threads_race(void) {
    BEGIN_TEST;

    // Used to tell the threads what to do.
    port_t w_port;
    status_t st = port_create("race_ctl", PORT_MODE_BROADCAST, &w_port);
    ASSERT_GE(st, 0, "could not create port");

    event_init(&race_evt, false, 0);

    thread_t *t1 = thread_create(
                       "rt0", &race_thread, (void *)0, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_t *t2 = thread_create(
                       "rt1", &race_thread, (void *)1, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    EXPECT_NONNULL(t1, "could not create race thread 0");
    EXPECT_NONNULL(t2, "could not create race thread 1");
    if (t1) {
        thread_set_real_time(t1);
        thread_resume(t1);
    }
    if (t2) {
        thread_set_real_time(t2);
        thread_resume(t2);
    }

    race_state_t rs = {};
    bool body_ok = (t1 && t2) ? two_threads_race_body(&rs, w_port) : false;

    /* The shutdown below runs even when the body failed partway: a race
     * thread may still be waiting at the starting line or for an instruction,
     * and if it is not driven to exit it stays blocked forever and the named
     * ports it owns poison every later run of this test with
     * ERR_ALREADY_EXISTS. Close the status read ports first so the threads
     * destroy the write sides after their readers are gone. */
    for (int tid = 0; tid < 2; ++tid) {
        if (rs.opened[tid]) {
            st = port_close(rs.r_port[tid]);
            EXPECT_GE(st, 0, "could not close status port");
        }
    }

    event_signal(&race_evt, true);
    st = port_write(w_port, &kQuit, 1);
    EXPECT_GE(st, 0, "could not write control port");

    int retcode;
    if (t1) {
        retcode = -1;
        thread_join(t1, &retcode, INFINITE_TIME);
        EXPECT_EQ(0, retcode, "race thread 0 exited with an error line number");
    }
    if (t2) {
        retcode = -1;
        thread_join(t2, &retcode, INFINITE_TIME);
        EXPECT_EQ(0, retcode, "race thread 1 exited with an error line number");
    }

    event_destroy(&race_evt);

    st = port_close(w_port);
    EXPECT_GE(st, 0, "could not close control port");

    st = port_destroy(w_port);
    EXPECT_GE(st, 0, "could not destroy control port");

    EXPECT_TRUE(body_ok, "race sequence failed");

    END_TEST;
}


typedef struct {
    port_t r_port;
    bool opened;
} pingpong_state_t;

/* The fallible middle of two_threads_basic. This runs while the two workers
 * are alive, so a failed ASSERT simply returns through here back into the
 * caller, which always destroys the ping port to unblock the workers and
 * joins them. */
static bool two_threads_basic_body(pingpong_state_t *s, port_t w_port) {
    BEGIN_TEST;

    // wait for the pong port to be created, the two threads race to do it.
    status_t st = ERR_NOT_FOUND;
    for (int tries = 0; tries < 500 && st == ERR_NOT_FOUND; ++tries) {
        st = port_open("pong_port", NULL, &s->r_port);
        if (st == ERR_NOT_FOUND)
            thread_sleep(10);
    }
    ASSERT_EQ(NO_ERROR, st, "could not open pong port");
    s->opened = true;

    // We have two threads listening to the ping port. Which both reply
    // on the pong port, so we get two packets in per packet out.
    const int passes = 256;

    port_packet_t packet_out = {{0xaf, 0x77, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};

    port_result_t pr;
    for (int ix = 0; ix != passes; ++ix) {
        const size_t count = 1 + ((unsigned int)rand() % 3);

        for (size_t jx = 0; jx != count; ++jx) {
            st = port_write(w_port, &packet_out, 1);
            ASSERT_GE(st, 0, "could not write port");
        }

        packet_out.value[0]++;
        packet_out.value[5]--;

        for (size_t jx = 0; jx != count * 2; ++jx) {
            st = port_read(s->r_port, WORKER_WAIT_TIMEOUT_MS, &pr);
            ASSERT_GE(st, 0, "could not read port");

            ASSERT_EQ(packet_out.value[0], pr.packet.value[0], "unexpected data in packet");
            ASSERT_EQ(packet_out.value[5], pr.packet.value[5], "unexpected data in packet");
        }
    }

    thread_sleep(100);

    // there should be no more packets to read.
    st = port_read(s->r_port, 0, &pr);
    ASSERT_EQ(ERR_TIMED_OUT, st, "unexpected extra packet");

    END_TEST;
}

static bool two_threads_basic(void) {
    BEGIN_TEST;

    port_t w_port;
    status_t st = port_create("ping_port", PORT_MODE_BROADCAST, &w_port);
    ASSERT_GE(st, 0, "could not create port");

    thread_t *t1 = thread_create(
                       "worker1", &ping_pong_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_t *t2 = thread_create(
                       "worker2", &ping_pong_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    EXPECT_NONNULL(t1, "could not create worker 1");
    EXPECT_NONNULL(t2, "could not create worker 2");
    if (t1)
        thread_resume(t1);
    if (t2)
        thread_resume(t2);

    pingpong_state_t s = {};
    bool body_ok = (t1 && t2) ? two_threads_basic_body(&s, w_port) : false;

    /* The shutdown below runs even when the body failed partway: the workers
     * sit blocked reading the ping port until its write side is destroyed,
     * and the named ports otherwise poison every later run of this test with
     * ERR_ALREADY_EXISTS. Close the pong read side first so the surviving
     * worker destroys the write side after its reader is gone. */
    if (s.opened) {
        st = port_close(s.r_port);
        EXPECT_GE(st, 0, "could not close pong port");
    }

    st = port_close(w_port);
    EXPECT_GE(st, 0, "could not close ping port");

    st = port_destroy(w_port);
    EXPECT_GE(st, 0, "could not destroy ping port");

    int retcode;
    if (t1) {
        retcode = -1;
        thread_join(t1, &retcode, INFINITE_TIME);
        EXPECT_EQ(0, retcode, "worker1 exited with an error line number");
    }
    if (t2) {
        retcode = -1;
        thread_join(t2, &retcode, INFINITE_TIME);
        EXPECT_EQ(0, retcode, "worker2 exited with an error line number");
    }

    EXPECT_TRUE(body_ok, "ping pong sequence failed");

    END_TEST;
}

#define CMD_PORT_CTX ((void*) 0x77)
#define TS1_PORT_CTX ((void*) 0x11)
#define TS2_PORT_CTX ((void*) 0x12)

typedef enum {
    ADD_PORT,
    QUIT
} action_t;

typedef struct {
    action_t what;
    port_t port;
} watcher_cmd;

static status_t send_watcher_cmd(port_t cmd_port, action_t action, port_t port) {
    watcher_cmd _cmd  = {action, port};
    return port_write(cmd_port, ((port_packet_t *) &_cmd), 1);;
}

static int group_watcher_thread(void *arg) {
    port_t watched[8] = {0};
    status_t st = port_open("grp_ctrl", CMD_PORT_CTX, &watched[0]);
    if (st < 0) {
        printf("could not open port, status = %d\n", st);
        return __LINE__;
    }

    size_t count = 1;
    port_t group;
    int ctx_count = -1;

    while (true) {
        st = port_group(watched, count, &group);
        if (st < 0) {
            printf("could not make group, status = %d\n", st);
            return __LINE__;
        }

        port_result_t pr;
        while (true) {
            st = port_read(group, INFINITE_TIME, &pr);
            if (st < 0) {
                printf("could not read port, status = %d\n", st);
                return __LINE__;
            }

            if (pr.ctx == CMD_PORT_CTX) {
                break;
            } else if (pr.ctx == TS1_PORT_CTX) {
                ctx_count += 1;
            } else if (pr.ctx == TS2_PORT_CTX) {
                ctx_count += 2;
            } else {
                printf("unknown context %p\n", pr.ctx);
                return __LINE__;
            }
        }

        // Either adding a port or exiting; either way close the
        // existing group port and create a new one if needed
        // at the top of the loop.

        port_close(group);
        watcher_cmd *wc = (watcher_cmd *) &pr.packet;

        if (wc->what == ADD_PORT) {
            watched[count++] = wc->port;
        }  else if (wc->what == QUIT) {
            break;
        } else {
            printf("unknown command %d\n", wc->what);
            return __LINE__;
        }
    }

    if (ctx_count !=  2) {
        printf("unexpected context count %d", ctx_count);
        return __LINE__;
    }

    printf("group watcher shutdown\n");

    for (size_t ix = 0; ix != count; ++ix) {
        st = port_close(watched[ix]);
        if (st < 0) {
            printf("failed to close read port, status = %d\n", st);
            return __LINE__;
        }
    }

    return 0;
}

static status_t make_port_pair(const char *name, void *ctx, port_t *write, port_t *read) {
    status_t st = port_create(name, PORT_MODE_UNICAST, write);
    if (st < 0)
        return st;
    st = port_open(name, ctx, read);
    if (st < 0) {
        // don't leave the half constructed named port behind
        port_close(*write);
        port_destroy(*write);
    }
    return st;
}

typedef struct {
    port_t w_port[2];
    port_t r_port[2];
    bool created[2];
    bool sent[2];
} group_state_t;

/* The fallible middle of group_basic, run while the watcher thread is alive.
 * Everything it creates is recorded in the state before use so that the
 * caller's shutdown path can release exactly what a failed ASSERT left behind.
 * Once a read port has been sent to the watcher (sent[i]), the watcher owns
 * closing it. */
static bool group_basic_body(group_state_t *gs, port_t cmd_port) {
    BEGIN_TEST;

    static const char *kTestPortNames[2] = { "tst_port1", "tst_port2" };
    void *const test_port_ctxs[2] = { TS1_PORT_CTX, TS2_PORT_CTX };

    for (int i = 0; i < 2; ++i) {
        status_t st = make_port_pair(kTestPortNames[i], test_port_ctxs[i],
                                     &gs->w_port[i], &gs->r_port[i]);
        ASSERT_GE(st, 0, "could not make port pair");
        gs->created[i] = true;
    }

    for (int i = 0; i < 2; ++i) {
        status_t st = send_watcher_cmd(cmd_port, ADD_PORT, gs->r_port[i]);
        ASSERT_GE(st, 0, "could not send ADD_PORT");
        gs->sent[i] = true;
    }

    thread_sleep(50);

    port_packet_t pp = {{0}};
    for (int i = 0; i < 2; ++i) {
        status_t st = port_write(gs->w_port[i], &pp, 1);
        ASSERT_GE(st, 0, "could not write test port");
    }

    END_TEST;
}

static bool group_basic(void) {
    BEGIN_TEST;

    // we spin a thread that connects to a well known port, then we
    // send two ports that it will add to a group port.
    port_t cmd_port;
    status_t st = port_create("grp_ctrl", PORT_MODE_UNICAST, &cmd_port);
    ASSERT_GE(st, 0, "could not create control port");

    thread_t *wt = thread_create(
                       "g_watcher", &group_watcher_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    if (wt == NULL) {
        // the watcher never existed so nothing is blocked; safe to bail directly
        port_close(cmd_port);
        port_destroy(cmd_port);
        ASSERT_NONNULL(wt, "could not create watcher thread");
    }
    thread_resume(wt);

    group_state_t gs = {};
    bool body_ok = group_basic_body(&gs, cmd_port);

    /* The shutdown below runs even when the body failed partway: the watcher
     * must always be told to quit and joined, otherwise it stays blocked on
     * its group port forever and the named control port poisons every later
     * run of this test with ERR_ALREADY_EXISTS. */
    st = send_watcher_cmd(cmd_port, QUIT, 0);
    EXPECT_GE(st, 0, "could not send QUIT");

    int retcode = -1;
    thread_join(wt, &retcode, INFINITE_TIME);
    EXPECT_EQ(0, retcode, "watcher thread exited with an error line number");

    for (int i = 0; i < 2; ++i) {
        if (!gs.created[i])
            continue;
        if (!gs.sent[i]) {
            // the watcher never took ownership of this read port
            st = port_close(gs.r_port[i]);
            EXPECT_GE(st, 0, "could not close read test port");
        }
        st = port_close(gs.w_port[i]);
        EXPECT_GE(st, 0, "could not close test port");
        st = port_destroy(gs.w_port[i]);
        EXPECT_GE(st, 0, "could not destroy test port");
    }

    st = port_close(cmd_port);
    EXPECT_GE(st, 0, "could not close control port");
    st = port_destroy(cmd_port);
    EXPECT_GE(st, 0, "could not destroy control port");

    EXPECT_TRUE(body_ok, "group watcher sequence failed");

    END_TEST;
}

typedef struct {
    port_t w_port[2];
    port_t r_port[2];
    bool created[2];
    port_t pg;
    bool grouped;
} group_dynamic_state_t;

/* The fallible middle of group_dynamic. Everything it creates is recorded in
 * the state before use so that the caller's cleanup path can release exactly
 * what a failed ASSERT left behind. */
static bool group_dynamic_body(group_dynamic_state_t *s) {
    BEGIN_TEST;

    status_t st = make_port_pair("tst_port1", TS1_PORT_CTX, &s->w_port[0], &s->r_port[0]);
    ASSERT_GE(st, 0, "could not make port pair 1");
    s->created[0] = true;

    st = make_port_pair("tst_port2", TS2_PORT_CTX, &s->w_port[1], &s->r_port[1]);
    ASSERT_GE(st, 0, "could not make port pair 2");
    s->created[1] = true;

    st = port_group(&s->r_port[0], 1, &s->pg);
    ASSERT_GE(st, 0, "could not create port group");
    s->grouped = true;

    port_packet_t pkt = { { 0 } };
    st = port_write(s->w_port[1], &pkt, 1);
    ASSERT_GE(st, 0, "could not write test port 2");

    port_result_t rslt;
    st = port_read(s->pg, 0, &rslt);
    ASSERT_EQ(ERR_TIMED_OUT, st, "port 2 is not in the group yet");

    // Attach the port that has been written to to the port group and ensure
    // that we can read from it.
    st = port_group_add(s->pg, s->r_port[1]);
    ASSERT_GE(st, 0, "could not add port 2 to the group");

    st = port_read(s->pg, 0, &rslt);
    ASSERT_GE(st, 0, "could not read the packet queued on port 2");

    // Write some data to a port then remove it from the port group and ensure
    // that we can't read from it.
    st = port_write(s->w_port[0], &pkt, 1);
    ASSERT_GE(st, 0, "could not write test port 1");

    st = port_group_remove(s->pg, s->r_port[0]);
    ASSERT_GE(st, 0, "could not remove port 1 from the group");

    st = port_read(s->pg, 0, &rslt);
    ASSERT_EQ(ERR_TIMED_OUT, st, "port 1 was removed from the group");

    END_TEST;
}

static bool group_dynamic(void) {
    BEGIN_TEST;

    group_dynamic_state_t s = {};
    bool body_ok = group_dynamic_body(&s);

    /* The cleanup below runs even when the body failed partway, so the named
     * test ports cannot leak and poison every later run of this test with
     * ERR_ALREADY_EXISTS. Close the group first so its members are detached
     * by the time they are closed. */
    status_t st;
    if (s.grouped) {
        st = port_close(s.pg);
        EXPECT_GE(st, 0, "could not close the port group");
    }
    for (int i = 0; i < 2; ++i) {
        if (!s.created[i])
            continue;
        /* the read ports have to be closed as well: destroying the write side
         * only detaches its readers, it does not free them */
        st = port_close(s.r_port[i]);
        EXPECT_GE(st, 0, "could not close read port");
        st = port_close(s.w_port[i]);
        EXPECT_GE(st, 0, "could not close test port");
        st = port_destroy(s.w_port[i]);
        EXPECT_GE(st, 0, "could not destroy test port");
    }

    EXPECT_TRUE(body_ok, "group sequence failed");

    END_TEST;
}

static event_t group_waiting_sync_evt;

static int receive_thread(void *arg) {
    port_t pg = (port_t)arg;

    // Try to read from an empty port group. When the other thread adds a port
    // to this port group, we should wake up and read the queued packet. The
    // bounded timeout means this thread always exits on its own, so the
    // parent's join cannot hang.
    port_result_t rslt;
    status_t st = port_read(pg, 500, &rslt);
    if (st < 0)
        return __LINE__;

    event_signal(&group_waiting_sync_evt, true);

    return 0;
}

typedef struct {
    port_t w_port;
    port_t r_port;
    bool created;
    port_t pg;
    bool grouped;
    thread_t *receiver;
} group_waiting_state_t;

/* The fallible middle of group_waiting. Everything it creates is recorded in
 * the state before use so that the caller's cleanup path can release exactly
 * what a failed ASSERT left behind. */
static bool group_waiting_body(group_waiting_state_t *s) {
    BEGIN_TEST;

    status_t st = make_port_pair("tst_port1", TS1_PORT_CTX, &s->w_port, &s->r_port);
    ASSERT_GE(st, 0, "could not make port pair");
    s->created = true;

    // Write something to this port group that currently has no receivers.
    port_packet_t pkt = { { 0 } };
    st = port_write(s->w_port, &pkt, 1);
    ASSERT_GE(st, 0, "could not write test port");

    // Create an empty port group.
    st = port_group(NULL, 0, &s->pg);
    ASSERT_GE(st, 0, "could not create an empty port group");
    s->grouped = true;

    s->receiver = thread_create(
                      "receiver",
                      &receive_thread,
                      (void *)s->pg,
                      DEFAULT_PRIORITY,
                      DEFAULT_STACK_SIZE
                  );
    ASSERT_NONNULL(s->receiver, "could not create receiver thread");
    thread_resume(s->receiver);

    // Wait for the other thread to block on the read.
    thread_sleep(20);

    // Adding a port that has data available to the port group should wake any
    // threads waiting on that port group.
    st = port_group_add(s->pg, s->r_port);
    ASSERT_GE(st, 0, "could not add the port to the group");

    st = event_wait_timeout(&group_waiting_sync_evt, 500);
    EXPECT_EQ(NO_ERROR, st, "receiver did not wake when a ready port joined the group");

    END_TEST;
}

/* Test the edge case where a read port with data available is added to a port
 * group that has a read-blocked receiver.
 */
static bool group_waiting(void) {
    BEGIN_TEST;

    event_init(&group_waiting_sync_evt, false, EVENT_FLAG_AUTOUNSIGNAL);

    group_waiting_state_t s = {};
    bool body_ok = group_waiting_body(&s);

    /* The cleanup below runs even when the body failed partway, so the named
     * test port cannot leak and poison every later run of this test with
     * ERR_ALREADY_EXISTS. The receiver always exits on its own -- its group
     * read is bounded -- so the join cannot hang. */
    if (s.receiver) {
        int retcode = -1;
        thread_join(s.receiver, &retcode, INFINITE_TIME);
        EXPECT_EQ(0, retcode, "receiver thread exited with an error line number");
    }

    status_t st;
    if (s.grouped) {
        st = port_close(s.pg);
        EXPECT_GE(st, 0, "could not close the port group");
    }

    if (s.created) {
        st = port_close(s.r_port);
        EXPECT_GE(st, 0, "could not close read port");

        st = port_close(s.w_port);
        EXPECT_GE(st, 0, "could not close test port");

        st = port_destroy(s.w_port);
        EXPECT_GE(st, 0, "could not destroy test port");
    }

    event_destroy(&group_waiting_sync_evt);

    EXPECT_TRUE(body_ok, "group waiting sequence failed");

    END_TEST;
}

BEGIN_TEST_CASE(port_tests)
RUN_TEST(single_thread_basic);
RUN_TEST(two_threads_basic);
RUN_TEST(two_threads_race);
RUN_TEST(group_basic);
RUN_TEST(group_dynamic);
RUN_TEST(group_waiting);
END_TEST_CASE(port_tests)
