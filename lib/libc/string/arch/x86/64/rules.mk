LOCAL_DIR := $(GET_LOCAL_DIR)

ASM_STRING_OPS := memcpy memset

MODULE_SRCS += \
	$(LOCAL_DIR)/memcpy.c \
	$(LOCAL_DIR)/memset.c

# filter out the C implementation
C_STRING_OPS := $(filter-out $(ASM_STRING_OPS),$(C_STRING_OPS))
