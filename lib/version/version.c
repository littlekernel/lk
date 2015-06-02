/*
 * Copyright (c) 2013 Google, Inc.
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
#include <lib/version.h>

#include <debug.h>
#include <stdio.h>

/* generated for us */
#include <buildid.h>

/* ARCH, PLATFORM, TARGET, PROJECT should be defined by the build system */

/* BUILDID is optional, and may be defined anywhere */
#ifndef BUILDID
#define BUILDID ""
#endif

const lk_version_t version = {
    .struct_version = VERSION_STRUCT_VERSION,
    .arch = ARCH,
    .platform = PLATFORM,
    .target = TARGET,
    .project = PROJECT,
    .buildid = BUILDID
};

void print_version(void)
{
    printf("version:\n");
    printf("\tarch:     %s\n", version.arch);
    printf("\tplatform: %s\n", version.platform);
    printf("\ttarget:   %s\n", version.target);
    printf("\tproject:  %s\n", version.project);
    printf("\tbuildid:  %s\n", version.buildid);
}

#if WITH_LIB_CONSOLE

#include <debug.h>
#include <lib/console.h>

static int cmd_version(int argc, const cmd_args *argv)
{
    print_version();
    return 0;
}

STATIC_COMMAND_START
{ "version", "print version", &cmd_version },
STATIC_COMMAND_END(version);

#endif // WITH_LIB_CONSOLE

