#pragma once

#include <stdint.h>
#include <sys/types.h>

typedef enum {
	NDBUG_CHANNEL_SYS,
	NDBUG_CHANNEL_USR,

	NDBUG_CHANNEL_COUNT,	// Count: always last.
} channel_t;

void ndbug_init(void);

ssize_t ndbug_read(const channel_t ch, const size_t n, uint8_t *buf);
ssize_t ndbug_write(const channel_t ch, const size_t n, uint8_t *buf);
