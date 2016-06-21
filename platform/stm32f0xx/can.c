#include <platform/can.h>

#include <assert.h>
#include <errno.h>
#include <pow2.h>
#include <stdbool.h>

#include <arch/arm/cm.h>
#include <kernel/mutex.h>
#include <lib/cbuf.h>
#include <platform/rcc.h>

typedef CAN_TypeDef stm32_can_t;
typedef CAN_TxMailBox_TypeDef stm32_can_tx_mailbox_t;
typedef CAN_FIFOMailBox_TypeDef stm32_can_rx_mailbox_t;

typedef enum {
    STM32_CAN_LOOPBACK_DISABLED = 0,
    STM32_CAN_LOOPBACK_ENABLED = CAN_BTR_LBKM,
} stm32_can_loopback_t;

#define STM32_CAN_BTR_BRP(x) (((x) - 1) & CAN_BTR_BRP)
#define STM32_CAN_BTR_TS1(x) ((((x) - 1) & 0xf) << 16)
#define STM32_CAN_BTR_TS2(x) ((((x) - 1) & 0x7) << 20)
#define STM32_CAN_BTR_SJW(x) ((((x) - 1) & 0x3) << 24)

static cbuf_t can_rx_buf;
static mutex_t can_tx_mutex;

void stm32_CEC_CAN_IRQ(void)
{
    arm_cm_irq_entry();
    bool resched = false;
    stm32_can_t *can = CAN;

    while (can->RF0R & CAN_RF0R_FMP0) {
        // If there's no space left in the rx buffer, disable the RX interrupt.
        if (cbuf_space_avail(&can_rx_buf) < sizeof(can_msg_t)) {
            can->IER &= ~CAN_IER_FMPIE0;
            break;
        }
        can_msg_t msg;
        stm32_can_rx_mailbox_t *mailbox = &can->sFIFOMailBox[0];

        uint32_t rir = mailbox->RIR;
        msg.ide = !!(rir & CAN_RI0R_IDE);
        msg.rtr = !!(rir & CAN_RI0R_RTR);
        msg.id = (rir >> 21) & ((1 << 11) - 1);
        if (msg.ide) {
            // Extended IDs untested.
            msg.id_ex = (rir >> 3) & ((1 << 18) - 1);
        }

        msg.dlc = mailbox->RDTR & CAN_RDT0R_DLC;

        uint32_t data;

        data = mailbox->RDLR;
        msg.data[0] = data & 0xff;
        msg.data[1] = (data >> 8) & 0xff;
        msg.data[2] = (data >> 16) & 0xff;
        msg.data[3] = (data >> 24) & 0xff;

        data = mailbox->RDHR;
        msg.data[4] = data & 0xff;
        msg.data[5] = (data >> 8) & 0xff;
        msg.data[6] = (data >> 16) & 0xff;
        msg.data[7] = (data >> 24) & 0xff;

        can->RF0R |= CAN_RF0R_RFOM0;

        cbuf_write(&can_rx_buf, &msg, sizeof(msg), false);
        resched = true;
    }

    arm_cm_irq_exit(resched);
}

stm32_can_tx_mailbox_t *stm32_can_select_empty_mailbox(stm32_can_t *can) {
    uint32_t tsr = can->TSR;

    if (tsr & CAN_TSR_TME0) {
        return &can->sTxMailBox[0];
    } else if (tsr & CAN_TSR_TME1) {
        return &can->sTxMailBox[1];
    } else if (tsr & CAN_TSR_TME2) {
        return &can->sTxMailBox[2];
    } else {
        return NULL;
    }
}

