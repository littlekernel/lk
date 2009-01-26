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
#include <reg.h>
#include <string.h>
#include <sys/types.h>
#include <platform/iomap.h>

#include "smem.h"

struct smem_ptn {
	char name[16];
	unsigned start;
	unsigned size;
	unsigned attr;
} __attribute__ ((__packed__));

struct smem_ptable {
#define _SMEM_PTABLE_MAGIC_1 0x55ee73aa
#define _SMEM_PTABLE_MAGIC_2 0xe35ebddb
	unsigned magic[2];
	unsigned version;
	unsigned len;
	struct smem_ptn parts[16];
} __attribute__ ((__packed__));

/* partition table from SMEM */
static struct smem_ptable smem_ptable;
static unsigned smem_apps_flash_start;

static void dump_smem_ptable(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		struct smem_ptn *p = &smem_ptable.parts[i];
		if (p->name[0] == '\0')
			continue;
		dprintf(SPEW, "%d: %s offs=0x%08x size=0x%08x attr: 0x%08x\n",
			i, p->name, p->start, p->size, p->attr);
	}
}

void smem_ptable_init(void)
{
	unsigned i;

	smem_apps_flash_start = 0xffffffff;

	i = smem_read_alloc_entry(SMEM_AARM_PARTITION_TABLE,
				    &smem_ptable, sizeof(smem_ptable));
	if (i != 0)
		return;

	if (smem_ptable.magic[0] != _SMEM_PTABLE_MAGIC_1 ||
	    smem_ptable.magic[1] != _SMEM_PTABLE_MAGIC_2)
		return;

	dump_smem_ptable();
	dprintf(INFO, "smem ptable found: ver: %d len: %d\n",
		smem_ptable.version, smem_ptable.len);

	for (i = 0; i < smem_ptable.len; i++) {
		if (!strcmp(smem_ptable.parts[i].name, "0:APPS"))
		    break;
	}
	if (i == smem_ptable.len)
		return;

	smem_apps_flash_start = smem_ptable.parts[i].start;
}

unsigned smem_get_apps_flash_start(void)
{
	return smem_apps_flash_start;
}
