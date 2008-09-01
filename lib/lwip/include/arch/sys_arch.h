/*
 * Copyright (c) 2006 Travis Geiselbrecht
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
#ifndef __LWIP_SYS_ARCH_H
#define __LWIP_SYS_ARCH_H

#include <kernel/thread.h>


struct sys_sem_struct {
	int count;
	wait_queue_t wait;
};

struct sys_mbox_struct {
	void *msg;
	wait_queue_t wait;
};

typedef struct sys_sem_struct *sys_sem_t;
typedef struct sys_mbox_struct *sys_mbox_t;
typedef thread_t *sys_thread_t;

#define SYS_MBOX_NULL ((sys_mbox_t)0)
#define SYS_SEM_NULL  ((sys_sem_t)0)


#endif

