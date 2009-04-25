/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <sys/types.h>
#include <lib/net/misc.h>
#include <string.h>

uint16_t ones_sum16(uint32_t sum, const void *_buf, int len)
{
	const uint16_t *buf = _buf;

	while(len >= 2) {
		sum += *buf++;
		if(sum & 0x80000000)
			sum = (sum & 0xffff) + (sum >> 16);
		len -= 2;
	}

	if (len) {
		uint8_t temp[2];
		temp[0] = *(uint8_t *) buf;
		temp[1] = 0;
		sum += *(uint16_t *) temp;
	}

	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return sum;
}

uint16_t cksum16(void *_buf, int len)
{
	return ~ones_sum16(0, _buf, len);
}

uint16_t cksum16_2(void *buf1, int len1, void *buf2, int len2)
{
	uint32_t sum;

	sum = ones_sum16(0, buf1, len1);
	return ~ones_sum16(sum, buf2, len2);
}

int cmp_netaddr(netaddr *addr1, netaddr *addr2)
{
	if(addr1->type != addr2->type)
		return -1;
	return memcmp(&addr1->addr[0], &addr2->addr[0], addr1->len);
}

