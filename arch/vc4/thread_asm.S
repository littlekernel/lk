.text
.global vc4_context_switch
// r0 is address to save the sp to
// r1 is the new sp to load
vc4_context_switch:
  stm lr, (--sp)
  stm r6-r15, (--sp)
  stm r16-r23, (--sp)
  st sp, (r0)
  mov sp, r1
  ldm r16-r23, (sp++)
  ldm r6-r15, (sp++)
  ldm pc,(sp++)
