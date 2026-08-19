This directory contains patches made to ARM/CMSIS for LK compatibility reasons.

Current import: ARM-software/CMSIS_5 release tag **5.9.0** (CMSIS-Core(M) 5.6),
`CMSIS/Core/Include` only.

`systick_nvic.patch` is the diff of the LK-local changes against that stock drop,
regenerated whenever CMSIS is re-imported. It covers two things:

- `SysTick_Config()` in every `core_*.h` sets the systick priority to max. That
  line is commented out because `arch/arm/arm-m/arch.c` sets the priority to
  medium so systick competes fairly with the rest of the exceptions in the
  system. The TrustZone variant `TZ_SysTick_Config_NS()` is left alone; it is
  gated on `__ARM_FEATURE_CMSE == 3` and is not compiled in any LK build.
- `cmsis_gcc.h` includes `<lk/compiler.h>` first, to avoid redundant definitions
  of macros the two headers share.

To re-import: drop the new stock `CMSIS/Core/Include` over `../Include` as one
commit, reapply the two changes above as a second commit, and regenerate this
patch from that second commit's diff.
