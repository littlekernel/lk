# LK Threading and Scheduler System

## Overview

The LK kernel provides a comprehensive preemptive multithreading system with a priority-based scheduler. The threading system is designed to support both single-core and multi-core (SMP) systems with features including real-time scheduling, thread synchronization primitives, and comprehensive debugging capabilities.

## Core Components

### Thread Structure

Each thread in the system is represented by a `thread_t` structure containing:

- **Identification**: Magic number, name, and list membership
- **Scheduling State**: Priority, state, remaining quantum, current CPU and pinned CPU
- **Stack Management**: Stack pointer, size, and bounds checking
- **Synchronization**: Wait queue blocking and return codes
- **Architecture-specific**: CPU registers and context
- **Statistics**: Per-thread runtime statistics (debug builds)
- **Thread Local Storage**: Array of TLS entries including errno

### Thread States

Threads can exist in one of the following states:

- **THREAD_SUSPENDED**: Newly created thread not yet running
- **THREAD_READY**: Thread ready to run, in the run queue
- **THREAD_RUNNING**: Currently executing thread
- **THREAD_BLOCKED**: Waiting on a synchronization primitive
- **THREAD_SLEEPING**: Sleeping for a specified time period
- **THREAD_DEATH**: Thread has exited and awaits cleanup

## Priority System

### Priority Levels

The scheduler uses a 32-level priority system (0-31):

- **LOWEST_PRIORITY (0)**: Minimum priority level
- **IDLE_PRIORITY (0)**: Reserved for idle threads
- **LOW_PRIORITY (8)**: Low priority tasks
- **DEFAULT_PRIORITY (16)**: Standard thread priority
- **HIGH_PRIORITY (24)**: High priority tasks
- **DPC_PRIORITY (30)**: Deferred Procedure Call priority
- **HIGHEST_PRIORITY (31)**: Maximum priority level

### Real-time Threads

Threads can be marked as real-time using `thread_set_real_time()`, which sets
`THREAD_FLAG_REAL_TIME`. There is no call to clear it. Real-time threads:

- Are never preempted by the quantum timer, at any priority. They run until they block, yield,
  or something on their own CPU triggers a reschedule
- Mark their CPU as real-time while running, if their priority is above `DEFAULT_PRIORITY`.
  Reschedule IPIs skip real-time CPUs, so other CPUs do not interrupt them

## Scheduler Algorithm

### Run Queue Management

The scheduler maintains:

- **Per-priority run queues**: Array of linked lists, one per priority level
- **Run queue bitmap**: Bit field indicating which priority levels have ready threads
- **Round-robin within priority**: Threads of equal priority are time-sliced
- **Head or tail insertion**: Threads woken from a wait queue or sleep, newly resumed threads
  and preempted threads with quantum left go to the head of their priority's queue. Yielding
  threads and threads whose quantum ran out go to the tail

### Thread Selection

The scheduler uses the following algorithm:

1. Find the highest priority level with ready threads (using `__builtin_clz`)
2. Select the first thread from that priority's run queue
3. On SMP, skip threads pinned to another CPU, moving on to lower priorities if needed
4. Fall back to idle thread if no threads are ready

### Preemption and Time Slicing

- **Quantum-based preemption**: A non-real-time thread gets a quantum of 5 ticks, refilled
  when it is switched in with none left
- **Timer-driven preemption**: With `PLATFORM_HAS_DYNAMIC_TIMER`, each CPU runs a 10ms periodic
  preemption timer while a non-real-time, non-idle thread is running, for a time slice of about
  50ms. Other platforms count the quantum down on the platform's periodic timer tick
- **Voluntary yielding**: `thread_yield()` puts the current thread at the tail of its run queue
- **Priority preemption**: A newly ready higher priority thread runs at the next reschedule on
  a CPU that may run it: when the waker passes `resched = true`, when a reschedule IPI or an
  interrupt handler requests a reschedule, or at the next quantum tick

## SMP (Symmetric Multiprocessing) Support

### Run Queue and Locking

- **Single global run queue**: All CPUs share one set of per-priority run queues. A CPU that
  reschedules takes the highest priority thread it is allowed to run
- **Global scheduler lock**: The run queues, thread states and all wait queues are protected by
  the `thread_lock` spinlock, taken with `THREAD_LOCK(state)` / `THREAD_UNLOCK(state)`, which
  also disable and restore interrupts
- **CPU state tracking**: `mp.idle_cpus` and `mp.realtime_cpus` record which CPUs are idle or
  running a real-time thread

### CPU Pinning

A thread can be pinned to a single CPU. There is no multi-CPU affinity mask.

```c
void thread_set_pinned_cpu(thread_t *t, int cpu);  // -1 unpins
int thread_pinned_cpu(const thread_t *t);          // -1 if unpinned
```

