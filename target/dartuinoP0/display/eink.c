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
#include <string.h>
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
} et011tt2_status_t;

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

typedef struct {
    // G1~4NUM[3:0]: Channel Number used for Gate Group 1~4. For example:
    //  2: GGx[2:0] ON
    //  15: GGx[15:0] ON
    uint8_t g1num:4;
    uint8_t __rs0:1;
    // G1~4UD: Gate Group 1~4 Up/Down Selection
    //  0: Down scan within Gate Group
    //  1: Up scan within Gate Group (default)
    uint8_t g1ud:1;
    // G1~4BS: Gate Group 1~4 Block/Select Selection
    //  0: Gate Select
    //  1: Gate Block
    uint8_t g1bs:1;
    // G1~4EN: Gate Group 1~4 Enable
    //  0: Disable
    //  1: Enable
    uint8_t g1en:1;
    //---- new byte
    uint8_t g2num:4;
    uint8_t __rs1:1;
    uint8_t g2ud:1;
    uint8_t g2bs:1;
    uint8_t g2en:1;
    //---- new byte
    uint8_t g3num:4;
    uint8_t __rs2:1;
    uint8_t g3ud:1;
    uint8_t g3bs:1;
    uint8_t g3en:1;
    //---- new byte
    uint8_t g4num:4;
    uint8_t __rs3:1;
    uint8_t g4ud:1;
    uint8_t g4bs:1;
    uint8_t g4en:1;
    //---- new byte
    // GSFB: Gate Select Forward/Backward
    //  0: Gate select backward
    //  1: Gate select forward
    uint8_t gsfb:1;
    // GBFB: Gate Block Forward/Backward
    //  0: Gate block backward
    //  1: Gate block forward
    uint8_t gbfb:1;
    uint8_t __rs4:2;
    // XOPT: XON Option
    //  0: No all gate on during vertical blanking in XON mode (default)
    //  1: All gate on during vertical blanking in XON mode
    uint8_t xopt:1;
    uint8_t __rs5:3;
} gate_group_settings_t;

typedef struct {
    uint8_t vbds:7;
    uint8_t __rs0:1;
} border_dc_v_settings_t;

typedef struct {
    // Low Power Voltage Selection
    uint8_t lpd_sel:2;
    uint8_t __rs0:6;
} lpdselect_t;

typedef struct {
    uint8_t pixel3:2;
    uint8_t pixel2:2;
    uint8_t pixel1:2;
    uint8_t pixel0:2;
} data_tranmission_t;

typedef struct {
    // X[7:0]: X-axis Start Point. X-axis start point for update display window.
    // NOTE: The X-axis start point needs to be a multiple of 4.
    uint8_t x;
    // Y[9:0]: Y-axis Start Point. Y-axis start point for update display window.
    uint8_t y_high:2;
    uint8_t __rs0:6;
    uint8_t y_low;
    // W[7:0]: X-axis Window Width. X-axis width for update display window.
    // NOTE: The width needs to be a multiple of 4.
    // NOTE: This needs to be set to W - 1.
    uint8_t w;
    uint8_t l_high:2;
    uint8_t __rs1:6;
    // L[9:0]: Y-axis Window Width. Y-axis width for update display window
    // NOTE: This needs to be set to L - 1.
    uint8_t l_low;
} data_transmission_window_t;

typedef struct {
    uint8_t mode:2;
    // DN_EN: Do-nothing function enabled
    //  0: Data follow VCOM function disable
    //  1: Data output follows VCOM LUT if new pixel data equal to old pixel
    //     data inside Update Display Area
    // NOTE: Do-nothing function is always active outside Update Display Area.
    uint8_t dn_en:1;
    // RGL_EN: REGAL function control
    //  0: REGAL function disable
    //  1: REGAL function enable
    uint8_t rgl_en:1;
    // PSCAN: Partial Scan control
    //  0: Partial Scan disable
    //  1: Partial Scan enable (Gate Scan within Display Window only)
    uint8_t pscan:1;
    uint8_t __rs0:3;
    //---- new byte
    // X[7:0]: X-axis Start Point. X-axis start point for update display window.
    // NOTE: The X-axis start point needs to be a multiple of 4.
    uint8_t x;
    // Y[9:0]: Y-axis Start Point. Y-axis start point for update display window.
    uint8_t y_high:2;
    uint8_t __rs1:6;
    //---- new byte
    uint8_t y_low;
    // W[7:0]: X-axis Window Width. X-axis width for update display window.
    // NOTE: The width needs to be a multiple of 4.
    // NOTE: This needs to be set to W - 1.
    uint8_t w;
    // L[9:0]: Y-axis Window Width. Y-axis width for update display window
    // NOTE: This needs to be set to L - 1.
    uint8_t l_high:2;
    uint8_t __rs2:6;
    uint8_t l_low;
} display_refresh_t;

