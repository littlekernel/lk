/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app.h>
#include <lk/debug.h>
#include <stdio.h>
#include <dev/accelerometer.h>
#include <lk/compiler.h>
#include <lk/console_cmd.h>

void read_xyz(void);

STATIC_COMMAND_START
STATIC_COMMAND("read_xyz", "read xyz vectors", (console_cmd_func)&read_xyz)
STATIC_COMMAND_END(accelerometer);

void read_xyz(void) {
    position_vector_t pos_vector;
    acc_read_xyz(&pos_vector);
    printf("X value = %f\n",pos_vector.x);
    printf("Y value = %f\n",pos_vector.y);
    printf("Z value = %f\n",pos_vector.z);

}

APP_START(accelerometer)
.flags = 0,
APP_END

