# LK Kernel Blocking Primitives

## Overview

The LK kernel provides a comprehensive set of synchronization primitives for coordinating access to shared resources and enabling communication between threads. These primitives are built on top of the wait queue system and provide different semantics for various synchronization patterns.

## Core Architecture

### Wait Queues Foundation

Most blocking primitives in LK are built upon wait queues (`wait_queue_t`) (with the exception of spinlocks), which provide the fundamental blocking and wakeup mechanisms:

- **Thread blocking**: Threads can be placed on wait queues and blocked until signaled
- **Timeout support**: All wait operations support optional timeouts
- **Wake semantics**: Support for waking one thread or all threads
- **Priority preservation**: Threads maintain their priority when blocked and resumed

Most code in the system will not use a wait queue directly, it acts as the building block for other primitives.

## Synchronization Primitives

### 1. Mutexes

Mutexes provide exclusive access to shared resources with ownership semantics.
See [`mutex.h`](../kernel/include/kernel/mutex.h) implementation details.

#### Structure

```c
typedef struct mutex {
    uint32_t magic;      // Magic number for validation
    int count;           // Contention counter
    thread_t *holder;    // Currently owning thread
    wait_queue_t wait;   // Wait queue for blocked threads
} mutex_t;
```

#### Key Properties

- **Ownership**: Only the thread that acquired the mutex can release it
- **Non-recursive**: A thread cannot acquire the same mutex multiple times
- **Thread context only**: Cannot be used from interrupt context
- **No priority inheritance**: A high priority waiter does not boost the holder

#### API

```c
void mutex_init(mutex_t *m); // Initialze a mutex to the default state.
void mutex_destroy(mutex_t *m);
status_t mutex_acquire_timeout(mutex_t *m, lk_time_t timeout);
status_t mutex_acquire(mutex_t *m); // Same as above but with infinite timeout.
status_t mutex_release(mutex_t *m); // Release the mutex, must be the holding thread.
bool is_mutex_held(const mutex_t *m); // Is the mutex held by the current thread?
```

#### Usage Example

```c
// Static initialization of the mutex. Equivalent to mutex_init().
mutex_t resource_lock = MUTEX_INITIAL_VALUE(resource_lock);

void protected_function(void) {
    status_t result = mutex_acquire(&resource_lock);
    if (result == NO_ERROR) {
        // Critical section - exclusive access to resource
        access_shared_resource();
        mutex_release(&resource_lock);
    }
}
```

#### C++ Wrapper

```cpp
class Mutex {
public:
    status_t acquire(lk_time_t timeout = INFINITE_TIME);
    status_t release();
    bool is_held();
};

class AutoLock {
public:
    explicit AutoLock(mutex_t *mutex);  // RAII lock acquisition
    ~AutoLock();                        // Automatic release
    void release();                     // Early release
};
```

### 2. Semaphores

Semaphores control access to a finite number of resources using a counter mechanism.
See [`semaphore.h`](../kernel/include/kernel/semaphore.h) implementation details.

#### Structure

```c
typedef struct semaphore {
    uint32_t magic;     // Magic number for validation
    int count;          // Available resource count
    wait_queue_t wait;  // Wait queue for blocked threads
} semaphore_t;
```

#### Key Properties

- **Resource counting**: Tracks available resource instances
- **No ownership**: Any thread can post/wait on a semaphore
- **Waiting needs thread context**: `sem_post(sem, false)` and `sem_trywait()` may be called
  from interrupt context
- **FIFO ordering**: Waiting threads are woken in FIFO order

#### API

```c
// Static initialization: semaphore_t s = SEMAPHORE_INITIAL_VALUE(s, count);
void sem_init(semaphore_t *sem, int initial_count);
void sem_destroy(semaphore_t *sem);
status_t sem_wait(semaphore_t *sem);                   // Infinite timeout
status_t sem_timedwait(semaphore_t *sem, lk_time_t timeout);
status_t sem_trywait(semaphore_t *sem);               // Non-blocking
int sem_post(semaphore_t *sem, bool resched);         // Signal availability
```

#### Usage Example