enum {
    PanelSetting                = 0x00,
    PowerSetting                = 0x01,
    PowerOff                    = 0x02,
    PowerOffSequenceSetting     = 0x03,
    PowerOn                     = 0x04,
    BoosterSoftStart            = 0x06,
    DeepSleep                   = 0x07,
    DisplayRefresh              = 0x12,
    DataStartTransmission2      = 0x13,
    DataStartTransmissionWindow = 0x14,
    KwgVcomLutRegister          = 0x20,
    KwgLutRegister              = 0x22,
    FtLutRegister               = 0x26,
    PllControl                  = 0x30,
    TemperatureSensor           = 0x40,
    TemperatureSensorEnable     = 0x41,
    TemperatureSensorWrite      = 0x42,
    TemperatureSensorRead       = 0x43,
    VcomAndDataIntervalSetting  = 0x50,
    LowPowerDetection           = 0x51,
    ResolutionSetting           = 0x61,
    GateGroupSetting            = 0x62,
    GateBlockSetting            = 0x63,
    GateSelectSetting           = 0x64,
    Revision                    = 0x70,
    GetStatus                   = 0x71,
    AutoMeasureVcom             = 0x80,
    VcomValue                   = 0x81,
    VcomDcSetting               = 0x82,
    BorderDcVoltageSetting      = 0x84,
    LpdSelect                   = 0xE4,
  };


enum booster_soft_start_min_off {
    soft_start_min_off_0p27us = 0b000,
    soft_start_min_off_0p34us = 0b001,
    soft_start_min_off_0p40us = 0b010,
    soft_start_min_off_0p50us = 0b011,
    soft_start_min_off_0p80us = 0b100,
    soft_start_min_off_1p54us = 0b101,
    soft_start_min_off_3p34us = 0b110,
    soft_start_min_off_6p58us = 0b111,
};

enum drive_strength {
    drive_strength_1 = 0b000,
    drive_strength_2 = 0b001,
    drive_strength_3 = 0b010,
    drive_strength_4 = 0b011,
    drive_strength_5 = 0b100,
    drive_strength_6 = 0b101,
    drive_strength_7 = 0b110,
    drive_strength_8 = 0b111,  // (strongest)
};

enum soft_start_period {
    soft_start_period_10ms = 0b00,
    soft_start_period_20ms = 0b01,
    soft_start_period_30ms = 0b10,
    soft_start_period_40ms = 0b11,
};

enum lpd_select_lpdsel {
    LPDSEL_2p2v = 0b00,
    LPDSEL_2p3v = 0b01,
    LPDSEL_2p4v = 0b10,
    LPDSEL_2p5v = 0b11, // (default)
};

#define PHYSICAL_WIDTH  240
#define PHYSICAL_HEIGHT 240
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
static bool check_busy(void) {
    return poll_gpio(GPIO_DISP_BUSY, 1, 50);
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
#if 0
    gpio_set(GPIO_DISP_CS, 0);
    HAL_SPI_Transmit(&SpiHandle, cmd_buf, sizeof(cmd_buf), HAL_MAX_DELAY);
    gpio_set(GPIO_DISP_CS, 1);
#else
    spi_write(&SpiHandle, cmd_buf, sizeof(cmd_buf), GPIO_DISP_CS);
#endif
}

void write_data(uint8_t *buf, size_t len) {
    set_data_parameter_mode();
    //enter_critical_section;
#if 0
    gpio_set(GPIO_DISP_CS, 0);
    HAL_SPI_Transmit(&SpiHandle, buf, len, HAL_MAX_DELAY);
    gpio_set(GPIO_DISP_CS, 1);
#else
    spi_write(&SpiHandle, buf, len, GPIO_DISP_CS);
#endif
    //exit_critical_section;
}

status_t read_data(uint8_t *buf, size_t len) {
    return spi_read(&SpiHandle, buf, len, GPIO_DISP_CS);
}

