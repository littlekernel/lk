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
#include <debug.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <arch/x86/descriptor.h>
#include <dev/pci.h>

static int last_bus = 0;

typedef struct {
	uint16_t size;
	void *offset;
	uint16_t selector;
} __PACKED irq_routing_options_t;

static int pci_type1_detect(void);
static int pci_bios_detect(void);

int pci_get_last_bus(void)
{
	return last_bus;
}

/*
 * pointers to installed PCI routines
 */
int (*g_pci_find_pci_device)(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index);
int (*g_pci_find_pci_class_code)(pci_location_t *state, uint32_t class_code, uint16_t index);

int (*g_pci_read_config_byte)(const pci_location_t *state, uint32_t reg, uint8_t *value);
int (*g_pci_read_config_half)(const pci_location_t *state, uint32_t reg, uint16_t *value);
int (*g_pci_read_config_word)(const pci_location_t *state, uint32_t reg, uint32_t *value);

int (*g_pci_write_config_byte)(const pci_location_t *state, uint32_t reg, uint8_t value);
int (*g_pci_write_config_half)(const pci_location_t *state, uint32_t reg, uint16_t value);
int (*g_pci_write_config_word)(const pci_location_t *state, uint32_t reg, uint32_t value);

int (*g_pci_get_irq_routing_options)(irq_routing_options_t *options, uint16_t *pci_irqs);
int (*g_pci_set_irq_hw_int)(const pci_location_t *state, uint8_t int_pin, uint8_t irq);


int pci_find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index)
{
	enter_critical_section();

	int res = g_pci_find_pci_device(state, device_id, vendor_id, index);

	exit_critical_section();

	return res;
}

int pci_find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index)
{
	enter_critical_section();

	int res = g_pci_find_pci_class_code(state, class_code, index);

	exit_critical_section();

	return res;
}

int pci_read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value)
{
	enter_critical_section();

	int res = g_pci_read_config_byte(state, reg, value);

	exit_critical_section();

	return res;
}
int pci_read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value)
{
	enter_critical_section();

	int res = g_pci_read_config_half(state, reg, value);

	exit_critical_section();

	return res;
}

int pci_read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value)
{
	enter_critical_section();

	int res = g_pci_read_config_word(state, reg, value);

	exit_critical_section();

	return res;
}

int pci_write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value)
{
	enter_critical_section();

	int res = g_pci_write_config_byte(state, reg, value);

	exit_critical_section();

	return res;
}

int pci_write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value)
{
	enter_critical_section();

	int res = g_pci_write_config_half(state, reg, value);

	exit_critical_section();

	return res;
}

int pci_write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value)
{
	enter_critical_section();

	int res = g_pci_write_config_word(state, reg, value);

	exit_critical_section();

	return res;
}


int pci_get_irq_routing_options(irq_routing_entry *entries, uint16_t *count, uint16_t *pci_irqs)
{
	enter_critical_section();

	irq_routing_options_t options;
	options.size = sizeof(irq_routing_entry) * *count;
	options.selector = DATA_SELECTOR;
	options.offset = entries;

	int res = g_pci_get_irq_routing_options(&options, pci_irqs);

	*count = options.size / sizeof(irq_routing_entry);

	exit_critical_section();

	return res;
}

int pci_set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq)
{
	enter_critical_section();

	int res = g_pci_set_irq_hw_int(state, int_pin, irq);

	exit_critical_section();

	return res;
}

void pci_init(void)
{
	if (!pci_bios_detect()) {
		dprintf(INFO, "pci bios functions installed\n");
		dprintf(INFO, "last pci bus is %d\n", last_bus);
	}
}

#define PCIBIOS_PRESENT                 0xB101
#define PCIBIOS_FIND_PCI_DEVICE         0xB102
#define PCIBIOS_FIND_PCI_CLASS_CODE     0xB103
#define PCIBIOS_GENERATE_SPECIAL_CYCLE  0xB106
#define PCIBIOS_READ_CONFIG_BYTE        0xB108
#define PCIBIOS_READ_CONFIG_WORD        0xB109
#define PCIBIOS_READ_CONFIG_DWORD       0xB10A
#define PCIBIOS_WRITE_CONFIG_BYTE       0xB10B
#define PCIBIOS_WRITE_CONFIG_WORD       0xB10C
#define PCIBIOS_WRITE_CONFIG_DWORD      0xB10D
#define PCIBIOS_GET_IRQ_ROUTING_OPTIONS 0xB10E
#define PCIBIOS_PCI_SET_IRQ_HW_INT      0xB10F

