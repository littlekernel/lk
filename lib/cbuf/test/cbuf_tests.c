/*
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/cbuf.h>
#include <lib/heap.h>
#include <lib/unittest.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <rand.h>
#include <stdlib.h>

static bool basic(void) {
    BEGIN_TEST;
    cbuf_t cbuf;

    cbuf_initialize(&cbuf, 16);

    EXPECT_EQ(15UL, cbuf_space_avail(&cbuf), "");;

    EXPECT_EQ(8UL, cbuf_write(&cbuf, "abcdefgh", 8, false), "");;

    EXPECT_EQ(7UL, cbuf_space_avail(&cbuf), "");

    // Only 7 bytes should fit since if we write all 16 bytes,
    // head == tail and we can't distinguish it from the start case.
    EXPECT_EQ(7UL, cbuf_write(&cbuf, "ijklmnop", 8, false), "");

    EXPECT_EQ(0UL, cbuf_space_avail(&cbuf), "");

    // Nothing should fit.
    EXPECT_EQ(0UL, cbuf_write(&cbuf, "XXXXXXXX", 8, false), "");

    EXPECT_EQ(0UL, cbuf_space_avail(&cbuf), "");

    // Read a few bytes.
    {
        char buf[32];
        EXPECT_EQ(3UL, cbuf_read(&cbuf, buf, 3, false), "");
        for (int i = 0; i < 3; ++i) {
            EXPECT_EQ(buf[i], 'a' + i, "");
        }

        // Try reading 32 bytes.
        EXPECT_EQ(12UL, cbuf_read(&cbuf, buf, 32, false), "");
        for (int i = 0; i < 12; ++i) {
            EXPECT_EQ(buf[i], 'd' + i, "");
        }
    }

    cbuf_reset(&cbuf);

    EXPECT_EQ(15UL, cbuf_space_avail(&cbuf), "");

    END_TEST;
}

bool random(void) {
    BEGIN_TEST;

    cbuf_t cbuf;
    cbuf_initialize(&cbuf, 32);

    // Random tests. Keep writing in random chunks up to 8 bytes, then
    // reading in chunks up to 8 bytes. Verify values.

    size_t pos_out = 0;
    size_t pos_in = 0;
    while (pos_in < 256) {
        if (pos_out < 256) {
            // Write up to 8 bytes.
            unsigned char buf_out[8];
            size_t to_write_random = rand() & 7;
            size_t to_write = MIN(to_write_random, 256 - pos_out);
            for (size_t i = 0; i < to_write; ++i) {
                buf_out[i] = pos_out + i;
            }
            // Advance the out pointer based on how many bytes fit.
            size_t wrote = cbuf_write(&cbuf, buf_out, to_write, false);
            EXPECT_LE(wrote, to_write, "");
            pos_out += wrote;
        }

        // Read up to 8 bytes, make sure they are right.
        if (pos_in < pos_out) {
            unsigned char buf_in[8];
            size_t to_read_random = rand() & 7;
            size_t to_read = MIN(to_read_random, pos_out - pos_in);
            size_t read = cbuf_read(&cbuf, buf_in, to_read, false);
            EXPECT_LE(read, to_read, "");

            for (size_t i = 0; i < read; ++i) {
                EXPECT_EQ(pos_in + i, buf_in[i], "");
            }

            pos_in += read;
        }

        EXPECT_LE(pos_in, pos_out, "");
    }

    free(cbuf.buf);

    END_TEST;
}

BEGIN_TEST_CASE(cbuf_tests)
RUN_TEST(basic)
RUN_TEST(random)
END_TEST_CASE(cbuf_tests)