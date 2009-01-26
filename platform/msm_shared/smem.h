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

#ifndef __PLATFORM_MSM_SHARED_SMEM_H
#define __PLATFORM_MSM_SHARED_SMEM_H

#include <sys/types.h>

struct smem_proc_comm
{
	unsigned command;
	unsigned status;
	unsigned data1;
	unsigned data2;
};

struct smem_heap_info
{
	unsigned initialized;
	unsigned free_offset;
	unsigned heap_remaining;
	unsigned reserved;
};

struct smem_alloc_info
{
	unsigned allocated;
	unsigned offset;
	unsigned size;
	unsigned reserved;
};

struct smem {
	struct smem_proc_comm		proc_comm[4];
	unsigned			version_info[32];
	struct smem_heap_info		heap_info;
	struct smem_alloc_info		alloc_info[128];
};

typedef enum {
	SMEM_SPINLOCK_ARRAY = 7,

	SMEM_AARM_PARTITION_TABLE = 9,

	SMEM_FIRST_VALID_TYPE = SMEM_SPINLOCK_ARRAY,
	SMEM_LAST_VALID_TYPE = SMEM_AARM_PARTITION_TABLE,
} smem_mem_type_t;

/* Note: buf MUST be 4byte aligned, and max_len MUST be a multiple of 4. */
unsigned smem_read_alloc_entry(smem_mem_type_t type, void *buf, int max_len);

#endif /* __PLATFORM_MSM_SHARED_SMEM_H */
