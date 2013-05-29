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
#include <app.h>
#include <debug.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <compiler.h>
#include <platform.h>
#include <dev/pci.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

/*
 * enumerates pci devices
 */
static void pci_list(void)
{
	pci_location_t state;
	uint16_t device_id, vendor_id;
	uint8_t header_type;
	uint8_t base_class, sub_class, interface;
	int busses = 0, devices = 0, lines = 0, devfn, ret;
	int c;

	printf("Scanning...\n");

	for (state.bus = 0; state.bus <= pci_get_last_bus(); state.bus++) {
		busses++;

		for (devfn = 0; devfn < 256; devfn++) {
			state.dev_fn = devfn;

			ret = pci_read_config_half(&state, PCI_CONFIG_VENDOR_ID, &vendor_id);
			if (ret != _PCI_SUCCESSFUL) goto error;

			ret = pci_read_config_half(&state, PCI_CONFIG_DEVICE_ID, &device_id);
			if (ret != _PCI_SUCCESSFUL) goto error;

			ret = pci_read_config_byte(&state, PCI_CONFIG_HEADER_TYPE, &header_type);
			if (ret != _PCI_SUCCESSFUL) goto error;

			ret = pci_read_config_byte(&state, PCI_CONFIG_CLASS_CODE_BASE, &base_class);
			if (ret != _PCI_SUCCESSFUL) goto error;

			ret = pci_read_config_byte(&state, PCI_CONFIG_CLASS_CODE_SUB, &sub_class);
			if (ret != _PCI_SUCCESSFUL) goto error;

			ret = pci_read_config_byte(&state, PCI_CONFIG_CLASS_CODE_INTR, &interface);
			if (ret != _PCI_SUCCESSFUL) goto error;

			if (vendor_id != 0xffff) {
				printf("%02x:%02x vendor_id=%04x device_id=%04x, header_type=%02x "
						"base_class=%02x, sub_class=%02x, interface=%02x\n", state.bus, state.dev_fn,
				       vendor_id, device_id, header_type, base_class, sub_class, interface);
				devices++;
				lines++;
			}

			if (~header_type & PCI_HEADER_TYPE_MULTI_FN) {
				// this is not a multi-function device, so advance to the next device
				devfn |= 7;
			}

			if (lines == 23) {
				printf("... press any key to continue, q to quit ...");
				while ((c = getchar()) < 0);
				printf("\n");
				lines = 0;

				if (c == 'q' || c == 'Q') goto quit;
			}
		}
	}

	printf("... done. Scanned %d busses, %d device/functions\n", busses, devices);
quit:
	return;

error:
	printf("Error while reading PCI config space: %02x\n", ret);
}

/*
 * a somewhat fugly pci config space examine/modify command. this should probably
 * be broken up a bit.
 */
