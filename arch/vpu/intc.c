#include <lk/debug.h>
#include <platform/interrupts.h>
#include <lk/err.h>
#include <platform/bcm28xx.h>
#include <assert.h>
#include <lk/reg.h>
#include <kernel/thread.h>

#include <arch/vc4_traps.h>

struct handlerArgPair {
  int_handler h;
  void *arg;
};
typedef void (*irqType)(void);

struct handlerArgPair irq_handlers[64];
// when an exception or interrupt occurs, the cpu will make sp into an alias pointing to r28
// it will then push pc and sr onto the new stack
// it will then read an entry from this vector table, and set the PC to that entry
// if the highest bit on this addr is set, the cpu will switch into supervisor mode
irqType vectorTable[128] __attribute__ ((section(".data.vectorTable")));

uint8_t irq_stack0[1024];

static const char* g_ExceptionNames[] = {
  "Zero",
  "Misaligned",
  "Division by zero",
  "Undefined instruction",
  "Forbidden instruction",
  "Illegal memory",
  "Bus error",
  "Floating point exception",
  "ISP",
  "Dummy",
  "ICache",
  "Vector core exception",
  "Bad L2 alias",
  "Breakpoint"
};

void set_interrupt(int intno, bool enable, int core) {
  uint32_t base = (core == 0) ? IC0_BASE : IC1_BASE;

  int offset = 0x10 + ((intno >> 3) << 2);
  uint32_t slot = 0xF << ((intno & 7) << 2);

  uint32_t v = *REG32(base + offset) & ~slot;
  *REG32(base + offset) = enable ? v | slot : v;
}

void intc_init(void) {
  uint32_t r28, sp;
  __asm__ volatile ("mov %0, r28" : "=r"(r28));
  __asm__ volatile ("mov %0, sp" : "=r"(sp));
  dprintf(INFO, "intc_init\nr28: 0x%x\nsp: 0x%x\n", r28, sp);
  // TODO
  for (int i=0; i<64; i++) {
    irq_handlers[0].h = 0; // is this needed? maybe .bss already took care of it?
  }
  // rather then call set_interrupt for each bit in each byte, just blanket clear all
  // this will disable every hardware irq
  volatile uint32_t *maskreg = (uint32_t*)(IC0_BASE + 0x10);
  for (int i=0; i<8; i++) maskreg[i] = 0;
  maskreg = (uint32_t*)(IC1_BASE + 0x10);
  for (int i=0; i<8; i++) maskreg[i] = 0;

  // https://github.com/hermanhermitage/videocoreiv/wiki/VideoCore-IV-Programmers-Manual#interrupts
  // processor internal exceptions
  vectorTable[0] = fleh_zero;
  vectorTable[1] = fleh_misaligned;
  vectorTable[2] = fleh_dividebyzero;
  vectorTable[3] = fleh_undefinedinstruction;
  vectorTable[4] = fleh_forbiddeninstruction;
  vectorTable[5] = fleh_illegalmemory;
  vectorTable[6] = fleh_buserror;
  vectorTable[7] = fleh_floatingpoint;
  vectorTable[8] = fleh_isp;
  vectorTable[9] = fleh_dummy;
  vectorTable[10] = fleh_icache;
  vectorTable[11] = fleh_veccore;
  vectorTable[12] = fleh_badl2alias;
  vectorTable[13] = fleh_breakpoint;
  for (int i=14; i<=31; i++) {
    vectorTable[i] = fleh_unknown;
  }
  // swi opcode handler
  for (int i=32; i<=63; i++) {
    vectorTable[i] = fleh_swi;
  }
  // external interrupts
  for (int i=64; i<=127; i++) {
    vectorTable[i] = (irqType)((uint32_t)fleh_irq | 1);
  }

  uint32_t irq_sp = (uint32_t)((irq_stack0 + sizeof(irq_stack0)) - 4);
  dprintf(INFO, "r28 = 0x%x\nirq_stack0: %p\nsizeof(irq_stack0): %d\n", irq_sp, irq_stack0, sizeof(irq_stack0));

  __asm__ volatile ("mov r28, %0": :"r"(irq_sp));

  *REG32(IC0_VADDR) = (uint32_t)vectorTable;
  *REG32(IC1_VADDR) = (uint32_t)vectorTable;

  if (((void *)*REG32(IC0_VADDR)) != vectorTable) {
    printf("vector table now at 0x%08x 0x%08x\n", *REG32(IC0_VADDR), (uint32_t)vectorTable);
    panic("vector table failed to install\n");
  }
}

