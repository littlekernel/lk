#ifndef __PLATFORM_STM32_CAN_H
#define __PLATFORM_STM32_CAN_H

#include <compiler.h>
#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>

typedef struct {
    unsigned id:11;  // Standard CAN identifier.
    unsigned id_ex:18;  // Extended CAN identifier.
    unsigned rtr:1;  // Remote transmit request.
    unsigned ide:1;  // Identifier extension.
    unsigned pad:1;

    uint8_t dlc;  // Data length.
    uint8_t data[8];
} __PACKED can_msg_t;

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
 * @return Negative error code on error, size of data queued on success.
 */
ssize_t can_send(const can_msg_t *msg);

/**
 * can_recv
 *
 * @param[out] msg Received message
 * @param[in] block If true, can_recv() will block until a message is received.
 *
 * @return Negative error code on error, size of data received on success.
 */
ssize_t can_recv(can_msg_t *msg, bool block);

#endif  // __PLATFORM_STM32_CAN_H
