LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/proc_comm.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/smem.c \
	$(LOCAL_DIR)/smem_ptable.c \
	$(LOCAL_DIR)/hsusb.c \
	$(LOCAL_DIR)/nand.c

