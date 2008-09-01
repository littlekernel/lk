LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/smc91c96.o

