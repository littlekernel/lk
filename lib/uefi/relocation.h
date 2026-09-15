#ifndef __LIB_UEFI_RELOCATION_H_
#define __LIB_UEFI_RELOCATION_H_

#include <stddef.h>

int relocate_image(char *image, size_t image_size);

#endif