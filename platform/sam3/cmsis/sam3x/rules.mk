LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include
INCLUDES += -I$(LOCAL_DIR)/source/templates

MODULE_SRCS += \
	$(LOCAL_DIR)/source/templates/system_sam3x.c \
	$(LOCAL_DIR)/source/templates/exceptions.c

