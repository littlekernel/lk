# top level project rules for the aboot-surf7k project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := surf-msm7k

MODULES += app/aboot

#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_UART=1
DEFINES += WITH_DEBUG_FBCON=1