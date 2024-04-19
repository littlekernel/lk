/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <lk/debug.h>
#include <string.h>
#include <stdio.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <platform.h>
#include <lk/console_cmd.h>
#include <lib/fs.h>

/* shell console hooks for manipulating the file system */

static char *cwd = NULL;

static void set_cwd(const char *path) {
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

static const char *get_cwd(void) {
    if (!cwd)
        return "/";
    return cwd;
}

static char *prepend_cwd(char *path, size_t len, const char *arg) {
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

static int cmd_ls(int argc, const console_cmd_args *argv) {
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
    char *tmppath = calloc(FS_MAX_PATH_LEN, 1);
    strlcpy(tmppath, path, FS_MAX_PATH_LEN);

    status_t err;
    struct dirent ent;
    while ((err = fs_read_dir(dhandle, &ent)) >= 0) {
        struct file_stat stat;
        filehandle *handle;

        // append our filename to the path
        tmppath[pathlen] = '\0';
        strlcat(tmppath, "/", FS_MAX_PATH_LEN);
        strlcat(tmppath, ent.name, FS_MAX_PATH_LEN);

        err = fs_open_file(tmppath, &handle);

        if (err < 0) {
            printf("error %d opening file '%s'\n", err, tmppath);
            continue;
        }

        // stat the file
        err = fs_stat_file(handle, &stat);
        fs_close_file(handle);
        if (err < 0) {
            printf("error %d statting file\n", err);
            continue;
        }

        printf("%c %-16llu %s\n", stat.is_dir ? 'D' : 'F', stat.size, ent.name);
    }

    fs_close_dir(dhandle);

err:
    free(path);
    return status;;
}

static int cmd_cd(int argc, const console_cmd_args *argv) {
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

static int cmd_pwd(int argc, const console_cmd_args *argv) {
    puts(get_cwd());

    return 0;
}

static int cmd_mkdir(int argc, const console_cmd_args *argv) {
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

static int cmd_mkfile(int argc, const console_cmd_args *argv) {
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

static int cmd_rm(int argc, const console_cmd_args *argv) {
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

static int cmd_stat(int argc, const console_cmd_args *argv) {
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

static int cmd_cat(int argc, const console_cmd_args *argv) {
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

static int cmd_df(int argc, const console_cmd_args *argv) {
    fs_dump_mounts();
    return NO_ERROR;
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
STATIC_COMMAND("df", "list mounts", &cmd_df)
STATIC_COMMAND_END(fs_shell);
