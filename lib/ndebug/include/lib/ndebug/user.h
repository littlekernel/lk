#pragma once

#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/shared_structs.h>

typedef struct {
    uint32_t magic;
    uint32_t type;
} ndebug_ctrl_packet_t;

#define NDEBUG_USR_MAX_PACKET_SIZE (NDEBUG_MAX_PACKET_SIZE - sizeof(ndebug_ctrl_packet_t))


// Read and write to the NDebug user channel.
ssize_t ndebug_read_usr(uint8_t *buf, const lk_time_t timeout);
ssize_t ndebug_write_usr(uint8_t *buf, const size_t n, const lk_time_t timeout);


// Wait for the host to establish a connection on the usr channel.
status_t ndebugusr_await_host(lk_time_t timeout);