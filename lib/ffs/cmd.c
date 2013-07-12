#include <stdio.h>
#include <string.h>

#include "ff.h"
#include <ffs.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

static int ffs_cmd(int argc, const cmd_args *argv)
{
	FRESULT res;

	if (argc < 2) {
		printf("ffs commands:\n");
usage:
	//	printf("%s mount <volume>\n", argv[0].str);
		printf("%s mkfs <volume>\n", argv[0].str);
		printf("%s mkdir <path>\n", argv[0].str);
		printf("%s touch <path>\n", argv[0].str);

		goto out;
	}

#if 0
	if (!strcmp(argv[1].str, "mount")) {
		if (argc < 3)
			goto usage;

		if (argv[2].u >= countof(ffs_work)) {
			printf("Error: unsupported volume %u\n", argv[2].u);
			goto out;
		}

		res = f_mount(argv[2].u, &ffs_work[argv[2].u]);
		if (res != FR_OK)
			printf("Error: %d\n", res);
	} else
#endif
	if (!strcmp(argv[1].str, "mkfs")) {
		if (argc < 3)
			goto usage;

		res = f_mkfs(argv[2].u, 1, 0);
		if (res != FR_OK)
			printf("Error: %d\n", res);
	} else if (!strcmp(argv[1].str, "mkdir")) {
		if (argc < 3)
			goto usage;

		res = f_mkdir(argv[2].str);
		if (res != FR_OK)
			printf("Error: %d\n", res);
	} else if (!strcmp(argv[1].str, "touch")) {
		FIL fil;

		if (argc < 3)
			goto usage;

		res = f_open(&fil, argv[2].str, FA_OPEN_ALWAYS);
		if (res != FR_OK)
			printf("Error: %d\n", res);
		else
			f_close(&fil);
	} else if (!strcmp(argv[1].str, "ls")) {
		FILINFO fno;
		DIR dir;
		char lfn[_MAX_LFN + 1];
		fno.lfname = lfn;
		fno.lfsize = sizeof(lfn);
		const char *path;
		char *fn;
		char wattr;

		if (argc < 3)
			path = "/";
		else
			path = argv[2].str;

		res = f_opendir(&dir, path);
		if (res != FR_OK) {
			printf("Error: %d\n", res);
			goto out;
		}

		while (1) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0)
				break;

			fn = *fno.lfname ? fno.lfname : fno.fname;
			wattr = fno.fattrib & AM_RDO ? '-' : 'w';

			if (fno.fattrib & AM_DIR)
				printf("dr%cxr%cxr%cx %10lu %s\n", wattr, wattr, wattr, fno.fsize, fn);
			else
				printf(" r%c-r%c-r%c- %10lu %s\n", wattr, wattr, wattr, fno.fsize, fn);
		}

		if (res != FR_OK)
			printf("Error: %d\n", res);
	}

out:
	return 0;
}

STATIC_COMMAND_START
{ "ffs", "ffs toolbox", &ffs_cmd },
STATIC_COMMAND_END(ffscommands);


#endif