`thread_set_pinned_cpu()` takes `thread_lock`, so it must not be called with it held; on non-SMP
builds it is an empty stub. What it does beyond recording the CPU depends on the thread's state:

- **The calling thread, pinned to another CPU**: migrates synchronously. The thread is requeued at
  the head of the run queue, the target CPU is sent a reschedule IPI, and the call returns once the
  thread is running there. Interrupts must be enabled, the target CPU must be active, and the idle
  thread cannot do this.
- **A READY thread**: the target CPU is sent a reschedule IPI so it picks the thread up.
- **A thread running on another CPU**: is not forced off it. It moves at that CPU's next
  reschedule, and the destination CPU is not woken.
- **Blocked, sleeping or suspended threads**: only record the CPU, which takes effect when the
  thread next becomes runnable. Pinning before `thread_resume()` is the simple case.

Unpinning (`-1`) only clears the field. `kernel/test/thread_tests.c` covers the self-migration
and repinning a queued thread.

### Inter-CPU Communication

- **Reschedule IPIs**: Resuming a thread or waking it from a wait queue sends a reschedule IPI
  to the CPU it is pinned to, or to every other active CPU if it is unpinned. CPUs running a real-time thread are skipped,
  and the local CPU is never signalled
- **CPU-specific idle threads**: Each CPU has its own idle thread, pinned to it
- **Per-CPU preemption timers**: Independent timer management per CPU

## Thread Management API

### Thread Creation

```c
thread_t *thread_create(const char *name,
                       thread_start_routine entry,
                       void *arg,
                       int priority,
                       size_t stack_size);

thread_t *thread_create_etc(thread_t *t,
                           const char *name,
                           thread_start_routine entry,
                           void *arg,
                           int priority,
                           void *stack,
                           size_t stack_size);
```

### Thread Control

```c
status_t thread_resume(thread_t *t);                           // Start suspended thread
void thread_exit(int retcode);                                 // Terminate current thread
void thread_sleep(lk_time_t delay);                            // Sleep for specified time
status_t thread_join(thread_t *t, int *retcode, lk_time_t timeout);
status_t thread_detach(thread_t *t);                           // Detach thread for auto-cleanup
status_t thread_detach_and_resume(thread_t *t);                // thread_detach(), then thread_resume()
```

### Thread Properties

```c
void thread_set_name(const char *name);                       // Rename the current thread
void thread_set_priority(int priority);                       // Current thread; clamped to 1..31, then reschedules
status_t thread_set_real_time(thread_t *t);                   // Mark as real-time
#if WITH_SMP
void thread_create_secondary_cpu_idle_thread(uint cpu);       // Create idle thread for secondary CPU
#endif
```

### Scheduler Control

```c
void thread_yield(void);      // Voluntary CPU yield
void thread_preempt(void);    // Handle preemption

// Internal. The caller must hold thread_lock and have put the thread in
// THREAD_BLOCKED on a queue of its own; use a wait queue instead.
void thread_block(void);
void thread_unblock(thread_t *t, bool resched);
```

## Wait Queues

### Purpose

Wait queues provide the foundation for thread synchronization primitives:

- Mutexes and semaphores
- Events
- Ports
- `thread_join()`

### API

```c
void wait_queue_init(wait_queue_t *wait);
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout);
int wait_queue_wake_one(wait_queue_t *wait, bool reschedule, status_t error);
int wait_queue_wake_all(wait_queue_t *wait, bool reschedule, status_t error);
void wait_queue_destroy(wait_queue_t *wait, bool reschedule);
```

Every call except `wait_queue_init()` must be made with `thread_lock` held, and so with
interrupts disabled:

```c
THREAD_LOCK(state);
status_t err = wait_queue_block(&queue, timeout);
THREAD_UNLOCK(state);
```

The wake calls may be made from interrupt context, with `reschedule` false.

### Timeout Support

- **INFINITE_TIME**: Block indefinitely
- **0**: Return immediately with ERR_TIMED_OUT
- **Positive values**: Block for specified milliseconds
- **Timer-based wakeup**: Automatic unblocking on timeout

## Memory Management

### Stack Management

- **Automatic allocation**: The stack is allocated from the heap when none is passed in
- **Custom stacks**: Support for user-provided stack memory
- **Stack bounds checking**: Debug-mode overflow detection (THREAD_STACK_BOUNDS_CHECK). A stack the
  kernel allocates gets `THREAD_STACK_PADDING_SIZE` bytes of fill pattern at its base, checked
  each time the thread is switched out
- **Stack usage tracking**: High-water mark monitoring (THREAD_STACK_HIGHWATER)
- **Delayed cleanup**: Safe stack deallocation after context switch

### Thread Structure Cleanup

The stack and thread structure are freed only if the kernel allocated them.

