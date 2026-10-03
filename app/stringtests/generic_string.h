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

__BEGIN_CDECLS

// The portable C string routines from lib/libc/string, under their own names so
// they stay in the image after an architecture replaces the libc versions.
void *generic_memcpy(void *dest, const void *src, size_t count);
void *generic_memmove(void *dest, const void *src, size_t count);
void *generic_memset(void *s, int c, size_t count);
int generic_memcmp(const void *cs, const void *ct, size_t count);
void *generic_memchr(const void *buf, int c, size_t len);
size_t generic_strlen(const char *s);
size_t generic_strnlen(const char *s, size_t count);
int generic_strcmp(const char *cs, const char *ct);
int generic_strncmp(const char *cs, const char *ct, size_t count);
char *generic_strchr(const char *s, int c);
char *generic_strrchr(const char *s, int c);
char *generic_strcpy(char *dest, const char *src);

__END_CDECLS