#define REGISTER_FORMAT_STRING(prefix) \
        prefix "  r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n" \
        prefix "  r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n" \
        prefix "  r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n" \
        prefix " r12: 0x%08x r13: 0x%08x r14: 0x%08x r15: 0x%08x\n" \
        prefix "  pc: 0x%08x  lr: 0x%08x  sr: 0x%08x\n"

void print_vpu_state(vc4_saved_state_t* pcb) {
        printf("VPU registers:\n");

        printf(
            REGISTER_FORMAT_STRING("   "),
            pcb->r0,
            pcb->r1,
            pcb->r2,
            pcb->r3,
            pcb->r4,
            pcb->r5,
            pcb->r6,
            pcb->r7,
            pcb->r8,
            pcb->r9,
            pcb->r10,
            pcb->r11,
            pcb->r12,
            pcb->r13,
            pcb->r14,
            pcb->r15,
            pcb->pc,
            pcb->lr,
            pcb->sr
        );

        printf("Exception info (IC0):\n");

        printf(
            "   src0: 0x%08x src1: 0x%08x vaddr: 0x%08x\n"
            "      C: 0x%08x    S: 0x%08x\n",
            *REG32(IC0_SRC0),
            *REG32(IC0_SRC1),
            *REG32(IC0_VADDR),
            *REG32(IC0_C),
            *REG32(IC0_S)
        );

        printf("Exception info (IC1):\n");

        printf(
            "   src0: 0x%08x src1: 0x%08x vaddr: 0x%08x\n"
            "      C: 0x%08x    S: 0x%08x\n",
            *REG32(IC1_SRC0),
            *REG32(IC1_SRC1),
            *REG32(IC1_VADDR),
            *REG32(IC1_C),
            *REG32(IC1_S)
        );
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
  assert(vector < 64);

  spin_lock_saved_state_t state;
  arch_interrupt_save(&state, 0);

  irq_handlers[vector].h = handler;
  irq_handlers[vector].arg = arg;

  arch_interrupt_restore(state, 0);
}

status_t unmask_interrupt(unsigned int vector) {
  assert(vector < 64);
  set_interrupt(vector, true, 0);
  return NO_ERROR;
}

static const char* exception_name(uint32_t n) {
  if (n >= (sizeof(g_ExceptionNames)/4)) return "unknown";
  return g_ExceptionNames[n];
}

// c mode handlers, called by interrupt.S

void sleh_fatal(vc4_saved_state_t* pcb, uint32_t n) {
  printf("Fatal VPU Exception: %s\n", exception_name(n));

  print_vpu_state(pcb);

  printf("We are hanging here ...\n");

  while(true) __asm__ volatile ("nop");
}

// upon entry to this function(before its prologue runs), sp and r0 point to a `struct vc4_saved_state_t`
// r0 (which lands in pcb) contains a copy of that sp from before the prologue
// some common values and offsets:
// r0 +   0: r23
// ...
// r0 +  92: r0
// r0 +  96: lr
// r0 + 100: sr
// r0 + 104: pc
void sleh_irq(vc4_saved_state_t* pcb, uint32_t tp) {
  uint32_t status = *REG32(IC0_S);
  uint32_t source = status & 0xFF;
  enum handler_return ret = INT_NO_RESCHEDULE;

  //uint32_t sp, sr;
  //__asm__ volatile ("mov %0, sp" : "=r"(sp));
  //__asm__ volatile ("mov %0, sr" : "=r"(sr));
  //dprintf(INFO, "sleh_irq\nsp: 0x%x\nsr: 0x%x\n", sp, sr);

  //dprintf(INFO, "VPU Received interrupt from source %d\n", source);

  switch (source) {
  case 64: // timer0
  case 73: // dwc2
  case 121: // uart
    assert(irq_handlers[source - 64].h);
    ret = irq_handlers[source - 64].h(irq_handlers[source - 64].arg);
    if (ret == INT_RESCHEDULE) {
      //dprintf(INFO, "pre-emptying\n");
      thread_preempt();
      //dprintf(INFO, "done preempt\n");
    }
    break;
  case INTERRUPT_ARM:
    // fired when the arm cpu writes to the arm->vpu mailbox
    break;
  default:
    print_vpu_state(pcb);
    panic("unknown interrupt source!");
  }
}

void sleh_swi(vc4_saved_state_t* pcb) {
  dprintf(INFO, "got SWI\n");
  print_vpu_state(pcb);
}
