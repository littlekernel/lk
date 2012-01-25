#include <debug.h>
#include <compiler.h>
#include <stdint.h>

/* externals */
extern unsigned int __data_start_rom, __data_start, __data_end;
extern unsigned int __bss_start, __bss_end;

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

void _start(void)
{
	/* copy data from rom */
	if (&__data_start != &__data_start_rom) {
		unsigned int *src = &__data_start_rom;
		unsigned int *dest = &__data_start;

		while (dest != &__data_end)
			*dest++ = *src++;
	}

	/* zero out bss */
	unsigned int *bss = &__bss_start;
	while (bss != &__bss_end)
		*bss++ = 0;

	for (;;)
		;
}
