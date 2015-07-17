#pragma once
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

status_t stmflash_init(uint32_t start, uint32_t length);

#ifdef __cplusplus
}
#endif
