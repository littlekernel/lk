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
#include <stdio.h>
#include <string.h>
#include <dev/udc.h>

#include "udc-common.h"

static udc_descriptor_t *udc_descriptor_alloc(unsigned type, unsigned num, unsigned len)
{
    struct udc_descriptor *desc;
    if ((len > 255) || (len < 2) || (num > 255) || (type > 255))
        return 0;

    if (!(desc = malloc(sizeof(struct udc_descriptor) + len)))
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

static void udc_descriptor_register(struct udc_descriptor *desc)
{
    desc->next = desc_list;
    desc_list = desc;
}

static unsigned udc_string_desc_alloc(const char *str)
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


static unsigned udc_ifc_desc_size(udc_gadget_t *g)
{
    return 9 + g->ifc_endpoints * 7;
}

static void udc_ifc_desc_fill(udc_gadget_t *g, unsigned ifcn, unsigned char *data)
{
    unsigned n;

    data[0] = 0x09;
    data[1] = TYPE_INTERFACE;
    data[2] = ifcn; // ifc number
    data[3] = 0x00; // alt number
    data[4] = g->ifc_endpoints;
    data[5] = g->ifc_class;
    data[6] = g->ifc_subclass;
    data[7] = g->ifc_protocol;
    data[8] = udc_string_desc_alloc(g->ifc_string);

    data += 9;
    for (n = 0; n < g->ifc_endpoints; n++) {
        udc_ept_desc_fill(g->ept[n], data);
        data += 7;
    }
}

void udc_create_descriptors(udc_device_t *device, udc_gadget_t *gadgetlist)
{
    udc_descriptor_t *desc;
    udc_gadget_t *gadget;
    unsigned size;
    uint8_t *data, *p;
    uint8_t n;

    // create our device descriptor
    desc = udc_descriptor_alloc(TYPE_DEVICE, 0, 18);
    data = desc->data;
    data[2] = 0x00; // usb spec rev 2.00
    data[3] = 0x02;
    data[4] = 0x00; // class
    data[5] = 0x00; // subclass
    data[6] = 0x00; // protocol
    data[7] = 0x40; // max packet size on ept 0
    data[8] = device->vendor_id;
    data[9] = device->vendor_id >> 8;
    data[10] = device->product_id;
    data[11] = device->product_id >> 8;
    data[12] = device->version_id;
    data[13] = device->version_id >> 8;
    data[14] = udc_string_desc_alloc(device->manufacturer);
    data[15] = udc_string_desc_alloc(device->product);
    data[16] = udc_string_desc_alloc(device->serialno);
    data[17] = 1; // number of configurations
    udc_descriptor_register(desc);

    // create our configuration descriptor
    size = 9;
    n = 0;
    for (gadget = gadgetlist; gadget; gadget = gadget->next) {
        size += udc_ifc_desc_size(gadget);
        n++;
    }
    desc = udc_descriptor_alloc(TYPE_CONFIGURATION, 0, size);
    data = desc->data;
    data[0] = 0x09;
    data[2] = size;
    data[3] = size >> 8;
    data[4] = n; // number of interfaces
    data[5] = 0x01; // configuration value
    data[6] = 0x00; // configuration string
    data[7] = 0x80; // attributes
    data[8] = 0x80; // max power (250ma) -- todo fix this

    n = 0;
    p = data + 9;
    for (gadget = gadgetlist; gadget; gadget = gadget->next) {
        udc_ifc_desc_fill(gadget, n++, p);
        p += udc_ifc_desc_size(gadget);
    }
    udc_descriptor_register(desc);
}
