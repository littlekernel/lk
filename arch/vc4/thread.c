#include <lk/debug.h>
#include <kernel/thread.h>

void vc4_context_switch(uint32_t *oldsp, uint32_t newsp);

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
  vc4_context_switch(&oldthread->arch.sp, newthread->arch.sp);
}

static inline void push(thread_t *t, uint32_t val) {
  // SP always points to the last valid value in the stack
  t->arch.sp -= 4;
  volatile uint32_t *ram32 = 0;
  ram32[t->arch.sp/4] = val;
}

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
  thread_t *ct = get_current_thread();

  int ret = ct->entry(ct->arg);

  thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
  t->arch.sp = (t->stack + t->stack_size) - 4;
  push(t, &initial_thread_func);
  push(t, 0); // the initial frame-pointer
}

void arch_dump_thread(thread_t *t) {
  if (t->state != THREAD_RUNNING) {
    dprintf(INFO, "\tarch: sp 0x%x\n", t->arch.sp);
  }
}
