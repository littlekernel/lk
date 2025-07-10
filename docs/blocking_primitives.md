# LK Kernel Blocking Primitives

## Overview

The LK kernel provides a comprehensive set of synchronization primitives for coordinating access to shared resources and enabling communication between threads. These primitives are built on top of the wait queue system and provide different semantics for various synchronization patterns.

## Core Architecture

### Wait Queues Foundation

All blocking primitives in LK are built upon wait queues (`wait_queue_t`), which provide the fundamental blocking and wakeup mechanisms:

- **Thread blocking**: Threads can be placed on wait queues and blocked until signaled
- **Timeout support**: All wait operations support optional timeouts
- **Wake semantics**: Support for waking one thread or all threads
- **Priority preservation**: Threads maintain their priority when blocked and resumed

### Locking Requirements

All synchronization primitives require the global thread lock (`thread_lock`) to be held when manipulating their internal state. This ensures atomic operations and prevents race conditions during state transitions.

## Synchronization Primitives

### 1. Mutexes

Mutexes provide exclusive access to shared resources with ownership semantics.

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
- **Priority inheritance**: Implicit through wait queue mechanism

#### API

```c
void mutex_init(mutex_t *m);
void mutex_destroy(mutex_t *m);
status_t mutex_acquire(mutex_t *m);                    // Infinite timeout
status_t mutex_acquire_timeout(mutex_t *m, lk_time_t timeout);
status_t mutex_release(mutex_t *m);
bool is_mutex_held(const mutex_t *m);                  // Check ownership
```

#### Usage Example

```c
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

#### Structure

```c
typedef struct semaphore {
    int magic;          // Magic number for validation
    int count;          // Available resource count
    wait_queue_t wait;  // Wait queue for blocked threads
} semaphore_t;
```

#### Key Properties

- **Resource counting**: Tracks available resource instances
- **No ownership**: Any thread can post/wait on a semaphore
- **Thread context only**: Cannot be used from interrupt context
- **FIFO ordering**: Waiting threads are woken in FIFO order

#### API

```c
void sem_init(semaphore_t *sem, unsigned int value);
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

#### Structure

```c
typedef struct event {
    int magic;          // Magic number for validation
    bool signaled;      // Current signal state
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
void event_init(event_t *e, bool initial, uint flags);
void event_destroy(event_t *e);
status_t event_wait(event_t *e);                      // Infinite timeout
status_t event_wait_timeout(event_t *e, lk_time_t timeout);
status_t event_signal(event_t *e, bool reschedule);
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

#### Structure

```c
typedef struct {
    char value[PORT_PACKET_LEN];  // Packet payload
} port_packet_t;

typedef struct {
    void *ctx;           // Associated context
    port_packet_t packet; // Message data
} port_result_t;
```

#### Port Types

- **Write Port**: Sending side of a communication channel
- **Read Port**: Receiving side of a communication channel
- **Port Group**: Collection of read ports for multiplexed waiting

#### Port Modes

```c
typedef enum {
    PORT_MODE_BROADCAST,   // Multiple readers can connect
    PORT_MODE_UNICAST,     // Single reader connection
    PORT_MODE_BIG_BUFFER   // Larger internal buffer
} port_mode_t;
```

#### Key Properties

- **Named channels**: Ports are identified by name strings
- **Buffered communication**: Internal buffering prevents blocking on send
- **Flexible topology**: Broadcast and unicast communication patterns
- **Context passing**: Associate arbitrary context with messages

#### API

```c
void port_init(void);
status_t port_create(const char *name, port_mode_t mode, port_t *port);
status_t port_destroy(port_t port);
status_t port_open(const char *name, void *ctx, port_t *port);
status_t port_close(port_t port);
status_t port_write(port_t port, const port_packet_t *pk, size_t count);
status_t port_read(port_t port, port_result_t *result);
status_t port_read_timeout(port_t port, port_result_t *result, lk_time_t timeout);

// Port groups for multiplexed reading
status_t port_group_create(port_t *group);
status_t port_group_add(port_t group, port_t port);
status_t port_group_remove(port_t group, port_t port);
status_t port_group_read(port_t group, port_result_t *result);
status_t port_group_read_timeout(port_t group, port_result_t *result, lk_time_t timeout);
```

#### Usage Example

```c
// Producer thread
void producer_thread(void *arg) {
    port_t write_port;
    port_create("data_channel", PORT_MODE_BROADCAST, &write_port);

    port_packet_t packet;
    // Fill packet with data
    fill_packet_data(&packet);

    port_write(write_port, &packet, 1);
    port_destroy(write_port);
}

// Consumer thread
void consumer_thread(void *arg) {
    port_t read_port;
    port_open("data_channel", NULL, &read_port);

    port_result_t result;
    if (port_read_timeout(read_port, &result, 1000) == NO_ERROR) {
        // Process received data
        process_packet_data(&result.packet);
    }

    port_close(read_port);
}
```

### 5. Spinlocks

Spinlocks provide lightweight mutual exclusion for short critical sections.

#### Structure

Architecture-specific spinlock implementation with common interface:

```c
typedef arch_spin_lock_t spin_lock_t;
typedef arch_spin_lock_saved_state_t spin_lock_saved_state_t;
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

