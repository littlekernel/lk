LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += TARGET_HAS_DEBUG_LED=1 CRYSTAL=54000000

PLATFORM := bcm28xx
ARCH := vpu

ifeq ($(BOOTCODE),1)
else
  MODULES += platform/bcm28xx/start4
endif

#include make/module.mk

