/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <printf.h>
#include <sys/types.h>
#include <lib/io.h>
#if defined(WITH_LIB_FS)
#include <lib/fs.h>
#endif // WITH_LIB_FS

__BEGIN_CDECLS

#if defined(WITH_LIB_FS)
struct fs_handle {
    filehandle *handle;
    off_t offset;
    bool readonly;
};
#endif // WITH_LIB_FS
typedef struct FILE {
#if defined(WITH_LIB_FS)
    union {
        io_handle_t *io;
        struct fs_handle fs_handle;
    };
    bool use_fs;
#else
    io_handle_t *io;
#endif // WITH_LIB_FS
} FILE;

extern FILE __stdio_FILEs[];

#define stdin  (&__stdio_FILEs[0])
#define stdout (&__stdio_FILEs[1])
#define stderr (&__stdio_FILEs[2])

#define EOF (-1)

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int fflush(FILE *stream);
int feof(FILE *stream);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);

int fputc(int c, FILE *fp);
#define putc(c, fp) fputc(c, fp)
int putchar(int c);

int fputs(const char *s, FILE *fp);
int puts(const char *str);

int fgetc(FILE *fp);
#define getc(fp) fgetc(fp)
int getchar(void);

char *fgets(char *s, int size, FILE *stream);

#if !DISABLE_DEBUG_OUTPUT
int printf(const char *fmt, ...) __PRINTFLIKE(1, 2);
int vprintf(const char *fmt, va_list ap);
#else
static inline int __PRINTFLIKE(1, 2) printf(const char *fmt, ...) { return 0; }
static inline int vprintf(const char *fmt, va_list ap) { return 0; }
#endif

int fprintf(FILE *fp, const char *fmt, ...) __PRINTFLIKE(2, 3);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int _fprintf_output_func(const char *str, size_t len, void *state);

int sprintf(char *str, const char *fmt, ...) __PRINTFLIKE(2, 3);
int snprintf(char *str, size_t len, const char *fmt, ...) __PRINTFLIKE(3, 4);
int vsprintf(char *str, const char *fmt, va_list ap);
int vsnprintf(char *str, size_t len, const char *fmt, va_list ap);


__END_CDECLS