```c
semaphore_t resource_pool;

void init_resource_pool(void) {
    sem_init(&resource_pool, 5);  // 5 available resources
}

void use_resource(void) {
    if (sem_wait(&resource_pool) == NO_ERROR) {
        // Use one resource
        use_shared_resource();
        sem_post(&resource_pool, true);  // Return resource
    }
}
```

#### Semaphore Semantics

- **Positive count**: Resources available, wait operations succeed immediately
- **Zero count**: No resources available, threads block on wait operations
- **Negative count**: Represents number of waiting threads

### 3. Events

Events provide signaling mechanisms for thread coordination and notification.
See [`event.h`](../kernel/include/kernel/event.h) implementation details.

#### Structure

```c
typedef struct event {
    uint32_t magic;     // Magic number for validation
    uint32_t signaled;  // Current signal state, accessed atomically
    uint flags;         // Behavior flags
    wait_queue_t wait;  // Wait queue for blocked threads
} event_t;
```

#### Event Flags

- **EVENT_FLAG_AUTOUNSIGNAL**: Automatically clear signal after waking one thread
- **Default (no flags)**: Remain signaled until manually cleared

#### Key Properties

- **State-based**: Maintains signaled/unsignaled state
- **Flexible semantics**: Support for one-shot and persistent signaling
- **Interrupt safe**: Can be signaled from interrupt context (with resched=false)
- **No ownership**: Any thread can signal or wait

#### API

```c
// Static initialization: event_t e = EVENT_INITIAL_VALUE(e, initial, flags);
void event_init(event_t *e, bool initial, uint flags);
void event_destroy(event_t *e);
status_t event_wait(event_t *e);                      // Infinite timeout
status_t event_wait_timeout(event_t *e, lk_time_t timeout);
int event_signal(event_t *e, bool reschedule);        // Returns number of threads woken
status_t event_unsignal(event_t *e);
bool event_initialized(event_t *e);
```

#### Usage Examples

#### One-shot Event (Auto-unsignal)

```c
event_t completion_event;

void init_completion(void) {
    event_init(&completion_event, false, EVENT_FLAG_AUTOUNSIGNAL);
}

void wait_for_completion(void) {
    event_wait(&completion_event);  // Blocks until signaled
}

void signal_completion(void) {
    event_signal(&completion_event, true);  // Wake one waiter
}
```

#### Persistent Event

```c
event_t ready_event;

void init_ready_state(void) {
    event_init(&ready_event, false, 0);  // No auto-unsignal
}

void wait_until_ready(void) {
    event_wait(&ready_event);  // All waiters proceed when signaled
}

void set_ready(void) {
    event_signal(&ready_event, true);  // Wake all waiters
}

void clear_ready(void) {
    event_unsignal(&ready_event);  // Manual clear
}
```

### 4. Ports

Ports provide message-passing communication channels between threads with buffering.
See [`port.h`](../kernel/include/kernel/port.h) implementation details.

#### Structure

```c
typedef struct {
    char value[PORT_PACKET_LEN];  // Packet payload
} port_packet_t;

typedef struct {
    void *ctx;            // ctx passed to port_open() for the read port
    port_packet_t packet; // Message data
} port_result_t;
```

`PORT_PACKET_LEN` is two pointers wide.

#### Port Types

- **Write Port**: Sending side of a communication channel
- **Read Port**: Receiving side of a communication channel
- **Port Group**: Collection of read ports for multiplexed waiting

#### Port Modes

```c
typedef enum {
    PORT_MODE_BROADCAST   = 0,  // Any number of read ports
    PORT_MODE_UNICAST     = 1,  // A single read port
    PORT_MODE_BIG_BUFFER  = 2,  // Flag, only valid as PORT_MODE_UNICAST | PORT_MODE_BIG_BUFFER
} port_mode_t;
```

#### Key Properties

- **Named channels**: Ports are identified by names of at most `PORT_NAME_LEN - 1` (11)
  characters
- **Non-blocking writes**: `port_write()` never blocks. A buffer holds 8 packets (64 with
  `PORT_MODE_BIG_BUFFER`), and a write that does not fit fails
- **Flexible topology**: Broadcast and unicast communication patterns
- **Per-reader context**: The `ctx` given to `port_open()` comes back in every result read
  through that read port, which identifies the source when reading a port group