#define PCIBIOS_SUCCESSFUL              0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED      0x81
#define PCIBIOS_BAD_VENDOR_ID           0x83
#define PCIBIOS_DEVICE_NOT_FOUND        0x86
#define PCIBIOS_BAD_REGISTER_NUMBER     0x87
#define PCIBIOS_SET_FAILED              0x88
#define PCIBIOS_BUFFER_TOO_SMALL        0x89

/*
 * far call structure used by BIOS32 routines
 */
static struct {
	uint32_t offset;
	uint16_t selector;
} __PACKED bios32_entry;

/*
 * BIOS32 entry header
 */
typedef struct {
	uint8_t magic[4];   // "_32_"
	void * entry;       // entry point
	uint8_t revision;
	uint8_t length;
	uint8_t checksum;
	uint8_t reserved[5];
} __PACKED pci_bios_info;

/*
 * scan for pci bios
 */
static const char * pci_bios_magic = "_32_";
static pci_bios_info *find_pci_bios_info(void)
{
	uint32_t *head = (uint32_t *) 0x000e0000;
	int8_t sum, *b;
	uint i;

	while (head < (uint32_t *) 0x000ffff0) {
		if (*head == *(uint32_t *) pci_bios_magic) {
			// perform the checksum
			sum = 0;
			b = (int8_t *) head;
			for (i=0; i < sizeof(pci_bios_info); i++) {
				sum += b[i];
			}

			if (sum == 0) {
				return (pci_bios_info *) head;
			}
		}

		head += 4;
	}

	return NULL;
}

/*
 * local BIOS32 PCI routines
 */
static int bios_find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index)
{
	uint32_t bx, ret;

	__asm__(
		"lcall *(%%edi)		\n\t"
		"jc 1f				\n\t"
		"xor %%ah,%%ah		\n"
		"1:"
		: "=b"(bx),
		  "=a"(ret)
		: "1"(PCIBIOS_FIND_PCI_DEVICE),
		  "c"(device_id),
		  "d"(vendor_id),
		  "S"(index),
		  "D"(&bios32_entry));
	
	state->bus = bx >> 8;
	state->dev_fn = bx & 0xFF;

	ret >>= 8;
	return ret & 0xFF;
}

static int bios_find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index)
{
	uint32_t bx, ret;

	__asm__(
		"lcall *(%%edi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=b"(bx),
		  "=a"(ret)
		: "1"(PCIBIOS_FIND_PCI_CLASS_CODE),
		  "c"(class_code),
		  "S"(index),
		  "D"(&bios32_entry));
	
	state->bus = bx >> 8;
	state->dev_fn = bx & 0xFF;

	ret >>= 8;
	return ret & 0xFF;
}


