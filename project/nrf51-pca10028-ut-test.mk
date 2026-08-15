# A minimal unit test project for the nrf51 (cortex-m0), small enough to fit in
# the 16k of RAM the qemu 'microbit' machine models. Boot it with:
#   qemu-system-arm -M microbit -kernel build-nrf51-pca10028-ut-test/lk.elf -nographic
MODULES += \
	app/shell \
	lib/unittest \

WITH_TESTS := true
GLOBAL_DEFINES += WITH_TESTS=1

# the 16k part, which is what the microbit machine models
NRF51_CHIP := nrf51822-qfaa

include project/target/nrf-pca10028.mk
