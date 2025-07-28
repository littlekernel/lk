
#ifndef __LIB_UEFI_THREAD_UTILS_H_
#define __LIB_UEFI_THREAD_UTILS_H_

#include <kernel/thread.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus

template <typename Func>
int functor_thread_trampoline(void *f) {
  auto callable = reinterpret_cast<Func *>(f);
  int ret = (*callable)();
  delete callable;
  return ret;
}

template <typename Func>
thread_t *thread_create_functor(const char *name, Func f, int priority,
                                size_t stack_size) {
  auto heap_functor = new decltype(f)(f);
  return thread_create(name, functor_thread_trampoline<Func>, heap_functor,
                       priority, stack_size);
}
#endif

#endif