static int bios_read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value)
{
	uint32_t bx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	__asm__(
		"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=c"(*value),
		  "=a"(ret)
		: "1"(PCIBIOS_READ_CONFIG_BYTE),
		  "b"(bx),
		  "D"(reg),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static int bios_read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value)
{
	uint32_t bx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	__asm__(
		"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=c"(*value),
		  "=a"(ret)
		: "1"(PCIBIOS_READ_CONFIG_WORD),
		  "b"(bx),
		  "D"(reg),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static int bios_read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value)
{
	uint32_t bx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	__asm__(
		"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=c"(*value),
		  "=a"(ret)
		: "1"(PCIBIOS_READ_CONFIG_DWORD),
		  "b"(bx),
		  "D"(reg),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static int bios_write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value)
{
	uint32_t bx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	 __asm__(
	 	"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=a"(ret)
		: "0"(PCIBIOS_WRITE_CONFIG_BYTE),
		  "c"(value),
		  "b"(bx),
		  "D"(reg),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static int bios_write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value)
{
	uint32_t bx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	 __asm__(
	 	"lcall *(%%esi)	\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=a"(ret)
		: "0"(PCIBIOS_WRITE_CONFIG_WORD),
		  "c"(value),
		  "b"(bx),
		  "D"(reg),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static int bios_write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value)
{
	uint32_t bx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	 __asm__(
	 	"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=a"(ret)
		: "0"(PCIBIOS_WRITE_CONFIG_DWORD),
		  "c"(value),
		  "b"(bx),
		  "D"(reg),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static int bios_get_irq_routing_options(irq_routing_options_t *route_buffer, uint16_t *pciIrqs)
{
	uint32_t ret;
	
	__asm__(
		"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=b"(*pciIrqs),
		  "=a"(ret)
		: "1"(PCIBIOS_GET_IRQ_ROUTING_OPTIONS),
		  "b"(0),
		  "D"(route_buffer),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xff;
}

static int bios_set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq)
{
	uint32_t bx, cx, ret;

	bx = state->bus;
	bx <<= 8;
	bx |= state->dev_fn;
	cx = irq;
	cx <<= 8;
	cx |= int_pin;
	__asm__(
		"lcall *(%%esi)			\n\t"
		"jc 1f					\n\t"
		"xor %%ah,%%ah			\n"
		"1:"
		: "=a"(ret)
		: "0"(PCIBIOS_PCI_SET_IRQ_HW_INT),
		  "b"(bx),
		  "c"(cx),
		  "S"(&bios32_entry));
	ret >>= 8;
	return ret & 0xFF;
}

static const char *pci_signature = "PCI ";
static int pci_bios_detect(void)
{
	pci_bios_info * pci = find_pci_bios_info();

	if (pci != NULL) {
		/*printf("Found PCI structure at %08x\n", (uint32_t) pci);

		printf("\nPCI header info:\n");
		printf("%c%c%c%c\n", pci->magic[0], pci->magic[1], pci->magic[2],
		    pci->magic[3]);
		printf("%08x\n", (uint32_t) pci->entry);
		printf("%d\n", pci->length * 16);
		printf("%d\n", pci->checksum);*/

		uint32_t adr, temp, len;
		uint8_t err;

		bios32_entry.offset = (uint32_t) pci->entry;
		bios32_entry.selector = CODE_SELECTOR;

		__asm__(
			"lcall *(%%edi)"
			: "=a"(err),	/* AL out=status */
			  "=b"(adr),	/* EBX out=code segment base adr */
			  "=c"(len),	/* ECX out=code segment size */
			  "=d"(temp)	/* EDX out=entry pt offset in code */
			: "0"(0x49435024),/* EAX in=service="$PCI" */
			  "1"(0),	/* EBX in=0=get service entry pt */
			  "D"(&bios32_entry)
		);

		if (err == 0x80) {
			dprintf(INFO, "BIOS32 found, but no PCI BIOS\n");
			return -1;
		}

		if (err != 0) {
			dprintf(INFO, "BIOS32 call to locate PCI BIOS returned %x\n", err);
			return -1;
		}

		bios32_entry.offset = adr + temp;

		// now call PCI_BIOS_PRESENT to get version, hw mechanism, and last bus
		uint16_t present, version, busses;
		uint32_t signature;
		__asm__(
			"lcall *(%%edi)		\n\t"
			"jc 1f				\n\t"
			"xor %%ah,%%ah		\n"
			"1:"
			: "=a"(present),
			  "=b"(version),
			  "=c"(busses),
			  "=d"(signature)
			: "0"(PCIBIOS_PRESENT),
			  "D"(&bios32_entry)
		);

		if (present & 0xff00) {
			dprintf(INFO, "PCI_BIOS_PRESENT call returned ah=%02x\n", present >> 8);
			return -1;
		}

		if (signature != *(uint32_t *)pci_signature) {
			dprintf(INFO, "PCI_BIOS_PRESENT call returned edx=%08x\n", signature);
			return -1;
		}

		//dprintf(DEBUG, "busses=%04x\n", busses);
		last_bus = busses & 0xff;

		g_pci_find_pci_device = bios_find_pci_device;
		g_pci_find_pci_class_code = bios_find_pci_class_code;

		g_pci_read_config_word = bios_read_config_word;
		g_pci_read_config_half = bios_read_config_half;
		g_pci_read_config_byte = bios_read_config_byte;

		g_pci_write_config_word = bios_write_config_word;
		g_pci_write_config_half = bios_write_config_half;
		g_pci_write_config_byte = bios_write_config_byte;

		g_pci_get_irq_routing_options = bios_get_irq_routing_options;
		g_pci_set_irq_hw_int = bios_set_irq_hw_int;

		return 0;
	}

	return -1;
}
