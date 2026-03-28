/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <hardware/regs/addressmap.h>
#include <hardware/regs/usb.h>
#include <hardware/structs/usb.h>
#include <hardware/structs/usb_dpram.h>
#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform/rp23xx.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#define NUM_EP      16
#define USBCTRL_IRQ 14 // From irqinfo.h: RP23XX_IRQ(USBCTRL_IRQ,14)

// USB DPRAM access from included headers
// #define usb_dpram ((usb_device_dpram_t *)USBCTRL_DPRAM_BASE)

static inline volatile uint32_t *usbc_reg_addr(uint32_t offset) {
    return (volatile uint32_t *)((volatile uint8_t *)usb_hw + offset);
}

static inline uint32_t usbc_reg_read(uint32_t offset) {
    return mmio_read32(usbc_reg_addr(offset));
}

static inline void usbc_reg_write(uint32_t offset, uint32_t value) {
    mmio_write32(usbc_reg_addr(offset), value);
}

static inline void usbc_reg_rmw(uint32_t offset, uint32_t clear_mask, uint32_t set_mask) {
    uint32_t value = usbc_reg_read(offset);
    value &= ~clear_mask;
    value |= set_mask;
    usbc_reg_write(offset, value);
}

struct ep_status {
    usbc_transfer_t *transfer;
    uint8_t *dpram_buf; // Pointer to DPRAM buffer
    uint16_t dpram_len; // Maximum length available in DPRAM
    bool in_progress;
};

static struct {
    bool do_resched;
    bool highspeed;
    bool connected;
    bool suspended;
    bool pending_set_address_valid;
    uint8_t pending_set_address;

    struct ep_status ep_in[NUM_EP];
    struct ep_status ep_out[NUM_EP];

    uint32_t dpram_offset; // Current offset in epx_data for buffer allocation
} usbc;

static void usbc_reset(void);
static void usbc_setup_endpoint_internal(ep_t ep, ep_dir_t dir, uint width, ep_type_t type);
static uint8_t *usbc_dpram_alloc(uint16_t len);
static void usbc_int_handler_impl(void);

static uint32_t usbc_sie_ctrl_value(bool pullup_en) {
    uint32_t v = 0;
    // Generate BUFF_STATUS events for each EP0 buffer completion.
    v |= USB_SIE_CTRL_EP0_INT_1BUF_BITS;
    if (pullup_en) {
        v |= USB_SIE_CTRL_PULLUP_EN_BITS;
    }
    return v;
}

static void usbc_cancel_all_transfers(status_t result) {
    for (uint i = 0; i < NUM_EP; i++) {
        if (usbc.ep_in[i].transfer) {
            usbc_transfer_t *t = usbc.ep_in[i].transfer;
            usbc.ep_in[i].transfer = NULL;
            usbc.ep_in[i].in_progress = false;
            t->result = result;
            if (t->callback) {
                t->callback(i, t);
            }
        }
        if (usbc.ep_out[i].transfer) {
            usbc_transfer_t *t = usbc.ep_out[i].transfer;
            usbc.ep_out[i].transfer = NULL;
            usbc.ep_out[i].in_progress = false;
            t->result = result;
            if (t->callback) {
                t->callback(i, t);
            }
        }
    }
}

// Allocate buffer space from DPRAM
static uint8_t *usbc_dpram_alloc(uint16_t len) {
    uint32_t avail = sizeof(usb_dpram->epx_data) - usbc.dpram_offset;
    if (len > avail) {
        return NULL;
    }
    uint8_t *buf = &usb_dpram->epx_data[usbc.dpram_offset];
    usbc.dpram_offset += len;
    return buf;
}

void rp23xx_usbc_early_init(void) {
    LTRACE_ENTRY;
    // Enable USB clock and PHY
    // This would typically be done in clock initialization
    // For now just initialize the DPRAM offset
    usbc.dpram_offset = 0;
}

