/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <nrfx.h>
/*
  To use the twim driver, NRFX_TWIM_ENABLED must be set to 1 in the GLOBAL_DEFINES
  additionally, NRFX_TWIM0_ENABLED and/or NRFX_TWIM1_ENABLED must also be 1 to specify
  which modules are active.  These would ideally be set in either a project rule.mk
  or in a targets rules.mk.

  The pins to be used for each of the active TWIM modules should be defined as:
  TWIM0_SCL_PIN, TWIM0_SDA_PIN  (if twim0 is used)
  TWIM1_SCL_PIN, TWIM1_SDA_PIN  (if twim1 is used)
  and ideally be defined in the targets include/gpioconfig.h since it is included here.
*/

#if (NRFX_TWIM_ENABLED)
#include <nrfx_log.h>
#include <nrfx_twim.h>
#include <dev/i2c.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <target/gpioconfig.h>


#define TWIM_MASTER_TIMEOUT_MS 1000

typedef struct twim_dev {
    nrfx_twim_t twim;
    event_t evt;
    mutex_t lock;
    nrfx_twim_evt_type_t result;
} twim_dev_t;

#if (NRFX_TWIM0_ENABLED)
static twim_dev_t twim0 = { NRFX_TWIM_INSTANCE(0),
                            EVENT_INITIAL_VALUE(twim0.evt, false, 0),
                            MUTEX_INITIAL_VALUE(twim0.lock),
                            0
                          };

void nrf52_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_twim_0_irq_handler();
    arm_cm_irq_exit(true);
}
#endif

#if (NRFX_TWIM1_ENABLED)
static twim_dev_t twim1 = { NRFX_TWIM_INSTANCE(1),
                            EVENT_INITIAL_VALUE(twim1.evt, false, 0),
                            MUTEX_INITIAL_VALUE(twim1.lock),
                            0
                          };

void nrf52_SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_twim_1_irq_handler();
    arm_cm_irq_exit(true);
}
#endif

static inline twim_dev_t *get_nrfx_twim(int bus) {
    switch (bus) {
#if (NRFX_TWIM0_ENABLED)
        case 0:
            return &twim0;
#endif
#if (NRFX_TWIM1_ENABLED)
        case 1:
            return &twim1;
#endif
        default:
            return NULL;
    }
}

void i2c_twim_evt_handler(nrfx_twim_evt_t const *p_event,void *p_context) {
    twim_dev_t *twim = (twim_dev_t *)p_context;
    twim->result = p_event->type;
    event_signal(&twim->evt, false);
}

void i2c_init_early(void) {}

void i2c_init() {
    nrfx_err_t status;

#if (NRFX_TWIM0_ENABLED)
    //Pins should be defined in target/gpioconfig.h
    const nrfx_twim_config_t twim0_config = NRFX_TWIM_DEFAULT_CONFIG(TWIM0_SCL_PIN, TWIM0_SDA_PIN);

    status = nrfx_twim_init(&twim0.twim, &twim0_config, i2c_twim_evt_handler, &twim0);
    if (status == NRFX_SUCCESS) {
        nrfx_twim_enable(&twim0.twim);
    } else {
        NRFX_LOG_ERROR("ERROR in twim0 init:%s \n",NRFX_LOG_ERROR_STRING_GET(status));
    }
#endif

#if (NRFX_TWIM1_ENABLED)
    //Pins should be defined in target/gpioconfig.h
    const nrfx_twim_config_t twim1_config = NRFX_TWIM_DEFAULT_CONFIG(TWIM1_SCL_PIN, TWIM1_SDA_PIN);

    status = nrfx_twim_init(&twim1, &twim1_config, i2c_twim_evt_handler, &twim1);
    if (status == NRFX_SUCCESS) {
        nrfx_twim_enable(&twim1);
    } else {
        NRFX_LOG_ERROR("ERROR in twim1 init:%s \n",NRFX_LOG_ERROR_STRING_GET(status));
    }
#endif
}

static status_t i2c_xfer(int bus, nrfx_twim_xfer_desc_t *desc) {
    twim_dev_t *twim = get_nrfx_twim(bus);
    if (twim == NULL) return ERR_INVALID_ARGS;

    mutex_acquire(&twim->lock);

    nrfx_err_t nrfx_status = nrfx_twim_xfer(&twim->twim, desc, 0);

    event_wait_timeout(&twim->evt, TWIM_MASTER_TIMEOUT_MS);
    event_unsignal(&twim->evt);
    mutex_release(&twim->lock);

    if (nrfx_status != NRFX_SUCCESS) {
        NRFX_LOG_ERROR("%s:%s \n", __func__, NRFX_LOG_ERROR_STRING_GET(nrfx_status));
        return (status_t)nrfx_status;
    }
    return NO_ERROR;
}

status_t i2c_transmit(int bus, uint8_t address, const void *buf, size_t count) {

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(address, (uint8_t *)buf, count);

    return i2c_xfer(bus, &xfer);
}

status_t i2c_receive(int bus, uint8_t address, void *buf, size_t count) {

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_RX(address, (uint8_t *)buf, count);

    return i2c_xfer(bus, &xfer);
}

status_t i2c_write_reg_bytes(int bus, uint8_t address, uint8_t reg, const uint8_t *val,
                             size_t cnt) {

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXTX(address, &reg, 1, (uint8_t *)val, cnt);

    return i2c_xfer(bus, &xfer);
}

status_t i2c_read_reg_bytes(int bus, uint8_t address, uint8_t reg, uint8_t *val, size_t cnt) {
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(address, &reg, 1, (uint8_t *)val, cnt);

    return i2c_xfer(bus, &xfer);
}

#endif // NRFX_TWIM_ENABLED
