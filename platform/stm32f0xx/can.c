#include <platform/can.h>

#include <errno.h>
#include <pow2.h>
#include <stdbool.h>

#include <arch/arm/cm.h>
#include <kernel/mutex.h>
#include <lib/cbuf.h>
#include <stm32f0xx_can.h>
#include <stm32f0xx_rcc.h>

static cbuf_t can_rx_buf;
static mutex_t can_tx_mutex;

void stm32_CEC_IRQ(void)
{
    arm_cm_irq_entry();
    bool resched = false;

    while(CAN_MessagePending(CAN, CAN_FIFO0)) {
        // If there's no space left in the rx buffer, disable the RX interrupt.
        if (cbuf_space_avail(&can_rx_buf) < sizeof(can_msg_t)) {
            CAN_ITConfig(CAN, CAN_IT_FMP0, DISABLE);
            break;
        }

        CanRxMsg CAN_msg;
        CAN_Receive(CAN, CAN_FIFO0, &CAN_msg);

        can_msg_t msg;
        msg.id = CAN_msg.StdId;
        msg.id_ex = CAN_msg.ExtId;
        msg.ide = CAN_msg.IDE;
        msg.rtr = CAN_msg.RTR;
        msg.dlc = CAN_msg.DLC;

        int i;
        for (i = 0; i < msg.dlc; i++) {
            msg.data[i] = CAN_msg.Data[i];
        }

        cbuf_write(&can_rx_buf, &msg, sizeof(msg), false);
        resched = true;
    }

    arm_cm_irq_exit(resched);
}

void can_init(bool loopback) {
    // initialize the RX cbuf with enough room for 4 can frames
    cbuf_initialize(&can_rx_buf, round_up_pow2_u32(sizeof(can_msg_t) * 4));

    mutex_init(&can_tx_mutex);

    // Enable CAN peripheral clock.
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN, ENABLE);

    // Reset CAN registers.
    CAN_DeInit(CAN);

    CAN_InitTypeDef init;

    // Config for loopback.
    init.CAN_TTCM = DISABLE;
    init.CAN_ABOM = DISABLE;
    init.CAN_AWUM = DISABLE;
    init.CAN_NART = DISABLE;
    init.CAN_RFLM = DISABLE;
    init.CAN_TXFP = DISABLE;
    init.CAN_Mode = loopback ? CAN_Mode_LoopBack : CAN_Mode_Normal;
    init.CAN_SJW = CAN_SJW_1tq;

    // CAN Baudrate = 125kbps (CAN clocked at 36 MHz)
    // XXX: this is probably wrong running at 48MHz
    init.CAN_BS1 = CAN_BS1_9tq;
    init.CAN_BS2 = CAN_BS2_8tq;
    init.CAN_Prescaler = 16;

    CAN_Init(CAN, &init);

    CAN_FilterInitTypeDef filter;
    filter.CAN_FilterNumber = 0;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0x0000;
    filter.CAN_FilterIdLow = 0x0000;
    filter.CAN_FilterMaskIdHigh = 0x0000;
    filter.CAN_FilterMaskIdLow = 0x0000;
    filter.CAN_FilterFIFOAssignment = 0;
    filter.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&filter);

    CAN_ITConfig(CAN, CAN_IT_FMP0, ENABLE);
    NVIC_EnableIRQ(CEC_IRQn);
}

ssize_t can_send(const can_msg_t *msg) {
    CanTxMsg CAN_msg;
    uint8_t mailbox;
    int i;

    CAN_msg.StdId = msg->id;
    CAN_msg.ExtId = msg->id_ex;
    CAN_msg.IDE = msg->ide;
    CAN_msg.RTR = msg->rtr;
    CAN_msg.DLC = msg->dlc;

    for (i = 0; i < msg->dlc; i++) {
        CAN_msg.Data[i] = msg->data[i];
    }

    mutex_acquire(&can_tx_mutex);
    mailbox = CAN_Transmit(CAN, &CAN_msg);
    mutex_release(&can_tx_mutex);

    if (mailbox == CAN_TxStatus_NoMailBox) {
        return -EWOULDBLOCK;
    } else {
        return msg->dlc;
    }
}

ssize_t can_recv(can_msg_t *msg, bool block) {
    size_t bytes_read;

    bytes_read = cbuf_read(&can_rx_buf, msg, sizeof(*msg), block);
    if (cbuf_space_avail(&can_rx_buf) >= sizeof(*msg)) {
        CAN_ITConfig(CAN, CAN_IT_FMP0, ENABLE);
    }

    return bytes_read > 0 ? msg->dlc : -EWOULDBLOCK;
}