int stm32_can_transmit(stm32_can_t *can, const can_msg_t *msg) {
    stm32_can_tx_mailbox_t *mailbox = stm32_can_select_empty_mailbox(can);
    if (mailbox == NULL) {
        return -EWOULDBLOCK;
    }

    /* Set up the Id */
    if (msg->ide) {
        // Extended IDs untested.
        mailbox->TIR = (msg->id << 21) | (msg->id_ex << 3) | CAN_TI0R_IDE
            | (msg->rtr ? CAN_TI0R_RTR : 0);
    } else {
        mailbox->TIR = (msg->id << 21) | (msg->rtr ? CAN_TI0R_RTR : 0);
    }

    /* Set up the DLC */
    mailbox->TDTR &= ~CAN_TDT0R_DLC;
    mailbox->TDTR |= msg->dlc & CAN_TDT0R_DLC;

    /* Set up the data field */
    mailbox->TDLR = msg->data[3] << 24 | msg->data[2] << 16
        | msg->data[1] << 8 | msg->data[0];
    mailbox->TDHR = msg->data[7] << 24 | msg->data[6] << 16
        | msg->data[5] << 8 | msg->data[4];

    mailbox->TIR |= CAN_TI0R_TXRQ;
    return 0;
}

void stm32_can_filter_set_mask32(uint32_t filter, uint32_t id, uint32_t mask) {
    DEBUG_ASSERT(filter <= 27);

    stm32_can_t *can = CAN;
    uint32_t filter_mask = 1 << filter;

    // Enter filter init mode.
    can->FMR |= CAN_FMR_FINIT;

    // Disable filter.
    can->FA1R &= ~filter_mask;

    // Set 32bit scale mode.
    can->FS1R |= filter_mask;

    can->sFilterRegister[filter].FR1 = id;
    can->sFilterRegister[filter].FR2 = mask;

    // Set ID & Mask mode.
    can->FM1R &= ~filter_mask;

    // We only support TX FIFO 0.
    can->FFA1R &= ~filter_mask;

    // Enable filter.
    can->FA1R |= filter_mask;

    // Exit filter init mode.
    can->FMR &= ~CAN_FMR_FINIT;
}

void can_init(bool loopback) {
    stm32_can_t *can = CAN;
    // initialize the RX cbuf with enough room for 4 can frames
    cbuf_initialize(&can_rx_buf, round_up_pow2_u32(sizeof(can_msg_t) * 4));

    mutex_init(&can_tx_mutex);

    // Enable CAN peripheral clock.
    stm32_rcc_set_enable(STM32_RCC_CLK_CAN, true);

    // Put CAN into init mode.
    can->MCR = CAN_MCR_INRQ;
    while (!(can->MSR & CAN_MSR_INAK)) {}

    // CAN Baudrate = 125kbps (CAN clocked at 36 MHz)
    // XXX: this is probably wrong running at 48MHz
    can->BTR =
        STM32_CAN_BTR_BRP(16) |
        STM32_CAN_BTR_TS1(9) |
        STM32_CAN_BTR_TS2(8) |
        STM32_CAN_BTR_SJW(1) |
        (loopback ? STM32_CAN_LOOPBACK_ENABLED : STM32_CAN_LOOPBACK_DISABLED);

    // Take CAN out of init mode.
    can->MCR &= ~CAN_MCR_INRQ;
    while (can->MSR & CAN_MSR_INAK) {}

    stm32_can_filter_set_mask32(0, 0x0, 0x0);

    // Enable FIFO 0 message pending interrupt
    can->IER |= CAN_IER_FMPIE0;
    NVIC_EnableIRQ(CEC_CAN_IRQn);
}

ssize_t can_send(const can_msg_t *msg)
{
    stm32_can_t *can = CAN;
    ssize_t ret;

    mutex_acquire(&can_tx_mutex);
    ret = stm32_can_transmit(can, msg);
    mutex_release(&can_tx_mutex);

    return ret;
}

ssize_t can_recv(can_msg_t *msg, bool block)
{
    stm32_can_t *can = CAN;
    size_t bytes_read;

    bytes_read = cbuf_read(&can_rx_buf, msg, sizeof(*msg), block);
    if (cbuf_space_avail(&can_rx_buf) >= sizeof(*msg)) {
        can->IER |= CAN_IER_FMPIE0;
    }

    return bytes_read > 0 ? msg->dlc : -EWOULDBLOCK;
}
