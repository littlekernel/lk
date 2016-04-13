#pragma once

#include <stdint.h>
#include <sys/types.h>

#define NDEBUG_MAX_PACKET_SIZE (64)

typedef enum {
    NDEBUG_CHANNEL_SYS,
    NDEBUG_CHANNEL_USR,

    NDEBUG_CHANNEL_COUNT,   // Count: always last.
} channel_t;

void ndebug_init(void);

ssize_t ndebug_usb_read(const channel_t ch, const size_t n,
                        const lk_time_t timeout, uint8_t *buf);
ssize_t ndebug_usb_write(const channel_t ch, const size_t n,
                         const lk_time_t timeout, uint8_t *buf);

status_t await_usb_online(lk_time_t timeout);