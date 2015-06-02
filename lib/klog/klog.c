/*
 * Copyright (c) 2013 Google, Inc.
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
#include <lib/klog.h>

#include <err.h>
#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <platform.h>
#include <lib/cksum.h>

#define LOCAL_TRACE 0

#ifndef MAX_KLOG_SIZE
#define MAX_KLOG_SIZE (32*1024)
#endif

#define KLOG_BUFFER_HEADER_MAGIC 'KLGB'

struct klog_buffer_header {
    uint32_t magic;
    uint32_t header_crc32;
    uint32_t log_count;
    uint32_t current_log;
    uint32_t total_size;
};

#define KLOG_HEADER_MAGIC 'KLOG'

struct klog_header {
    uint32_t magic;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t data_checksum;
    uint8_t  data[0];
};

/* current klog buffer */
static struct klog_buffer_header *klog_buf;

/* current klog */
static struct klog_header *klog;

static struct klog_header *find_nth_log(uint log)
{
    DEBUG_ASSERT(klog_buf);
    DEBUG_ASSERT(klog_buf->magic == KLOG_BUFFER_HEADER_MAGIC);
    DEBUG_ASSERT(log < klog_buf->log_count);

    struct klog_header *k = (struct klog_header *)(klog_buf + 1);
    while (log > 0) {
        DEBUG_ASSERT(k->magic == KLOG_HEADER_MAGIC);

        uint8_t *ptr = (uint8_t *)k->data;
        ptr += k->size;
        k = (struct klog_header *)ptr;

        log--;
    }

    return k;
}

static uint32_t get_checksum_klog_buffer_header(const struct klog_buffer_header *kb)
{
    DEBUG_ASSERT(kb);
    DEBUG_ASSERT(kb->magic == KLOG_BUFFER_HEADER_MAGIC);

    return crc32(0, (const void *)(&kb->header_crc32 + 1), sizeof(*kb) - 8);
}

static uint32_t get_checksum_klog_data(const struct klog_header *k)
{
    DEBUG_ASSERT(k);
    DEBUG_ASSERT(k->magic == KLOG_HEADER_MAGIC);

    uint32_t sum = 0;
    for (uint i = 0; i < k->size; i++) {
        sum += k->data[i];
    }

    return sum;
}

static void checksum_klog_buffer_header(struct klog_buffer_header *kb)
{
    DEBUG_ASSERT(kb);
    DEBUG_ASSERT(kb->magic == KLOG_BUFFER_HEADER_MAGIC);

    kb->header_crc32 = get_checksum_klog_buffer_header(kb);
}

static void checksum_klog_data(struct klog_header *k)
{
    DEBUG_ASSERT(k);
    DEBUG_ASSERT(k->magic == KLOG_HEADER_MAGIC);

    k->data_checksum = get_checksum_klog_data(k);
}

void klog_init(void)
{
}

status_t klog_create(void *_ptr, size_t len, uint count)
{
    uint8_t *ptr = _ptr;
    LTRACEF("ptr %p len %zu count %u\n", ptr, len, count);

    /* check args */
    if (!ptr)
        return ERR_INVALID_ARGS;
    if (count == 0)
        return ERR_INVALID_ARGS;

    /* check that the size is big enough */
    if (len < (sizeof(struct klog_buffer_header) + sizeof(struct klog_header) * count + 4 * count))
        return ERR_INVALID_ARGS;

    /* set up the buffer header */
    klog_buf = (struct klog_buffer_header *)ptr;
    klog_buf->magic = KLOG_BUFFER_HEADER_MAGIC;
    klog_buf->log_count = count;
    klog_buf->current_log = 0;
    klog_buf->total_size = len;
    checksum_klog_buffer_header(klog_buf);
    ptr += sizeof(struct klog_buffer_header);

    /* set up each buffer */
    uint bufsize = len - sizeof(struct klog_buffer_header) - sizeof(struct klog_header) * count;
    bufsize /= count;
    bufsize = ROUNDDOWN(bufsize, 4);
    while (count > 0) {
        klog = (struct klog_header *)ptr;
        klog->magic = KLOG_HEADER_MAGIC;
        klog->size = bufsize;
        klog->head = 0;
        klog->tail = 0;
        klog->data_checksum = 0;
        memset(klog + 1, 0, bufsize);
        checksum_klog_data(klog);
        ptr += sizeof(struct klog_header) + bufsize;
        count--;
    }

    klog_set_current_buffer(0);

    DEBUG_ASSERT(klog_buf);
    DEBUG_ASSERT(klog);

    return NO_ERROR;
}

