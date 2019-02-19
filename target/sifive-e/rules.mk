LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := sifive
VARIANT := sifive_e

MEMSIZE ?= 0x4000     # 16KB
GLOBAL_DEFINES += TARGET_HAS_DEBUG_LED=1

# target code will set the master frequency to 16Mhz
GLOBAL_DEFINES += SIFIVE_FREQ=16000000

MODULE_SRCS := $(LOCAL_DIR)/target.c

include make/module.mk

