ST CMSIS device package for the STM32H5 family, trimmed to the STM32H563.

Imported from https://github.com/STMicroelectronics/cmsis-device-h5 at tag
**v1.7.0**. Only the pieces an LK build needs are carried here: the family
header, the H563 device header, and SystemInit/SystemCoreClockUpdate. The
startup files are not imported because LK supplies its own vector table in
`platform/stm32h5xx/vectab.c`, and `partition_stm32h5xx.h` is not imported
because it is the SAU/TrustZone configuration template, which a secure state
build with TrustZone disabled does not use.

`USE_HAL_DRIVER` is intentionally left undefined, so `stm32h5xx.h` does not
pull in the ST HAL. The HAL is not imported at all; `platform/stm32h5xx`
drivers are written against these register definitions directly, following
the way `platform/stm32f0xx` uses its CMSIS import.

Local changes against the stock drop, kept in their own commit so they show
up as a single diff:

- `system_stm32h5xx.c`: the vector table relocation at the end of
  `SystemInit()` is removed. It unconditionally wrote `SCB->VTOR`, which
  interferes with LK's management of the vector table. `platform/stm32f4xx`
  carries the same change to its vendor system file for the same reason.

To re-import: drop the new stock files over this directory as one commit,
reapply the change above as a second commit, and update this file.
