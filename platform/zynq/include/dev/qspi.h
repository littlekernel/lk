/*
 * Copyright (c) 2014 Travis Geiselbrecht
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

