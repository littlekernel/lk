#include <lib/ndebug/system/consoleproxy.h>

#include <lib/io.h>
#include <lib/ndebug/system/mux.h>


void consoleproxy_print_callback(print_callback_t *cb, 
                                 const char *str, size_t len)
{
    if (ndebug_sys_connected()) {
        // TODO(gkalsi): Infinite time is broken here because it'll cause the 
        // console thread to stall indefinitely. Maybe make it fail after 1s 
        // or something like that?
        ndebug_write_sys(str, len, NDEBUG_SYS_CHANNEL_COMMAND, INFINITE_TIME);
    }
}

static print_callback_t cb = {
    .entry = { 0 },
    .print = consoleproxy_print_callback,
    .context = NULL
};

void consoleproxy_init(void)
{
    register_print_callback(&cb);
}
