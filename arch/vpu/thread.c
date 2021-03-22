#include <lk/debug.h>
#include <kernel/thread.h>

void vc4_context_switch(struct arch_thread *oldsp, struct arch_thread *newsp);

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
  uint32_t r28, sp;
  __asm__ volatile ("mov %0, r28" : "=r"(r28));
  __asm__ volatile ("mov %0, sp" : "=r"(sp));
  //dprintf(INFO, "arch_context_switch\nr28: 0x%x\nsp: 0x%x\n", r28, sp);
  //dprintf(INFO, "switching (%s) -> %p(%s)\n", oldthread->name, newthread->arch.sp, newthread->name);
  //dprintf(INFO, "old: %p %s\nSP: 0x%x\nSR: 0x%x\n", oldthread, oldthread->name, oldthread->arch.sp, oldthread->arch.sr);
  //dprintf(INFO, "new: %p %s\nSP: 0x%x\nSR: 0x%x\n", newthread, newthread->name, newthread->arch.sp, newthread->arch.sr);
  vc4_context_switch(&oldthread->arch, &newthread->arch);
  //dprintf(INFO, "switched\n\n");
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
  //uint32_t own_sp;

  //__asm__ volatile ("mov %0, sp": "=r"(own_sp));
  //dprintf(INFO, "thread %p(%s) starting with sp near 0x%x\n", ct, ct->name, own_sp);
  spin_unlock(&thread_lock);
  arch_enable_ints();

  int ret = ct->entry(ct->arg);

  thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
  //printf("thread %p(%s) has a stack of %p+0x%x\n", t, t->name, t->stack, t->stack_size);
  t->arch.sp = (uint32_t)((t->stack + t->stack_size) - 4);
  __asm__ volatile ("mov %0, sr": "=r"(t->arch.sr));
  t->arch.sr &= ~0x40000000; // disable irq in the new thread
  //printf("initial sr: 0x%x\n", t->arch.sr);
  push(t, (uint32_t)(&initial_thread_func)); // lr
  for (int i=6; i<=23; i++) {
    push(t, 0); // r${i}
  }
}

void arch_dump_thread(thread_t *t) {
  if (t->state != THREAD_RUNNING) {
    dprintf(INFO, "\tarch: sp 0x%x\n\t      sr: 0x%x\n", t->arch.sp, t->arch.sr);
  }
  /* else if (t->state == THREAD_RUNNING) {
    uint32_t sp;
    __asm__ volatile ("mov %0, sp" : "=r"(sp));
    dprintf(INFO, "\tarch: sp ~0x%x\n", sp);
  }*/
}
