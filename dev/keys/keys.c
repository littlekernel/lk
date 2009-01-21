/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <bits.h>
#include <debug.h>
#include <string.h>
#include <dev/keys.h>

static unsigned long key_bitmap[BITMAP_NUM_WORDS(MAX_KEYS)];

void keys_init(void)
{
	memset(key_bitmap, 0, sizeof(key_bitmap));
}

void keys_post_event(uint16_t code, int16_t value)
{
	if (code >= MAX_KEYS) {
		dprintf(INFO, "Invalid keycode posted: %d\n", code);
		return;
	}

	/* TODO: Implement an actual event queue if it becomes necessary */
	if (value)
		bitmap_set(key_bitmap, code);
	else
		bitmap_clear(key_bitmap, code);

//	dprintf(INFO, "key state change: %d %d\n", code, value);
}

int keys_get_state(uint16_t code)
{
	if (code >= MAX_KEYS) {
		dprintf(INFO, "Invalid keycode requested: %d\n", code);
		return -1;
	}
	return bitmap_test(key_bitmap, code);
}
