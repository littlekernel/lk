#pragma once

// heavily copied from bcm2708_chip/sdc_ctrl.h
// from the rpi-open-firmware repo
// (from the v3d source release??)
// TODO, replace this with the official headers

#define SD_CS_SDUP_SET                                     0x00008000
#define SD_CS_DEL_KEEP_SET                                 0x00040000
#define SD_MR_RDATA_SET                                    0x00ff0000
#define SD_MR_RDATA_LSB                                    16
#define SD_MR_RW_SET                                       0x10000000
#define SD_MR_HI_Z_SET                                     0x20000000
#define SD_MR_TIMEOUT_SET                                  0x40000000
#define SD_MR_DONE_SET                                     0x80000000

#define CM_SDCCTL 0x7e1011a8
#define CM_SDCCTL_SRC_CLR                                  0xfffffff0
#define CM_SDCCTL_ENAB_SET                                 0x00000010
#define CM_SDCCTL_BUSY_SET                                 0x00000080
#define CM_SDCCTL_CTRL_CLR                                 0xffff0fff
#define CM_SDCCTL_CTRL_LSB                                 12
#define CM_SDCCTL_ACCPT_SET                                0x00010000
#define CM_SDCCTL_UPDATE_SET                               0x00020000
#define CM_SDCCTL_UPDATE_CLR                               0xfffdffff
#define CM_SDCDIV 0x7e1011ac
#define CM_SDCDIV_DIV_LSB                                  12
#define CM_SDCCTL_CTRL_SET                                 0x0000f000

#define SD_CS   0x7ee00000
#define SD_CS_RESTRT_SET                                   0x00000001
#define SD_CS_EN_SET                                       0x00000002
#define SD_CS_DPD_SET                                      0x00000004
#define SD_CS_STBY_SET                                     0x00000008
#define SD_CS_STATEN_SET                                   0x00000040
#define SD_CS_STOP_SET                                     0x00000080
#define SD_CS_ASHDN_T_LSB                                  19
#define SD_SA   0x7ee00004
#define SD_SA_POWSAVE_SET                                  0x00000001
#define SD_SA_CLKSTOP_SET                                  0x00000080
#define SD_SA_PGEHLDE_SET                                  0x00000100
#define SD_SA_RFSH_T_LSB                                   16
#define SD_SB   0x7ee00008
#define SD_SB_COLBITS_LSB                                  0
#define SD_SB_ROWBITS_LSB                                  2
#define SD_SB_BANKLOW_LSB                                  5
#define SD_SB_EIGHTBANK_SET                                0x00000010
#define SD_SB_REORDER_SET                                  0x00000080
#define SD_SC   0x7ee0000c
#define SD_SC_WL_LSB                                       0
#define SD_SC_T_WTR_LSB                                    4
#define SD_SC_T_WR_LSB                                     8
#define SD_SC_T_RRD_LSB                                    20
#define SD_SC_T_RFC_LSB                                    24
#define SD_PT2  0x7ee00010
#define SD_PT2_T_INIT5_LSB                                 0
#define SD_PT1  0x7ee00014
#define SD_PT1_T_INIT1_LSB                                 0
#define SD_PT1_T_INIT3_LSB                                 8
#define SD_PHYC 0x7ee00060
#define SD_PHYC_PHYRST_SET                                 0x00000001
#define SD_MRT  0x7ee00064
#define SD_MRT_T_MRW_LSB                                   0
#define SD_MR   0x7ee00090
#define SD_SD   0x7ee00094
#define SD_SD_T_RCD_LSB                                    0
#define SD_SD_T_RPpb_LSB                                   4
#define SD_SD_T_RAS_LSB                                    8
#define SD_SD_T_XP_LSB                                     16
#define SD_SD_T_RC_LSB                                     20
#define SD_SD_T_RPab_LSB                                   28
#define SD_SE   0x7ee00098
#define SD_SE_T_XSR_LSB                                    0
#define SD_SE_T_RTP_LSB                                    8
#define SD_SE_T_FAW_LSB                                    12
#define SD_SE_RL_LSB                                       20
#define SD_SE_RL_EN_LSB                                    28

#define APHY_CSR_GLBL_ADDR_DLL_RESET    0x7ee06004
#define APHY_CSR_GLBL_ADR_DLL_LOCK_STAT   0x7ee06020
#define APHY_CSR_DDR_PLL_GLOBAL_RESET   0x7ee06024
#define APHY_CSR_DDR_PLL_POST_DIV_RESET 0x7ee06028
#define APHY_CSR_DDR_PLL_VCO_FREQ_CNTRL0  0x7ee0602c
#define APHY_CSR_DDR_PLL_VCO_FREQ_CNTRL1  0x7ee06030
#define APHY_CSR_DDR_PLL_MDIV_VALUE       0x7ee06034
#define APHY_CSR_DDR_PLL_LOCK_STATUS      0x7ee06048
#define APHY_CSR_DDR_PLL_PWRDWN         0x7ee06058
#define APHY_CSR_ADDR_PAD_DRV_SLEW_CTRL 0x7ee06068
#define APHY_CSR_ADDR_PVT_COMP_CTRL     0x7ee06070
#define APHY_CSR_ADDR_PVT_COMP_STATUS   0x7ee06078
#define APHY_CSR_PHY_BIST_CNTRL_SPR     0x7ee06080

#define DPHY_CSR_GLBL_DQ_DLL_RESET        0x7ee07004
#define DPHY_CSR_GLBL_MSTR_DLL_LOCK_STAT  0x7ee07018
#define DPHY_CSR_BOOT_READ_DQS_GATE_CTRL  0x7ee07040
#define DPHY_CSR_DQ_PHY_MISC_CTRL         0x7ee07048
#define DPHY_CSR_DQ_PAD_DRV_SLEW_CTRL     0x7ee0704c
#define DPHY_CSR_DQ_PAD_MISC_CTRL         0x7ee07050
#define DPHY_CSR_DQ_PVT_COMP_CTRL         0x7ee07054
#define DPHY_CSR_DQ_PVT_COMP_STATUS       0x7ee0705c

#define A2W_PASSWORD                                             0x5a000000

#define A2W_XOSC_CTRLR                                           HW_REGISTER_RW( 0x7e102990 )
#define A2W_SMPS_LDO0                                            0x7e1020d0
#define A2W_SMPS_LDO1 0x7e1020d4

// copied from rpi-open-firmware
#define LPDDR2_MR_DEVICE_FEATURE_2 2
#define LPDDR2_MR_IO_CONFIG        3
#define LPDDR2_MR_MANUFACTURER_ID  5
#define LPDDR2_MR_METRICS          8
#define LPDDR2_MR_CALIBRATION      10
