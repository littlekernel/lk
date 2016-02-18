LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/fdt.c \
    $(LOCAL_DIR)/fdt_addresses.c \
    $(LOCAL_DIR)/fdt_empty_tree.c \
    $(LOCAL_DIR)/fdt_ro.c \
    $(LOCAL_DIR)/fdt_rw.c \
    $(LOCAL_DIR)/fdt_strerror.c \
    $(LOCAL_DIR)/fdt_sw.c \
    $(LOCAL_DIR)/fdt_wip.c

MODULE_COMPILEFLAGS += -Wno-sign-compare

include make/module.mk
