
#ifndef __LIB_UEFI_HEADER_
#define __LIB_UEFI_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

int load_pe_file(const char *blkdev);

#ifdef __cplusplus
}
#endif

#endif