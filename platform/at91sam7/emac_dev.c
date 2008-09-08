/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <debug.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <platform/at91sam7.h>
#include <platform/debug.h>

#include <dev/ethernet.h>

#include <malloc.h>
#include <string.h>

#include <hw/mii.h>

void emac_init_send(void);

#define PHYA 31

static unsigned mi_rd(AT91EMAC *emac, unsigned addr)
{
    addr &= 0x1f;

    thread_sleep(20);
    
    emac->MAN =
        (1 << 30) |    /* sof:  01 */
        (2 << 28) |    /* rw:   10 = read */
        (PHYA << 23) | /* phya: PHYA */
        (addr << 18) | /* rega: addr */
        (2 << 16);     /* code: 10 */

    while(!(emac->NSR & NSR_IDLE)) ;

    thread_sleep(20);
    return emac->MAN & 0xffff;
}

static void mi_wr(AT91EMAC *emac, unsigned addr, unsigned val)
{
    addr &= 0x1f;
    val &= 0xffff;
    
    emac->MAN =
        (1 << 30) |    /* sof:  01 */
        (1 << 28) |    /* rw:   01 = read */
        (PHYA << 23) | /* phya: PHYA */
        (addr << 18) | /* rega: addr */
        (2 << 16) |    /* code: 10 */
        val;           /* data: val */

    while(!(emac->NSR & NSR_IDLE)) ;
}

#define PIN_EMAC_ALL   0x3ffff
#define PIN_PHY_PD     (1 << 18)
#define PIN_PHY_IRQ    (1 << 26)

#define PIN_PHYAD0     (1 << 26)
#define PIN_PHYAD1     (1 << 14)
#define PIN_PHYAD2     (1 << 13)
#define PIN_PHYAD3     (1 << 6)
#define PIN_PHYAD4     (1 << 5)
#define PIN_LPBK       (1 << 15)
#define PIN_ISOLATE    (1 << 7)
#define PIN_RMII_MODE  (1 << 16)
#define PIN_RMII_BTB   (1 << 4)

/* select RMII w/ BTB, 100mbps duplex autonegotiate 
** disable ISOLATE and LPBK 
** phya=00001b
*/
#define PIN_RESET_LOW  (PIN_LPBK)

int emac_init(void)
{
    AT91EMAC *emac = AT91EMAC_ADDR;
    AT91PIO *piob = AT91PIOB_ADDR;
    AT91PMC *pmc = AT91PMC_ADDR;
    AT91RSTC *rstc = AT91RSTC_ADDR;

    dprintf(INFO, "emac_init()\n");
    
        /* enable clock to EMAC */
    pmc->PCER = (1 << PID_EMAC);

    thread_sleep(10);
    
        /* for reset, all lines are gpio inputs and pullups are
           enabled or disabled per strapping mode defined above */
    piob->pio_enable = PIN_EMAC_ALL | PIN_PHY_PD | PIN_PHY_IRQ;
    piob->select_a = PIN_EMAC_ALL;
    piob->pullup_enable = PIN_EMAC_ALL | PIN_PHY_IRQ;
    piob->pullup_disable = PIN_LPBK | PIN_ISOLATE | PIN_RMII_MODE;
    piob->output_disable = PIN_EMAC_ALL;

        /* PHY PD becomes output and high (no powerdown mode */
    piob->data_set = PIN_PHY_PD;
    piob->output_enable = PIN_PHY_PD;

    thread_sleep(30);
    
    dprintf(INFO, "emac_init() - reset phy\n");

        /* assert the RST line and wait until the it deasserts */
    rstc->CR = RSTC_KEY | RSTC_EXTRST;
    while(rstc->SR & RSTC_NRSTL) ;

    thread_sleep(30);
    
        /* after reset all the gpios are assigned to the EMAC,
           except for PHY_PD (which remains output and high) */
    piob->pio_disable = PIN_EMAC_ALL;

    emac->USRIO = USRIO_CLKEN;
    
    thread_sleep(1000);
    
    dprintf(INFO, "emac_init() - read state\n");
    
    emac->NCR = NCR_MPE;
    emac->NCFG = NCFG_CLK_d64;

    dprintf(INFO, "bcr = %x\n", mi_rd(emac, MII_REG_BCR));
    dprintf(INFO, "id1 = %x\n", mi_rd(emac, MII_REG_PHY_ID1));
    dprintf(INFO, "id2 = %x\n", mi_rd(emac, MII_REG_PHY_ID2));

#if 0
    unsigned state, last;
    last = 0xff;
    
    for(;;) {
        state = mi_rd(emac, MII_REG_100TX_PHY) & MII_100TX_MODE_MASK;
        if(last != state) {
            last = state;
            char *name;
            switch(state) {
            case MII_100TX_MODE_AUTO:
                name = "auto negotiate";
                break;
            case MII_100TX_MODE_10T_H:
                name = "10-T half duplex";
                break;
            case MII_100TX_MODE_10T_F:
                name = "10-T full duplex";
                break;
            case MII_100TX_MODE_100TX_H:
                name = "100-TX half duplex";
                break;
            case MII_100TX_MODE_100TX_F:
                name = "100-TX full duplex";
                break;
            case MII_100TX_MODE_ISOLATE:
                name = "isolate";
                break;
            default:
                name = "unknown";
            }
            dprintf(INFO, "link state: %s\n", name);
        }
        thread_sleep(100);
    } 
#endif

    emac_init_send();
    
    return 0;
}

#define XMIT_ENTRY_COUNT 32
static emac_xmit_entry xmit_list[XMIT_ENTRY_COUNT];
static unsigned xmit_next = 0;
static mutex_t xmit_lock;

void emac_init_send(void)
{
    AT91EMAC *emac = AT91EMAC_ADDR;
    int i;

    for(i = 0; i < XMIT_ENTRY_COUNT; i++) {
        xmit_list[i].info = XMIT_USED;
        xmit_list[i].addr = 0;
    }
    xmit_list[i-1].info |= XMIT_LAST;

    emac->NCFG = NCFG_CLK_d64 | NCFG_SPD | NCFG_FD;
    emac->NCR = NCR_TE | NCR_MPE;
    emac->TBQP = (unsigned) xmit_list;

    mutex_init(&xmit_lock);
}

int ethernet_send(void *data, unsigned len)
{
    AT91EMAC *emac = AT91EMAC_ADDR;

    emac_xmit_entry *xe;
    int waited = 0;

    mutex_acquire(&xmit_lock);
    
    xe = xmit_list + xmit_next;

    while(!(xe->info & XMIT_USED)) {
        thread_yield();
        waited++;
    }

    if(waited) dprintf(INFO, "W%d\n",waited);
    
    if(xe->addr != 0) {
        free((void*) xe->addr);
    }

    xe->addr = (unsigned) data;
    if(xmit_next == (XMIT_ENTRY_COUNT - 1)) {
        xe->info = XMIT_LENGTH(len) | XMIT_LAST | XMIT_WRAP;
        xmit_next = 0;
    } else {
        xe->info = XMIT_LENGTH(len) | XMIT_LAST;
        xmit_next++;
    }

    emac->NCR |= NCR_TSTART;

    mutex_release(&xmit_lock);

    return 0;
}

