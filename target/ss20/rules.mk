LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := sun4m

MEMBASE ?= 0x00000000
MEMSIZE ?= 0x01f00000 # limit to just under 32MB to allow page tables to sticka round by firmware
