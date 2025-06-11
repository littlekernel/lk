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

// The three main standard io file descriptors that just
// point to the main console.
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

    // TODO: handle more open modes

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

// Inner version of fread that returns a ssize_t to include the internal error code
// along with the length.
static ssize_t fread_error(void *ptr, size_t size, size_t count, FILE *stream) {
    if (count == 0 || size == 0) {
        return 0;
    }
#if defined(WITH_LIB_FS)
    if (stream->use_fs) {
        ssize_t rsize = fs_read_file(stream->fs_handle.handle, ptr,
                             stream->fs_handle.offset, size * count);
        if (rsize <= 0) {
            return rsize;
        }
        stream->fs_handle.offset += rsize;
        return rsize / size;
    }
#endif // WITH_LIB_FS
    ssize_t rsize = io_read(stream->io, ptr, size * count);
    if (rsize <= 0) {
        return rsize;
    }
    return rsize / size;
}

// Inner version of fwrite that returns a ssize_t to include the internal error code
// along with the length.
static ssize_t fwrite_error(const void *ptr, size_t size, size_t count, FILE *stream) {
    if (count == 0 || size == 0) {
        return 0;
    }
#if defined(WITH_LIB_FS)
    if (stream->use_fs) {
        if (stream->fs_handle.readonly) {
            // return error here?
            return 0;
        }

        // TODO: deal with append

        ssize_t wsize = fs_write_file(stream->fs_handle.handle, ptr,
                                     stream->fs_handle.offset, size * count);
        if (wsize <= 0) {
            return wsize;
        }
        stream->fs_handle.offset += wsize;

        return wsize / size;
    }
#endif
    ssize_t wsize = io_write(stream->io, ptr, size * count);
    if (wsize <= 0) {
        return wsize;
    }
    return wsize / size;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
    ssize_t read = fread_error(ptr, size, count, stream);
    if (read < 0) {
        // TODO: save error for ferror()
        return 0;
    }
    return read;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
    ssize_t written = fwrite_error(ptr, size, count, stream);
    if (written < 0) {
        // TODO: save error for ferror()
        return 0;
    }
    return written;
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

    size_t written = fwrite(&c, /*size=*/1, /*count=*/1, fp);
    if (written == 0) {
        return EOF;
    }
    return c;
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char *str) {
    int err = fputs(str, stdout);
    if (err >= 0) {
        err = fputc('\n', stdout);
    }
    return err;
}

int fputs(const char *s, FILE *fp) {
    size_t len = strlen(s);
    if (len == 0) {
        return 0;
    }

    size_t written = fwrite(s, /*size=*/1, /*count=*/len, fp);
    if (written == 0) {
        return EOF;
    }
    return written;
}

int fgetc(FILE *fp) {
    char c;

    size_t err = fread(&c, /*size=*/1, /*count=*/1, fp);
    if (err == 0) {
        return EOF;
    }
    return c;
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
        *cs++ = c;
        if (c == '\n') {
            break;
        }
    }

    *cs = '\0';
    return (c == EOF && cs == s) ? NULL : s;
}

int _fprintf_output_func(const char *str, size_t len, void *state) {
    FILE *fp = (FILE *)state;

    return fwrite_error(str, /*size=*/1, /*count=*/len, fp);
}
