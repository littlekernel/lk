#ifndef __LIB_UEFI_BLOCKIO_PROTOCOL_H_
#define __LIB_UEFI_BLOCKIO_PROTOCOL_H_

#include "types.h"

EfiStatus open_block_device(EfiHandle handle, void **intf);

#endif
