#include <debug.h>
#include <compiler.h>
#include <stdint.h>

void _nmi(void)
{
	printf("nmi\n");
	halt();
}

void _hardfault(void)
{
	printf("hardfault\n");
	halt();
}

void _memmanage(void)
{
	printf("memmanage\n");
	halt();
}

void _busfault(void)
{
	printf("busfault\n");
	halt();
}

void _usagefault(void)
{
	printf("usagefault\n");
	halt();
}

void _svc(void)
{
	printf("svc\n");
	halt();
}

void _pendsv(void)
{
	printf("pendsv\n");
	halt();
}

void _systick(void)
{
	printf("systick\n");
	halt();
}


