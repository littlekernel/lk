LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_CFLAGS += -Wno-unused-variable -Wno-sign-compare

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/k_sin.c \
        $(LOCAL_DIR)/s_sin.c \
        $(LOCAL_DIR)/s_sinf.c \
        $(LOCAL_DIR)/k_cos.c \
        $(LOCAL_DIR)/s_cos.c \
        $(LOCAL_DIR)/s_cosf.c \
        $(LOCAL_DIR)/k_tan.c \
        $(LOCAL_DIR)/s_tan.c \
        $(LOCAL_DIR)/s_tanf.c \
        $(LOCAL_DIR)/e_sqrt.c \
        $(LOCAL_DIR)/e_sqrtf.c \
        $(LOCAL_DIR)/k_rem_pio2.c \
        $(LOCAL_DIR)/s_floor.c \
        $(LOCAL_DIR)/s_scalbn.c \
        $(LOCAL_DIR)/s_copysign.c

include make/module.mk
