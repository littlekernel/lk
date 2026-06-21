LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS := \
    $(LOCAL_DIR)/uacpi_lk.c \
    $(wildcard $(LOCAL_DIR)/source/*.c)

# uACPI uses some constructs that may trigger warnings in strict builds
MODULE_COMPILEFLAGS := \
    -Wno-unused-parameter \
    -Wno-missing-declarations \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-type-limits \
    -Wno-shadow \
    -Wno-strict-prototypes \
    -Wno-double-promotion \
    -Wno-discarded-qualifiers

include make/module.mk
