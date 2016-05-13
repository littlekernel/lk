#include <assert.h>
#include <err.h>
#include <debug.h>
#include <rand.h>
#include <stdlib.h>
#include <trace.h>
#include <dev/display.h>
#include <kernel/spinlock.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>
#include <target/tqlogo.h>
#include <target/et011tt2v1.h>
#include <string.h>
// TODO The eink driver should not include stm headers. We likely need INIT to store
// a spihandle and then spi functions use it some other way
#include <stm32f7xx.h>
#include <platform/spi.h>
#include <platform.h>


static uint8_t framebuffer[FBSIZE];

/* The following tables are copied verbatim with comments from the verily driver */
// TODO(nicholasewalt): Update LUTs once they are provided by Eink.
// TODO(nicholasewalt): Investigate truncating and compressing LUTs. Current
// tables are only 62 frames long and are highly compressible, however new
// tables will likely have less potential gains.
// FTLUT
// White REAGL LUT for K/W/Gray mode.
//
// F/T waveform for up to 128 frames, 1 byte per frame.
//
// For one frame:
//
// |    D7  D6    |    D5  D4    |    D3  D2    |    D1  D0    |
// |--------------+--------------+--------------+--------------|
// |  F1_KF[1:0]  |  F1_KT[1:0]  |  F1_WF[1:0]  |  F1_WT[1:0]  |
//
// NOTE: These bit definitions are not explained in more detail in the
// datasheet.
//
static const uint8_t lut_ft[128] = {
    0x55, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x5A, 0x5A,
    0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// KWG_VCOM
// VCOM LUT for K/W/Gray mode.
//
// VCOM LUT for up to 128 frames, 2 bits per frame.
//
// For one frame:
//
// |    D7  D6    |    D5  D4    |    D3  D2    |    D1  D0    |
// |--------------+--------------+--------------+--------------|
// |  VCOM1[1:0]  |  VCOM2[1:0]  |  VCOM3[1:0]  |  VCOM4[1:0]  |
//
// VCOM(1~128)[1:0]: VCOM voltage level of Frame 1~128, respectively.
//  00b: VCOM output VCOMDC
//  01b: VCOM output VSH+VCOM_DC (VCOMH)
//  10b: VCOM output VSL+VCOM_DC (VCOML)
//  11b: VCOM output floating.
//
static const uint8_t lut_kwg_vcom[32] = {
    0x55, 0x6A, 0xA5, 0x55, 0x55, 0x55, 0x55, 0x55, 0x56, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// KWG_LUT
// Source LUT for K/W/Gray mode.
// NOTE: This LUT was ripped from the Hulk Jig firmware, however it did not
// include waveforms for transitioning into or out of gray*. nicholasewalt@
// added preliminary white to gray* transitions, though they do not have good
// mixing and other gray* transitions are still unsupported.
//
// Pixel waveforms for up to 128 frames, 4 bytes per frame.
// If waveforms of one frame are all 11b, the update precedue will stop.
//
// For one frame:
//
// |    D7  D6    |    D5  D4    |    D3  D2    |    D1  D0    |
// |--------------+--------------+--------------+--------------|
// | F1_P0C0[1:0] | F1_P0C1[1:0] | F1_P0C2[1:0] | F1_P0C3[1:0] |
// | F1_P1C0[1:0] | F1_P1C1[1:0] | F1_P1C2[1:0] | F1_P1C3[1:0] |
// | F1_P2C0[1:0] | F1_P2C1[1:0] | F1_P2C2[1:0] | F1_P2C3[1:0] |
// | F1_P3C0[1:0] | F1_P3C1[1:0] | F1_P3C2[1:0] | F1_P3C3[1:0] |
//
// P0C0/P0C1/P0C2/P0C3: black to black/gray1/gray2/white respectively
// P1C0/P1C1/P1C2/P1C3: gray1 to black/gray1/gray2/white respectively
// P2C0/P2C1/P2C2/P2C3: gray2 to black/gray1/gray2/white respectively
// P3C0/P3C1/P3C2/P3C3: white to black/gray1/gray2/white respectively
//
// P0~3C0~3[1:0]:
//  00b: GND
//  01b: VSH
//  10b: VSL
//  11b: VSHR
//
static const uint8_t lut_kwg[512] = {
    0x41, 0x81, 0x81, 0x81, 0x41, 0x81, 0x81, 0x81, 0x41, 0x81, 0x81, 0x81,
    0x41, 0x81, 0x81, 0x81, 0x41, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x82,
    0x81, 0x81, 0x81, 0x82, 0x81, 0x81, 0x81, 0x82, 0x81, 0x81, 0x81, 0x82,
    0x81, 0x81, 0x81, 0x82, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a,
    0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a,
    0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a,
    0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a,
    0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* cja: end of imported LUTs */
#define LOCAL_TRACE 0

#if WITH_LIB_CONSOLE
#include <lib/console.h>
SPI_HandleTypeDef SpiHandle;
bool spi_inited = false;
spin_lock_t lock;
spin_lock_saved_state_t state;




#define EINK_WHITE      0xFF
#define EINK_BLACK      0x00

static bool poll_gpio(uint32_t gpio, bool desired, uint8_t timeout)
{
    lk_time_t now = current_time();
    uint32_t current;

    while ((current = gpio_get(gpio)) != desired) {
        if (current_time() - now > timeout) {
            break;
        }
    }

    return (current == desired);
}

/* The display pulls the BUSY line low while BUSY and releases it when done */
static bool check_busy(uint8_t dispnum) {
    if (dispnum==0) {
        return poll_gpio(GPIO_DISP0_BUSY, 1, 100);
    } else {
        return poll_gpio(GPIO_DISP1_BUSY, 1, 100);
    }
}

static inline uint32_t get_cs_pin(uint8_t disp_num) {
    if (disp_num==0) {
        return GPIO_DISP0_CS;
    } else {
        return GPIO_DISP1_CS;
    }
}


static inline void assert_reset(void) {
    gpio_set(GPIO_DISP_RST, 0);
}

static inline void release_reset(void) {
    gpio_set(GPIO_DISP_RST, 1);
}


void write_cmd(uint8_t disp_num, uint8_t cmd) {
    uint16_t cmd16;

    cmd16 = (uint16_t) cmd;
    spi_write(&SpiHandle, (uint8_t *)&cmd16,1, get_cs_pin(disp_num));
}

void write_data(uint8_t disp_num, const uint8_t *buf, size_t len) {
    //set_data_parameter_mode();
    uint16_t txbyte;
    //enter_critical_section;
    //spin_lock_irqsave(&lock, state);
    for (size_t i = 0; i < len; i++) {
        txbyte = ((uint16_t) buf[i]) | 0x0100;
        spi_write(&SpiHandle, (uint8_t *)&txbyte,1, get_cs_pin(disp_num));
    }
    //spin_unlock_irqrestore(&lock, state);
    //exit_critical_section;
}

void write_param(uint8_t disp_num, uint8_t param) {

    uint16_t data16;
    data16 = (uint16_t) param | 0x0100;
    spi_write(&SpiHandle, (uint8_t *)&data16,1, get_cs_pin(disp_num));
    //printf("%x\n",get_cs_pin(disp_num));
}



void write_burst_data(uint8_t *buf, size_t len) {
    //enter_critical_section;
    int chunk = 256;
    spin_lock_irqsave(&lock, state);
    gpio_set(GPIO_DISP0_CS, 0);
    for (size_t off = 0; off < len; off += chunk) {
        spi_write(&SpiHandle, buf + off, chunk, 0);
    }
    gpio_set(GPIO_DISP0_CS, 1);
    spin_unlock_irqrestore(&lock, state);
    //exit_critical_section;
}

status_t read_data(uint8_t *buf, size_t len) {
    return spi_read(&SpiHandle, buf, len, GPIO_DISP0_CS);
}

status_t get_status(et011tt2_status_t *status) {
    status_t err;

    if (status == NULL) {
        return ERR_INVALID_ARGS;
    }

    write_cmd(0,GetStatus);
    err = read_data((uint8_t *) status, sizeof(et011tt2_status_t));

    return err;
}

status_t eink_disp_init(uint8_t disp_num){
    TRACE_ENTRY;
        status_t err = NO_ERROR;

    //if (!check_busy(disp_num)) {
    //    printf("Device %d is still busy after initial reset!\n",disp_num);
    //    return ERR_GENERIC;
    //}


    // Configure power settings
    write_cmd(disp_num, PowerSetting);
    write_param(disp_num, 0x03);
    write_param(disp_num, 0x00);
    write_param(disp_num, 0x1c);
    write_param(disp_num, 0x1c);
    write_param(disp_num, 0x00);

    // Power on display
    write_cmd(disp_num, PowerOn);

    // Configure panel settings
    write_cmd(disp_num, PanelSetting);
    write_param(disp_num, 0x0f);
    write_param(disp_num, 0x86);

    // Configure Boost
    write_cmd(disp_num, BoosterSoftStart);
    write_param(disp_num, 0x3e);
    write_param(disp_num, 0x3e);
    write_param(disp_num, 0x3e);

    // Initialize -> Check_Busy
    write_cmd(disp_num, VcomAndDataIntervalSetting);
    write_param(disp_num, 0x01);
    write_param(disp_num, 0x20);
    write_param(disp_num, 0x10);

    write_cmd(disp_num, ResolutionSetting);
    write_param(disp_num,  (HRES-1) | 0x03);
    write_param(disp_num,  (VRES-1) >> 8 );
    write_param(disp_num,  (VRES-1) & 0xFF);


    write_cmd(disp_num, GateGroupSetting);
    write_param(disp_num, 0x89);
    write_param(disp_num, 0x89);
    write_param(disp_num, 0xcb);
    write_param(disp_num, 0xcb);
    write_param(disp_num, 0x03);

    write_cmd(disp_num, BorderDcVoltageSetting);
    write_param(disp_num, 0x4e);

    write_cmd(disp_num, LpdSelect);
    write_param(disp_num, 0x02);

    write_cmd(disp_num, FtLutRegister);
    write_data(disp_num, lut_ft, sizeof(lut_ft));

    write_cmd(disp_num, KwgVcomLutRegister);
    write_data(disp_num, lut_kwg_vcom, sizeof(lut_kwg_vcom));

    write_cmd(disp_num, KwgLutRegister);
    write_data(disp_num, lut_kwg, sizeof(lut_kwg));

    write_cmd(disp_num, VcomDcSetting);
    write_param(disp_num, 0x22);


    //if (!check_busy(disp_num)) {
    //    printf("Device is still busy after Power On and configuration\n");
    //    return ERR_GENERIC;
    //}

    memset(framebuffer, 0xff, sizeof(framebuffer));
    return eink_refresh(disp_num);

err:
    TRACE_EXIT;
    return err;




}



status_t eink_init(void) {
    TRACE_ENTRY;
    status_t err = NO_ERROR;
    spin_lock_init(&lock);

    if (!spi_inited) {
        SpiHandle.Instance               = SPI2;
        SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
        SpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
        SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
        SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
        SpiHandle.Init.DataSize          = SPI_DATASIZE_9BIT;
        SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
        SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
        SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
        SpiHandle.Init.CRCPolynomial     = 0;
        SpiHandle.Init.NSS               = SPI_NSS_SOFT;
        SpiHandle.Init.Mode              = SPI_MODE_MASTER;

        err = spi_init(&SpiHandle);
        if (err != HAL_OK) {
            printf("Failed to init spi\n");
        }

        spi_inited = true;
    }

    // VDD (wired straight to 3v3)
    spin(2000);         // Delay 2 ms
    assert_reset();     // RST_LOW
    spin(30);           // Delay 30 us
    release_reset();    // RST_HIGH
    
    eink_disp_init(0);
    eink_disp_init(1);

    printf("0---%x\n",get_cs_pin(0));
    printf("1---%x\n",get_cs_pin(1));

err:
    TRACE_EXIT;
    return err;
}




static int eink_dumpfb(uint8_t disp_num, uint8_t * buff, uint32_t count)
{
    // DTMW
    write_cmd(disp_num, DataStartTransmissionWindow);
    write_param(disp_num, 0x00);
    write_param(disp_num, 0x00);
    write_param(disp_num, 0x00);
    write_param(disp_num,  (HRES -1) | 0x03 );
    write_param(disp_num,  (VRES -1) >> 8 );
    write_param(disp_num,  (VRES -1) & 0xff);

    // DTM2
    write_cmd(disp_num, DataStartTransmission2);
    write_data(disp_num, buff, count);

    // DRF
    write_cmd(disp_num, DisplayRefresh);
    write_param(disp_num, 0x00);
    write_param(disp_num, 0x00);
    write_param(disp_num, 0x00);
    write_param(disp_num, 0x00);
    write_param(disp_num,  (HRES -1) | 0x03 );
    write_param(disp_num,  (VRES -1) >> 8 );
    write_param(disp_num,  (VRES -1) & 0xff);

    return 0;
}
static int cmd_eink_fill(int argc, const cmd_args *argv)
{
    uint16_t x,y,count,val;

    x = argv[1].i;
    y = argv[2].i;
    val = argv[3].i;
    count = argv[4].i;

    memset(framebuffer, 0xff, FBSIZE);
    memset(framebuffer+ x + y*(240>>2), val,count);
    //memset(framebuffer+(240>>4)*10, 0xcc, 10 );
    eink_dumpfb(0,framebuffer,FBSIZE);
    eink_dumpfb(1,framebuffer,FBSIZE);

    return 0;
}

static int cmd_eink1(int argc, const cmd_args *argv)
{
    memset(framebuffer, 0xff, FBSIZE);
    eink_dumpfb(0,framebuffer,FBSIZE);
    eink_dumpfb(1,framebuffer,FBSIZE);
    return 0;
}

static int cmd_eink0(int argc, const cmd_args *argv)
{
    memset(framebuffer, 0x00, sizeof(framebuffer));
    eink_dumpfb(0,framebuffer,FBSIZE);
    eink_dumpfb(1,framebuffer,FBSIZE);
    return 0;
}

int eink_refresh(uint8_t disp_num) {
    return eink_dumpfb(disp_num,framebuffer,FBSIZE);
}

static int cmd_eink_logo(int argc, const cmd_args *argv)
{
    eink_dumpfb(0,logo,sizeof(logo));
    return eink_dumpfb(1,logo,sizeof(logo));
}

static int cmd_eink(int argc, const cmd_args *argv)
{
    return eink_init();
}

uint8_t * get_eink_framebuffer(void) {
    return framebuffer;
}

static struct display_framebuffer eink_framebuffer;

static uint8_t clamp_px(uint8_t px)
{
    if (px) return 0x3;
    return 0x0;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
    DEBUG_ASSERT(image);

    // Convert the framebuffer into something that the e-ink display will
    // understand.
    // TODO(gkalsi): Note that we're ignoring starty and endy right now. Just
    //               dump the whole display for now and we can worry about
    //               partial updates later.

    // memset(framebuffer, 0, FBSIZE);
    uint8_t *fb = (uint8_t *)framebuffer;
    uint8_t *px = (uint8_t *)image->pixels;
    for (unsigned int col = 0; col < 60; col++) {
        for (unsigned int row = 0; row < 240; row++) {
            fb[col + row * 60]  = clamp_px((px[col * 2 + row * 120] & 0xC0) >> 6) << 4;
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120] & 0x0C) >> 2) << 6;
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120 + 1] & 0xC0) >> 6);
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120 + 1] & 0x0C) >> 2) << 2;
        }
    }

    return eink_dumpfb(0,framebuffer, FBSIZE);
}

