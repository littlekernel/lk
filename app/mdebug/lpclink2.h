
// gpio configuration for JTAG
#define PIN_LED		PIN(1,1)
#define PIN_RESET	PIN(2,5)
#define PIN_RESET_TXEN	PIN(2,6)
#define PIN_TMS_TXEN	PIN(1,5)	// SGPIO15=6
#define PIN_TMS		PIN(1,6)	// SGPIO14=6
#define PIN_TCK		PIN(1,17)	// SGPIO11=6
#define PIN_TDI		PIN(1,18)	// SGPIO12=6
#define PIN_TDO		PIN(1,14)	// U1_RXD=1, SGPIO10=6

#define GPIO_LED	GPIO(0,8)
#define GPIO_RESET	GPIO(5,5)
#define GPIO_RESET_TXEN	GPIO(5,6)
#define GPIO_TMS_TXEN	GPIO(1,8)
#define GPIO_TMS	GPIO(1,9)
#define GPIO_TCK	GPIO(0,12)
#define GPIO_TDI	GPIO(0,13)
#define GPIO_TDO	GPIO(1,7)

// alternate names for SWD
#define PIN_SWDIO_TXEN	PIN_TMS_TXEN
#define PIN_SWDIO	PIN_TMS
#define PIN_SWCLK	PIN_TCK
#define PIN_SWO		PIN_TDO

#define GPIO_SWDIO_TXEN	GPIO_TMS_TXEN
#define GPIO_SWDIO	GPIO_TMS
#define GPIO_SWCLK	GPIO_TCK

