/*
 * Copyright (c) 2013 Google, Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/version.h>

#include <lk/debug.h>
#include <stdio.h>
#include <lk/init.h>
#include <lk/console_cmd.h>

/* generated for us */
#include <buildid.h>

/* ARCH, PLATFORM, TARGET, PROJECT should be defined by the build system */

/* BUILDID is optional, and may be defined anywhere */
#ifndef BUILDID
#define BUILDID ""
#endif

const lk_version_t lk_version = {
    .struct_version = VERSION_STRUCT_VERSION,
    .arch = ARCH,
    .platform = PLATFORM,
    .target = TARGET,
    .project = PROJECT,
    .buildid = BUILDID
};

void print_version(void) {
    printf("version:\n");
    printf("\tarch:     %s\n", lk_version.arch);
    printf("\tplatform: %s\n", lk_version.platform);
    printf("\ttarget:   %s\n", lk_version.target);
    printf("\tproject:  %s\n", lk_version.project);
    printf("\tbuildid:  %s\n", lk_version.buildid);
}

static int cmd_version(int argc, const console_cmd_args *argv) {
    print_version();
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("version", "print version", &cmd_version)
STATIC_COMMAND_END(version);

#if LK_DEBUGLEVEL > 0
// print the version string if any level of debug is set
LK_INIT_HOOK(version, (void *)&print_version, LK_INIT_LEVEL_HEAP - 1);
#endif
