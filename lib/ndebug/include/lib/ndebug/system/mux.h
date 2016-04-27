#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <lib/ndebug/shared_structs.h>

void ndebug_sys_init(void);

bool ndebug_sys_connected(void);

ssize_t ndebug_sys_read(uint8_t **buf, const sys_channel_t ch, 
						const lk_time_t timeout);

ssize_t ndebug_write_sys(uint8_t *buf, const size_t n, const sys_channel_t ch,
						 const lk_time_t timeout);