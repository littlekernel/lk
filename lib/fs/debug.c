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
        printf("%s create <path> [size]\n", argv[0].str);
        printf("%s mkdir <path>\n", argv[0].str);
        printf("%s read <path> [<offset>] [<len>]\n", argv[0].str);
        printf("%s write <path> <string> [<offset>]\n", argv[0].str);
        printf("%s stat <file>\n", argv[0].str);
        printf("%s dir <path>\n", argv[0].str);
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
    } else if (!strcmp(argv[1].str, "create")) {
        int err;
        filehandle *handle;

        if (argc < 3)
            goto notenoughargs;

        err = fs_create_file(argv[2].str, &handle, (argc > 3) ? argv[3].u : 0);
        if (err < 0) {
            printf("error %d creating file\n", err);
            return err;
        }

        fs_close_file(handle);
    } else if (!strcmp(argv[1].str, "mkdir")) {
        int err;

        if (argc < 3)
            goto notenoughargs;

        err = fs_make_dir(argv[2].str);
        if (err < 0) {
            printf("error %d making directory\n", err);
            return err;
        }
    } else if (!strcmp(argv[1].str, "read")) {
        int err;
        char *buf;
        off_t off;
        size_t len;
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

        if (argc < 4)
            off = 0;

        else
            off = argv[3].u;

        if (argc < 5)
            len = stat.size - off;

        else
            len = argv[4].u;

        buf = malloc(len + 1);

        err = fs_read_file(handle, buf, off, len);
        if (err < 0) {
            printf("error %d reading file\n", err);
            free(buf);
            fs_close_file(handle);
            return err;
        }

        buf[len] = '\0';
        printf("%s\n", buf);
        free(buf);
        fs_close_file(handle);
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
    } else if (!strcmp(argv[1].str, "stat")) {
        int err;
        struct file_stat stat;
        filehandle *handle;

        if (argc < 3)
            goto notenoughargs;

        err = fs_open_file(argv[2].str, &handle);
        if (err < 0) {
            printf("error %d opening file\n", err);
            return err;
        }

        err = fs_stat_file(handle, &stat);
        if (err < 0) {
            printf("error %d statting file\n", err);
            fs_close_file(handle);
            return err;
        }

        printf("stat successful:\n");
        printf("\tis_dir: %d\n", stat.is_dir ? 1 : 0);
        printf("\tsize: %lld\n", stat.size);

        fs_close_file(handle);
    } else if (!strcmp(argv[1].str, "dir")) {
        dirhandle *handle;

        if (argc < 3)
            goto notenoughargs;

        status_t err = fs_open_dir(argv[2].str, &handle);
        if (err < 0) {
            printf("error %d opening dir\n", err);
            return err;
        }

        struct dirent ent;
        while ((err = fs_read_dir(handle, &ent)) >= 0) {
            printf("\t%s\n", ent.name);
        }

        fs_close_dir(handle);
    } else {
        printf("unrecognized subcommand\n");
        goto usage;
    }

    return rc;
}

#endif

#endif

