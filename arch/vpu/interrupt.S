.macro SaveRegsLower
  stm lr, (--sp)
  nop
  nop
  nop
  nop
  stm r0-r5, (--sp)
  nop
  nop
  nop
  nop
.endm

.macro SaveRegsUpper
  stm r6-r15, (--sp)
  stm r16-r23, (--sp)
.endm

.macro SaveRegsAll
  SaveRegsLower
  SaveRegsUpper
.endm

.macro ExceptionHandler label, exception_number
.global fleh_\label
fleh_\label:
  SaveRegsLower
  mov r1, \exception_number
  b irq_fatal
.endm

.text
irq_fatal:
  SaveRegsUpper
  mov r0, sp
  b sleh_fatal

  ExceptionHandler zero,                  #0
  ExceptionHandler misaligned,            #1
  ExceptionHandler dividebyzero,          #2
  ExceptionHandler undefinedinstruction,  #3
  ExceptionHandler forbiddeninstruction,  #4
  ExceptionHandler illegalmemory,         #5
  ExceptionHandler buserror,              #6
  ExceptionHandler floatingpoint,         #7
  ExceptionHandler isp,                   #8
  ExceptionHandler dummy,                 #9
  ExceptionHandler icache,                #10
  ExceptionHandler veccore,               #11
  ExceptionHandler badl2alias,            #12
  ExceptionHandler breakpoint,            #13
  ExceptionHandler unknown,               #14

.global fleh_irq
fleh_irq:
  SaveRegsAll
  mov r0, sp
  mov r1, r29
  bl sleh_irq
return_from_exception:
  ldm r16-r23, (sp++)
  ldm r6-r15, (sp++)
  ldm r0-r5, (sp++)
  ld lr, (sp++)
  rti

.global fleh_swi
fleh_swi:
  stm lr, (--sp)
  stm r0-r5, (--sp)
  stm r6-r15, (--sp)
  stm r16-r23, (--sp)
  mov r0, sp
  bl sleh_swi
  ldm r16-r23, (sp++)
  ldm r6-r15, (sp++)
  ldm r0-r5, (sp++)
  ld lr, (sp++)
  rti
