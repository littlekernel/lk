/*
 * Copyright (c) 2014 Brian Swetland
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

#include "devicetree.h"

#define DT_MAGIC	0xD00DFEED
#define DT_NODE_BEGIN	1
#define DT_NODE_END	2
#define DT_PROP		3
#define DT_END		9

typedef struct dt_slice slice_t;

u32 dt_rd32(u8 *data) {
	return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

void dt_wr32(u32 n, u8 *data) {
	*data++ = n >> 24;
	*data++ = n >> 16;
	*data++ = n >> 8;
	*data = n;
}

/* init subslice from slice, returning 0 if successful */
static int sslice(slice_t *src, slice_t *dst, u32 off, u32 len) {
	if (off >= src->size)
		return -1;
	if (len >= src->size)
		return -1;
	if ((off + len) > src->size)
		return -1;
	dst->data = src->data + off;
	dst->size = len;
	return 0;
}

/* return nonzero if slice is empty */
static inline int sempty(slice_t *s) {
	return s->size == 0;
}

/* read be32 from slice,
 * or 0 (and make slice empty) if slice is too small
 */
static u32 su32(slice_t *s) {
	if (s->size < 4) {
		s->size = 0;
		return 0;
	} else {
		u32 n = (s->data[0] << 24) | (s->data[1] << 16) | (s->data[2] << 8) | s->data[3];
		s->size -= 4;
		s->data += 4;
		return n;
	}
}

/* return pointer to data in slice,
 * or 0 (and make slice empty) if slice is too small
 */
static void *sdata(slice_t *s, u32 len) {
	if (len > s->size) {
		s->size = 0;
		return 0;
	} else {
		void *data = s->data;
		s->size -= len;
		s->data += len;
		while (len & 3) {
			if (s->size) {
				s->size++;
				s->data++;
			}
			len++;
		}
		return data;
	}
}

/* return pointer to string in slice,
 * or "" (and make slice empty) if slice is too small
 */
static const char *sstring(slice_t *s) {
	u32 sz = s->size;
	u8 *end = s->data;
	const char *data;
	while (sz-- > 0) {
		if (*end++ == 0) {
			while (((end - s->data) & 3) && (sz > 0)) {
				end++;
				sz--;
			}
			data = (const char*) s->data;
			s->size = sz;
			s->data = end;
			return data;
		}
	}
	s->size = 0;
	return "";
}

static int oops(devicetree_t *dt, const char *msg) {
	if (dt->error)
		dt->error(msg);
	return -1;
}

int dt_init(devicetree_t *dt, void *data, u32 len) {
	slice_t s;

	dt->top.data = data;
	dt->top.size = len;

	s = dt->top;

	dt->hdr.magic = su32(&s);
	dt->hdr.size = su32(&s);
	dt->hdr.off_struct = su32(&s);
	dt->hdr.off_strings = su32(&s);
	dt->hdr.off_reserve = su32(&s);
	dt->hdr.version = su32(&s);
	dt->hdr.version_compat = su32(&s);
	dt->hdr.boot_cpuid = su32(&s);
	dt->hdr.sz_strings = su32(&s);
	dt->hdr.sz_struct = su32(&s);

	if (dt->hdr.magic != DT_MAGIC)
		return oops(dt, "bad magic");
	if (dt->hdr.size > dt->top.size)
		return oops(dt, "bogus size field");
	if (dt->hdr.version != 17)
		return oops(dt, "version != 17");
	if (sslice(&dt->top, &dt->dt, dt->hdr.off_struct, dt->hdr.sz_struct))
		return oops(dt, "invalid structure off/len");
	if (sslice(&dt->top, &dt->ds, dt->hdr.off_strings, dt->hdr.sz_strings))
		return oops(dt, "invalid strings off/len");

	return 0;
}

int dt_walk(devicetree_t *dtree, dt_node_cb ncb, dt_prop_cb pcb, void *cookie) {
	const char *p;
	void *data;
	u32 depth = 0;
	slice_t dt, ds;
	u32 sz, str;

	dt = dtree->dt;
	ds = dtree->ds;

	while (!sempty(&dt)) {
		u32 type = su32(&dt);
		switch (type) {
		case DT_END:
			if (depth)
				return oops(dtree, "unexpected DT_END");
			return 0;
		case DT_NODE_BEGIN:
			depth++;
			p = sstring(&dt);
			if (ncb(depth, p, cookie))
				return 0;
			break;
		case DT_NODE_END:
			if (depth == 0)
				return oops(dtree, "unexpected NODE_END");
			depth--;
			break;
		case DT_PROP:
			if (depth == 0)
				return oops(dtree, "PROP outside of NODE");
			sz = su32(&dt);
			str = su32(&dt);
			data = sdata(&dt, sz);
			if (pcb((const char*) (ds.data + str), data, sz, cookie))
				return 0;
			break;
		default:
			return oops(dtree, "invalid node type");

		}
	}

	if (depth != 0)
		return oops(dtree, "incomplete tree");

	return 0;
}
