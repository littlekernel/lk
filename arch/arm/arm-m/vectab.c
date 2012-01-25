#include <compiler.h>
#include <stdint.h>

static uint8_t initial_stack[1024];

extern void _start(void);
extern void _nmi(void);
extern void _hardfault(void);
extern void _memmanage(void);
extern void _busfault(void);
extern void _usagefault(void);
extern void _svc(void);
extern void _pendsv(void);
extern void _pendsv(void);
extern void _systick(void);

const void * const __SECTION(".text.boot") vectab[] =
{
	/* arm exceptions */
	initial_stack + sizeof(initial_stack),
	_start,
	_nmi, // nmi
	_hardfault, // hard fault
	_memmanage, // mem manage
	_busfault, // bus fault
	_usagefault, // usage fault
	0, // reserved
	0, // reserved
	0, // reserved
	0, // reserved
	_svc, // svcall
	0, // debug monitor
	0, // reserved
	_pendsv, // pendsv
	_systick, // systick

	// XXX include irq init from platform space here
};



