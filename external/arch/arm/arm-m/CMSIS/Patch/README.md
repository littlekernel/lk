This directory contains patches made to ARM/CMSIS for LK compatibility reasons.

Current import: ARM-software/CMSIS_5 release tag **5.9.0** (CMSIS-Core(M) 5.6),
`CMSIS/Core/Include` only.

`systick_nvic.patch` is the diff of the LK-local changes against that stock drop,
regenerated whenever CMSIS is re-imported. It covers three things:

- `SysTick_Config()` in every `core_*.h` sets the systick priority to max. That
  line is commented out because `arch/arm/arm-m/arch.c` sets the priority to
  medium so systick competes fairly with the rest of the exceptions in the
  system. The TrustZone variant `TZ_SysTick_Config_NS()` is left alone; it is
  gated on `__ARM_FEATURE_CMSE == 3` and is not compiled in any LK build.
- `cmsis_gcc.h` includes `<lk/compiler.h>` first, to avoid redundant definitions
  of macros the two headers share. `cmsis_armclang.h` does the same, for the
  same reason, now that plain clang reaches it.
- `cmsis_compiler.h` routes plain clang (`__clang__` without `__ARMCC_VERSION`)
  to `cmsis_armclang.h` rather than letting it fall through to `cmsis_gcc.h` on
  the strength of `__GNUC__`. 5.9.0's `cmsis_gcc.h` guards its Armv8-M Mainline
  blocks on `__ARM_ARCH_8M_MAIN__` alone, which gcc defines for Cortex-M55 and
  M85 but clang does not -- clang defines the more precise ACLE macro
  `__ARM_ARCH_8_1M_MAIN__`, so those blocks vanished and `__set_BASEPRI` and
  friends went undeclared. `cmsis_armclang.h` already tests both macros
  everywhere and carries no armclang-only constructs. Upstream reached the same
  conclusion in CMSIS 6, which ships a dedicated `cmsis_clang.h`; drop this hunk
  if the import is ever moved to 6.

To re-import: drop the new stock `CMSIS/Core/Include` over `../Include` as one
commit, reapply the three changes above as a second commit, and regenerate this
patch from that second commit's diff:

    git diff <stock-import-commit> -- external/arch/arm/arm-m/CMSIS/Include \
        > external/arch/arm/arm-m/CMSIS/Patch/systick_nvic.patch
