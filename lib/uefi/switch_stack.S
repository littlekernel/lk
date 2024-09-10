/*
 * Copyright (C) 2024 The Android Open Source Project
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
 
#include <lk/asm.h>


// int call_with_stack(void *stack, int (*fp)(), int param1, int param2);
FUNCTION(call_with_stack)
stp     fp, lr, [sp, #-16]!
mov     fp, sp

sub     x0,x0,16
mov     x6,sp
str     x6,[x0]
mov     sp,x0
mov     x5,x1
mov     x0,x2
mov     x1,x3
blr     x5
ldr     x6,[sp]
mov     sp,x6

ldp     fp, lr, [sp], 16
ret     lr
