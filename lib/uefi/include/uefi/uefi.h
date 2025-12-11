
#ifndef __LIB_UEFI_HEADER_
#define __LIB_UEFI_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

int load_pe_blockdev(const char *blkdev);
int load_pe_fs(const char *path);

#ifdef __cplusplus
}
#endif

#endif
