#pragma once

struct __attribute__((__packed__)) ndebug_ctrl_packet_t {
    uint32_t magic;
    uint32_t type;
};

#define NDEBUG_CTRL_PACKET_MAGIC (0x4354524C)

#define NDEBUG_CTRL_CMD_RESET (0x01)
#define NDEBUG_CTRL_CMD_DATA (0x02)
#define NDEBUG_CTRL_CMD_ESTABLISHED (0x03)

#define NDEBUG_USB_CLASS_USER_DEFINED (0xFF)
#define NDEBUG_SUBCLASS (0x02)

#define NDEBUG_PROTOCOL_LK_SYSTEM (0x01)
#define NDEBUG_PROTOCOL_SERIAL_PIPE (0x02)