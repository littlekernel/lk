/*
 * Copyright (c) 2015 Travis Geiselbrecht
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

#include <err.h>
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <trace.h>
#include <stdlib.h>
#include <platform.h>
#include <lib/console.h>
#include <lib/fs.h>

/* shell console hooks for manipulating the file system */

#if WITH_LIB_CONSOLE

static char *cwd = NULL;

static void set_cwd(const char *path)
{
    if (!path) {
        free(cwd);
        cwd = NULL;
        return;
    }

    size_t len = strlen(path) + 1;
    char *new_cwd = realloc(cwd, len);
    if (new_cwd) {
        cwd = new_cwd;
        memcpy(cwd, path, len);
    }
}

static const char *get_cwd(void)
{
    if (!cwd)
        return "/";
    return cwd;
}

static char *prepend_cwd(char *path, size_t len, const char *arg)
{
    path[0] = '\0';

    if (!arg || arg[0] != '/') {
        strlcat(path, get_cwd(), len);
        if (arg && path[strlen(path) - 1] != '/')
            strlcat(path, "/", len);
    }
    if (arg) {
        strlcat(path, arg, len);
    }

    return path;
}

static int cmd_ls(int argc, const cmd_args *argv)
{
    status_t status = NO_ERROR;

    // construct the path
    char *path = malloc(FS_MAX_PATH_LEN);
    prepend_cwd(path, FS_MAX_PATH_LEN, (argc >= 2) ? argv[1].str : NULL);

    dirhandle *dhandle;
    status = fs_open_dir(path, &dhandle);
    if (status < 0) {
        printf("error %d opening dir '%s'\n", status, path);
        goto err;
    }

    size_t pathlen = strlen(path);

    status_t err;
    struct dirent ent;
    while ((err = fs_read_dir(dhandle, &ent)) >= 0) {
        struct file_stat stat;
        filehandle *handle;

        // append our filename to the path
        strlcat(path, "/", FS_MAX_PATH_LEN);
        strlcat(path, ent.name, FS_MAX_PATH_LEN);

        err = fs_open_file(path, &handle);

        // restore the old path
        path[pathlen] = '\0';

        if (err < 0) {
            printf("error %d opening file '%s'\n", err, path);
            continue;
        }

        // stat the file
        err = fs_stat_file(handle, &stat);
        fs_close_file(handle);
        if (err < 0) {
            printf("error %d statting file\n", err);
            continue;
        }

        printf("%c %16llu %s\n", stat.is_dir ? 'd' : ' ', stat.size, ent.name);
    }

    fs_close_dir(dhandle);

err:
    free(path);
    return status;;
}

static int cmd_cd(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        set_cwd(NULL);
    } else {
        char *path = malloc(FS_MAX_PATH_LEN);
        prepend_cwd(path, FS_MAX_PATH_LEN, (argc >= 2) ? argv[1].str : NULL);
        fs_normalize_path(path);

        if (strlen(path) == 0) {
            set_cwd(NULL);
        } else {
            set_cwd(path);
        }
        free(path);
    }
    puts(get_cwd());

    return 0;
}

static int cmd_pwd(int argc, const cmd_args *argv)
{
    puts(get_cwd());

    return 0;
}

static int cmd_mkdir(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough arguments\n");
        printf("usage: %s <path>\n", argv[0].str);
        return -1;
    }

    char *path = malloc(FS_MAX_PATH_LEN);

    int status = fs_make_dir(prepend_cwd(path, FS_MAX_PATH_LEN, argv[1].str));
    if (status < 0) {
        printf("error %d making directory '%s'\n", status, path);
    }

    free(path);
    return status;
}

static int cmd_mkfile(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough arguments\n");
        printf("usage: %s <path> [length]\n", argv[0].str);
        return -1;
    }

    char *path = malloc(FS_MAX_PATH_LEN);
    prepend_cwd(path, FS_MAX_PATH_LEN, argv[1].str);

    filehandle *handle;
    status_t status = fs_create_file(path, &handle, (argc >= 2) ? argv[2].u : 0);
    if (status < 0) {
        printf("error %d making file '%s'\n", status, path);
        goto err;
    }

    fs_close_file(handle);

err:
    free(path);
    return status;
}

static int cmd_rm(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough arguments\n");
        printf("usage: %s <path>\n", argv[0].str);
        return -1;
    }

    char *path = malloc(FS_MAX_PATH_LEN);
    prepend_cwd(path, FS_MAX_PATH_LEN, argv[1].str);

    status_t err = fs_remove_file(path);
    if (err < 0) {
        printf("error %d removing file '%s'\n", err, path);
        return err;
    }

    return 0;
}

static int cmd_stat(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough arguments\n");
        printf("usage: %s <path>\n", argv[0].str);
        return -1;
    }

    int status;
    struct file_stat stat;
    filehandle *handle;

    char *path = malloc(FS_MAX_PATH_LEN);
    prepend_cwd(path, FS_MAX_PATH_LEN, argv[1].str);

    status = fs_open_file(path, &handle);
    if (status < 0) {
        printf("error %d opening file '%s'\n", status, path);
        goto err;
    }

    status = fs_stat_file(handle, &stat);

    fs_close_file(handle);

    if (status < 0) {
        printf("error %d statting file\n", status);
        goto err;
    }

    printf("stat successful:\n");
    printf("\tis_dir: %d\n", stat.is_dir ? 1 : 0);
    printf("\tsize: %lld\n", stat.size);


err:
    free(path);
    return status;
}

static int cmd_cat(int argc, const cmd_args *argv)
{
    status_t status = NO_ERROR;

    if (argc < 2) {
        printf("not enough arguments\n");
        printf("usage: %s <path>\n", argv[0].str);
        return -1;
    }

    char *path = malloc(FS_MAX_PATH_LEN);
    prepend_cwd(path, FS_MAX_PATH_LEN, argv[1].str);

    filehandle *handle;
    status = fs_open_file(path, &handle);
    if (status < 0) {
        printf("error %d opening file '%s'\n", status, path);
        goto err;
    }

    char buf[64];
    ssize_t read_len;
    off_t offset = 0;
    while ((read_len = fs_read_file(handle, buf, offset, sizeof(buf))) > 0) {
        for (int i = 0; i < read_len; i++) {
            putchar(buf[i]);
        }

        offset += read_len;
    }

    fs_close_file(handle);

err:
    free(path);
    return status;
}

STATIC_COMMAND_START
STATIC_COMMAND("ls", "dir listing", &cmd_ls)
STATIC_COMMAND("cd", "change dir", &cmd_cd)
STATIC_COMMAND("pwd", "print working dir", &cmd_pwd)
STATIC_COMMAND("mkdir", "make dir", &cmd_mkdir)
STATIC_COMMAND("mkfile", "make file", &cmd_mkfile)
STATIC_COMMAND("rm", "remove file", &cmd_rm)
STATIC_COMMAND("stat", "stat file", &cmd_stat)
STATIC_COMMAND("cat", "cat file", &cmd_cat)
STATIC_COMMAND_END(fs_shell);

#endif