ssize_t klog_recover(void *_ptr)
{
    uint8_t *ptr = _ptr;
    LTRACEF("ptr %p\n", ptr);

    if (!ptr)
        return ERR_INVALID_ARGS;

    /* look for header at pointer */
    struct klog_buffer_header *kbuf = (struct klog_buffer_header *)ptr;
    if (kbuf->magic != KLOG_BUFFER_HEADER_MAGIC)
        return ERR_NOT_FOUND;
    uint32_t crc = get_checksum_klog_buffer_header(kbuf);
    if (crc != kbuf->header_crc32)
        return ERR_NOT_FOUND;

    /* some sanity checks */
    if (kbuf->total_size > MAX_KLOG_SIZE)
        return ERR_NOT_FOUND;
    if (kbuf->current_log >= kbuf->log_count)
        return ERR_NOT_FOUND;

    /* walk the list of klogs, validating */
    ptr += sizeof(struct klog_buffer_header);
    for (uint i = 0; i < kbuf->log_count; i++) {
        struct klog_header *k = (struct klog_header *)ptr;

        /* validate the individual klog */
        if (k->magic != KLOG_HEADER_MAGIC)
            return ERR_NOT_FOUND;

        /* validate some fields */
        if ((k->size > 0) && (k->size & 3))
            return ERR_NOT_FOUND;
        if (k->size > MAX_KLOG_SIZE)
            return ERR_NOT_FOUND;
        if (k->head >= k->size)
            return ERR_NOT_FOUND;
        if (k->tail >= k->size)
            return ERR_NOT_FOUND;

        /* data checksum */
        if (k->data_checksum) {
            crc = get_checksum_klog_data(k);
            if (crc != k->data_checksum)
                return ERR_NOT_FOUND;
        }

        ptr += sizeof(struct klog_header) + k->size;
    }

    /* everything checks out */
    klog_buf = kbuf;
    klog_set_current_buffer(klog_buf->current_log);

    LTRACEF("found buffer at %p, current log %u (%p) head %u tail %u size %u\n",
            klog_buf, klog_buf->current_log, klog, klog->head, klog->tail, klog->size);

    return NO_ERROR;
}

uint klog_buffer_count(void)
{
    if (!klog_buf)
        return 0;

    DEBUG_ASSERT(klog_buf);
    DEBUG_ASSERT(klog_buf->magic == KLOG_BUFFER_HEADER_MAGIC);

    return klog_buf->log_count;
}

uint klog_current_buffer(void)
{
    if (!klog_buf)
        return 0;

    DEBUG_ASSERT(klog_buf);
    DEBUG_ASSERT(klog_buf->magic == KLOG_BUFFER_HEADER_MAGIC);

    return klog_buf->current_log;
}

status_t klog_set_current_buffer(uint buffer)
{
    if (!klog_buf)
        return ERR_NOT_FOUND;

    DEBUG_ASSERT(klog_buf);
    DEBUG_ASSERT(klog_buf->magic == KLOG_BUFFER_HEADER_MAGIC);

    if (buffer >= klog_buf->log_count)
        return ERR_INVALID_ARGS;

    /* find the nth buffer */
    klog = find_nth_log(buffer);

    /* update the klog buffer header */
    if (buffer != klog_buf->current_log) {
        klog_buf->current_log = buffer;
        checksum_klog_buffer_header(klog_buf);
    }

    return NO_ERROR;
}

#include <arch/ops.h>

ssize_t klog_read(char *buf, size_t len, int buf_id)
{
    size_t offset = 0;
    size_t tmp_len;
    iovec_t vec[2];
    LTRACEF("read (len %zu, buf %u)\n", len, buf_id);

    DEBUG_ASSERT(klog);
    DEBUG_ASSERT(klog->magic == KLOG_HEADER_MAGIC);

    /* If a klog wraps around at the end then it becomes two iovecs with
     * tail being the start of 1 and head being the end of 0. This means we
     * need to check where we are in the overall klog to properly determine
     * which iovec we want to read from */
    int vec_cnt = klog_get_buffer(buf_id, vec);
    if (vec_cnt < 1)
        return vec_cnt;

    tmp_len = MIN(len, vec[0].iov_len);
    memcpy(buf, (const char *)vec[0].iov_base, tmp_len);
    offset += tmp_len;
    len -= tmp_len;

    if (len != 0 && vec_cnt > 1) {
        tmp_len = MIN(len, vec[1].iov_len);
        memcpy(buf + offset, (const char *)vec[1].iov_base, tmp_len);
        offset += tmp_len;
    }

    /* Since iovecs are generated by get_buffer we only need to update the tail pointer */
    klog->tail += offset;
    if (klog->tail >= klog->size)
        klog->tail -= klog->size;

    return offset;
}

