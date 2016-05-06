LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# MODULE_DEPS := \

MODULE_SRCS += \
  $(LOCAL_DIR)/ndebug.c \
  $(LOCAL_DIR)/system/consoleproxy.c \
  $(LOCAL_DIR)/system/mux.c \
  $(LOCAL_DIR)/user.c \


include make/module.mk
