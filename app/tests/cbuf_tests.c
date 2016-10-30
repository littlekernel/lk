#include <assert.h>
#include <debug.h>
#include <err.h>
#include <lib/cbuf.h>
#include <lib/console.h>
#include <lib/heap.h>
#include <rand.h>
#include <stdlib.h>

#define ASSERT_EQ(a, b)                                            \
    do {                                                           \
        int _a = (a);                                              \
        int _b = (b);                                              \
        if (_a != _b) {                                            \
            panic("%d != %d (%s:%d)\n", a, b, __FILE__, __LINE__); \
        }                                                          \
    } while (0);

#define ASSERT_LEQ(a, b)                                               \
    do {                                                               \
        int _a = (a);                                                  \
        int _b = (b);                                                  \
        if (_a > _b) {                                                 \
            panic("%d not <= %d (%s:%d)\n", a, b, __FILE__, __LINE__); \
        }                                                              \
    } while (0);

int cbuf_tests(int argc, const cmd_args *argv)
{
    cbuf_t cbuf;

    printf("running basic tests...\n");

    cbuf_initialize(&cbuf, 16);

    ASSERT_EQ(15, cbuf_space_avail(&cbuf));

    ASSERT_EQ(8, cbuf_write(&cbuf, "abcdefgh", 8, false));

    ASSERT_EQ(7, cbuf_space_avail(&cbuf));

    // Only 7 bytes should fit since if we write all 16 bytes,
    // head == tail and we can't distinguish it from the start case.
    ASSERT_EQ(7, cbuf_write(&cbuf, "ijklmnop", 8, false));

    ASSERT_EQ(0, cbuf_space_avail(&cbuf));

    // Nothing should fit.
    ASSERT_EQ(0, cbuf_write(&cbuf, "XXXXXXXX", 8, false));

    ASSERT_EQ(0, cbuf_space_avail(&cbuf));

    // Read a few bytes.
    {
        char buf[32];
        ASSERT_EQ(3, cbuf_read(&cbuf, buf, 3, false));
        for (int i = 0; i < 3; ++i) {
            ASSERT_EQ(buf[i], 'a' + i);
        }

        // Try reading 32 bytes.
        ASSERT_EQ(12, cbuf_read(&cbuf, buf, 32, false));
        for (int i = 0; i < 12; ++i) {
            ASSERT_EQ(buf[i], 'd' + i);
        }
    }

    cbuf_reset(&cbuf);

    ASSERT_EQ(15, cbuf_space_avail(&cbuf));

    // Random tests. Keep writing in random chunks up to 8 bytes, then
    // reading in chunks up to 8 bytes. Verify values.

    int pos_out = 0;
    int pos_in = 0;
    printf("running random tests...\n");
    while (pos_in < 256) {
        if (pos_out < 256) {
            // Write up to 8 bytes.
            char buf_out[8];
            int to_write_random = rand() & 7;
            int to_write = MIN(to_write_random, 256 - pos_out);
            for (int i = 0; i < to_write; ++i) {
                buf_out[i] = pos_out + i;
            }
            // Advance the out pointer based on how many bytes fit.
            int wrote = cbuf_write(&cbuf, buf_out, to_write, false);
            ASSERT_LEQ(wrote, to_write);
            pos_out += wrote;
        }

        // Read up to 8 bytes, make sure they are right.
        if (pos_in < pos_out) {
            char buf_in[8];
            int to_read_random = rand() & 7;
            int to_read = MIN(to_read_random, pos_out - pos_in);
            int read = cbuf_read(&cbuf, buf_in, to_read, false);
            ASSERT_LEQ(read, to_read);

            for (int i = 0; i < read; ++i) {
                ASSERT_EQ(pos_in + i, buf_in[i]);
            }

            pos_in += read;
        }

        ASSERT_LEQ(pos_in, pos_out);
    }

    free(cbuf.buf);

    printf("cbuf tests passed\n");

    return NO_ERROR;
}