void rp23xx_usbc_init(void) {
    LTRACE_ENTRY;

    memset(&usbc, 0, sizeof(usbc));
    usbc.dpram_offset = 0;

    // Clear and reset the controller
    usbc_reset();

    // Configure for device mode - clear HOST_NDEVICE in main ctrl
    usbc_reg_rmw(USB_MAIN_CTRL_OFFSET, USB_MAIN_CTRL_HOST_NDEVICE_BITS, 0);

    // Configure the SIE controller
    // Force an explicit device-mode-safe SIE control value.
    usbc_reg_write(USB_SIE_CTRL_OFFSET, usbc_sie_ctrl_value(false));

    // Route controller to internal PHY and allow software connect control.
    usbc_reg_write(USB_USB_MUXING_OFFSET,
                   USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS);

    // In self-powered/device bringup, force VBUS detect true so device logic can run
    // even when external VBUS detect wiring is not present.
    usbc_reg_write(USB_USB_PWR_OFFSET,
                   USB_USB_PWR_VBUS_DETECT_BITS |
                       USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS |
                       USB_USB_PWR_VBUS_EN_BITS |
                       USB_USB_PWR_VBUS_EN_OVERRIDE_EN_BITS);

    // Set up EP0 control endpoint
    usbc_setup_endpoint_internal(0, USB_OUT, 0x40, USB_CTRL);
    usbc_setup_endpoint_internal(0, USB_IN, 0x40, USB_CTRL);

    // Enable USB interrupts
    NVIC_SetPriority(USBCTRL_IRQ, 2);
    NVIC_ClearPendingIRQ(USBCTRL_IRQ);
    NVIC_EnableIRQ(USBCTRL_IRQ);

    // Enable interrupt sources
    uint32_t inte = USB_INTE_BUFF_STATUS_BITS | // Buffer complete
                    USB_INTE_BUS_RESET_BITS |   // Bus reset
                    USB_INTE_SETUP_REQ_BITS |   // Setup packet received
                    USB_INTE_DEV_RESUME_FROM_HOST_BITS |
                    USB_INTE_DEV_SUSPEND_BITS;
    usbc_reg_write(USB_INTE_OFFSET, inte);

    // Enable the USB controller
    usbc_reg_write(USB_MAIN_CTRL_OFFSET, USB_MAIN_CTRL_CONTROLLER_EN_BITS);

    // Start disconnected; usb_start() drives usbc_set_active(true)
    usbc.connected = false;

    uint32_t sie_status = usbc_reg_read(USB_SIE_STATUS_OFFSET);
    LTRACEF("rp23xx init done (pullup=off, irq=%u, sie_ctrl=0x%08x, sie_status=0x%08x)\n",
            USBCTRL_IRQ, usbc_reg_read(USB_SIE_CTRL_OFFSET), sie_status);
}

static void usbc_reset(void) {
    LTRACE_ENTRY;

    // Disable controller
    usbc_reg_write(USB_MAIN_CTRL_OFFSET, 0);

    // Re-enable controller
    usbc_reg_write(USB_MAIN_CTRL_OFFSET, USB_MAIN_CTRL_CONTROLLER_EN_BITS);

    // Clear all latched SIE status sources (W1C bits).
    usbc_reg_write(USB_SIE_STATUS_OFFSET,
                   USB_SIE_STATUS_BUS_RESET_BITS |
                       USB_SIE_STATUS_SETUP_REC_BITS |
                       USB_SIE_STATUS_SUSPENDED_BITS |
                       USB_SIE_STATUS_RESUME_BITS);

    // Clear all endpoint state
    memset(usbc.ep_in, 0, sizeof(usbc.ep_in));
    memset(usbc.ep_out, 0, sizeof(usbc.ep_out));

    usbc.dpram_offset = 0;
}

// Interrupt handler - called by the vector table
void USBCTRL_IRQ_IRQHandler(void) {
    arm_cm_irq_entry();
    usbc_int_handler_impl();
    arm_cm_irq_exit(usbc.do_resched);
}

