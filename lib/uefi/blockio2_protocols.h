#ifndef __LIB_UEFI_BLOCKIO2_PROTOCOL_H_
#define __LIB_UEFI_BLOCKIO2_PROTOCOL_H_

#include <uefi/types.h>

EfiStatus open_async_block_device(EfiHandle handle, void **intf);

#endif
