.text
.global vc4_context_switch
// r0 is address of the old arch_thread
// r1 is address of the new arch_thread
vc4_context_switch:
  // save all state that is caller saved
  stm lr, (--sp)
  stm r6-r15, (--sp)
  stm r16-r23, (--sp)


  // swap stacks
  st sp, (r0)
  mov r2, sr
  st r2, (r0+4)

  ld sp, (r1)
  mov r3, sp
  //swi 0

  // restore all state from stack
  ldm r16-r23, (sp++)
  ldm r6-r15, (sp++)
  ldm r0, (sp++)
  mov lr, r0

  lea r0, vc4_context_switch_finish
  st r0, (--sp)
  ld r1, (r1+4) // load saved SR
  st r1, (--sp)
  rti

oldvc4_context_switch:
  stm lr, (--sp)
  stm r6-r15, (--sp)
  stm r16-r23, (--sp)
  st sp, (r0)
  mov sp, r1
  ldm r16-r23, (sp++)
  ldm r6-r15, (sp++)
  ldm pc,(sp++)

vc4_context_switch_finish:
  rts