- **Runtime only**: Ports are heap objects with no static initializer

#### API

```c
// Write side. If the name exists, the existing port is returned with ERR_ALREADY_EXISTS.
status_t port_create(const char *name, port_mode_t mode, port_t *port);
// All or nothing per read port. ERR_NOT_ENOUGH_BUFFER if no read port is open and the
// port's own buffer is full, ERR_PARTIAL_WRITE if any read port's buffer was full.
status_t port_write(port_t port, const port_packet_t *pk, size_t count);

// Read side. Fails with ERR_NOT_FOUND until the write port exists.
status_t port_open(const char *name, void *ctx, port_t *port);
// Read from a read port or a port group. A timeout of 0 polls.
status_t port_read(port_t port, lk_time_t timeout, port_result_t *result);

// Port groups, read with port_read(). A read port can be in one group at a time.
status_t port_group(port_t *ports, size_t count, port_t *group);
status_t port_group_add(port_t group, port_t port);
status_t port_group_remove(port_t group, port_t port);

// Closes any kind of port. A closed write port can still be opened and read.
status_t port_close(port_t port);
// Write ports only, and only once closed. Read ports drain what is queued, then
// return ERR_CANCELLED.
status_t port_destroy(port_t port);
```

#### Usage Example

```c
static port_t write_port, read_port;

// Set up both ends before the threads start. Packets queued while no read port
// is open are handed to the first one to open, but port_destroy() drops them.
status_t channel_init(void) {
    status_t err = port_create("data", PORT_MODE_UNICAST, &write_port);
    if (err < 0)
        return err;
    return port_open("data", NULL, &read_port);
}

static int producer_thread(void *arg) {
    port_packet_t packet;
    fill_packet_data(&packet);

    if (port_write(write_port, &packet, 1) < 0) {
        // the reader's buffer is full
    }

    // A write port must be closed before it is destroyed.
    port_close(write_port);
    port_destroy(write_port);
    return 0;
}

static int consumer_thread(void *arg) {
    port_result_t result;
    status_t err;
    while ((err = port_read(read_port, 1000, &result)) == NO_ERROR) {
        process_packet_data(&result.packet);
    }
    // ERR_TIMED_OUT, or ERR_CANCELLED once the write port is destroyed and drained
    port_close(read_port);
    return 0;
}
```

Waiting on several read ports at once:

```c
port_t group;
port_t members[] = { read_port_a, read_port_b };
port_group(members, 2, &group);

port_result_t result;
port_read(group, INFINITE_TIME, &result);  // result.ctx tells which member it came from

port_close(group);  // does not close the members
```

### 5. Spinlocks

Spinlocks provide lightweight mutual exclusion for short critical sections.
See [`spinlock.h`](../kernel/include/kernel/spinlock.h) implementation details.

#### Structure

Architecture-specific spinlock implementation with common interface. Each architecture's
`<arch/spinlock.h>` defines `spin_lock_t` (a word sized integer) and `SPIN_LOCK_INITIAL_VALUE`:

```c
typedef unsigned long spin_lock_t;  // arm64; other architectures use unsigned int or unsigned long
```

#### Key Properties

- **Non-blocking**: Busy-wait instead of sleeping
- **Interrupt safe**: Can be used from interrupt context
- **Short critical sections**: Designed for brief atomic operations
- **No recursion**: Cannot be acquired recursively
- **Priority inversion risk**: Can cause priority inversion

#### API

```c
void spin_lock_init(spin_lock_t *lock);
void spin_lock(spin_lock_t *lock);                    // Assumes interrupts disabled
int spin_trylock(spin_lock_t *lock);                  // Non-blocking attempt
void spin_unlock(spin_lock_t *lock);
bool spin_lock_held(spin_lock_t *lock);

// Wrapper functions that disable and restore interrupts, saving interrupt state
// into 'state'.
arch_interrupt_saved_state_t spin_lock_irqsave(spin_lock_t *lock);
void spin_unlock_irqrestore(spin_lock_t *lock, arch_interrupt_saved_state_t state);
```

#### Usage Example

