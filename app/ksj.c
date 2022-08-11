/*
 * Copyright 2022 CoAsia Nexell Inc. All Rights Reserved.
 * Author: sjkong@coasia.com (Sukjin Kong)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <lk/console_cmd.h>
#include <lk/err.h>

int ksj_lk(int argc, const console_cmd_args *argv)
{
    if(argc != 2)
    {
        return ERR_INVALID_ARGS;
    }

    printf("%s\n", __func__);

    return NO_ERROR;
}

STATIC_COMMAND_START
STATIC_COMMAND("ksj", "help", &ksj_lk)
STATIC_COMMAND_END(ksj);