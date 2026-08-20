/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stddef.h>
#include <sys/types.h>

__BEGIN_CDECLS

/*
 * ARM semihosting: a call into the debugger or emulator hosting the system.
 * Under qemu these are enabled with -semihosting-config; on real hardware a
 * debugger has to be attached and listening. If nobody is handling the call
 * the trapping instruction raises a debug exception instead, which on most
 * targets lands in the hard fault handler, so only call these on a system
 * that is known to be hosted.
 */

/* operation numbers, from the ARM semihosting spec */
#define SEMIHOST_SYS_WRITE0      0x04
#define SEMIHOST_SYS_READC       0x07
#define SEMIHOST_SYS_GET_CMDLINE 0x15
#define SEMIHOST_SYS_EXIT        0x18

/* reason codes for SEMIHOST_SYS_EXIT */
#define ADP_STOPPED_APPLICATION_EXIT 0x20026
#define ADP_STOPPED_RUN_TIME_ERROR   0x20023

/*
 * Raw semihosting call. op is the operation number, param is the operation
 * specific argument, which is usually a pointer to a block of arguments but
 * for a few operations is an immediate value.
 */
long semihosting_call(ulong op, void *param);

/* Write a null terminated string to the host's console. */
void semihosting_puts(const char *str);

/*
 * Retrieve the command line the host was started with. On qemu this is the
 * space separated list of arg= values passed to -semihosting-config.
 *
 * Copies at most len bytes, including the null terminator, into buf.
 * Returns the length of the string not counting the terminator, or
 * ERR_NOT_ENOUGH_BUFFER if it does not fit.
 */
ssize_t semihosting_get_cmdline(char *buf, size_t len);

/*
 * Ask the host to terminate the system. Passing ADP_STOPPED_APPLICATION_EXIT
 * exits qemu with status 0, any other reason code exits with status 1.
 * Does not return if the host is listening.
 */
void semihosting_exit(ulong reason);

__END_CDECLS
