


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