#include <lk/reg.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <sys/types.h>


// TOTAL HACK: just happens to be where the SCC is mapped on openbios on ss20.
static volatile uint8_t * const control = (volatile uint8_t *)0xffdcf004;
static volatile uint8_t * const data = (volatile uint8_t *)0xffdcf006;

static inline uint8_t scc_read(volatile uint8_t *addr) {
    return *addr;
}

static inline void scc_write(volatile uint8_t *addr, uint8_t val) {
   *addr = val;
}

void platform_dputc(char c) {

    if (c == '\n') {
        while ((scc_read(control) & 0x04) == 0) {
            // spin
        }
        scc_write(data, '\r');
    }

    while ((scc_read(control) & 0x04) == 0) {
        // spin
    }
    scc_write(data, c);
}

int platform_dgetc(char *c, bool wait) {
    if (wait) {
        while ((scc_read(control) & 0x01) == 0) {
            // spin
        }
        *c = scc_read(data);
        return 0;
    } else {
        if ((scc_read(control) & 0x01) != 0) {
            *c = scc_read(data);
            return 0;
        }
        return -1;
    }
}

void platform_early_init(void) {
}

void platform_init(void) {
}

/* timer stubs */
status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return NO_ERROR;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return NO_ERROR;
}

void platform_stop_timer(void) {
}

lk_time_t current_time(void) {
    return 0;
}

lk_bigtime_t current_time_hires(void) {
    return 0;
}