static void eink_flush(uint starty, uint endy)
{
    display_present(&eink_framebuffer.image, starty, endy);
}

status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    DEBUG_ASSERT(fb);
    // Build the framebuffer.

    eink_framebuffer.image.format = IMAGE_FORMAT_MONO_8;
    eink_framebuffer.image.stride = HRES;
    eink_framebuffer.image.rowbytes = HRES;

    eink_framebuffer.image.pixels = malloc(HRES * VRES);
    eink_framebuffer.image.width = HRES;
    eink_framebuffer.image.height = HRES;
    eink_framebuffer.flush = eink_flush;
    eink_framebuffer.format = DISPLAY_FORMAT_RGB_111;   // TODO(gkalsi): This is not RGB, we're lying
    *fb = eink_framebuffer;
    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    DEBUG_ASSERT(info);

    info->format = IMAGE_FORMAT_MONO_8;
    info->width = HRES;
    info->height = HRES;

    return NO_ERROR;
}


STATIC_COMMAND_START
STATIC_COMMAND("eink", "eink init", &cmd_eink)
STATIC_COMMAND("eink1", "eink fill white", &cmd_eink1)
STATIC_COMMAND("eink0", "eink fill black", &cmd_eink0)
STATIC_COMMAND("einkfill", "eink fill x y val count", &cmd_eink_fill)
STATIC_COMMAND("einklogo", "tqlogo", &cmd_eink_logo)
STATIC_COMMAND_END(eink);

#endif




