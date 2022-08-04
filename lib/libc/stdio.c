/*
 * Copyright (c) 2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/err.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <platform/debug.h>

#if defined(WITH_LIB_FS)
#define STDIO_FIELDS .use_fs = false,
#else
#define STDIO_FIELDS
#endif // WITH_LIB_FS

#define DEFINE_STDIO_DESC(id)   \
    [(id)]  = {                 \
        .io = &console_io,      \
        STDIO_FIELDS            \
    }

FILE __stdio_FILEs[3] = {
    DEFINE_STDIO_DESC(0), /* stdin */
    DEFINE_STDIO_DESC(1), /* stdout */
    DEFINE_STDIO_DESC(2), /* stderr */
};
#undef DEFINE_STDIO_DESC

FILE *fopen(const char *filename, const char *mode) {
#if defined(WITH_LIB_FS)
    FILE *stream = (FILE *) malloc(sizeof(FILE));
    if (stream == NULL) {
        return NULL;
    }

    stream->use_fs = true;
    stream->fs_handle.offset = 0;
    stream->fs_handle.readonly = (!strchr(mode, 'w') && !strchr(mode, 'a'));

    status_t ret = fs_open_file(filename, &(stream->fs_handle.handle));

    if (ret == ERR_NOT_FOUND && !stream->fs_handle.readonly) {
        ret = fs_create_file(filename, &(stream->fs_handle.handle), 0);
    }

    if (ret != NO_ERROR) {
        free(stream);
        return NULL;
    }

    if (strchr(mode, 'a')) {
        struct file_stat stat;
        if (NO_ERROR == fs_stat_file(stream->fs_handle.handle, &stat)) {
            stream->fs_handle.offset = stat.size;
        }
    }

    return stream;
#endif // WITH_LIB_FS
    return NULL;
}

int fclose(FILE *stream) {
#if defined(WITH_LIB_FS)
    if (stream && !stream->use_fs) {
        fs_close_file(stream->fs_handle.handle);
        free(stream);
    }
#endif // WITH_LIB_FS
    return 0;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
#if defined(WITH_LIB_FS)
    if (stream->use_fs) {
        size_t rsize = fs_read_file(stream->fs_handle.handle, ptr,
                             stream->fs_handle.offset, size * count);
        stream->fs_handle.offset += rsize;
        return rsize / size;
    }
#endif // WITH_LIB_FS
    return io_read(stream->io, ptr, size * count) / size;
}

int fflush(FILE *stream) {
    return 0;
}

int feof(FILE *stream) {
#if defined(WITH_LIB_FS)
    if (!stream->use_fs) {
        return 0;
    }
    struct file_stat stat;
    if (NO_ERROR != fs_stat_file(stream->fs_handle.handle, &stat)) {
        return 1;
    }
    return (uint64_t)stream->fs_handle.offset >= stat.size;
#endif // WITH_LIB_FS
    return 0;
}

static inline off_t clamp(off_t val, off_t min, off_t max) {
    const off_t t = (val < min) ? min : val;
    return (t > max) ? max : t;
}

int fseek(FILE *stream, long offset, int whence) {
#if defined(WITH_LIB_FS)
    if (!stream->use_fs) {
        return 0;
    }
    struct file_stat stat;
    if (NO_ERROR != fs_stat_file(stream->fs_handle.handle, &stat)) {
        return -1;
    }

    switch (whence) {
        case SEEK_SET:
            stream->fs_handle.offset = clamp(offset, 0, stat.size);
            break;
        case SEEK_CUR:
            stream->fs_handle.offset = clamp(stream->fs_handle.offset + offset, 0, stat.size);
            break;
        case SEEK_END:
            stream->fs_handle.offset = clamp(stat.size - offset, 0, stat.size);
            break;
        default:
            return -1;
    }
#endif // WITH_LIB_FS
    return 0;
}

long ftell(FILE *stream) {
#if defined(WITH_LIB_FS)
    if (!stream->use_fs) {
        return 0;
    }
    return stream->fs_handle.offset;
#endif // WITH_LIB_FS
    return 0;
}


int fputc(int _c, FILE *fp) {
    unsigned char c = _c;
    return fwrite(&c, /*size=*/1, /*count=*/1, fp);
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char *str) {
    int err = fputs(str, stdout);
    if (err >= 0)
        err = fputc('\n', stdout);
    return err;
}

int fputs(const char *s, FILE *fp) {
    size_t len = strlen(s);
    return fwrite(s, /*size=*/1, /*count=*/len, fp);
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
    if (stream == stdin) {
        return 0;
    }

    if (stream == stdout || stream == stderr) {
        size_t bytes_written;

        if (size == 0 || count == 0)
            return 0;

        // fast path for size == 1
        if (likely(size == 1)) {
            return io_write(stream->io, ptr, count);
        }

        bytes_written = io_write(stream->io, ptr, size * count);
        return bytes_written / size;
    }

#if defined(WITH_LIB_FS)
    if (stream->fs_handle.readonly) {
        return 0;
    }

    size_t wsize = fs_write_file(stream->fs_handle.handle, ptr,
                                 stream->fs_handle.offset, size * count);
    stream->fs_handle.offset += wsize;

    return wsize / size;
#endif // WITH_LIB_FS
    return 0;
}

int fgetc(FILE *fp) {
    char c;
    ssize_t ret = fread(&c, /*size=*/1, /*count=*/1, fp);

    return (ret > 0) ? c : EOF;
}

int getchar(void) {
    return getc(stdin);
}

char *fgets(char *s, int size, FILE *stream) {
    int c = -1;
    char *cs = s;

    if (size < 1) {
        return NULL;
    }

    while (--size > 0 && (c = fgetc(stream)) != EOF) {
        if ((*cs++ = c) == '\n') {
            break;
        }
    }

    *cs = '\0';
    return (c == EOF && cs == s) ? NULL : s;
}

int _fprintf_output_func(const char *str, size_t len, void *state) {
    FILE *fp = (FILE *)state;
    return fwrite(str, /*size=*/1, /*count=*/len, fp);
}

int vfprintf(FILE *fp, const char *fmt, va_list ap) {
    return _printf_engine(&_fprintf_output_func, (void *)fp, fmt, ap);
}

int fprintf(FILE *fp, const char *fmt, ...) {
    va_list ap;
    int err;

    va_start(ap, fmt);
    err = vfprintf(fp, fmt, ap);
    va_end(ap);
    return err;
}

#if !DISABLE_DEBUG_OUTPUT
int printf(const char *fmt, ...) {
    va_list ap;
    int err;

    va_start(ap, fmt);
    err = vfprintf(stdout, fmt, ap);
    va_end(ap);

    return err;
}

int vprintf(const char *fmt, va_list ap) {
    return vfprintf(stdout, fmt, ap);
}
#endif // !DISABLE_DEBUG_OUTPUT
