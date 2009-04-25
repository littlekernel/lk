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
#include <debug.h>
#include <err.h>
#include <compiler.h>
#include <sys/types.h>
#include <lib/net/icmp.h>
#include <lib/net/misc.h>

typedef struct icmp_header {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
} __PACKED icmp_header;

typedef struct icmp_echo_header {
	icmp_header preheader;
	uint16_t identifier;
	uint16_t sequence;
} __PACKED icmp_echo_header;

int icmp_input(cbuf *buf, ifnet *i, ipv4_addr source_ipaddr)
{
	icmp_header *header;
	int err;

	header = (icmp_header *)cbuf_get_ptr(buf, 0);

#if NET_CHATTY
	printf("icmp_message: header type %d, code %d, checksum 0x%x, length %Ld\n", header->type, header->code, header->checksum, (long long)cbuf_get_len(buf));
	printf(" buffer len %d\n", cbuf_get_len(buf));
#endif

	// calculate the checksum on the whole thing
	if(cbuf_ones_cksum16(buf, 0, 0xffff) != 0) {
		printf("icmp message fails cksum\n");
#if NET_CHATTY
	{
		int i;
		for(i=0; i<cbuf_get_len(buf); i++) {
			if((i % 8) == 0) {
				printf("\n0x%x: ", i);
			}
			printf("0x%x ", *(unsigned char *)cbuf_get_ptr(buf, i));
		}
		printf("\n");
	}
#endif
		err = ERR_NET_BAD_PACKET;
		goto ditch_message;
	}

	switch(header->type) {
		case 0: { // echo reply
			break;
		}
		case 8: { // echo request
			icmp_echo_header *eheader = (icmp_echo_header *)header;

			// bounce this message right back
			eheader->preheader.type = 0; // echo reply
			eheader->preheader.checksum = 0;
			eheader->preheader.checksum = cbuf_ones_cksum16(buf, 0, 0xffff);
			return ipv4_output(buf, source_ipaddr, IP_PROT_ICMP);
		}
#if NET_CHATTY
		default:
			printf("unhandled icmp message\n");
#endif
	}

	err = NO_ERROR;

ditch_message:
	cbuf_free_chain(buf);

	return err;
}

