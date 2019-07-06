/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/* a highly Zynq specific qspi interface */

struct qspi_ctxt {
    uint32_t cfg;
    uint32_t khz;
    bool linear_mode;
};

int qspi_set_speed(struct qspi_ctxt *qspi, uint32_t khz);
int qspi_init(struct qspi_ctxt *qspi, uint32_t khz);
int qspi_enable_linear(struct qspi_ctxt *qspi);
int qspi_disable_linear(struct qspi_ctxt *qspi);
void qspi_rd(struct qspi_ctxt *qspi, uint32_t cmd, uint32_t asize, uint32_t *data, uint32_t count);
void qspi_wr(struct qspi_ctxt *qspi, uint32_t cmd, uint32_t asize, uint32_t *data, uint32_t count);
void qspi_wr1(struct qspi_ctxt *qspi, uint32_t cmd);
void qspi_wr2(struct qspi_ctxt *qspi, uint32_t cmd);
void qspi_wr3(struct qspi_ctxt *qspi, uint32_t cmd);
uint32_t qspi_rd1(struct qspi_ctxt *qspi, uint32_t cmd);

/* set 0 for chip select */
void qspi_cs(struct qspi_ctxt *qspi, unsigned int cs);