char klog_getc(int buf_id)
{
    char c = '\0';
    int err = klog_read(&c, 1, buf_id);

    return (err < 0) ? err : c;
}

char klog_getchar(void)
{
    return klog_getc(-1);
}

/* Returns whether the currently selected klog contains data */
bool klog_has_data(void)
{
    DEBUG_ASSERT(klog);

    return (klog->head != klog->tail);
}

static size_t klog_puts_len(const char *str, size_t len)
{
    LTRACEF("puts '%s'\n", str);

    DEBUG_ASSERT(klog);
    DEBUG_ASSERT(klog->magic == KLOG_HEADER_MAGIC);

    LTRACEF("before write head %u tail %u size %u\n", klog->head, klog->tail, klog->size);
    uint32_t deltasum = 0;
    size_t count = 0;
    while (count < len && *str) {
        /* compute the delta checksum */
        deltasum += *str - klog->data[klog->head];

        /* store the data */
        klog->data[klog->head] = *str;

        /* bump the head */
        uint newhead = klog->head + 1;
        if (newhead >= klog->size)
            newhead -= klog->size;
        DEBUG_ASSERT(newhead < klog->size);

        /* bump the tail if the head collided with it */
        if (klog->tail == newhead) {
            uint newtail = klog->tail + 1;
            if (newtail >= klog->size)
                newtail -= klog->size;
            DEBUG_ASSERT(newtail < klog->size);
            klog->tail = newtail;
        }

        /* writeback the head */
        klog->head = newhead;

        str++;
        count++;
    }
    LTRACEF("after write head %u tail %u\n", klog->head, klog->tail);

    klog->data_checksum += deltasum;

    LTRACEF("kputs len %u\n", count);

    return count;
}

void klog_putchar(char c)
{
    klog_puts_len(&c, 1);
}

void klog_puts(const char *str)
{
    if (!klog_buf)
        return;

    klog_puts_len(str, SIZE_MAX);
}

static int _klog_output_func(const char *str, size_t len, void *state)
{
    return klog_puts_len(str, len);
}

void klog_printf(const char *fmt, ...)
{
    if (!klog_buf)
        return;

    va_list ap;
    va_start(ap, fmt);
    _printf_engine(&_klog_output_func, NULL, fmt, ap);
    va_end(ap);
}

void klog_vprintf(const char *fmt, va_list ap)
{
    if (!klog_buf)
        return;

    _printf_engine(&_klog_output_func, NULL, fmt, ap);
}

int klog_get_buffer(int buffer, iovec_t *vec)
{
    if (!klog_buf)
        return 0;
    if (!vec)
        return ERR_INVALID_ARGS;
    if (buffer >= 0 && (uint)buffer >= klog_buf->log_count)
        return ERR_INVALID_ARGS;

    struct klog_header *k;
    if (buffer < 0)
        k = klog;
    else
        k = find_nth_log(buffer);

    DEBUG_ASSERT(k);
    DEBUG_ASSERT(k->magic == KLOG_HEADER_MAGIC);

    vec[0].iov_base = &k->data[k->tail];
    if (k->head == k->tail) {
        return 0;
    } else if (k->head > k->tail) {
        /* single run of data, between tail and head */
        vec[0].iov_len = k->head - k->tail;

        return 1;
    } else {
        vec[0].iov_len = k->size - k->tail;

        /* two segments */
        vec[1].iov_base = &k->data[0];
        vec[1].iov_len = k->head;

        return 2;
    }
}

