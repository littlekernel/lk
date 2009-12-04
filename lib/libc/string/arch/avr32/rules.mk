LOCAL_DIR := $(GET_LOCAL_DIR)

ASM_STRING_OPS := 

OBJS += \

# filter out the C implementation
C_STRING_OPS := $(filter-out $(ASM_STRING_OPS),$(C_STRING_OPS))

