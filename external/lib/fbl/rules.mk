# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := lib/libcpp

MODULE_SRCS += \
    $(LOCAL_DIR)/all.cpp \
    $(LOCAL_DIR)/alloc_checker.cpp \
    $(LOCAL_DIR)/string_buffer.cpp \
    $(LOCAL_DIR)/string_piece.cpp \
    $(LOCAL_DIR)/string_printf.cpp \
    $(LOCAL_DIR)/string.cpp

include make/module.mk
