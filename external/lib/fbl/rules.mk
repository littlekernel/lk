# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

#
# Userspace library.
#

MODULE := $(LOCAL_DIR)

#MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/alloc_checker.cpp \
    $(LOCAL_DIR)/string_buffer.cpp \
    $(LOCAL_DIR)/string_piece.cpp \
    $(LOCAL_DIR)/string_printf.cpp \
    $(LOCAL_DIR)/string.cpp

#MODULE_PACKAGE := src

include make/module.mk