void klog_dump(int buffer)
{
    iovec_t vec[2];

    int err = klog_get_buffer(buffer, vec);
    if (err <= 0)
        return;

    for (uint i = 0; i < vec[0].iov_len; i++)
        putchar(*((const char *)vec[0].iov_base + i));
    if (err > 1) {
        for (uint i = 0; i < vec[1].iov_len; i++)
            putchar(*((const char *)vec[1].iov_base + i));
    }
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

#define KLOG_RETENTION_TEST 0
#if KLOG_RETENTION_TEST
#include <platform/retention.h>

_RETENTION_NOCLEAR(static uint8_t klog_test_buf[512]);
#endif

static int cmd_klog(int argc, const cmd_args *argv)
{
    status_t err;

    if (argc < 2) {
notenoughargs:
        printf("ERROR not enough arguments\n");
usage:
        printf("usage: %s create <size> <count>\n", argv[0].str);
#if KLOG_RETENTION_TEST
        printf("usage: %s createret \n", argv[0].str);
        printf("usage: %s recoverret \n", argv[0].str);
#endif
        printf("usage: %s getbufcount\n", argv[0].str);
        printf("usage: %s getbufnum\n", argv[0].str);
        printf("usage: %s getbufptr\n", argv[0].str);
        printf("usage: %s setbufnum <num>\n", argv[0].str);
        printf("usage: %s puts <string>\n", argv[0].str);
        printf("usage: %s read <length> <buffer num>\n", argv[0].str);
        printf("usage: %s printftest\n", argv[0].str);
        printf("usage: %s dump [buffer num]\n", argv[0].str);
        printf("usage: %s vec [buffer num]\n", argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "create")) {
        if (argc < 4) goto notenoughargs;

        uint size = argv[2].u;
        uint count = argv[3].u;

        void *ptr = malloc(size);
        if (!ptr) {
            printf("error allocating memory for klog\n");
            return -1;
        }
        err = klog_create(ptr, size, count);
        printf("klog_create returns %d\n", err);
        if (err < 0)
            free(ptr);
#if KLOG_RETENTION_TEST
    } else if (!strcmp(argv[1].str, "createret")) {
        err = klog_create(klog_test_buf, sizeof(klog_test_buf), 1);
        printf("klog_create returns %d\n", err);
    } else if (!strcmp(argv[1].str, "recoverret")) {
        err = klog_recover(klog_test_buf);
        printf("klog_recover returns %d\n", err);
#endif
    } else if (!strcmp(argv[1].str, "getbufcount")) {
        printf("%d buffers\n", klog_buffer_count());
    } else if (!strcmp(argv[1].str, "getbufnum")) {
        printf("%d current buffer\n", klog_current_buffer());
    } else if (!strcmp(argv[1].str, "getbufptr")) {
        printf("ptr %p\n", klog);
    } else if (!strcmp(argv[1].str, "setbufnum")) {
        if (argc < 3) goto notenoughargs;

        err = klog_set_current_buffer(argv[2].u);
        printf("klog_set_current_buffer returns %d\n", err);
    } else if (!strcmp(argv[1].str, "puts")) {
        if (argc < 3) goto notenoughargs;

        klog_puts(argv[2].str);
        klog_putchar('\n');
    } else if (!strcmp(argv[1].str, "read")) {
        if (argc < 4) goto notenoughargs;
        size_t len = argv[2].u;
        int buf_id = argv[3].i;
        char *buf = malloc(len);
        if (!buf) {
            printf("error allocating memory for klog read\n");
            return -1;
        }
        size_t count = klog_read(buf, len, buf_id);
        if (count > 0) {
            printf("read %zu byte(s): \"", count);
            for (size_t i = 0; i < count; i++)
                putchar(buf[i]);
            putchar('\"');
            putchar('\n');
        } else {
            printf("read returned error: %d\n", count);
        }
        free(buf);
    } else if (!strcmp(argv[1].str, "getc")) {
        if (argc < 3) goto notenoughargs;
        int buf_id = argv[2].i;
        printf("read: '%c'\n", klog_getc(buf_id));
    } else if (!strcmp(argv[1].str, "printftest")) {
        klog_printf("a plain string\n");
        klog_printf("numbers: %d %d %d %u\n", 1, 2, 3, 99);
        klog_printf("strings: '%s' '%s'\n", "a little string", "another one");
    } else if (!strcmp(argv[1].str, "dump")) {
        int buffer = -1;

        if (argc >= 3)
            buffer = argv[2].u;

        klog_dump(buffer);
    } else if (!strcmp(argv[1].str, "vec")) {
        int buffer = -1;

        if (argc >= 3)
            buffer = argv[2].u;

        iovec_t vec[2];
        memset(vec, 0x99, sizeof(vec));
        int err = klog_get_buffer(buffer, vec);
        printf("klog_get_buffer returns %d\n", err);
        printf("vec %d: base %p, len %zu\n", 0, vec[0].iov_base, vec[0].iov_len);
        printf("vec %d: base %p, len %zu\n", 1, vec[1].iov_base, vec[1].iov_len);
    } else {
        printf("ERROR unknown command\n");
        goto usage;
    }

    return 0;
}

STATIC_COMMAND_START
{ "klog", "commands for manipulating klog", &cmd_klog },
STATIC_COMMAND_END(klog);

#endif // WITH_LIB_CONSOLE

