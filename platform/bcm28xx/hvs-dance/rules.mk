LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	platform/bcm28xx/hvs

MODULE_SRCS += \
	$(LOCAL_DIR)/dance.c

$(BUILDDIR)/$(LOCAL_DIR)/pi-logo.h: $(BUILDDIR)/$(LOCAL_DIR)/pi-logo.tga
	cat $< | tools/bin2h.py --before 'uint8_t pilogo[] = {' --after '};' > $@

$(BUILDDIR)/$(LOCAL_DIR)/pi-logo.tga: $(LOCAL_DIR)/RPi-Logo-Reg-SCREEN.png
	convert $< -compress RLE $@

GLOBAL_INCLUDES += $(BUILDDIR)/$(LOCAL_DIR)

include make/module.mk
