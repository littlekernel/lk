#ifndef __LIB_UEFI_BLOCKIO_PROTOCOL_H_
#define __LIB_UEFI_BLOCKIO_PROTOCOL_H_

#include <uefi/types.h>

EfiStatus open_block_device(EfiHandle handle, void **intf);

EfiStatus list_block_devices(size_t *num_handles, EfiHandle **buf);

#endif
