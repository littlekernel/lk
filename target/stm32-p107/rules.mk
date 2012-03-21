LOCAL_DIR := $(GET_LOCAL_DIR)

STM32_CHIP := stm32f107

PLATFORM := stm32f1xx

INCLUDES += -I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/init.o

