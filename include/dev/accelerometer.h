/*
 * Copyright (c) 2015 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

typedef struct {
    double x;
    double y;
    double z;
} position_vector_t;

status_t acc_read_xyz(position_vector_t *pos_vector);

