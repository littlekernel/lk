#define CM_PLLB                 0x7e101170
#define CM_PLLB_LOADARM_SET                                0x00000001
#define CM_PLLB_HOLDARM_SET                                0x00000002
#define CM_PLLB_ANARST_SET                                 0x00000100
#define CM_PLLB_DIGRST_SET                                 0x00000200
#define CM_ARMCTL               0x7e1011b0
#define CM_ARMCTL_ENAB_SET                                 0x00000010

#define A2W_XOSC_CTRL           0x7e102190
#define A2W_XOSC_CTRL_DDREN_SET                            0x00000010
#define A2W_XOSC_CTRL_PLLAEN_SET                           0x00000040
#define A2W_XOSC_CTRL_PLLBEN_SET                           0x00000080

#define A2W_BASE                (BCM_PERIPH_BASE_VIRT + 0x102000)

#define A2W_PASSWORD                                             0x5a000000
#define A2W_PLLA_FRAC_MASK                                    0x000fffff
#define A2W_PLLB_FRAC_MASK                                    0x000fffff
#define A2W_PLLC_FRAC_MASK                                    0x000fffff
#define A2W_PLLD_FRAC_MASK                                    0x000fffff
#define A2W_PLLH_FRAC_MASK                                    0x000fffff

#define A2W_PLLA_ANA0           (A2W_BASE + 0x010)
#define A2W_PLLC_ANA0           (A2W_BASE + 0x030)
#define A2W_PLLD_ANA0           (A2W_BASE + 0x050)
#define A2W_PLLH_ANA0           (A2W_BASE + 0x070)
#define A2W_PLLB_ANA0           (A2W_BASE + 0x0f0)

#define A2W_PLLA_FRAC           (A2W_BASE + 0x200)
#define A2W_PLLC_FRAC           (A2W_BASE + 0x220)
#define A2W_PLLD_FRAC           (A2W_BASE + 0x240)
#define A2W_PLLH_FRAC           (A2W_BASE + 0x260)
#define A2W_PLLB_FRAC           (A2W_BASE + 0x2e0)
#define A2W_PLLB_ARM            0x7e1023e0
#define A2W_PLLC_CORE1          (A2W_BASE + 0x420)
#define A2W_PLLC_CORE0          (A2W_BASE + 0x620)
#define A2W_PLLC_CORE0_DIV_SET                             0x000000ff

#define A2W_PLLA_CTRL           (A2W_BASE + 0x100)
#define A2W_PLLA_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLA_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLA_CTRL_PDIV_LSB                             12
#define A2W_PLLC_CTRL           (A2W_BASE + 0x120)
#define A2W_PLLC_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLC_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLC_CTRL_PDIV_LSB                             12
#define A2W_PLLD_CTRL           (A2W_BASE + 0x140)
#define A2W_PLLD_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLD_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLD_CTRL_PDIV_LSB                             12
#define A2W_PLLH_CTRL           (A2W_BASE + 0x160)
#define A2W_PLLH_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLH_CTRL_NDIV_SET                             0x000000ff
#define A2W_PLLH_CTRL_PDIV_LSB                             12
#define A2W_PLLB_CTRL           (A2W_BASE + 0x1e0)
#define A2W_PLLB_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLB_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLB_CTRL_PDIV_LSB                             12