- **Detached threads**: Free themselves on exit (`THREAD_FLAG_DETACHED`)
- **Joined threads**: `thread_join()` frees the thread once it has exited
- **Delayed free**: An exiting detached thread is still running on its stack, so it frees
  with heap_delayed_free()

## Thread Local Storage (TLS)

### Predefined TLS Slots

Available TLS entries depend on build configuration:

- **TLS_ENTRY_CONSOLE**: Current console context (requires `WITH_LIB_CONSOLE`)
- **TLS_ENTRY_UTHREAD**: User-mode thread context (requires `WITH_LIB_UTHREAD`)
- **TLS_ENTRY_LKUSER**: LK user library context (requires `WITH_LIB_LKUSER`)
- **TLS_ENTRY_ERRNO**: Thread-safe errno value (always available)

### TLS API

```c
uintptr_t tls_get(uint entry);
uintptr_t tls_set(uint entry, uintptr_t val);  // Macro; entry must be a compile-time constant. Returns the old value
```

### Usage Notes

Conditional TLS entries are only included when their corresponding library is compiled in. A new thread inherits every TLS entry from the thread that created it, except `TLS_ENTRY_ERRNO`, which is always available and starts at 0 to support thread-safe errno handling.

## Debugging and Statistics

### Thread Statistics (THREAD_STATS)

Per-thread statistics:

- Total runtime
- Schedule count
- Last run timestamp

Per-CPU statistics, printed by the `threadstats` shell command:

- Idle time
- Context switches
- Preemptions and yields
- Reschedule IPIs

### Debug Features

- **Magic number validation**: Corruption detection
- **Stack overflow detection**: Fill pattern check on context switch (there are no guard pages)
- **Thread state validation**: Assertion checking
- **Thread dumping**: Complete thread state inspection

### Debug API

```c
void dump_thread(const thread_t *t);          // Dump single thread
void arch_dump_thread(const thread_t *t);     // Architecture-specific thread dump
void dump_all_threads(void);                  // Dump all threads
void dump_all_threads_unlocked(void);         // Dump all threads (no lock held)
void dump_threads_stats(void);                // Dump per-thread statistics
```

## Initialization

### Early Initialization

```c
void thread_init_early(void);
```

- Initialize global data structures
- Create bootstrap thread
- Set up run queues and thread list

### Full Initialization

```c
void thread_init(void);
```

- Initialize preemption timers
- Complete threading system setup

### SMP Initialization

```c
void thread_secondary_cpu_init_early(void);  // Per-CPU early init
void thread_secondary_cpu_entry(void);       // Secondary CPU entry point
void thread_become_idle(void);               // Become idle thread
```

## Performance Considerations

### Scheduler Overhead

- **Thread selection**: The run queue bitmap finds the highest ready priority in O(1); threads
  pinned to other CPUs are skipped by walking that priority's queue
- **Minimal context switch overhead**: Architecture-optimized switching
- **Single scheduler lock**: Every scheduler and wait queue operation takes the global
  `thread_lock`

There is no priority inheritance. A high priority thread waiting on a mutex does not boost
the holder.

## Integration Points

### Architecture Layer

- **arch_thread_initialize()**: Architecture-specific thread setup called during thread creation
- **arch_context_switch()**: Low-level context switching implementation
- **arch_dump_thread()**: Architecture-specific thread state display (called by dump_thread)
- **arch_get_current_thread()**: Retrieve current thread pointer
- **arch_set_current_thread()**: Set current thread pointer

### Platform Layer

- **Timer integration**: Preemption timer management
- **Power management**: CPU idle state handling
- **Debug LED control**: Visual debugging support

### Virtual Memory

- **Address space switching**: A context switch calls `vmm_context_switch()` when the incoming
  thread's `aspace` differs from the outgoing one

## Error Handling

### Common Error Codes

- **ERR_TIMED_OUT**: Wait operation timeout
- **ERR_NOT_BLOCKED**: Thread not in blocked state
- **ERR_THREAD_DETACHED**: Operation on detached thread
- **ERR_OBJECT_DESTROYED**: Wait queue destroyed
- **ERR_INVALID_ARGS**: Invalid function arguments

### Panic Conditions

- Stack overflow detection
- Invalid thread magic numbers
- Inconsistent thread state
- Failed assertions in debug builds

## Best Practices

### Thread Creation

- Use appropriate priority levels
- Specify reasonable stack sizes
- Choose meaningful thread names
- Pin a thread only when it must run on a particular CPU

### Synchronization

- Hold `thread_lock` around wait queue calls
- Handle timeout conditions appropriately
- Avoid priority inversion through careful design; there is no priority inheritance
- Use real-time threads sparingly

### Resource Management

- Detach threads that don't need join semantics
- Handle thread cleanup properly
- Monitor stack usage in debug builds
- Use TLS appropriately for per-thread data

This threading and scheduler system provides a robust foundation for kernel and application development in the LK operating system, with careful attention to performance and debugging capabilities.
