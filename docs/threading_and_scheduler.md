# LK Threading and Scheduler System

## Overview

The LK kernel provides a comprehensive preemptive multithreading system with a priority-based scheduler. The threading system is designed to support both single-core and multi-core (SMP) systems with features including real-time scheduling, thread synchronization primitives, and comprehensive debugging capabilities.

## Core Components

### Thread Structure

Each thread in the system is represented by a `thread_t` structure containing:

- **Identification**: Magic number, name, and list membership
- **Scheduling State**: Priority, state, remaining quantum, and CPU affinity
- **Stack Management**: Stack pointer, size, and bounds checking
- **Synchronization**: Wait queue blocking and return codes
- **Architecture-specific**: CPU registers and context
- **Statistics**: Runtime statistics and performance counters (debug builds)

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

Threads can be marked as real-time using `thread_set_real_time()`. Real-time threads:

- Bypass preemption timers when priority > DEFAULT_PRIORITY
- Receive preferential scheduling treatment
- Are tracked separately for CPU load balancing

## Scheduler Algorithm

### Run Queue Management

The scheduler maintains:

- **Per-priority run queues**: Array of linked lists, one per priority level
- **Run queue bitmap**: Bit field indicating which priority levels have ready threads
- **Round-robin within priority**: Threads of equal priority are time-sliced

### Thread Selection

The scheduler uses the following algorithm:

1. Find the highest priority level with ready threads (using `__builtin_clz`)
2. Select the first thread from that priority's run queue
3. For SMP systems, consider CPU affinity and pinning
4. Fall back to idle thread if no threads are ready

### Preemption and Time Slicing

- **Quantum-based preemption**: Non-real-time threads receive time slices
- **Timer-driven preemption**: Periodic timer interrupts check quantum expiration
- **Voluntary yielding**: Threads can yield CPU with `thread_yield()`
- **Priority preemption**: Higher priority threads immediately preempt lower priority ones

## SMP (Symmetric Multiprocessing) Support

### CPU Affinity

- **Pinned threads**: Can be restricted to specific CPUs
- **Load balancing**: Unpinned threads distribute across available CPUs
- **CPU state tracking**: Idle, busy, and real-time CPU states

### Inter-CPU Communication

- **Reschedule IPIs**: Wake up remote CPUs for newly ready threads
- **CPU-specific idle threads**: Each CPU has its own idle thread
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
status_t thread_resume(thread_t *t);          // Start suspended thread
void thread_exit(int retcode);               // Terminate current thread
void thread_sleep(lk_time_t delay);          // Sleep for specified time
status_t thread_join(thread_t *t, int *retcode, lk_time_t timeout);
status_t thread_detach(thread_t *t);         // Detach thread for auto-cleanup
```

### Thread Properties

```c
void thread_set_name(const char *name);      // Change thread name
void thread_set_priority(int priority);      // Change thread priority
status_t thread_set_real_time(thread_t *t);  // Mark as real-time
```

### Scheduler Control

```c
void thread_yield(void);      // Voluntary CPU yield
void thread_preempt(void);    // Handle preemption
void thread_block(void);      // Block current thread
void thread_unblock(thread_t *t, bool resched); // Unblock thread
```

## Wait Queues

### Purpose

Wait queues provide the foundation for thread synchronization primitives:

- Mutexes and semaphores
- Condition variables
- Event notifications
- Resource waiting

### API

```c
void wait_queue_init(wait_queue_t *wait);
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout);
int wait_queue_wake_one(wait_queue_t *wait, bool reschedule, status_t error);
int wait_queue_wake_all(wait_queue_t *wait, bool reschedule, status_t error);
void wait_queue_destroy(wait_queue_t *wait, bool reschedule);
```

### Timeout Support

- **INFINITE_TIME**: Block indefinitely
- **0**: Return immediately with ERR_TIMED_OUT
- **Positive values**: Block for specified milliseconds
- **Timer-based wakeup**: Automatic unblocking on timeout

## Memory Management

### Stack Management

- **Automatic allocation**: Default stack allocation during thread creation
- **Custom stacks**: Support for user-provided stack memory
- **Stack bounds checking**: Debug-mode overflow detection (THREAD_STACK_BOUNDS_CHECK)
- **Stack usage tracking**: High-water mark monitoring (THREAD_STACK_HIGHWATER)
- **Delayed cleanup**: Safe stack deallocation after context switch

### Thread Structure Cleanup

- **Reference counting**: Automatic cleanup for detached threads
- **Join semantics**: Manual cleanup for joined threads
- **Delayed free**: Safe memory reclamation using heap_delayed_free()

## Thread Local Storage (TLS)

### Predefined TLS Slots

- **TLS_ENTRY_CONSOLE**: Current console context
- **TLS_ENTRY_UTHREAD**: User-mode thread context
- **TLS_ENTRY_LKUSER**: LK user library context

### TLS API

```c
uintptr_t tls_get(uint entry);
uintptr_t tls_set(uint entry, uintptr_t val);
```

## Debugging and Statistics

### Thread Statistics (THREAD_STATS)

Per-thread statistics:

- Total runtime
- Schedule count
- Last run timestamp

Per-CPU statistics:

- Idle time
- Context switches
- Preemptions and yields
- Reschedule IPIs

### Debug Features

- **Magic number validation**: Corruption detection
- **Stack overflow detection**: Bounds checking with guard pages
- **Thread state validation**: Assertion checking
- **Thread dumping**: Complete thread state inspection

### Debug API

```c
void dump_thread(thread_t *t);        // Dump single thread
void dump_all_threads(void);          // Dump all threads
void dump_threads_stats(void);        // Dump statistics
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

- **O(1) thread selection**: Bitmap-based priority queue
- **Minimal context switch overhead**: Architecture-optimized switching
- **Lockless fast paths**: Reduced lock contention where possible

### SMP Scalability

- **CPU-local data structures**: Reduced cache line bouncing
- **Intelligent load balancing**: CPU affinity and pinning support
- **Efficient IPI usage**: Targeted CPU wakeups

### Real-time Responsiveness

- **Priority inheritance**: Through wait queue mechanisms
- **Bounded latency**: Real-time thread preemption guarantees
- **Timer precision**: High-resolution timer support

## Integration Points

### Architecture Layer

- **arch_thread_initialize()**: Architecture-specific thread setup
- **arch_context_switch()**: Low-level context switching
- **arch_get/set_current_thread()**: Current thread management

### Platform Layer

- **Timer integration**: Preemption timer management
- **Power management**: CPU idle state handling
- **Debug LED control**: Visual debugging support

### Virtual Memory

- **Address space switching**: VMM integration for process isolation
- **Stack allocation**: Virtual memory for thread stacks

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
- Consider CPU affinity for performance-critical threads

### Synchronization

- Always use proper locking with wait queues
- Handle timeout conditions appropriately
- Avoid priority inversion through careful design
- Use real-time threads sparingly

### Resource Management

- Detach threads that don't need join semantics
- Handle thread cleanup properly
- Monitor stack usage in debug builds
- Use TLS appropriately for per-thread data

This threading and scheduler system provides a robust foundation for kernel and application development in the LK operating system, with careful attention to performance, scalability, and debugging capabilities.
