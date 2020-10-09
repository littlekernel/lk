#pragma once

#include <platform/bcm28xx.h>
#include <lk/console_cmd.h>

int cmd_dpi_start(int argc, const cmd_args *argv);
int cmd_dpi_move(int argc, const cmd_args *argv);
int cmd_dpi_count(int argc, const cmd_args *argv);

#define DPI_C (BCM_PERIPH_BASE_VIRT + 0x208000)

#define BIT(b) (1 << b)

#define FORMAT(n) ((n & 0x7) << 11)
#define ORDER(n) ((n & 0x3) << 14)

// copied from linux/drivers/gpu/drm/vc4/vc4_dpi.c
# define DPI_OUTPUT_ENABLE_MODE		BIT(16)
/* Reverses the polarity of the corresponding signal */
# define DPI_PIXEL_CLK_INVERT		BIT(10)
# define DPI_HSYNC_INVERT		BIT(9)
# define DPI_VSYNC_INVERT		BIT(8)
# define DPI_OUTPUT_ENABLE_INVERT	BIT(7)

/* Outputs the signal the falling clock edge instead of rising. */
# define DPI_HSYNC_NEGATE		BIT(6)
# define DPI_VSYNC_NEGATE		BIT(5)
# define DPI_OUTPUT_ENABLE_NEGATE	BIT(4)

/* Disables the signal */
# define DPI_HSYNC_DISABLE		BIT(3)
# define DPI_VSYNC_DISABLE		BIT(2)
# define DPI_OUTPUT_ENABLE_DISABLE	BIT(1)

/* Power gate to the device, full reset at 0 -> 1 transition */
# define DPI_ENABLE			BIT(0)
