/*
 * Copyright (c) 2015 Brian Swetland
 * Copyright (c) 2008 Google, Inc.
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

#include <malloc.h>
#include <printf.h>
#include <string.h>
#include <dev/udc.h>

#include "udc-common.h"

udc_descriptor_t *udc_descriptor_alloc(unsigned type, unsigned num, unsigned len)
{
	struct udc_descriptor *desc;
	if ((len > 255) || (len < 2) || (num > 255) || (type > 255))
		return 0;

	if(!(desc = malloc(sizeof(struct udc_descriptor) + len)))
		return 0;

	desc->next = 0;
	desc->tag = (type << 8) | num;
	desc->len = len;
	desc->data[0] = len;
	desc->data[1] = type;

	return desc;
}

static udc_descriptor_t langid_list = {
	.tag = 0x0300,
	.len = 4,
	.data = { 0x04, TYPE_STRING, 0x09, 0x04 }, // EN_US
};

static struct udc_descriptor *desc_list = &langid_list;
static unsigned next_string_id = 1;

udc_descriptor_t *udc_descriptor_find(unsigned tag)
{
	udc_descriptor_t *desc = desc_list;
	while (desc != NULL) {
		if (desc->tag == tag) {
			return desc;
		}
		desc = desc->next;
	}
	printf("cant find %08x\n", tag);
	return NULL;
}

void udc_descriptor_register(struct udc_descriptor *desc)
{
	desc->next = desc_list;
	desc_list = desc;
}

unsigned udc_string_desc_alloc(const char *str)
{
	unsigned len;
	struct udc_descriptor *desc;
	unsigned char *data;

	if (next_string_id > 255)
		return 0;

	if (!str)
		return 0;

	len = strlen(str);
	desc = udc_descriptor_alloc(TYPE_STRING, next_string_id, len * 2 + 2);
	if (!desc)
		return 0;
	next_string_id++;

	/* expand ascii string to utf16 */
	data = desc->data + 2;
	while (len-- > 0) {
		*data++ = *str++;
		*data++ = 0;
	}

	udc_descriptor_register(desc);
	return desc->tag & 0xff;
}


