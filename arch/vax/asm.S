/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

// void vax_context_switch(struct vax_pcb *newpcb);
FUNCTION(vax_context_switch)
    .word 0 // nothing saved

    // on the old stack save the current PSL and the PC to the exit of this function,
    // to be popped by the svpctx instruction
    movpsl -(%sp)
    moval  .Lreturn, -(%sp)

    // load the new pcb into r0
    movl    4(%ap),%r0

    // save the full state of the cpu, switching to interrupt stack
    svpctx

    mtpr    %r0,$0x10   // load the new PCBB

    // load new process context, leaves the new PSL and PC on the stack
    ldpctx

    // return to the new thread
    rei

.Lreturn:
    // when an old thread is switch back to, arrange for the return address to be here
    ret

// trampoline when initially starting a thread to get from a fake saved process
// context to the C world, which requires a calls instruction.
.globl vax_initial_thread_func
vax_initial_thread_func:
    calls   $0,initial_thread_func
    halt

