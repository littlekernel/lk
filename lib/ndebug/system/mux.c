#include <lib/ndebug/system/mux.h>

#include <assert.h>
#include <trace.h>
#include <kernel/event.h>
#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/shared_structs.h>
#include <lib/ndebug/system/consoleproxy.h>
#include <err.h>
#include <string.h>


typedef struct {
    event_t event;
    uint8_t *buf;
    ssize_t retcode;
} ch_t;

ch_t channels[NDEBUG_SYS_CHANNEL_COUNT];
static volatile bool connected = false;


static int reader_thread(void *argv)
{
    uint8_t buf[NDEBUG_MAX_PACKET_SIZE];

    while (true) {
        status_t result =
            ndebug_await_connection(NDEBUG_CHANNEL_SYS, INFINITE_TIME);
        if (result != NO_ERROR) {
            dprintf(INFO, "await_connection failed; status = %d\n", result);
            continue;
        }
        connected = true;

        while (true) {
            ssize_t bytes_read = ndebug_usb_read(
                NDEBUG_CHANNEL_SYS, sizeof(buf), INFINITE_TIME, buf);
            if (bytes_read < 0) break;
            if (bytes_read < (ssize_t)sizeof(ndebug_system_packet_t)) continue;

            ndebug_system_packet_t *pkt = (ndebug_system_packet_t *)buf;
            if (pkt->ctrl.magic != NDEBUG_CTRL_PACKET_MAGIC) continue;

            switch (pkt->ctrl.type) {
                case NDEBUG_CTRL_CMD_RESET: {
                    for (sys_channel_t ch = NDEBUG_SYS_CHANNEL_CONSOLE; 
                         ch != NDEBUG_SYS_CHANNEL_COUNT; ch++) {
                        channels[ch].retcode = ERR_CHANNEL_CLOSED;
                        event_signal(&channels[ch].event, false);
                    }
                    connected = false;
                    break;
                }
                case NDEBUG_CTRL_CMD_DATA: {
                    // TODO(gkalsi): Make sure the channel is actually blocked
                    // on a read (ready to recv).
                    if (pkt->channel >= NDEBUG_SYS_CHANNEL_COUNT) continue;
                    ch_t *channel = &channels[pkt->channel];
                    memcpy(channel->buf, buf, bytes_read);
                    channel->retcode = bytes_read;
                    event_signal(&channel->event, false);
                    break;
                }
            }  // switch
            if (!connected) break;
        }
    }
    return 0;
}

void ndebug_sys_init(void)
{
    // Initialize the Channels.
    for (sys_channel_t ch = NDEBUG_SYS_CHANNEL_CONSOLE; 
         ch != NDEBUG_SYS_CHANNEL_COUNT; ch++) {
        event_init(&channels[ch].event, false, EVENT_FLAG_AUTOUNSIGNAL);
        channels[ch].buf = malloc(NDEBUG_MAX_PACKET_SIZE);
        channels[ch].retcode = 0;
    }
    
    // Start the reader thread.
    thread_resume(
        thread_create("ndebug mux reader", &reader_thread, NULL,
                      DEFAULT_PRIORITY, DEFAULT_STACK_SIZE)
    );

    // Initialize subsystems.
    consoleproxy_init();
}

static ssize_t ndebug_write_sys_internal(uint8_t *buf, const size_t n, 
                                         const sys_channel_t ch,
                                         const lk_time_t timeout,
                                         const uint32_t type)
{
    // TODO(gkalsi): Packetize long writes? Maybe?
    if (n > (NDEBUG_MAX_PACKET_SIZE - sizeof(ndebug_system_packet_t)))
        return ERR_TOO_BIG;

    uint8_t write_buf[NDEBUG_MAX_PACKET_SIZE];
    uint8_t *cursor = write_buf;

    ndebug_system_packet_t *pkt = (ndebug_system_packet_t *)cursor;
    pkt->ctrl.magic = NDEBUG_CTRL_PACKET_MAGIC;
    pkt->ctrl.type = type;
    pkt->channel = ch;

    cursor += sizeof(ndebug_system_packet_t);
    memcpy(cursor, buf, n);

    ssize_t written = ndebug_usb_write(
        NDEBUG_CHANNEL_SYS, 
        n + sizeof(ndebug_system_packet_t),
        5000, /* TODO(gkalsi): timeout? */
        write_buf
    );

    return written;
}

ssize_t ndebug_write_sys(uint8_t *buf, const size_t n, const sys_channel_t ch,
                         const lk_time_t timeout)
{
    return ndebug_write_sys_internal(buf, n, ch, timeout, NDEBUG_CTRL_CMD_DATA);
}

ssize_t ndebug_sys_read(uint8_t **buf, const sys_channel_t ch, 
                        const lk_time_t timeout)
{
    DEBUG_ASSERT(ch < NDEBUG_SYS_CHANNEL_COUNT);

    // Send a message to the host that a channel has become ready.
    uint32_t data[2];
    data[0] = NDEBUG_SYS_CHANNEL_READY;
    data[1] = ch;
    ssize_t written =
        ndebug_write_sys_internal((uint8_t *)data, sizeof(data), 
                                  ch, timeout, NDEBUG_CTRL_CMD_CTRL);
    // TODO(gkalsi): Handle case where this write fails? Retry? Abort?
    if (written < 0) {
        printf("WRITE FAILED\n");
    }

    // Block on an event and wait for the reader thread to signal us
    ch_t *channel = &channels[ch];
    event_wait_timeout(&channel->event, timeout);

    *buf = channel->buf;

    return channel->retcode;
}

bool ndebug_sys_connected(void)
{
    return connected;
}
