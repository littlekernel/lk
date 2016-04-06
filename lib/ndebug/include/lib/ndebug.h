#pragma once

#include <stdint.h>
#include <sys/types.h>

typedef enum {
    NDEBUG_CHANNEL_SYS,
    NDEBUG_CHANNEL_USR,

    NDEBUG_CHANNEL_COUNT,   // Count: always last.
} channel_t;

void ndebug_init(void);

ssize_t ndebug_read(const channel_t ch, const size_t n, uint8_t *buf);
ssize_t ndebug_write(const channel_t ch, const size_t n, uint8_t *buf);
