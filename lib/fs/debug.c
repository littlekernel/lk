/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <string.h>
#include <lib/console.h>
#include <lib/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <platform.h>

static void test_normalize(const char *in)
{
    char path[1024];

    strlcpy(path, in, sizeof(path));
    fs_normalize_path(path);
    printf("'%s' -> '%s'\n", in, path);
}

#if 0
    test_normalize("/");
    test_normalize("/test");
    test_normalize("/test/");
    test_normalize("test/");
    test_normalize("test");
    test_normalize("/test//");
    test_normalize("/test/foo");
    test_normalize("/test/foo/");
    test_normalize("/test/foo/bar");
    test_normalize("/test/foo/bar//");
    test_normalize("/test//foo/bar//");
    test_normalize("/test//./foo/bar//");
    test_normalize("/test//./.foo/bar//");
    test_normalize("/test//./..foo/bar//");
    test_normalize("/test//./../foo/bar//");
    test_normalize("/test/../foo");
    test_normalize("/test/bar/../foo");
    test_normalize("../foo");
    test_normalize("../foo/");
    test_normalize("/../foo");
    test_normalize("/../foo/");
    test_normalize("/../../foo");
    test_normalize("/bleh/../../foo");
    test_normalize("/bleh/bar/../../foo");
    test_normalize("/bleh/bar/../../foo/..");
    test_normalize("/bleh/bar/../../foo/../meh");
#endif

#if defined(WITH_LIB_CONSOLE)

#if LK_DEBUGLEVEL > 1
static int cmd_fs(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("fs", "fs debug commands", &cmd_fs)
STATIC_COMMAND_END(fs);

extern int fs_mount_type(const char *path, const char *device, const char *name);

static int cmd_fs(int argc, const cmd_args *argv)
{
    int rc = 0;

    if (argc < 2) {
notenoughargs:
        printf("not enough arguments:\n");
usage:
        printf("%s mount <path> <type> [device]\n", argv[0].str);
        printf("%s unmount <path>\n", argv[0].str);
        printf("%s write <path> <string> [<offset>]\n", argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "mount")) {
        int err;

        if (argc < 4)
            goto notenoughargs;

        err = fs_mount(argv[2].str, argv[3].str,
                (argc >= 5) ? argv[4].str : NULL);

        if (err < 0) {
            printf("error %d mounting device\n", err);
            return err;
        }
    } else if (!strcmp(argv[1].str, "unmount")) {
        int err;

        if (argc < 3)
            goto notenoughargs;

        err = fs_unmount(argv[2].str);
        if (err < 0) {
            printf("error %d unmounting device\n", err);
            return err;
        }
    } else if (!strcmp(argv[1].str, "write")) {
        int err;
        off_t off;
        filehandle *handle;
        struct file_stat stat;

        if (argc < 3)
            goto notenoughargs;

        err = fs_open_file(argv[2].str, &handle);
        if (err < 0) {
            printf("error %d opening file\n", err);
            return err;
        }

        err = fs_stat_file(handle, &stat);
        if (err < 0) {
            printf("error %d stat'ing file\n", err);
            fs_close_file(handle);
            return err;
        }

        if (argc < 5)
            off = stat.size;
        else
            off = argv[4].u;

        err = fs_write_file(handle, argv[3].str, off, strlen(argv[3].str));
        if (err < 0) {
            printf("error %d writing file\n", err);
            fs_close_file(handle);
            return err;
        }

        fs_close_file(handle);
    } else {
        printf("unrecognized subcommand\n");
        goto usage;
    }

    return rc;
}

#endif

#endif

