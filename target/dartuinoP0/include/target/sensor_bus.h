/*
 * Copyright (c) 2015 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __SENSOR_BUS_H
#define __SENSOR_BUS_H

#include <sys/types.h>
#include <dev/accelerometer.h>

status_t sensor_bus_init_early(void);

void sensor_bus_init(void);

status_t acc_read(uint8_t address, uint8_t *data);

status_t acc_flush(uint8_t *tbuff, uint8_t *rbuff, uint8_t numbytes);


#endif
