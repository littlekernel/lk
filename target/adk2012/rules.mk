LOCAL_DIR := $(GET_LOCAL_DIR)

SAM_CHIP := sam3x8h
PLATFORM := sam3

INCLUDES += -I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/init.o

