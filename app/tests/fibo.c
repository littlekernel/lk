/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <rand.h>
#include <err.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

static int fibo_thread(void *argv)
{
	int fibo = (int)argv;

	thread_t *t[2];

	if (fibo == 0)
		return 0;
	if (fibo == 1)
		return 1;

	t[0] = thread_create("fibo", &fibo_thread, (void *)(fibo - 1), DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	if (!t[0]) {
		printf("error creating thread for fibo %d\n", fibo-1);
		return 0;
	}
	t[1] = thread_create("fibo", &fibo_thread, (void *)(fibo - 2), DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	if (!t[1]) {
		printf("error creating thread for fibo %d\n", fibo-2);
		thread_resume(t[0]);
		thread_join(t[0], NULL, INFINITE_TIME);
		return 0;
	}

	thread_resume(t[0]);
	thread_resume(t[1]);

	int retcode0, retcode1;

	thread_join(t[0], &retcode0, INFINITE_TIME);
	thread_join(t[1], &retcode1, INFINITE_TIME);

	return retcode0 + retcode1;
}

int fibo(int argc, const cmd_args *argv)
{

	if (argc < 2) {
		printf("not enough args\n");
		return -1;
	}

	thread_t *t = thread_create("fibo", &fibo_thread, (void *)argv[1].u, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	thread_resume(t);

	int retcode;
	thread_join(t, &retcode, INFINITE_TIME);

	printf("fibo %d\n", retcode);

	return NO_ERROR;
}