status_t get_status(et011tt2_status_t *status) {
    status_t err;

    if (status == NULL) {
        return ERR_INVALID_ARGS;
    }

    write_cmd(GetStatus);
    err = read_data((uint8_t *) status, sizeof(et011tt2_status_t));

    return err;
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


static int cmd_eink(int argc, const cmd_args *argv)
{
    status_t err = NO_ERROR;
/*
    volatile stm32f7_spi_t *spi = (stm32f7_spi_t *)SPI2;
    SPIx_CR1_t CR1 = {
        .br          = fpclk_div_256;
        .cpha        = cpha_first_transition;
        .cpol        = cpol_clk_idle_low;
        .mstr        = mstr_spi_master;
        .rxonly      = rxonly_full_duplex;
        .crcen       = hw_crc_disable;
        .bidi_mode   = bidi_mode_1;
        .bidi_output = bidi_output_enable;
        .lsb_first   = 0;
        .ssm         = ssm_disabled;
        .ssi         = 0;
     cr1 = spi->CR1;
    uint16_t cr2 = spi->CR2;
    uint16_t sr = spi->SR;*/



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
        .btpha_min_off          = soft_start_min_off_3p34us,
        .btpha_drive_strength   = drive_strength_8,
        .btpha_soft_start       = soft_start_period_10ms,
        .btphb_min_off          = soft_start_min_off_3p34us,
        .btphb_drive_strength   = drive_strength_8,
        .btphb_soft_start       = soft_start_period_10ms,
        .btphc_min_off          = soft_start_min_off_3p34us,
        .btphc_drive_strength   = drive_strength_8,
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

    border_dc_v_settings_t bdvs = {
        .vbds = 0x4e, // -4.0V
    };

    gate_group_settings_t ggs = {
        .g1num = 0x09,  // GG1[9:0] ON
        .g1ud = 0,      // GG1[9] -> GG1[0]
        .g2num = 0x09,  // GG2[9:0] ON
        .g2ud = 0,      // GG2[9] -> GG2[0]
        .g3num = 0x0B,  // GG3[11:0] ON
        .g3ud = 0,      // GG3[11] -> GG3[0]
        .g3bs = 1,      // Gate block
        .g4num = 0x0B,  // GG4[11:0] ON
        .g4ud = 0,      // GG4[11] -> GG4[0]
        .g4bs = 1,      // Gate block
    };

    lpdselect_t lpds = {
        .lpd_sel = LPDSEL_2p4v,
    };

    display_refresh_t dr = {
        .dn_en  = 1,
        .pscan  = 0,
        .rgl_en = 0,
        .x      = 0,
        .y_low  = 0,
        .w      = 240,
        .l_high = 0,
        .l_low  = 240,
    };

    data_transmission_window_t dtw = {
        .x = 0,
        .y_low = 0,
        .w = 240,
        .l_high = 0,
        .l_low = 240,
    };

    // VDD (wired straight to 3v3)
    spin(2000);         // Delay 2 ms
    assert_reset();     // RST_LOW
    spin(30);           // Delay 30 us
    release_reset();    // RST_HIGH

    if (!check_busy()) {
        printf("Device is still busy after initial reset!\n");
        return ERR_GENERIC;
    }

    // Configure Boost
    write_cmd(BoosterSoftStart);
    write_data((uint8_t *)&booster, sizeof(booster));

    // Power on display
    write_cmd(PowerOn);

    // Initialize -> Check_Busy
    if (!check_busy()) {
        printf("Device is still busy after Power On!\n");
        return ERR_GENERIC;
    }

    /* Quick buffer to toss at it */
    uint8_t buf[128];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = i % 255;
    }

    // DTMW
    write_cmd(DataStartTransmissionWindow);
    write_data((uint8_t *) &dtw, sizeof(dtw));

    // DTM2
    write_cmd(DataStartTransmission2);
    write_data(buf, sizeof(buf));

    // DRF
    write_cmd(DisplayRefresh);
    write_data((uint8_t *)&dr, sizeof(dr));

    // Check_Busy
    if (!check_busy()) {
        printf("Device is still busy after Display Refresh!\n");
        return ERR_GENERIC;
    }

    // POF
    write_cmd(PowerOff);

    // Check_Busy
    if (!check_busy()) {
        printf("Device is still busy after Power Off!\n");
        return ERR_GENERIC;
    }

    // DSLP
    uint8_t sleepbuf = 0b10100101;
    write_cmd(DeepSleep);
    write_data(&sleepbuf, sizeof(sleepbuf));
    //
    return 0;


/*
 *  Config writes that need to be verified against the tables and defaults to ensure
 *  I'm not missing important predefined bits.
    write_cmd(VcomAndDataIntervalSetting);
    write_data((uint8_t *)&vdi, sizeof(vdi));

    write_cmd(ResolutionSetting);
    write_data((uint8_t *)&rs, sizeof(rs));

    write_cmd(GateGroupSetting);
    write_data((uint8_t *)&ggs, sizeof(ggs));

    write_cmd(BorderDcVoltageSetting);
    write_data((uint8_t *)&bdvs, sizeof(bdvs));

    write_cmd(LpdSelect);
    write_data((uint8_t *)&lpds, sizeof(lpds));

    write_cmd(FtLutRegister);
    write_data(lut_ft, sizeof(lut_ft));

    write_cmd(KwgVcomLutRegister);
    write_data(lut_kwg_vcom, sizeof(lut_kwg_vcom));

    write_cmd(KwgLutRegister);
    write_data(lut_kwg, sizeof(lut_kwg));

*/
err:
    return err;
}

STATIC_COMMAND_START
STATIC_COMMAND("eink", "eink commands", &cmd_eink)
STATIC_COMMAND_END(eink);

#endif




