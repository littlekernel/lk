/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
/*
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include <err.h>
#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <stdlib.h>
#include <string.h>
#include <lib/gfx.h>
#include <dev/gpio.h>
#include <dev/display.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <arch/ops.h>
#include <arch/arm/cm.h>
#include <platform.h>
#include <platform/stm32.h>
#include <platform/eth.h>

#if WITH_LIB_MINIP
#include <lib/minip.h>
#include <lib/pktbuf.h>
#endif

#define LOCAL_TRACE 0

/* LAN8742A PHY Address*/
#define LAN8742A_PHY_ADDRESS            0x00
/* DP83848 PHY Address*/
#define DP83848_PHY_ADDRESS             0x01
/* KSZ8721 PHY Address*/
#define KSZ8721_PHY_ADDRESS             0x01

struct eth_status {
    ETH_HandleTypeDef EthHandle;

    eth_phy_itf eth_phy;
    event_t rx_event;

    /* allocated directly out of DTCM below */
    ETH_DMADescTypeDef  *DMARxDscrTab;  // ETH_RXBUFNB
    ETH_DMADescTypeDef  *DMATxDscrTab;  // ETH_TXBUFNB
    uint8_t             *Rx_Buff;       // ETH_RXBUFNB * ETH_RX_BUF_SIZE
    uint8_t             *Tx_Buff;       // ETH_TXBUFNB * ETH_TX_BUF_SIZE
};

static struct eth_status eth;

static int eth_rx_worker(void *arg);

#if WITH_LIB_MINIP
static int eth_send_raw_pkt(pktbuf_t *p);
#endif

status_t eth_init(const uint8_t *mac_addr, eth_phy_itf eth_phy)
{
    LTRACE_ENTRY;

    DEBUG_ASSERT(mac_addr);

    eth.eth_phy = eth_phy;

    /* Enable ETHERNET clock  */
    __HAL_RCC_ETH_CLK_ENABLE();

    eth.EthHandle.Instance = ETH;
    eth.EthHandle.Init.MACAddr = (uint8_t *)mac_addr;
    eth.EthHandle.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    eth.EthHandle.Init.Speed = ETH_SPEED_100M;
    eth.EthHandle.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    switch (eth_phy) {
        case PHY_DP83848:
            eth.EthHandle.Init.MediaInterface = ETH_MEDIA_INTERFACE_MII;
            eth.EthHandle.Init.PhyAddress = DP83848_PHY_ADDRESS;
            break;
        case PHY_LAN8742A:
            eth.EthHandle.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
            eth.EthHandle.Init.PhyAddress = LAN8742A_PHY_ADDRESS;
            break;
        case PHY_KSZ8721:
            eth.EthHandle.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
            eth.EthHandle.Init.PhyAddress = KSZ8721_PHY_ADDRESS;
            break;
        default:
            return ERR_NOT_CONFIGURED;
    }

    eth.EthHandle.Init.RxMode = ETH_RXINTERRUPT_MODE;
    //eth.EthHandle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE; // XXX icmp checksums corrupted if stack stuff valid checksum
    eth.EthHandle.Init.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE;

    /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
    if (HAL_ETH_Init(&eth.EthHandle) != HAL_OK)
        return ERR_NOT_CONFIGURED;

    /* allocate descriptor and buffer memory from DTCM */
    /* XXX do in a more generic way */
#if MEMBASE == 0x20000000
#error DTCM will collide with MEMBASE
#endif
    addr_t tcm_ptr = RAMDTCM_BASE;

    eth.DMATxDscrTab = (void *)tcm_ptr;
    tcm_ptr += sizeof(*eth.DMATxDscrTab) * ETH_TXBUFNB;
    eth.DMARxDscrTab = (void *)tcm_ptr;
    tcm_ptr += sizeof(*eth.DMARxDscrTab) * ETH_RXBUFNB;

    eth.Tx_Buff = (void *)tcm_ptr;
    tcm_ptr += ETH_TX_BUF_SIZE * ETH_TXBUFNB;
    eth.Rx_Buff = (void *)tcm_ptr;
    tcm_ptr += ETH_RX_BUF_SIZE * ETH_RXBUFNB;

    /* Initialize Tx Descriptors list: Chain Mode */
    HAL_ETH_DMATxDescListInit(&eth.EthHandle, eth.DMATxDscrTab, eth.Tx_Buff, ETH_TXBUFNB);

    /* Initialize Rx Descriptors list: Chain Mode  */
    HAL_ETH_DMARxDescListInit(&eth.EthHandle, eth.DMARxDscrTab, eth.Rx_Buff, ETH_RXBUFNB);

    /* Enable MAC and DMA transmission and reception */
    HAL_ETH_Start(&eth.EthHandle);

#if 0
    // XXX DP83848 specific
    /**** Configure PHY to generate an interrupt when Eth Link state changes ****/
    /* Read Register Configuration */
    uint32_t regvalue;
    HAL_ETH_ReadPHYRegister(&eth.EthHandle, PHY_MICR, &regvalue);

    regvalue |= (PHY_MICR_INT_EN | PHY_MICR_INT_OE);

    /* Enable Interrupts */
    HAL_ETH_WritePHYRegister(&eth.EthHandle, PHY_MICR, regvalue );

    /* Read Register Configuration */
    HAL_ETH_ReadPHYRegister(&eth.EthHandle, PHY_MISR, &regvalue);

    regvalue |= PHY_MISR_LINK_INT_EN;

    /* Enable Interrupt on change of link status */
    HAL_ETH_WritePHYRegister(&eth.EthHandle, PHY_MISR, regvalue);
#endif

    /* set up an event to block the rx thread on */
    event_init(&eth.rx_event, false, EVENT_FLAG_AUTOUNSIGNAL);

    /* start worker thread */
    thread_resume(thread_create("eth_rx", &eth_rx_worker, NULL, HIGH_PRIORITY, DEFAULT_STACK_SIZE));

    /* enable interrupts */
    HAL_NVIC_EnableIRQ(ETH_IRQn);

    LTRACE_EXIT;

    return NO_ERROR;
}

