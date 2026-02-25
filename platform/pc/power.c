/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86.h>
#include <lk/debug.h>
#include <platform.h>
#include <platform/keyboard.h>

/*
 * 0xCF9: Reset Control Register (RCR). Common on PCI/ISA chipsets and modern
 * Intel PCH systems; writing 0x02 then 0x06 requests a hard reset.
 * See Intel PCH datasheets (Reset Control Register / CF9h).
 */
#define RESET_CTRL_PORT 0xcf9

/*
 * 0x92: System Control Port A ("Fast A20/Init"). Bit 0 is often wired as a
 * fast reset request on PC-compatible systems.
 */
#define FAST_RESET_PORT 0x92

/*
 * ACPI PM1 control ports used by popular emulators/virtual firmware wiring:
 * - 0x604: QEMU's commonly used PM1a control port for soft poweroff
 * - 0xB004: Bochs/older QEMU compatibility port
 * ACPI details are in the ACPI specification (PM1 control + SLP_EN).
 */
#define QEMU_ACPI_PM1A_CNT_PORT  0x604
#define BOCHS_ACPI_PM1A_CNT_PORT 0xb004
#define ACPI_SLP_EN              (1u << 13)

static void pc_reboot(void) {
    platform_i8042_keyboard_reset();

    outp(RESET_CTRL_PORT, 0x02);
    outp(RESET_CTRL_PORT, 0x06);

    outp(FAST_RESET_PORT, (uint8_t)(inp(FAST_RESET_PORT) | 0x01));
}

static void pc_shutdown(void) {
    outpw(QEMU_ACPI_PM1A_CNT_PORT, ACPI_SLP_EN);
    outpw(BOCHS_ACPI_PM1A_CNT_PORT, ACPI_SLP_EN);
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    dprintf(INFO, "pc: halt action %s, reason %s\n",
            platform_halt_action_string(suggested_action),
            platform_halt_reason_string(reason));
    platform_halt_default(suggested_action, reason, &pc_reboot, &pc_shutdown);
}