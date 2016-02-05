#ifndef __PLATFORM_STM32_CAN_H
#define __PLATFORM_STM32_CAN_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    unsigned id:11;  // Standard CAN identifier.
    unsigned id_ex:18;  // Extended CAN identifier.
    unsigned rtr:1;  // Remote transmit request.
    unsigned ide:1;  // Identifier extension.
    unsigned pad:1;

    uint8_t dlc;  // Data length.
    uint8_t data[8];
} __attribute__((packed)) can_msg_t;

/**
 * can_init
 *
 * Initialize the CAN peripheral.
 *
 * @param[in] loopback If true, puts the can interface in loopback mode.
 */
void can_init(bool loopback);

/**
 * can_send
 *
 * Queues a can message to be sent.  Does not block if there is no space
 * in the CAN mailboxes.
 *
 * @param[in] msg Message to send.
 *
 * @return True if the message was queued.
 */
bool can_send(const can_msg_t *msg);

/**
 * can_recv
 *
 * @param[out] msg Received message
 * @param[in] block If true, can_recv() will block until a message is received.
 *
 * @return True if a message was received.
 */
bool can_recv(can_msg_t *msg, bool block);

#endif  // __PLATFORM_STM32_CAN_H
