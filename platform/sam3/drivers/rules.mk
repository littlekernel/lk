LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += \
	-I$(LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/pio/pio.o \
	$(LOCAL_DIR)/pmc/pmc.o \
	$(LOCAL_DIR)/tc/tc.o \
	$(LOCAL_DIR)/uart/uart.o \
	$(LOCAL_DIR)/wdt/wdt.o \