static void usbc_int_handler_impl(void) {
    uint32_t ints = usbc_reg_read(USB_INTS_OFFSET);
    uint32_t sie_ack = 0;

    uint32_t noisy_ints = USB_INTS_DEV_SOF_BITS | USB_INTS_DEV_SUSPEND_BITS;
    uint32_t trace_ints = ints & ~noisy_ints;
    if (trace_ints) {
        LTRACEF("ints: 0x%x\n", ints);
    }

    if (ints & USB_INTS_SETUP_REQ_BITS) {
        // Setup packet has been written to DPRAM setup_packet
        union usb_callback_args args;
        args.setup = (struct usb_setup *)usb_dpram->setup_packet;
        LTRACEF("setup bmReq=0x%02x bReq=0x%02x wValue=0x%04x wIndex=0x%04x wLen=%u\n",
                args.setup->request_type, args.setup->request,
                args.setup->value, args.setup->index, args.setup->length);
        usbc_callback(USB_CB_SETUP_MSG, &args);
        usbc.do_resched = true;
        sie_ack |= USB_SIE_STATUS_SETUP_REC_BITS;
    }

    if (ints & USB_INTS_BUS_RESET_BITS) {
        LTRACEF("bus reset\n");
        // Do not hard-reset the whole controller here; it can cause repeated
        // disconnect/reconnect loops. Re-arm EP0 and drop outstanding transfers.
        usbc_cancel_all_transfers(ERR_CANCELLED);
        usbc.suspended = false;
        usbc.pending_set_address_valid = false;
        usbc_setup_endpoint_internal(0, USB_OUT, 0x40, USB_CTRL);
        usbc_setup_endpoint_internal(0, USB_IN, 0x40, USB_CTRL);
        usbc_callback(USB_CB_RESET, NULL);
        usbc.do_resched = true;
        sie_ack |= USB_SIE_STATUS_BUS_RESET_BITS;
    }

    if (ints & USB_INTS_BUFF_STATUS_BITS) {
        // Check which endpoints have completed transfers
        uint32_t buff_status = usbc_reg_read(USB_BUFF_STATUS_OFFSET);

        // Process each endpoint. Bit layout is EPn_IN on even bits,
        // EPn_OUT on odd bits.
        for (uint i = 0; i < NUM_EP; i++) {
            // Check OUT endpoint
            if (buff_status & (1u << (i * 2 + 1))) {
                if (usbc.ep_out[i].transfer) {
                    usbc_transfer_t *t = usbc.ep_out[i].transfer;
                    usbc.ep_out[i].transfer = NULL;
                    usbc.ep_out[i].in_progress = false;

                    // Get bytes transferred from buffer control register
                    uint32_t buf_ctrl = usb_dpram->ep_buf_ctrl[i].out;
                    uint16_t len = buf_ctrl & USB_BUF_CTRL_LEN_MASK;
                    t->bufpos = len;
                    t->result = 0;
                    if (t->callback) {
                        t->callback(i, t);
                    }
                    usbc.do_resched = true;
                }
            }

            // Check IN endpoint
            if (buff_status & (1u << (i * 2))) {
                // EP0 IN complete means status/data stage finished; re-arm EP0 OUT
                // so we can receive next setup/status packet.
                if (i == 0) {
                    usb_dpram->ep_buf_ctrl[0].out =
                        USB_BUF_CTRL_AVAIL | (0x40 & USB_BUF_CTRL_LEN_MASK);
                }

                // Apply deferred address once EP0 status IN has completed.
                if (i == 0 && usbc.pending_set_address_valid) {
                    usbc_reg_rmw(USB_ADDR_ENDP_OFFSET,
                                 USB_ADDR_ENDP_ADDRESS_BITS,
                                 usbc.pending_set_address & 0x7f);
                    LTRACEF("applied deferred address %u\n", usbc.pending_set_address);
                    usbc.pending_set_address_valid = false;
                }

                if (usbc.ep_in[i].transfer) {
                    usbc_transfer_t *t = usbc.ep_in[i].transfer;
                    usbc.ep_in[i].transfer = NULL;
                    usbc.ep_in[i].in_progress = false;

                    // Get bytes transferred
                    uint32_t buf_ctrl = usb_dpram->ep_buf_ctrl[i].in;
                    uint16_t len = buf_ctrl & USB_BUF_CTRL_LEN_MASK;
                    t->bufpos = len;
                    t->result = 0;
                    if (t->callback) {
                        t->callback(i, t);
                    }
                    usbc.do_resched = true;
                }
            }
        }

        // Ack processed endpoint-complete bits.
        usbc_reg_write(USB_BUFF_STATUS_OFFSET, buff_status);
    }

    if (ints & USB_INTS_DEV_SUSPEND_BITS) {
        if (!usbc.suspended) {
            LTRACEF("suspend\n");
            usbc_callback(USB_CB_SUSPEND, NULL);
            usbc.do_resched = true;
            usbc.suspended = true;
        }
        sie_ack |= USB_SIE_STATUS_SUSPENDED_BITS;
    }

    if (ints & USB_INTS_DEV_RESUME_FROM_HOST_BITS) {
        if (usbc.suspended) {
            LTRACEF("resume\n");
            usbc_callback(USB_CB_RESUME, NULL);
            usbc.do_resched = true;
            usbc.suspended = false;
        }
        sie_ack |= USB_SIE_STATUS_RESUME_BITS;
    }

    // Clear processed SIE-latched interrupt sources.
    if (sie_ack) {
        usbc_reg_write(USB_SIE_STATUS_OFFSET, sie_ack);
    }
}

