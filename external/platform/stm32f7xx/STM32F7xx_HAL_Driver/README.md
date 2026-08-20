ST STM32Cube HAL for the STM32F7 family, plus the CMSIS device headers under
`CMSIS/`.

**This tree is a hard fork and cannot be diffed or merged against a stock ST
drop.** Read this before attempting to update it.

## Version

STM32Cube_FW_F7 **V1.0.1, 25-June-2015** — the first F7 Cube release. ST has
long since moved on (1.17.x at the time of writing), so this is roughly a
decade behind. The CMSIS device headers here cover only `stm32f745xx`,
`stm32f746xx` and `stm32f756xx`; there is no F76x/F77x support.

## Why a re-import is expensive

- The entire vendor tree was reindented to LK style in `55679a481`
  ("mass reformat ST's code", 135 files, ~141k lines changed). Nothing here
  matches ST's formatting any more, so a stock drop produces a whole-tree
  diff and there is no useful three way merge.
- Several independent functional changes are carried in-tree on top of that:
  - `Inc/stm32f7xx_hal_qspi.h` / `Src/stm32f7xx_hal_qspi.c`: the
    `QSPI_CommandTypeDef` parameters were made const and the
    `HAL_QSPI_AutoPolling*` contract was changed so the caller sets `NbData`
    (`05cd5e979`, also kept as `platform/stm32f7xx/patch/qspi_const.patch`),
    plus tracing and a controller reset (`7d06370f7`).
  - `lk_hal.c`: an LK-authored file inside the vendor tree that overrides
    `HAL_GetTick`/`HAL_Delay` in terms of LK's `current_time()`.
  - `Inc/stm32f7xx_hal_uart.h`: overrun handling (`4895ead73`).
  - `Inc/stm32f7xx_hal_conf.h`: ETH phy configuration stripped (`45a76ea6c`).
  - `Inc/stm32f7xx_hal_nand.h`, `Inc/stm32f7xx_hal_rcc.h`: warning fixes.
  - `CMSIS/stm32f7xx.h`: `USE_HAL_DRIVER` is left defined, unlike the F0 and
    F4 imports, so the device header pulls in the whole HAL.
- Roughly 300 HAL call sites depend on it: `platform/stm32f7xx` (notably
  `qspi.c`, which is written against the *modified* QSPI API) and the three
  targets that use this platform (`stm32f746g-disco`, `stm32746g-eval2`,
  `dartuinoP0`).

## Policy

Frozen. It builds and boots, and the projects using it are in CI. Updating it
means re-deriving every change above by hand and porting `qspi.c` back to the
stock QSPI signatures — treat that as its own project, and only take it on if
a new feature actually needs a newer HAL (SDMMC is the likely trigger, since
no STM32 platform in the tree has an SD driver).

Newer STM32 ports should not copy this arrangement. `platform/stm32f0xx` and
`platform/stm32h5xx` show the alternative: import the CMSIS device headers,
leave `USE_HAL_DRIVER` undefined, and write the drivers against the registers.