// IRQ-safe variants
void spin_lock_irqsave(spin_lock_t *lock, spin_lock_saved_state_t *state);
void spin_unlock_irqrestore(spin_lock_t *lock, spin_lock_saved_state_t state);
```

#### Usage Example

```c
spin_lock_t hardware_lock = SPIN_LOCK_INITIAL_VALUE;

void access_hardware_register(void) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&hardware_lock, &state);

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
    void lock_irqsave(spin_lock_saved_state_t *state);
    void unlock_irqrestore(spin_lock_saved_state_t state);
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

#### API

```c
void wait_queue_init(wait_queue_t *wait);
void wait_queue_destroy(wait_queue_t *wait, bool reschedule);
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout);
int wait_queue_wake_one(wait_queue_t *wait, bool reschedule, status_t error);
int wait_queue_wake_all(wait_queue_t *wait, bool reschedule, status_t error);
status_t thread_unblock_from_wait_queue(thread_t *t, status_t error);
```

#### Timeout Handling

- **INFINITE_TIME**: Block indefinitely until signaled
- **0**: Return immediately with ERR_TIMED_OUT if would block
- **Positive values**: Block for specified milliseconds, return ERR_TIMED_OUT on expiration

### Priority Inheritance

While not explicitly implemented, the LK synchronization primitives provide implicit priority inheritance through the wait queue mechanism:

- **FIFO ordering**: Threads are generally woken in the order they were blocked
- **Priority-based scheduling**: High-priority threads are scheduled immediately when unblocked
- **Head insertion**: Newly unblocked threads are inserted at the head of their priority run queue

### Error Handling

#### Common Error Codes

- **NO_ERROR**: Operation successful
- **ERR_TIMED_OUT**: Operation timed out
- **ERR_NOT_READY**: Resource not available (try operations)
- **ERR_INVALID_ARGS**: Invalid parameters
- **ERR_OBJECT_DESTROYED**: Primitive was destroyed while waiting
- **ERR_NOT_ENOUGH_BUFFER**: Insufficient buffer space (ports)
- **ERR_THREAD_DETACHED**: Thread was detached (join operations)

#### Panic Conditions

Debug builds include assertions that will panic on:

- **Double acquisition**: Attempting to acquire a mutex already owned
- **Invalid release**: Releasing a mutex not owned by current thread
- **Magic number corruption**: Corrupted primitive structures
- **Invalid state transitions**: Inconsistent internal state

### SMP Considerations

#### CPU Wakeup

When threads are unblocked, the scheduler automatically handles CPU wakeup:

- **Pinned threads**: Wake the specific CPU where the thread is pinned
- **Unpinned threads**: Wake all CPUs except the local one
- **Load balancing**: Distribution across available CPUs

#### Memory Ordering

- **Architecture barriers**: Spinlocks include appropriate memory barriers
- **Cache coherency**: Hardware ensures cache coherence for shared data
- **Atomic operations**: Underlying atomic primitives ensure consistency

## Best Practices

### Choosing the Right Primitive

1. **Mutexes**: For exclusive access to shared resources with ownership
2. **Semaphores**: For counting resources or limiting concurrency
3. **Events**: For signaling and notification between threads
4. **Ports**: For message passing and producer-consumer patterns
5. **Spinlocks**: For very short critical sections or interrupt contexts

### Performance Considerations

#### Mutex vs Spinlock Trade-offs

**Use Mutexes when:**

- Critical sections may be long
- Thread context is available
- Resource contention is possible
- Priority inheritance is needed

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
semaphore_t empty_slots, full_slots;
mutex_t buffer_lock;

void producer(void) {
    sem_wait(&empty_slots);      // Wait for space
    mutex_acquire(&buffer_lock); // Protect buffer
    add_to_buffer(data);
    mutex_release(&buffer_lock);
    sem_post(&full_slots, true); // Signal data available
}

void consumer(void) {
    sem_wait(&full_slots);       // Wait for data
    mutex_acquire(&buffer_lock); // Protect buffer
    data = remove_from_buffer();
    mutex_release(&buffer_lock);
    sem_post(&empty_slots, true); // Signal space available
}
```

#### Event Notification

```c
// Using events for completion notification
event_t work_complete;

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

- **Thread context**: Cannot be used from interrupt handlers
- **Interrupts enabled**: Should not be called with interrupts disabled
- **No spinlocks held**: Deadlock risk if thread spinlocks are held

### Scheduler Integration

- **Automatic blocking**: Primitives automatically use `thread_block()`
- **Priority preservation**: Blocked threads maintain their priority
- **Fair scheduling**: FIFO ordering within priority levels
- **Efficient wakeup**: Minimal overhead for thread state transitions

### Memory Management

- **Static initialization**: All primitives support compile-time initialization
- **Dynamic allocation**: Runtime initialization for dynamically allocated primitives
- **Cleanup requirements**: Proper destruction prevents resource leaks
- **Magic number validation**: Debug builds validate primitive integrity

This comprehensive set of blocking primitives provides the foundation for safe, efficient multi-threaded programming in the LK kernel, supporting everything from simple mutual exclusion to complex communication patterns.