status_t usbc_setup_endpoint(ep_t ep, ep_dir_t dir, uint width, ep_type_t type) {

    if (ep >= NUM_EP) {
        return ERR_INVALID_ARGS;
    }

    // EP0 has fixed buffers
    if (ep == 0) {
        if (dir == USB_OUT) {
            usbc.ep_out[0].dpram_buf = usb_dpram->ep0_buf_a;
            usbc.ep_out[0].dpram_len = sizeof(usb_dpram->ep0_buf_a);
        } else {
            usbc.ep_in[0].dpram_buf = usb_dpram->ep0_buf_a;
            usbc.ep_in[0].dpram_len = sizeof(usb_dpram->ep0_buf_a);
        }
        usbc_setup_endpoint_internal(ep, dir, width, type);
        return NO_ERROR;
    }

    // For other endpoints, allocate from DPRAM
    uint8_t *buf = usbc_dpram_alloc(width);
    if (!buf) {
        LTRACEF("Failed to allocate DPRAM for endpoint\n");
        return ERR_NO_MEMORY;
    }

    if (dir == USB_OUT) {
        usbc.ep_out[ep].dpram_buf = buf;
        usbc.ep_out[ep].dpram_len = width;
    } else {
        usbc.ep_in[ep].dpram_buf = buf;
        usbc.ep_in[ep].dpram_len = width;
    }

    usbc_setup_endpoint_internal(ep, dir, width, type);

    return NO_ERROR;
}

static void usbc_setup_endpoint_internal(ep_t ep, ep_dir_t dir, uint width, ep_type_t type) {
    LTRACEF("Setting up endpoint %u, dir %u, width %u, type %u\n", ep, dir, width, type);

    // For EP0, enable both directions
    if (ep == 0) {
        uint32_t ctrl = usb_dpram->ep_ctrl[0].out;
        ctrl |= EP_CTRL_ENABLE_BITS;
        usb_dpram->ep_ctrl[0].out = ctrl;

        ctrl = usb_dpram->ep_ctrl[0].in;
        ctrl |= EP_CTRL_ENABLE_BITS;
        usb_dpram->ep_ctrl[0].in = ctrl;

        // Prime EP0 OUT buffer for setup/data packets.
        usb_dpram->ep_buf_ctrl[0].out = USB_BUF_CTRL_AVAIL | (0x40 & USB_BUF_CTRL_LEN_MASK);
        return;
    }

    // For other endpoints, set up the control register
    if (ep > 0) {
        if (dir == USB_OUT) {
            uint32_t ctrl = usb_dpram->ep_ctrl[ep - 1].out;
            ctrl = (ctrl & ~0x3f) | (type & 0x3f); // Set type bits
            ctrl |= EP_CTRL_ENABLE_BITS;
            usb_dpram->ep_ctrl[ep - 1].out = ctrl;
        } else {
            uint32_t ctrl = usb_dpram->ep_ctrl[ep - 1].in;
            ctrl = (ctrl & ~0x3f) | (type & 0x3f); // Set type bits
            ctrl |= EP_CTRL_ENABLE_BITS;
            usb_dpram->ep_ctrl[ep - 1].in = ctrl;
        }
    }

    // Initialize endpoint state
    if (dir == USB_OUT) {
        usbc.ep_out[ep].in_progress = false;
    } else {
        usbc.ep_in[ep].in_progress = false;
    }
}