```c
spin_lock_t hardware_lock = SPIN_LOCK_INITIAL_VALUE;

void access_hardware_register(void) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&hardware_lock);

    // Brief critical section
    write_hardware_register(value);

    spin_unlock_irqrestore(&hardware_lock, state);
}
```

#### C++ Wrapper

```cpp
class SpinLock {
public:
    void lock();
    int trylock();
    void unlock();
    bool is_held();
    arch_interrupt_saved_state_t lock_irqsave();
    void unlock_irqrestore(arch_interrupt_saved_state_t state);
};

class AutoSpinLock {
public:
    explicit AutoSpinLock(spin_lock_t *lock);  // RAII with IRQ save
    ~AutoSpinLock();
    void release();
};

class AutoSpinLockNoIrqSave {
public:
    explicit AutoSpinLockNoIrqSave(spin_lock_t *lock);  // RAII without IRQ save
    ~AutoSpinLockNoIrqSave();
    void release();
};
```

## Advanced Topics

### Wait Queues

The foundation primitive underlying all blocking synchronization:
See [`wait.h`](../kernel/include/kernel/wait.h) implementation details.

#### API

```c
void wait_queue_init(wait_queue_t *wait);
void wait_queue_destroy(wait_queue_t *wait, bool reschedule);
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout);
int wait_queue_wake_one(wait_queue_t *wait, bool reschedule, status_t error);
int wait_queue_wake_all(wait_queue_t *wait, bool reschedule, status_t error);
status_t thread_unblock_from_wait_queue(thread_t *t, status_t error);
```

Every call except `wait_queue_init()` must be made with the global `thread_lock` held
(`THREAD_LOCK(state)` / `THREAD_UNLOCK(state)` from `thread.h`), and so with interrupts
disabled. The wake calls may be made from interrupt context with `reschedule` false.

#### Timeout Handling

- **INFINITE_TIME**: Block indefinitely until signaled
- **0**: Return immediately with ERR_TIMED_OUT if would block
- **Positive values**: Block for specified milliseconds, return ERR_TIMED_OUT on expiration

### Priority Inheritance

The wait queues provide no mechanism to handle priority inversion at this time. All threads are woken in FIFO order.

### Error Handling

#### Common Error Codes

- **NO_ERROR**: Operation successful
- **ERR_TIMED_OUT**: Operation timed out
- **ERR_NOT_READY**: Resource not available (try operations)
- **ERR_INVALID_ARGS**: Invalid parameters
- **ERR_OBJECT_DESTROYED**: Primitive was destroyed while waiting
- **ERR_NOT_ENOUGH_BUFFER**: Port write did not fit, no read port open
- **ERR_PARTIAL_WRITE**: Port write did not fit in at least one read port's buffer
- **ERR_CANCELLED**: Port read after the write port was destroyed and drained
- **ERR_THREAD_DETACHED**: Thread was detached (join operations)

#### Panic Conditions

Debug builds include assertions that will panic on:

- **Double acquisition**: Attempting to acquire a mutex already owned
- **Invalid release**: Releasing a mutex not owned by current thread
- **Magic number corruption**: Corrupted primitive structures
- **Invalid state transitions**: Inconsistent internal state

### SMP Considerations

#### CPU Wakeup

When threads are unblocked, the scheduler sends reschedule IPIs to other CPUs:

- **Pinned threads**: The CPU the thread is pinned to
- **Unpinned threads**: Every other active CPU; the first to reschedule takes the thread from
  the shared run queue
- CPUs running a real-time thread are skipped. The waking CPU switches only when the caller
  reschedules: `resched = true`, or an interrupt handler returning `INT_RESCHEDULE`

#### Memory Ordering

- **Architecture barriers**: All primitives include appropriate memory barriers
- **Cache coherency**: Hardware ensures cache coherence for shared data
- **Atomic operations**: Underlying atomic primitives ensure consistency

## Best Practices

### Choosing the Right Primitive

1. **Mutexes**: For exclusive access to shared resources with ownership
2. **Semaphores**: For counting resources or limiting concurrency
3. **Events**: For signaling and notification between threads and from IRQ context
4. **Ports**: For message passing and producer-consumer patterns
5. **Spinlocks**: For very short critical sections or interrupt contexts

