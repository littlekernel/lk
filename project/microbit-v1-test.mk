# A basic project for the BBC micro:bit v1 (cortex-m0, 16k of RAM). Boots and
# drops into a shell, which is about all the 16k has room for. Mostly useful as
# a smoke test of the cortex-m0 build.
# Boot it with:
#   scripts/do-qemuarm -0
# or by hand:
#   qemu-system-arm -M microbit -kernel build-microbit-v1-test/lk.elf -nographic
MODULES += \
	app/shell \

include project/target/microbit-v1.mk