status_t usbc_queue_rx(ep_t ep, usbc_transfer_t *transfer) {

    if (ep >= NUM_EP) {
        return ERR_INVALID_ARGS;
    }

    // Check if endpoint is already in use
    if (usbc.ep_out[ep].in_progress) {
        return ERR_BUSY;
    }

    // Copy setup to our buffer
    if (!usbc.ep_out[ep].dpram_buf) {
        return ERR_NOT_CONFIGURED;
    }

    usbc.ep_out[ep].transfer = transfer;
    usbc.ep_out[ep].in_progress = true;

    // Set up buffer control register to indicate buffer is available
    uint32_t buf_ctrl = USB_BUF_CTRL_AVAIL | (MIN(transfer->buflen, usbc.ep_out[ep].dpram_len) & USB_BUF_CTRL_LEN_MASK);
    usb_dpram->ep_buf_ctrl[ep].out = buf_ctrl;

    return NO_ERROR;
}

status_t usbc_queue_tx(ep_t ep, usbc_transfer_t *transfer) {

    if (ep >= NUM_EP) {
        return ERR_INVALID_ARGS;
    }

    // Check if endpoint is already in use
    if (usbc.ep_in[ep].in_progress) {
        return ERR_BUSY;
    }

    if (!usbc.ep_in[ep].dpram_buf) {
        return ERR_NOT_CONFIGURED;
    }

    usbc.ep_in[ep].transfer = transfer;
    usbc.ep_in[ep].in_progress = true;

    // Copy data from user buffer to DPRAM
    uint16_t len = MIN(transfer->buflen, usbc.ep_in[ep].dpram_len);
    if (len > 0 && transfer->buf) {
        memcpy(usbc.ep_in[ep].dpram_buf, transfer->buf, len);
    }

    // Set up buffer control register to indicate buffer is full (has data to send)
    uint32_t buf_ctrl = USB_BUF_CTRL_FULL | USB_BUF_CTRL_AVAIL | (len & USB_BUF_CTRL_LEN_MASK);
    // Toggle DATA0/DATA1 based on endpoint
    buf_ctrl |= (ep % 2 == 0) ? USB_BUF_CTRL_DATA0_PID : USB_BUF_CTRL_DATA1_PID;
    usb_dpram->ep_buf_ctrl[ep].in = buf_ctrl;

    return NO_ERROR;
}

status_t usbc_flush_ep(ep_t ep) {

    if (ep >= NUM_EP) {
        return ERR_INVALID_ARGS;
    }

    // Clear endpoint state
    usbc.ep_in[ep].in_progress = false;
    usbc.ep_out[ep].in_progress = false;
    usbc.ep_in[ep].transfer = NULL;
    usbc.ep_out[ep].transfer = NULL;

    return NO_ERROR;
}

