/*
 *  nomutex.c
 *
 *  This code demonstrates the need for mutual exclusion, by
 *  creating a number of threads that all access the same global
 *  variables.
 *
 */

#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lk/console_cmd.h>
#include <lk/err.h>

#include <kernel/thread.h>

/*
 *  The number of threads that we want to have running
 *  simultaneously.
 */

#define NUMTHREADS      1       // try with one thread, then with 4 to show problem
/*
 *  the global variables that the threads compete for.
 *  To demonstrate contention, there are two variables
 *  that have to be updated "atomically".  With RR
 *  scheduling, there is a possibility that one thread
 *  will update one of the variables, and get preempted
 *  by another thread, which will update both.  When our
 *  original thread runs again, it will continue the
 *  update, and discover that the variables are out of
 *  sync.
 *
 *      Note: Error checking has been left out in much of this example
 *      to increase readability.  Production code should not leave out
 *      this error checking.
 */

static volatile unsigned var1;
static volatile unsigned var2;
static int cnt;

static int update_thread(void *);

static volatile int done;

int nomutex(int argc, const console_cmd_args *argv)
{
	thread_t *threadID[NUMTHREADS]; // a place to hold the thread ID's
    status_t ret; // a place to hold the thread status
	int i;

	var1 = var2 = 0; /* initialize to known values */

	printf("mutex_sync:  starting; creating threads\n");

	/*
	 *  create the threads.  As soon as each thread_create
	 *  call is done, the thread has been started.
	 */

	for (i = 0; i < NUMTHREADS; i++)
	{
		threadID[i] = thread_create("nomutex", &update_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
		ret = thread_resume(threadID[i]);
		if ( ret != 0 )
		{
			fprintf(stderr, "thread_resume failed: %s\n", strerror(ret));
			return ERR_NO_MEMORY;
		}
	}

	/*
	 *  let the other threads run for a while
	 */

	thread_sleep(15000);

    /*
     * Tell the threads to exit.
     */

    done = 1;

    // wait for them to exit
    for (i = 0; i < NUMTHREADS; i++)
    {
        ret = thread_join(threadID[i], NULL, INFINITE_TIME);
        if ( ret != 0 )
        {
            fprintf(stderr, "thread_join failed: %s\n", strerror(ret));
            return ERR_NO_MEMORY;
        }
    }

	printf("all done, var1 is %u, var2 is %u\n", var1, var2);

	return NO_ERROR;
}

/*
 *  the actual thread.
 *
 *  The thread ensures that var1 == var2.  If this is not the
 *  case, the thread sets var1 = var2, and prints a message.
 *
 *  Var1 and Var2 are incremented.
 *
 *  Looking at the source, if there were no "synchronization" problems,
 *  then var1 would always be equal to var2.  Run this program and see
 *  what the actual result is...
 */

static void do_work()
{
	static volatile unsigned var3;

	var3++;
	/* For faster/slower processors, may need to tune this program by
	 * modifying the frequency of this printf -- add/remove a 0
	 */
	if (!(var3 % 10000000))
        printf("thread did some work\n");
}

static int
update_thread(void *i)
{
	while (!done)
	{
		if (var1 != var2)
		{
            printf("var1 (%u) is not equal to var2 (%u)!\n", var1, var2);
			var1 = var2;
		}

		/* do some work here */
		do_work();

		var1 += 2;
		var1--;
		var2 += 2;
		var2--;
	}
	return NO_ERROR;
}

STATIC_COMMAND_START
STATIC_COMMAND("nomutex", "help", &nomutex)
STATIC_COMMAND_END(nomutex);