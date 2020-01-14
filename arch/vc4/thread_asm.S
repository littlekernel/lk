.text
.global vc4_context_switch
// r0 is address to save the sp to
// r1 is the new sp to load
vc4_context_switch:
  stm r6,lr,(--sp)
  st sp, (r0)
  mov sp, r1
  ldm r6,pc,(sp++)