### Performance Considerations

#### Mutex vs Spinlock Trade-offs

**Use Mutexes when:**

- Critical sections may be long
- Thread context is available
- Resource contention is possible

**Use Spinlocks when:**

- Critical sections are very short (< 100 cycles)
- Interrupt context or high-priority threads
- Low contention expected
- Immediate response required

#### Avoiding Priority Inversion

- **Keep critical sections short**: Minimize time holding locks
- **Consistent lock ordering**: Prevent deadlock with multiple locks
- **Avoid nested locking**: Reduce complexity and deadlock risk
- **Use appropriate primitives**: Match primitive to use case

### Common Patterns

#### Producer-Consumer

```c
// Using semaphores
semaphore_t empty_slots = SEMAPHORE_INITIAL_VALUE(empty_slots, NUM_SLOTS);
semaphore_t full_slots = SEMAPHORE_INITIAL_VALUE(full_slots, 0);
mutex_t buffer_lock = MUTEX_INITIAL_VALUE(buffer_lock);

void producer(int data) {
    sem_wait(&empty_slots);      // Wait for space
    mutex_acquire(&buffer_lock); // Protect buffer
    add_to_buffer(data);
    mutex_release(&buffer_lock);
    sem_post(&full_slots, true); // Signal data available
}

int consumer(void) {
    sem_wait(&full_slots);       // Wait for data
    mutex_acquire(&buffer_lock); // Protect buffer
    int data = remove_from_buffer();
    mutex_release(&buffer_lock);
    sem_post(&empty_slots, true); // Signal space available
    return data;
}
```

#### Event Notification

```c
// Using events for completion notification
event_t work_complete = EVENT_INITIAL_VALUE(work_complete, false, 0);

void worker_thread(void) {
    // Perform work
    do_work();

    // Signal completion
    event_signal(&work_complete, true);
}

void coordinator_thread(void) {
    // Start work
    start_work();

    // Wait for completion
    event_wait(&work_complete);

    // Process results
    process_results();
}
```

#### Resource Pool Management

```c
// Using semaphores for resource pool
semaphore_t resource_pool;
mutex_t pool_lock;
resource_t *resources[MAX_RESOURCES];

void init_pool(void) {
    sem_init(&resource_pool, MAX_RESOURCES);
    mutex_init(&pool_lock);
    // Initialize resource array
}

resource_t *acquire_resource(void) {
    if (sem_wait(&resource_pool) == NO_ERROR) {
        mutex_acquire(&pool_lock);
        resource_t *res = find_free_resource();
        mark_resource_used(res);
        mutex_release(&pool_lock);
        return res;
    }
    return NULL;
}

void release_resource(resource_t *res) {
    mutex_acquire(&pool_lock);
    mark_resource_free(res);
    mutex_release(&pool_lock);
    sem_post(&resource_pool, true);
}
```

## Integration with Threading System

### Lock Context Requirements

All blocking primitives (except spinlocks) require:

- **Thread context to wait**: Acquire and wait calls cannot be made from interrupt handlers.
  `event_signal()` and `sem_post()` may be called from interrupt context with `resched = false`
- **Interrupts enabled**: Calls that may block must not be made with interrupts disabled
- **No spinlocks held**: Deadlock risk if thread spinlocks are held

### Scheduler Integration

- **Blocking**: Primitives block through `wait_queue_block()` with `thread_lock` held
- **Priority preservation**: Blocked threads maintain their priority
- **Wakeup placement**: Woken threads go to the head of their priority's run queue, ahead of
  threads that were already ready at that priority

### Memory Management

- **Static initialization**: `MUTEX_INITIAL_VALUE`, `SEMAPHORE_INITIAL_VALUE`,
  `EVENT_INITIAL_VALUE`, `WAIT_QUEUE_INITIAL_VALUE` and `SPIN_LOCK_INITIAL_VALUE`. Ports are
  created at runtime only
- **Dynamic allocation**: Runtime initialization for dynamically allocated primitives
- **Cleanup requirements**: Proper destruction prevents resource leaks
- **Magic number validation**: Debug builds check the magic of mutexes, events and wait queues.
  Semaphores are checked only through their embedded wait queue
