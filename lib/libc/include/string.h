/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

void *memchr (void const *, int, size_t) __PURE;
int   memcmp (void const *, const void *, size_t) __PURE;
void *memcpy (void *, void const *, size_t);
void *memmove(void *, void const *, size_t);
void *memset (void *, int, size_t);

char       *strcat(char *, char const *);
char       *strchr(char const *, int) __PURE;
int         strcmp(char const *, char const *) __PURE;
int         strcasecmp(char const *, char const *) __PURE;
char       *strcpy(char *, char const *);
char       *strerror(int) __CONST;
size_t      strlen(char const *) __PURE;
char       *strncat(char *, char const *, size_t);
int         strncmp(char const *, char const *, size_t) __PURE;
char       *strncpy(char *, char const *, size_t);
char       *strpbrk(char const *, char const *) __PURE;
char       *strrchr(char const *, int) __PURE;
size_t      strspn(char const *, char const *) __PURE;
size_t      strcspn(const char *s, const char *) __PURE;
char       *strstr(char const *, char const *) __PURE;
char       *strtok(char *, char const *);
int         strcoll(const char *s1, const char *s2) __PURE;
size_t      strxfrm(char *dest, const char *src, size_t n) __PURE;
char       *strdup(const char *str) __MALLOC;

/* non standard */
void   bcopy(void const *, void *, size_t);
void   bzero(void *, size_t);
size_t strlcat(char *, char const *, size_t);
size_t strlcpy(char *, char const *, size_t);
int    strncasecmp(char const *, char const *, size_t)  __PURE;
int    strnicmp(char const *, char const *, size_t) __PURE;
size_t strnlen(char const *s, size_t count) __PURE;

__END_CDECLS

