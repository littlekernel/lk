#include <stdio.h>
#include <kernel/mutex.h>
#include <malloc.h>
#include <dev/driver.h>
#include <dev/class/block.h>
#include <err.h>

#include "ff.h"
#include "diskio.h"

static struct {
	FATFS work;
	struct device *dev;
} mount_table[_VOLUMES];

status_t ffs_mount(size_t index, struct device *dev)
{
	FRESULT res;

	if (index >= countof(mount_table))
		return ERR_INVALID_ARGS;
	
	if (dev && mount_table[index].dev)
		return ERR_ALREADY_MOUNTED;
	
	if (dev) {
		mount_table[index].dev = dev;

		res = f_mount(index, &mount_table[index].work);
		if (res != FR_OK)
			return ERR_INVALID_ARGS;
	} else {
		mount_table[index].dev = NULL;

		res = f_mount(index, NULL);
		if (res != FR_OK)
			return ERR_NOT_FOUND;
	}

	return NO_ERROR;
}

#if _USE_LFN == 3
void *ff_memalloc(UINT size)
{
	return malloc(size);
}

void ff_memfree(void *mblock)
{
	free(mblock);
}
#endif

#if _FS_REENTRANT
int ff_cre_syncobj(BYTE vol, _SYNC_t *sobj)
{
	*sobj = malloc(sizeof(mutex_t));
	if (!*sobj)
		return false;

	mutex_init(*sobj);
	return true;
}

int ff_del_syncobj(_SYNC_t sobj)
{
	mutex_destroy(sobj);
	free(sobj);
	return true;
}

int ff_req_grant(_SYNC_t sobj)
{
	mutex_acquire(sobj);
	return true;
}

void ff_rel_grant(_SYNC_t sobj)
{
	mutex_release(sobj);
}
#endif

DSTATUS disk_initialize(BYTE pdrv)
{
	return RES_OK;
}

DSTATUS disk_status(BYTE pdrv)
{
	return RES_OK;
}

DRESULT disk_read(BYTE pdrv, BYTE* buf, DWORD sector, BYTE count)
{
	ssize_t ret;

	struct device *dev = mount_table[pdrv].dev;
	if (!dev)
		return RES_NOTRDY;
	
	ret = class_block_read(dev, sector, buf, count);
	if (ret < 0)
		return RES_ERROR;
	
	return RES_OK;
}

#if	_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE* buf, DWORD sector, BYTE count)
{
	ssize_t ret;

	struct device *dev = mount_table[pdrv].dev;
	if (!dev)
		return RES_NOTRDY;
	
	ret = class_block_write(dev, sector, buf, count);
	if (ret < 0)
		return RES_ERROR;
	
	return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buf)
{
	ssize_t ret;

	struct device *dev = mount_table[pdrv].dev;
	if (!dev)
		return RES_NOTRDY;

	switch (cmd) {
		case GET_SECTOR_SIZE: {
			WORD *val = buf;

			ret = class_block_get_size(dev);
			if (ret < 0)
				return RES_ERROR;

			*val = ret;
		}
		break;

		case GET_BLOCK_SIZE: {
			DWORD *val = buf;

			ret = class_block_get_size(dev);
			if (ret < 0)
				return RES_ERROR;

			*val = ret;
		}
		break;

		case GET_SECTOR_COUNT: {
			DWORD *val = buf;

			ret = class_block_get_count(dev);
			if (ret < 0)
				return RES_ERROR;

			*val = ret;
		}
		break;
	}

	return RES_OK;
}

DWORD get_fattime(void)
{
	return
			((2013 - 1980) << 25) | // year
			(1 << 21) | // month
			(1 << 16) | // day
			(0 << 11) | // hour
			(0 << 5) | // minute
			(0 << 0) // even second
			;
}

FILE *fopen(const char *filename, const char *mode)
{
	FRESULT res;
	BYTE flags = 0;
	FIL *fil;
	int i;
	
	fil = malloc(sizeof(FIL));
	if (!fil)
		return NULL;
	
	for (i=0; mode[i] != 0; i++) {
		switch (mode[i]) {
			case 'w':
				flags |= FA_WRITE | FA_CREATE_ALWAYS;
				break;

			case 'r':
				flags |= FA_READ;
				break;

			case '+':
				flags |= FA_READ | FA_WRITE;
				break;
		}
	}
	
	res = f_open(fil, filename, flags);
	if (res != FR_OK) {
		free(fil);
		return NULL;
	}

	return (FILE *) fil;
}

int fclose(FILE *stream)
{
	FRESULT res;
	FIL *fil = (FIL *) stream;

	res = f_close(fil);
	if (res != FR_OK)
		return -1;
	
	free(fil);
	return 0;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
	FRESULT res;
	FIL *fil = (FIL *) stream;
	UINT bread;

	res = f_read(fil, ptr, size * count, &bread);
	if (res != FR_OK)
		return 0;
	
	return bread;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
	FRESULT res;
	FIL *fil = (FIL *) stream;
	UINT bwrite;

	res = f_write(fil, ptr, size * count, &bwrite);
	if (res != FR_OK)
		return 0;
	
	return bwrite;
}

int fflush(FILE *stream)
{
	FRESULT res;
	FIL *fil;

	if (!stream)
		return 0;
	
	fil = (FIL *) stream;

	res = f_sync(fil);
	if (res != FR_OK)
		return -1;
	
	return 0;
}

int feof(FILE *stream)
{
	FIL *fil = (FIL *) stream;

	return f_eof(fil);
}

int fseek(FILE *stream, long offset, int whence)
{
	FRESULT res;
	FIL *fil = (FIL *) stream;
	long o;

	switch (whence) {
		case SEEK_SET:
			o = offset;
			break;

		case SEEK_CUR:
			o = offset + f_tell(fil);
			break;

		case SEEK_END:
			o = f_size(fil) + offset;
			if (o < 0)
				o = 0;
			break;

		default:
			return -1;
	}

	res = f_lseek(fil, o);
	if (res != FR_OK)
		return -1;
	
	return 0;
}

long ftell(FILE *stream)
{
	FIL *fil = (FIL *) stream;

	return f_tell(fil);
}

