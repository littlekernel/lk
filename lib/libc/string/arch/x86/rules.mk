LOCAL_DIR := $(GET_LOCAL_DIR)

ASM_STRING_OPS := #bcopy bzero memcpy memmove memset

MODULE_SRCS += \
	#$(LOCAL_DIR)/memcpy.S \
	#$(LOCAL_DIR)/memset.S

# filter out the C implementation
C_STRING_OPS := $(filter-out $(ASM_STRING_OPS),$(C_STRING_OPS))

