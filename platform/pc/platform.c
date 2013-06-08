/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <debug.h>
#include <arch/x86/mmu.h>
#include <platform.h>
#include "platform_p.h"
#include <platform/pc.h>
#include <platform/multiboot.h>
#include <platform/console.h>
#include <platform/keyboard.h>
#include <dev/pci.h>
#include <dev/uart.h>

extern multiboot_info_t *_multiboot_info;
extern unsigned int _heap_end;

void platform_init_mmu_mappings(void)
{
	/* do some memory map initialization */
}


void platform_init_multiboot_info(void)
{
	unsigned int i;

	if (_multiboot_info) {
		if (_multiboot_info->flags & MB_INFO_MEM_SIZE) {
			_heap_end = _multiboot_info->mem_upper * 1024;
		}

		if (_multiboot_info->flags & MB_INFO_MMAP) {
			memory_map_t *mmap = (memory_map_t *) (_multiboot_info->mmap_addr - 4);

			dprintf(SPEW, "mmap length: %u\n", _multiboot_info->mmap_length);

			for (i=0; i < _multiboot_info->mmap_length / sizeof(memory_map_t); i++) {
				dprintf(SPEW, "base=%08x, length=%08x, type=%02x\n",
				        mmap[i].base_addr_low, mmap[i].length_low, mmap[i].type);

				if (mmap[i].type == MB_MMAP_TYPE_AVAILABLE && mmap[i].base_addr_low >= _heap_end) {
					_heap_end = mmap[i].base_addr_low + mmap[i].length_low;
				} else if (mmap[i].type != MB_MMAP_TYPE_AVAILABLE && mmap[i].base_addr_low >= _heap_end) {
					/*
					 * break on first memory hole above default heap end for now.
					 * later we can add facilities for adding free chunks to the
					 * heap for each segregated memory region.
					 */
					break;
				}
			}
		}
	}
}

void platform_early_init(void)
{
	platform_init_uart();

	/* update the heap end so we can take advantage of more ram */
	platform_init_multiboot_info();

	/* get the text console working */
	platform_init_console();

	/* initialize the interrupt controller */
	platform_init_interrupts();

	/* initialize the timer */
	platform_init_timer();
}

void platform_init(void)
{
	uart_init();

	platform_init_keyboard();

	pci_init();
}

