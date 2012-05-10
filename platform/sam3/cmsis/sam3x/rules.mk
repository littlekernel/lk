LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include
INCLUDES += -I$(LOCAL_DIR)/source/templates

OBJS += \
	$(LOCAL_DIR)/source/templates/system_sam3x.o \
	$(LOCAL_DIR)/source/templates/exceptions.o

