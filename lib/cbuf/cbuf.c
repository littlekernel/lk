/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
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
#include <stdlib.h>
#include <debug.h>
#include <pow2.h>
#include <string.h>
#include <assert.h>
#include <lib/cbuf.h>
#include <kernel/event.h>

#define LOCAL_TRACE 0

#define INC_POINTER(cbuf, ptr, inc) \
    modpow2(((ptr) + (inc)), (cbuf)->len_pow2)

void cbuf_initialize(cbuf_t *cbuf, size_t len)
{
	DEBUG_ASSERT(cbuf);
	DEBUG_ASSERT(len > 0);
	DEBUG_ASSERT(ispow2(len));

	cbuf->head = 0;
	cbuf->tail = 0;
	cbuf->len_pow2 = log2_uint(len);
	cbuf->buf = malloc(len);
	event_init(&cbuf->event, false, 0);

	LTRACEF("len %zd, len_pow2 %u\n", len, cbuf->len_pow2);
}

size_t cbuf_space_avail(cbuf_t *cbuf)
{
	uint consumed = modpow2((uint)(cbuf->head - cbuf->tail), cbuf->len_pow2);
	return valpow2(cbuf->len_pow2) - consumed - 1;
}

size_t cbuf_write(cbuf_t *cbuf, const void *_buf, size_t len, bool canreschedule)
{
	const char *buf = (const char *)_buf;

	LTRACEF("len %zd\n", len);

	DEBUG_ASSERT(cbuf);
	DEBUG_ASSERT(_buf);
	DEBUG_ASSERT(len < valpow2(cbuf->len_pow2));

	enter_critical_section();

	size_t write_len;
	size_t pos = 0;

	while (pos < len && cbuf_space_avail(cbuf) > 0) {
		if (cbuf->head >= cbuf->tail) {
			write_len = MIN(valpow2(cbuf->len_pow2) - cbuf->head, len - pos);
		} else {
			write_len = MIN(cbuf->tail - cbuf->head - 1, len - pos);
		}

		// if it's full, abort and return how much we've written
		if (write_len == 0) {
			break;
		}

		memcpy(cbuf->buf + cbuf->head, buf + pos, write_len);

		cbuf->head = INC_POINTER(cbuf, cbuf->head, write_len);
		pos += write_len;
	}

	if (cbuf->head != cbuf->tail)
		event_signal(&cbuf->event, canreschedule);

	exit_critical_section();

	return pos;
}

size_t cbuf_read(cbuf_t *cbuf, void *_buf, size_t buflen, bool block)
{
	char *buf = (char *)_buf;

	DEBUG_ASSERT(cbuf);
	DEBUG_ASSERT(_buf);

	enter_critical_section();

	if (block)
		event_wait(&cbuf->event);

	// see if there's data available
	size_t ret = 0;
	if (cbuf->tail != cbuf->head) {
		size_t pos = 0;

		// loop until we've read everything we need
		// at most this will make two passes to deal with wraparound
		while (pos < buflen && cbuf->tail != cbuf->head) {
			size_t read_len;
			if (cbuf->head > cbuf->tail) {
				// simple case where there is no wraparound
				read_len = MIN(cbuf->head - cbuf->tail, buflen - pos);
			} else {
				// read to the end of buffer in this pass
				read_len = MIN(valpow2(cbuf->len_pow2) - cbuf->tail, buflen - pos);
			}

			memcpy(buf + pos, cbuf->buf + cbuf->tail, read_len);

			cbuf->tail = INC_POINTER(cbuf, cbuf->tail, read_len);
			pos += read_len;
		}

		if (cbuf->tail == cbuf->head) {
			// we've emptied the buffer, unsignal the event
			event_unsignal(&cbuf->event);
		}

		ret = pos;
	}

	exit_critical_section();

	return ret;
}

size_t cbuf_write_char(cbuf_t *cbuf, char c, bool canreschedule)
{
	DEBUG_ASSERT(cbuf);

	enter_critical_section();

	size_t ret = 0;
	if (cbuf_space_avail(cbuf) > 0) {
		cbuf->buf[cbuf->head] = c;

		cbuf->head = INC_POINTER(cbuf, cbuf->head, 1);
		ret = 1;

		if (cbuf->head != cbuf->tail)
			event_signal(&cbuf->event, canreschedule);
	}

	exit_critical_section();

	return ret;
}

size_t cbuf_read_char(cbuf_t *cbuf, char *c, bool block)
{
	DEBUG_ASSERT(cbuf);
	DEBUG_ASSERT(c);

	enter_critical_section();

	if (block)
		event_wait(&cbuf->event);

	// see if there's data available
	size_t ret = 0;
	if (cbuf->tail != cbuf->head) {

		*c = cbuf->buf[cbuf->tail];
		cbuf->tail = INC_POINTER(cbuf, cbuf->tail, 1);

		if (cbuf->tail == cbuf->head) {
			// we've emptied the buffer, unsignal the event
			event_unsignal(&cbuf->event);
		}

		ret = 1;
	}

	exit_critical_section();

	return ret;
}

