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
irqType __attribute__ ((aligned (512))) vectorTable[144]; // might only need to be 128 entries

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
  // TODO
  for (int i=0; i<64; i++) {
    irq_handlers[0].h = 0; // is this needed? maybe .bss already took care of it?
    set_interrupt(i, false, 0);
    set_interrupt(i, false, 1);
  }
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
    vectorTable[i] = fleh_irq;
  }
  // external interrupts
  for (int i=64; i<=127; i++) {
    vectorTable[i] = fleh_irq;
  }

  *REG32(IC0_VADDR) = vectorTable;
  *REG32(IC1_VADDR) = vectorTable;

  if (*REG32(IC0_VADDR) != vectorTable) {
    printf("vector table now at 0x%08lx 0x%08lx\n", *REG32(IC0_VADDR), (uint32_t)vectorTable);
    panic("vector table failed to install");
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
  __asm__ volatile("di");
  irq_handlers[vector].h = handler;
  irq_handlers[vector].arg = arg;
  __asm__ volatile("ei");
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

void sleh_irq(vc4_saved_state_t* pcb, uint32_t tp) {
  uint32_t status = *REG32(IC0_S);
  uint32_t source = status & 0xFF;
  uint32_t cs;
  int ret;

  //dprintf(INFO, "VPU Received interrupt from source %d\n", source);

  switch (source) {
  case 64: // timer0
  case 121: // uart
    assert(irq_handlers[source - 64].h);
    ret = irq_handlers[source - 64].h(irq_handlers[source - 64].arg);
    if (ret == INT_RESCHEDULE) thread_preempt();
    break;
  case INTERRUPT_ARM:
    // fired when the arm cpu writes to the arm->vpu mailbox
    break;
  default:
    print_vpu_state(pcb);
    panic("unknown interrupt source!");
  }
}
