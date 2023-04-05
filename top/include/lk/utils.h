/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2020-2023 Google LLC.
 */

#pragma once

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(*(_a)))

#define KB (1024UL)
#define MB (1024UL * 1024UL)
#define GB (1024UL * 1024UL * 1024UL)

#define RETURN_IF_ERROR(expr) \
        do { \
            status_t err_ = (expr); \
            if (err_ != NO_ERROR) { \
                return err_; \
            } \
        } while (0)