static int pci_config(int argc, const cmd_args *argv)
{
	pci_location_t loc;
	pci_config_t config;
	uint32_t offset;
	unsigned int i;
	int ret;

	if (argc < 5) {
		return -1;
	}

	if (!strcmp(argv[2].str, "dump")) {
		loc.bus = atoui(argv[3].str);
		loc.dev_fn = atoui(argv[4].str);

		for (i=0; i < sizeof(pci_config_t); i++) {
			ret = pci_read_config_byte(&loc, i, (uint8_t *) &config + i);
			if (ret != _PCI_SUCCESSFUL) goto error;
		}

		printf("Device at %02x:%02x vendor id=%04x device id=%04x\n", loc.bus,
		       loc.dev_fn, config.vendor_id, config.device_id);
		printf("command=%04x status=%04x pi=%02x sub cls=%02x base cls=%02x\n",
		       config.command, config.status, config.program_interface,
		       config.sub_class, config.base_class);

		for (i=0; i < 6; i+=2) {
			printf("bar%d=%08x  bar%d=%08x\n", i, config.base_addresses[i],
			       i+1, config.base_addresses[i+1]);
		}
	} else if (!strcmp(argv[2].str, "rb") || !strcmp(argv[2].str, "rh") || !strcmp(argv[2].str, "rw")) {
		if (argc != 6) {
			return -1;
		}

		loc.bus = atoui(argv[3].str);
		loc.dev_fn = atoui(argv[4].str);
		offset = atoui(argv[5].str);

		switch (argv[2].str[1]) {
			case 'b': {
				uint8_t value;
				ret = pci_read_config_byte(&loc, offset, &value);
				if (ret != _PCI_SUCCESSFUL) goto error;

				printf("byte at device %02x:%02x config offset %04x: %02x\n", loc.bus, loc.dev_fn, offset, value);
			}
			break;

			case 'h': {
				uint16_t value;
				ret = pci_read_config_half(&loc, offset, &value);
				if (ret != _PCI_SUCCESSFUL) goto error;

				printf("half at device %02x:%02x config offset %04x: %04x\n", loc.bus, loc.dev_fn, offset, value);
			}
			break;

			case 'w': {
				uint32_t value;
				ret = pci_read_config_word(&loc, offset, &value);
				if (ret != _PCI_SUCCESSFUL) goto error;

				printf("word at device %02x:%02x config offset %04x: %08x\n", loc.bus, loc.dev_fn, offset, value);
			}
			break;
		}
	} else if (!strcmp(argv[2].str, "mb") || !strcmp(argv[2].str, "mh") || !strcmp(argv[2].str, "mw")) {
		if (argc != 7) {
			return -1;
		}

		loc.bus = atoui(argv[3].str);
		loc.dev_fn = atoui(argv[4].str);
		offset = atoui(argv[5].str);

		switch (argv[2].str[1]) {
			case 'b': {
				uint8_t value = atoui(argv[6].str);
				ret = pci_write_config_byte(&loc, offset, value);
				if (ret != _PCI_SUCCESSFUL) goto error;

				printf("byte to device %02x:%02x config offset %04x: %02x\n", loc.bus, loc.dev_fn, offset, value);
			}
			break;

			case 'h': {
				uint16_t value = atoui(argv[6].str);
				ret = pci_write_config_half(&loc, offset, value);
				if (ret != _PCI_SUCCESSFUL) goto error;

				printf("half to device %02x:%02x config offset %04x: %04x\n", loc.bus, loc.dev_fn, offset, value);
			}
			break;

			case 'w': {
				uint32_t value = atoui(argv[6].str);
				ret = pci_write_config_word(&loc, offset, value);
				if (ret != _PCI_SUCCESSFUL) goto error;

				printf("word to device %02x:%02x config offset %04x: %08x\n", loc.bus, loc.dev_fn, offset, value);
			}
			break;
		}
	} else {
		return -1;
	}

	return 0;

error:
	printf("Error while reading PCI config space: %02x\n", ret);
	return -2;
}

static int pci_cmd(int argc, const cmd_args *argv)
{
	if (argc < 2) {
		printf("pci commands:\n");
usage:
		printf("%s list\n", argv[0].str);
		printf("%s config dump <bus> <devfn>\n", argv[0].str);
		printf("%s config <rb|rh|rw> <bus> <devfn> <offset>\n", argv[0].str);
		printf("%s config <mb|mh|mw> <bus> <devfn> <offset> <value>\n", argv[0].str);
		goto out;
	}

	if (!strcmp(argv[1].str, "list")) {
		pci_list();
	} else if (!strcmp(argv[1].str, "config")) {
		if (pci_config(argc, argv)) {
			goto usage;
		}
	} else {
		goto usage;
	}

out:
	return 0;
}

STATIC_COMMAND_START
{ "pci", "pci toolbox", &pci_cmd },
STATIC_COMMAND_END(pcitests);

#endif

APP_START(pcitests)
APP_END

