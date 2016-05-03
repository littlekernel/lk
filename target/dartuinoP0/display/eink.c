#include <assert.h>
#include <err.h>
#include <debug.h>
#include <rand.h>
#include <stdlib.h>
#include <trace.h>
#include <dev/display.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>
// TODO The eink driver should not include stm headers. We likely need INIT to store
// a spihandle and then spi functions use it some other way
#include <stm32f7xx.h>
#include <platform/spi.h>
#include <platform.h>

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
static uint8_t lut_ft[128] = {
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
static uint8_t lut_kwg_vcom[32] = {
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
static uint8_t lut_kwg[512] = {
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

enum {
    PanelSetting = 0x00,
    PowerSetting = 0x01,
    PowerOff = 0x02,
    PowerOffSequenceSetting = 0x03,
    PowerOn = 0x04,
    BoosterSoftStart = 0x06,
    DeepSleep = 0x07,
    DisplayRefresh = 0x12,
    DataStartTransmission2 = 0x13,
    DataStartTransmissionWindow = 0x14,
    KwgVcomLutRegister = 0x20,
    KwgLutRegister = 0x22,
    FtLutRegister = 0x26,
    PllControl = 0x30,
    TemperatureSensor = 0x40,
    TemperatureSensorEnable = 0x41,
    TemperatureSensorWrite = 0x42,
    TemperatureSensorRead = 0x43,
    VcomAndDataIntervalSetting = 0x50,
    LowPowerDetection = 0x51,
    ResolutionSetting = 0x61,
    GateGroupSetting = 0x62,
    GateBlockSetting = 0x63,
    GateSelectSetting = 0x64,
    Revision = 0x70,
    GetStatus = 0x71,
    AutoMeasureVcom = 0x80,
    VcomValue = 0x81,
    VcomDcSetting = 0x82,
    BorderDcVoltageSetting = 0x84,
    LpdSelect = 0xE4,
  };

typedef struct {
    // Gate power selection
    // 0: External gate power from VGH/VGL pins
    // 1: Internal DC/DC function for generating VGH/VGL
    uint8_t vg_en:1;
    // Source power selection
    // 0: External source power from VDH/VDL pins
    // 1: Internal DC/DC function for generating VDH/VDL
    uint8_t vs_en:1;
    uint8_t __rs0:6;
    //---- new byte
    // VG_LVL[1:0]: Gate Voltage Level selection
    //  Bit definitions in power settings enum.
    uint8_t vg_lvl:2;
    // VCOM_HV: VCOM Voltage Level
    //  0: VCOMH=VSH+VCOMDC, VCOML=VSL+VCOMDC (default)
    //  1: VCOML=VGH, VCOML=VGL
    uint8_t vcom_hv:1;
    uint8_t __rs1:5;
    //---- new byte
    // VSH_LVL[5:0]: Internal positive source voltage level for K/W
    // (range: +2.4V ~ +11.0V / step:0.2V / default : +10.0V)
    uint8_t vsh_lvl:6;
    uint8_t __rs2:2;
    //---- new byte
    // VSL_LVL[5:0]: Internal negative source voltage level for K/W
    // (range: -2.4V ~ -11.0V / step:0.2V / default : -10.0V)
    uint8_t vsl_lvl:6;
    uint8_t __rs3:2;
    //---- new byte
    uint8_t vshr_lvl:6;
    uint8_t __rs4:2;
} pwr_settings_t;

typedef struct {
    uint8_t btpha_min_off:3;
    uint8_t btpha_drive_strength:3;
    uint8_t btpha_soft_start:2;
    //---- new byte
    uint8_t btphb_min_off:3;
    uint8_t btphb_drive_strength:3;
    uint8_t btphb_soft_start:2;
    //---- new byte
    uint8_t btphc_min_off:3;
    uint8_t btphc_drive_strength:3;
} booster_settings_t;

typedef struct {
    uint8_t busy_n:1;
    uint8_t pof:1;
    uint8_t pon:1;
    uint8_t data_flag:1;
    uint8_t i2c_busyn:1;
    uint8_t i2c_err:1;
    uint8_t __rs0:2;
} eink_status_t;

typedef struct {
    // DDX[1:0]: Data polarity
    //  0: Inverted
    //  1: Normal (default)
    uint8_t ddx:1;
    uint8_t __rs0:3;
    // VBD[1:0]: Border output selection
    uint8_t bdd:2;
    // BDV: Border DC Voltage control
    //  0: Border Output DC Voltage Function disabled (default)
    //  1: Border Output DC Voltage Function enabled
    uint8_t bdv:1;
    // BDZ: Border Hi-Z control
    //  0: Border Output Hi-Z disabled (default)
    //  1: Border Output Hi-Z enabled
    uint8_t bdz:1;
    //---- new byte
    // CDI[9:0]: VCOM to Source interval. Interval time setting from VCOM to
    // source dat.
    //  000 0000 000b ~ 11 1111 1111b: 1 Hsync ~ 1023 Hsync, respectively.
    //  (Default: 018h: 25 Hsync)
    uint8_t cdi_high:2;
    uint8_t __rs1:2;
    // DCI[3:0]: Source to VCOM interval. Interval time setting from source
    // data to VCOM.
    //  0000b ~ 1111b: 1 Hsync ~ 16 Hsync, respectively. (Default: 011b: 4
    //  Hsync)
    uint8_t dci:4;
    //---- new byte
    uint8_t cdi_low;
} vcom_data_int_settings_t;

typedef struct {
    uint8_t __rs0:2;
    // HRES[7:2]: Horizontal display resolution.
    //  00000b ~ 11111b: 4 ~ 256 lines
    uint8_t hres:6;
    //---- new byte
    // VRES[9:0]: Vertical display resolution
    //  0000000000b ~ 1111111111b: 1 ~ 1024 lines
    uint8_t vres_high:2;
    uint8_t __rs1:6;
    //---- new byte
    uint8_t vres_low;
} resolution_settings_t;


enum booster_soft_start_min_off {
    BOOSTER_0p27us = 0b000,
    BOOSTER_0p34us = 0b001,
    BOOSTER_0p40us = 0b010,
    BOOSTER_0p50us = 0b011,
    BOOSTER_0p80us = 0b100,
    BOOSTER_1p54us = 0b101,
    BOOSTER_3p34us = 0b110,
    BOOSTER_6p58us = 0b111,
};

enum start_drive_strength {
    SDS_1 = 0b000,
    SDS_2 = 0b001,
    SDS_3 = 0b010,
    SDS_4 = 0b011,
    SDS_5 = 0b100,
    SDS_6 = 0b101,
    SDS_7 = 0b110,
    SDS_8 = 0b111,  // (strongest)
};

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

static inline void assert_reset(void) {
    gpio_set(GPIO_DISP_RST, 0);
}

static inline void release_reset(void) {
    gpio_set(GPIO_DISP_RST, 1);
}

static inline void set_data_command_mode(void) {
    gpio_set(GPIO_DISP_DC, 0);
}

static inline void set_data_parameter_mode(void) {
    gpio_set(GPIO_DISP_DC, 1);
}

void write_cmd(uint8_t cmd) {
    uint8_t cmd_buf[1];

    cmd_buf[0] = cmd;
    set_data_command_mode();
    spi_write(&SpiHandle, cmd_buf, sizeof(cmd_buf), GPIO_DISP_CS);
    set_data_parameter_mode();
}

void write_data(uint8_t *buf, size_t len) {
    spi_write(&SpiHandle, buf, len, GPIO_DISP_CS);
}

void read_data(uint8_t *buf, size_t len) {
    spi_read(&SpiHandle, buf, len, GPIO_DISP_CS);
}

void get_status(void) {
    // NOT IMPLEMENTED, just wait and pray the busy-ness works out :D
    thread_sleep(50);
}

  // SpiBus must be configured in half-duplex mode. Display supports up to 6600
  // Kb/s baud rate. All GPIO should be configured active high. The orientation
  // parameter represents the virtual display orientation relative to the
  // physical display.
  //
  // The parameter disconnect_reset indicates if reset should float when
  // inactive.
  //
  // GPIO:
  //  chip_select - CSB - SPI chip select pin (active low).
  //  data_command - DC - Command/parameter select for 4-wire serial mode:
  //   L: command
  //   H: parameter
  //  reset - RST_N - Global reset pin (active low).
  //  busy - BUSY_N - Indicates timing controller status (active low):
  //   L: Driver is busy, data/VCOM is transforming.
  //   H: Not busy. Host side can send command/data to driver.

#define PHYSICAL_WIDTH  240
#define PHYSICAL_HEIGHT 240

static int cmd_eink(int argc, const cmd_args *argv)
{
    status_t err = NO_ERROR;
    TRACE_ENTRY;

    if (!spi_inited) {
        SpiHandle.Instance               = SPI2;
        SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
        SpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
        SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
        SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
        SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
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

    /* Documented driver steps */
    pwr_settings_t pwr = {
        .vsh_lvl  = 0x1C, // +8V
        .vsl_lvl  = 0x1C, // -8V
        .vshr_lvl = 0x00, // +2.4V
    };

    booster_settings_t booster = {
        .btpha_min_off          = BOOSTER_3p34us,
        .btpha_drive_strength   = SDS_5,
        .btphb_min_off          = BOOSTER_3p34us,
        .btphb_drive_strength   = SDS_5,
        .btphc_min_off          = BOOSTER_3p34us,
        .btphc_drive_strength   = SDS_5,
    };

    vcom_data_int_settings_t vdi = {
        .bdd     = 0x00, // PC30
        .dci     = 0x02, // 3 Hsync
        .cdi_low = 0x10, // 17 Hsync
    };

    resolution_settings_t rs = {
        .hres     = (PHYSICAL_WIDTH - 1) >> 2,
        .vres_low = PHYSICAL_HEIGHT - 1,
    };

    // Hold Display in reset
    assert_reset();

    // Pull down chip select
    gpio_set(GPIO_DISP_CS, 1);

    // Set data_command to 'Command'
    gpio_set(GPIO_DISP_DC, 0);

    if (!poll_gpio(GPIO_DISP_BUSY, 0, 50)) {
        printf("err: Display should be BUSY while held in RST.\n");
        goto err;
    }

    // Pull display out of reset
    release_reset();

    // datasheet says to wait 1ms coming out of reset
    if (!poll_gpio(GPIO_DISP_BUSY, 1, 1)) {
        printf("err: Display should not be BUSY after coming out of reset.\n");
        goto err;
    }

    write_cmd(PowerSetting);
    write_data((uint8_t *)&pwr, sizeof(pwr));

    // TODO: Need to get half duplex working for status checks
    get_status();

    write_cmd(PowerOn);
    if (!poll_gpio(GPIO_DISP_BUSY, 1, 1)) {
        printf("err: Display should not be BUSY after power_on\n");
        goto err;
    }

    write_cmd(BoosterSoftStart);
    write_data((uint8_t *)&booster, sizeof(booster));

    write_cmd(VcomAndDataIntervalSetting);
    write_data((uint8_t *)&vdi, sizeof(vdi));

    write_cmd(ResolutionSetting);
    write_data((uint8_t *)&rs, sizeof(rs));

err:
    return err;
}

STATIC_COMMAND_START
STATIC_COMMAND("eink", "eink commands", &cmd_eink)
STATIC_COMMAND_END(eink);

#endif




