/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY
#include <sys/types.h> // for status_t
#endif

#define NO_ERROR                (0)
#define ERR_GENERIC             (-1)
#define ERR_NOT_FOUND           (-2)
#define ERR_NOT_READY           (-3)
#define ERR_NO_MSG              (-4)
#define ERR_NO_MEMORY           (-5)
#define ERR_ALREADY_STARTED     (-6)
#define ERR_NOT_VALID           (-7)
#define ERR_INVALID_ARGS        (-8)
#define ERR_NOT_ENOUGH_BUFFER   (-9)
#define ERR_NOT_SUSPENDED       (-10)
#define ERR_OBJECT_DESTROYED    (-11)
#define ERR_NOT_BLOCKED         (-12)
#define ERR_TIMED_OUT           (-13)
#define ERR_ALREADY_EXISTS      (-14)
#define ERR_CHANNEL_CLOSED      (-15)
#define ERR_OFFLINE             (-16)
#define ERR_NOT_ALLOWED         (-17)
#define ERR_BAD_PATH            (-18)
#define ERR_ALREADY_MOUNTED     (-19)
#define ERR_IO                  (-20)
#define ERR_NOT_DIR             (-21)
#define ERR_NOT_FILE            (-22)
#define ERR_RECURSE_TOO_DEEP    (-23)
#define ERR_NOT_SUPPORTED       (-24)
#define ERR_TOO_BIG             (-25)
#define ERR_CANCELLED           (-26)
#define ERR_NOT_IMPLEMENTED     (-27)
#define ERR_CHECKSUM_FAIL       (-28)
#define ERR_CRC_FAIL            (-29)
#define ERR_CMD_UNKNOWN         (-30)
#define ERR_BAD_STATE           (-31)
#define ERR_BAD_LEN             (-32)
#define ERR_BUSY                (-33)
#define ERR_THREAD_DETACHED     (-34)
#define ERR_I2C_NACK            (-35)
#define ERR_ALREADY_EXPIRED     (-36)
#define ERR_OUT_OF_RANGE        (-37)
#define ERR_NOT_CONFIGURED      (-38)
#define ERR_NOT_MOUNTED         (-39)
#define ERR_FAULT               (-40)
#define ERR_NO_RESOURCES        (-41)
#define ERR_BAD_HANDLE          (-42)
#define ERR_ACCESS_DENIED       (-43)
#define ERR_PARTIAL_WRITE       (-44)

#define ERR_USER_BASE           (-16384)
