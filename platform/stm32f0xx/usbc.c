/*
 * Copyright (c) 2015 Travis Geiselbrecht
 * Copyright (c) 2015 Erik Gilling
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <reg.h>
#include <trace.h>
#include <debug.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <err.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <arch/arm/cm.h>
#include <platform/rcc.h>
#include <platform/stm32.h>
#include <platform/usbc.h>

#include <stm32f0xx_hal_pcd.h>

#define LOCAL_TRACE 0

#define NUM_EP 5

// This driver is a shim between the LK usbc api and the STMico Cube api.
// Ideally this would be a full native driver.  Given how long it took to
// debug this one, I don't have the patience to make it native.

struct ep_status {
    bool ack_ep0_in;
    usbc_transfer_t *transfer;
};

static struct {
    bool do_resched;

    struct ep_status ep_in[NUM_EP];
    struct ep_status ep_out[NUM_EP];

    uint32_t pma_highwater;

    PCD_HandleTypeDef handle;
} usbc;

uint32_t stm32_usbc_pma_alloc(uint32_t size) {
    // TODO(konkers): Fail on OOM
    uint32_t addr = usbc.pma_highwater;
    usbc.pma_highwater += size;
    return addr;
}

void stm32_usbc_early_init(stm32_usb_clk_t clock_source)
{
    // TODO(konkers): add usb clock source to rcc.
#if 0
    if (clock_source == STM32_USB_CLK_HSI48) {
        RCC_USBCLKConfig(RCC_USBCLK_HSI48);
    } else {
        RCC_USBCLKConfig(RCC_USBCLK_PLLCLK);
    }
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
#endif
}

void stm32_usbc_init(void)
{
    LTRACE_ENTRY;

    usbc.pma_highwater = 0x40;

    // Set LL Driver parameters
    usbc.handle.Instance = USB;
    usbc.handle.Init.dev_endpoints = 4;
    usbc.handle.Init.ep0_mps = 0x40;
    usbc.handle.Init.phy_itface = PCD_PHY_EMBEDDED;
    usbc.handle.Init.speed = PCD_SPEED_FULL;
    usbc.handle.Init.low_power_enable = 0;
    usbc.handle.Init.lpm_enable = 0;
    usbc.handle.Init.battery_charging_enable = 0;

    // Initialize LL Driver
    HAL_PCD_Init(&usbc.handle);

    HAL_PCDEx_PMAConfig(&usbc.handle, 0x00, PCD_SNG_BUF,
                        stm32_usbc_pma_alloc(0x40));
    HAL_PCDEx_PMAConfig(&usbc.handle, 0x80, PCD_SNG_BUF,
                        stm32_usbc_pma_alloc(0x40));

}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    LTRACE_ENTRY;
    // Enable USB FS Clock
    stm32_rcc_set_enable(STM32_RCC_CLK_USB, true);
    NVIC_EnableIRQ(USB_IRQn);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    LTRACEF("epnum %u\n", epnum);

    if (epnum == 0) {
        usbc_ep0_ack();
    } else if (usbc.ep_out[epnum].transfer) {
        // completing a transfer
        usbc_transfer_t *t = usbc.ep_out[epnum].transfer;
        usbc.ep_out[epnum].transfer = NULL;

        LTRACEF("completing transfer %p\n", t);

        PCD_EPTypeDef *ep = &hpcd->OUT_ep[epnum];
        t->bufpos = ep->xfer_count;
        t->result = 0;
        t->callback(epnum, t);
        usbc.do_resched = true;
    }
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    PCD_EPTypeDef *ep = &hpcd->IN_ep[epnum];
    LTRACEF("epnum %u, xfer count %u len %u\n", epnum, ep->xfer_count, ep->xfer_len);

    if (epnum == 0) {
        // TODO(konkers): implement multi packet.
        struct ep_status *ep = &usbc.ep_in[0];
        if (ep->ack_ep0_in) {
            // in transfer done, ready for receive status
            HAL_PCD_EP_Receive(&usbc.handle, 0, 0, 0);
        }
    } else {
        // in transfer done
        if (usbc.ep_in[epnum].transfer) {
            // completing a transfer
            usbc_transfer_t *t = usbc.ep_in[epnum].transfer;
            usbc.ep_in[epnum].transfer = NULL;

            LTRACEF("completing transfer %p\n", t);

            PCD_EPTypeDef *ep = &hpcd->IN_ep[epnum];
            t->bufpos = ep->xfer_count;
            t->result = 0;
            t->callback(epnum, t);
            usbc.do_resched = true;
        }
    }
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    //LTRACE_ENTRY;

    union usb_callback_args args;
    args.setup = (struct usb_setup *)hpcd->Setup;

    usbc_callback(USB_CB_SETUP_MSG, &args);
    usbc.do_resched = true;
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    //LTRACE_ENTRY;
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    LTRACE_ENTRY;


    /* fail all the outstanding transactions */
    for (uint i = 0; i < NUM_EP; i++) {
        if (usbc.ep_in[i].transfer) {
            usbc_transfer_t *t = usbc.ep_in[i].transfer;
            usbc.ep_in[i].transfer = NULL;
            t->result = ERR_CANCELLED;
            t->callback(i, t);
        }
        if (usbc.ep_out[i].transfer) {
            usbc_transfer_t *t = usbc.ep_out[i].transfer;
            usbc.ep_out[i].transfer = NULL;
            t->result = ERR_CANCELLED;
            t->callback(i, t);
        }
    }

    HAL_PCD_EP_Open(&usbc.handle, 0, 0x40, PCD_EP_TYPE_CTRL);
    HAL_PCD_EP_Open(&usbc.handle, 0x80, 0x40, PCD_EP_TYPE_CTRL);

    usbc_callback(USB_CB_RESET, NULL);
    usbc.do_resched = true;
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    LTRACE_ENTRY;
    usbc_callback(USB_CB_SUSPEND, NULL);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    LTRACE_ENTRY;
    usbc_callback(USB_CB_RESUME, NULL);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    LTRACEF("epnum %u\n", epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    LTRACEF("epnum %u\n", epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    LTRACE_ENTRY;
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    LTRACE_ENTRY;
}

status_t usbc_set_active(bool active)
{
    LTRACEF("active %u\n", active);

    if (active) {
        HAL_PCD_Start(&usbc.handle);
    } else {
        HAL_PCD_Stop(&usbc.handle);
    }

    return NO_ERROR;
}

void usbc_set_address(uint8_t address)
{
    LTRACEF("address %u\n", address);
    HAL_PCD_SetAddress(&usbc.handle, address);
}

void usbc_ep0_ack(void)
{
    LTRACE;

    struct ep_status *ep = &usbc.ep_in[0];
    ep->ack_ep0_in = false;
    HAL_PCD_EP_Transmit(&usbc.handle, 0, 0, 0);
}

void usbc_ep0_stall(void)
{
    LTRACE;

    HAL_PCD_EP_SetStall(&usbc.handle, 0x80);
}

void usbc_ep0_send(const void *buf, size_t len, size_t maxlen)
{
    LTRACEF("buf %p, len %zu, maxlen %zu\n", buf, len, maxlen);

    struct ep_status *ep = &usbc.ep_in[0];
    ep->ack_ep0_in = true;
    HAL_PCD_EP_Transmit(&usbc.handle, 0, (void *)buf, MIN(len, maxlen));
}

void usbc_ep0_recv(void *buf, size_t len, ep_callback cb)
{
    PANIC_UNIMPLEMENTED;
}

status_t usbc_setup_endpoint(ep_t ep, ep_dir_t dir, uint width, ep_type_t type)
{
    LTRACEF("ep %u dir %u width %u\n", ep, dir, width);

    DEBUG_ASSERT(ep <= NUM_EP);
    // PCD_EP_TYPE* and USB_* have the same values.  Let's make sure that
    // doesn't change.
    DEBUG_ASSERT(PCD_EP_TYPE_CTRL == USB_CTRL);
    DEBUG_ASSERT(PCD_EP_TYPE_ISOC == USB_ISOC);
    DEBUG_ASSERT(PCD_EP_TYPE_BULK == USB_BULK);
    DEBUG_ASSERT(PCD_EP_TYPE_INTR == USB_INTR);

    uint8_t ep_addr = ep | ((dir == USB_IN) ? 0x80 : 0);

    HAL_PCDEx_PMAConfig(&usbc.handle, ep_addr, PCD_SNG_BUF,
                        stm32_usbc_pma_alloc(width));

    HAL_StatusTypeDef ret = HAL_PCD_EP_Open(&usbc.handle, ep_addr, width, type);

    return (ret == HAL_OK) ? NO_ERROR : ERR_GENERIC;
}

bool usbc_is_highspeed(void)
{
    return false;
}

status_t usbc_queue_rx(ep_t ep, usbc_transfer_t *transfer)
{
    LTRACEF("ep %u, transfer %p (buf %p, buflen %zu)\n", ep, transfer, transfer->buf, transfer->buflen);

    DEBUG_ASSERT(ep <= NUM_EP);
    DEBUG_ASSERT(usbc.ep_out[ep].transfer == NULL);

    usbc.ep_out[ep].transfer = transfer;
    HAL_PCD_EP_Receive(&usbc.handle, ep, transfer->buf, transfer->buflen);

    return NO_ERROR;
}

status_t usbc_queue_tx(ep_t ep, usbc_transfer_t *transfer)
{
    LTRACEF("ep %u, transfer %p (buf %p, buflen %zu)\n", ep, transfer, transfer->buf, transfer->buflen);

    DEBUG_ASSERT(ep <= NUM_EP);
    DEBUG_ASSERT(usbc.ep_in[ep].transfer == NULL);

    usbc.ep_in[ep].transfer = transfer;
    HAL_PCD_EP_Transmit(&usbc.handle, ep, transfer->buf, transfer->buflen);

    return NO_ERROR;
}

void stm32_USB_IRQ(void)
{
    arm_cm_irq_entry();
    //LTRACE_ENTRY;

    usbc.do_resched = false;
    HAL_PCD_IRQHandler(&usbc.handle);

    arm_cm_irq_exit(usbc.do_resched);
}
