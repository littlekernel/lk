LOCAL_DIR := $(GET_LOCAL_DIR)

ASM_STRING_OPS := bcopy bzero memcpy memmove memset

OBJS += \
	$(LOCAL_DIR)/memcpy.o \
	$(LOCAL_DIR)/memset.o

# filter out the C implementation
C_STRING_OPS := $(filter-out $(ASM_STRING_OPS),$(C_STRING_OPS))

