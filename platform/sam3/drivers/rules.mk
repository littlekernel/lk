LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/pio/pio.c \
	$(LOCAL_DIR)/pmc/pmc.c \
	$(LOCAL_DIR)/tc/tc.c \
	$(LOCAL_DIR)/uart/uart.c \
	$(LOCAL_DIR)/wdt/wdt.c \