void stm32_ETH_IRQ(void)
{
    arm_cm_irq_entry();

    HAL_ETH_IRQHandler(&eth.EthHandle);

    arm_cm_irq_exit(true);
}

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    event_signal(&eth.rx_event, false);
}

static status_t eth_send(const void *buf, size_t len)
{
    status_t err;
    __IO ETH_DMADescTypeDef *DmaTxDesc;

    LTRACEF("buf %p, len %zu\n", buf, len);

    DmaTxDesc = eth.EthHandle.TxDesc;

    /* is the buffer available? */
    if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != 0) {
        LTRACEF("tx buffer not available\n");
        err = ERR_IO;
        goto error;
    }

    uint8_t *buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);
    memcpy(buffer, buf, len);

    HAL_StatusTypeDef e = HAL_ETH_TransmitFrame(&eth.EthHandle, len);

    err = (e == HAL_OK) ? NO_ERROR : ERR_IO;

error:
    /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
    if ((eth.EthHandle.Instance->DMASR & ETH_DMASR_TUS) != 0) {
        /* Clear TUS ETHERNET DMA flag */
        eth.EthHandle.Instance->DMASR = ETH_DMASR_TUS;

        /* Resume DMA transmission*/
        eth.EthHandle.Instance->DMATPDR = 0;
    }

    return err;
}

static int eth_rx_worker(void *arg)
{
    for (;;) {
#if 0
        status_t event_err = event_wait_timeout(&eth.rx_event, 1000);
        if (event_err == ERR_TIMED_OUT) {
            /* periodically poll the phys status register */
            /* XXX specific to DP83848 */
            uint32_t val;

            /* Read PHY_MISR */
            /* seems to take about 30 usecs */
            HAL_ETH_ReadPHYRegister(&eth.EthHandle, PHY_MISR, &val);

            /* Check whether the link interrupt has occurred or not */
            if (val & PHY_LINK_INTERRUPT) {
                /* Read PHY_SR*/
                HAL_ETH_ReadPHYRegister(&eth.EthHandle, PHY_SR, &val);

                /* Check whether the link is up or down*/
                if (val & PHY_LINK_STATUS) {
                    printf("eth: link up\n");
                    //netif_set_link_up(link_arg->netif);
                } else {
                    printf("eth: link down\n");
                    //netif_set_link_down(link_arg->netif);
                }
            }
        } else {
#else
        status_t event_err = event_wait(&eth.rx_event);
        if (event_err >= NO_ERROR) {
#endif
            // XXX probably race with the event here
            while (HAL_ETH_GetReceivedFrame_IT(&eth.EthHandle) == HAL_OK) {
                LTRACEF("got packet len %u, buffer %p, seg count %u\n", eth.EthHandle.RxFrameInfos.length,
                        (void *)eth.EthHandle.RxFrameInfos.buffer,
                        eth.EthHandle.RxFrameInfos.SegCount);

#if WITH_LIB_MINIP
                /* allocate a pktbuf header, point it at our rx buffer, and pass up the stack */
                pktbuf_t *p = pktbuf_alloc_empty();
                if (p) {
                    pktbuf_add_buffer(p, (void *)eth.EthHandle.RxFrameInfos.buffer, eth.EthHandle.RxFrameInfos.length,
                                      0, 0, NULL, NULL);
                    p->dlen = eth.EthHandle.RxFrameInfos.length;

                    minip_rx_driver_callback(p);

                    pktbuf_free(p, true);
                }
#endif

                /* Release descriptors to DMA */
                /* Point to first descriptor */
                __IO ETH_DMADescTypeDef *dmarxdesc;

                dmarxdesc = eth.EthHandle.RxFrameInfos.FSRxDesc;
                /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
                for (uint i=0; i< eth.EthHandle.RxFrameInfos.SegCount; i++) {
                    dmarxdesc->Status |= ETH_DMARXDESC_OWN;
                    dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
                }

                /* Clear Segment_Count */
                eth.EthHandle.RxFrameInfos.SegCount =0;

                /* When Rx Buffer unavailable flag is set: clear it and resume reception */
                if ((eth.EthHandle.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET) {
                    /* Clear RBUS ETHERNET DMA flag */
                    eth.EthHandle.Instance->DMASR = ETH_DMASR_RBUS;
                    /* Resume DMA reception */
                    eth.EthHandle.Instance->DMARPDR = 0;
                }
            }
        }
    }

    return 0;
}

#if WITH_LIB_MINIP

status_t stm32_eth_send_minip_pkt(pktbuf_t *p)
{
    LTRACEF("p %p, dlen %zu, eof %u\n", p, p->dlen, p->flags & PKTBUF_FLAG_EOF);

    DEBUG_ASSERT(p && p->dlen);

    if (!(p->flags & PKTBUF_FLAG_EOF)) {
        /* can't handle multi part packets yet */
        PANIC_UNIMPLEMENTED;

        return ERR_NOT_IMPLEMENTED;
    }

    status_t err = eth_send(p->data, p->dlen);

    pktbuf_free(p, true);

    return err;
}

#endif


