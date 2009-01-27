/*
 * Copyright (c) 2009, Google Inc.
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

#include <debug.h>
#include <dev/keys.h>
#include <dev/gpio_keypad.h>
#include <lib/ptable.h>
#include <dev/flash.h>

#define BOARD_FLASH_OFFSET	378

static struct ptable flash_ptable;

/* for these partitions, start will be offset by either what we get from
 * smem, or from the above offset if smem is not useful. Also, we should
 * probably have smem_ptable code populate our flash_ptable.
 *
 * When smem provides us with a full partition table, we can get rid of
 * this altogether.
 *
 */
static struct ptentry board_part_list[] = {
	{
		.start = 0,
		.length = 1,
		.name = "aboot",
	},
	{
		.start = 2,
		.length = 40,
		.name = "boot",
	},
	{
		.start = 58,
		.length = 608 /* 76MB */,
		.name = "system",
	},
	{
		.start = 666,
		.length = 157 + 1024,
		.name = "userdata",
	},
	{
		.name = "",
	},
};

void smem_ptable_init(void);
unsigned smem_get_apps_flash_start(void);

void keypad_init(void);

void target_init(void)
{
	struct ptentry *ptn;
	unsigned offset;

	dprintf(INFO, "target_init()\n");

	keys_init();
	keypad_init();

	ptable_init(&flash_ptable);
	smem_ptable_init();

	offset = smem_get_apps_flash_start();
	if (offset == 0xffffffff)
		offset = BOARD_FLASH_OFFSET;

	for (ptn = &board_part_list[0]; ptn->name[0]; ptn++) {
		ptable_add(&flash_ptable, ptn->name, offset + ptn->start,
			   ptn->length, ptn->flags);
	}

	ptable_dump(&flash_ptable);

	flash_init(&flash_ptable);
}
