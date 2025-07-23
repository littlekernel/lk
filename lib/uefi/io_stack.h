/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <stddef.h>
#include <uefi/types.h>

#include "switch_stack.h"

constexpr size_t kIoStackSize = 1024ul * 32;

void* get_io_stack();

template <typename Arg1, EfiStatus (*Func)(Arg1)>
EfiStatus (*switch_stack_wrapper())(Arg1 arg) {
  auto trampoline = [](Arg1 arg) -> EfiStatus {
    void* io_stack = reinterpret_cast<char*>(get_io_stack()) + kIoStackSize;
    return static_cast<EfiStatus>(call_with_stack(io_stack, Func, arg));
  };
  return trampoline;
}

template <typename Arg1, typename Arg2, EfiStatus (*Func)(Arg1, Arg2)>
EfiStatus (*switch_stack_wrapper())(Arg1 arg1, Arg2 arg2) {
  auto trampoline = [](Arg1 arg1, Arg2 arg2) -> EfiStatus {
    void* io_stack = reinterpret_cast<char*>(get_io_stack()) + kIoStackSize;
    return static_cast<EfiStatus>(call_with_stack(io_stack, Func, arg1, arg2));
  };
  return trampoline;
}

template <typename Arg1, typename Arg2, typename Arg3,
          EfiStatus (*Func)(Arg1, Arg2, Arg3)>
EfiStatus (*switch_stack_wrapper())(Arg1 arg1, Arg2 arg2, Arg3 arg3) {
  auto trampoline = [](Arg1 arg1, Arg2 arg2, Arg3 arg3) -> EfiStatus {
    void* io_stack = reinterpret_cast<char*>(get_io_stack()) + kIoStackSize;
    return static_cast<EfiStatus>(
        call_with_stack(io_stack, Func, arg1, arg2, arg3));
  };
  return trampoline;
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
          EfiStatus (*Func)(Arg1, Arg2, Arg3, Arg4)>
EfiStatus (*switch_stack_wrapper())(Arg1, Arg2, Arg3, Arg4) {
  auto trampoline = [](Arg1 arg1, Arg2 arg2, Arg3 arg3,
                       Arg4 arg4) -> EfiStatus {
    void* io_stack = reinterpret_cast<char*>(get_io_stack()) + kIoStackSize;
    return static_cast<EfiStatus>(
        call_with_stack(io_stack, Func, arg1, arg2, arg3, arg4));
  };
  return trampoline;
}