status_t usbc_set_active(bool active) {
    LTRACEF("set_active(%u)\n", active ? 1 : 0);

    if (active) {
        // Ensure a clean attach edge: disconnect briefly, then reconnect.
        usbc_reg_write(USB_SIE_CTRL_OFFSET, usbc_sie_ctrl_value(false));
        for (volatile uint i = 0; i < 200000; ++i) {
            __asm__ volatile("nop");
        }
        usbc_reg_write(USB_SIE_CTRL_OFFSET, usbc_sie_ctrl_value(true));

        // Force DP pull-up through PHY-direct override as a bring-up fallback.
        usbc_reg_write(USB_USBPHY_DIRECT_OFFSET, USB_USBPHY_DIRECT_DP_PULLUP_EN_BITS);
        usbc_reg_write(USB_USBPHY_DIRECT_OVERRIDE_OFFSET,
                       USB_USBPHY_DIRECT_OVERRIDE_DP_PULLUP_EN_OVERRIDE_EN_BITS);

        usbc.connected = true;
        uint32_t sie_status = usbc_reg_read(USB_SIE_STATUS_OFFSET);
        LTRACEF("pullup enabled, sie=0x%08x connected=%u vbus=%u linestate=%u\n",
                sie_status,
                (sie_status & USB_SIE_STATUS_CONNECTED_BITS) ? 1 : 0,
                (sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS) ? 1 : 0,
                (uint)((sie_status & USB_SIE_STATUS_LINE_STATE_BITS) >> USB_SIE_STATUS_LINE_STATE_LSB));

        // Give PHY/connect logic a short moment and sample again.
        for (volatile uint i = 0; i < 100000; ++i) {
            __asm__ volatile("nop");
        }
        sie_status = usbc_reg_read(USB_SIE_STATUS_OFFSET);
        LTRACEF("post-attach sie=0x%08x connected=%u vbus=%u linestate=%u\n",
                sie_status,
                (sie_status & USB_SIE_STATUS_CONNECTED_BITS) ? 1 : 0,
                (sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS) ? 1 : 0,
                (uint)((sie_status & USB_SIE_STATUS_LINE_STATE_BITS) >> USB_SIE_STATUS_LINE_STATE_LSB));
    } else {
        // Disable pull-up to disconnect from host
        usbc_reg_write(USB_USBPHY_DIRECT_OVERRIDE_OFFSET, 0);
        usbc_reg_write(USB_USBPHY_DIRECT_OFFSET, 0);

        usbc_reg_write(USB_SIE_CTRL_OFFSET, usbc_sie_ctrl_value(false));
        usbc.connected = false;
        LTRACEF("pullup disabled\n");
    }

    return NO_ERROR;
}

void usbc_set_address(uint8_t address) {
    LTRACEF("set_address(%u) deferred\n", address);

    // USB requires address to take effect after status stage completion.
    usbc.pending_set_address = address & 0x7f;
    usbc.pending_set_address_valid = true;
}

void usbc_ep0_ack(void) {
    LTRACE_ENTRY;
    // Send ZLP (Zero Length Packet) on EP0 IN
    usb_dpram->ep_buf_ctrl[0].in = USB_BUF_CTRL_FULL | USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_DATA1_PID;
}

void usbc_ep0_stall(void) {
    LTRACE_ENTRY;
    // Stall both IN and OUT on EP0
    uint32_t buf_ctrl = usb_dpram->ep_buf_ctrl[0].out;
    buf_ctrl |= USB_BUF_CTRL_STALL;
    usb_dpram->ep_buf_ctrl[0].out = buf_ctrl;

    buf_ctrl = usb_dpram->ep_buf_ctrl[0].in;
    buf_ctrl |= USB_BUF_CTRL_STALL;
    usb_dpram->ep_buf_ctrl[0].in = buf_ctrl;
}

void usbc_ep0_send(const void *buf, size_t len, size_t maxlen) {
    LTRACEF("buf %p, len %zd, maxlen %zd\n", buf, len, maxlen);

    // Copy data to EP0 IN buffer in DPRAM
    len = MIN(len, maxlen);
    len = MIN(len, sizeof(usb_dpram->ep0_buf_a));

    if (len > 0 && buf) {
        memcpy(usb_dpram->ep0_buf_a, buf, len);
    }

    // Mark buffer as full and ready to send
    uint32_t buf_ctrl = USB_BUF_CTRL_FULL | USB_BUF_CTRL_AVAIL |
                        USB_BUF_CTRL_DATA1_PID | (len & USB_BUF_CTRL_LEN_MASK);
    usb_dpram->ep_buf_ctrl[0].in = buf_ctrl;
}

void usbc_ep0_recv(void *buf, size_t len, ep_callback cb) {
    LTRACEF("buf %p, len %zd\n", buf, len);

    // Set up EP0 OUT for receiving
    len = MIN(len, sizeof(usb_dpram->ep0_buf_b));

    // Create a transfer
    usbc_transfer_t *transfer = malloc(sizeof(usbc_transfer_t));
    if (transfer) {
        transfer->buf = buf;
        transfer->buflen = len;
        transfer->callback = cb;
        transfer->result = 0;
        transfer->bufpos = 0;
        transfer->extra = NULL;

        usbc_queue_rx(0, transfer);
    }
}

bool usbc_is_highspeed(void) {
    return usbc.highspeed;
}

void usbc_init(void) {
    LTRACE_ENTRY;

    // Initialize USB controller
    rp23xx_usbc_init();
}