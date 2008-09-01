#ifndef __LWIP_CC_H
#define __LWIP_CC_H

#include <debug.h>
#include <compiler.h>

typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   int     u32_t;
typedef signed     int     s32_t;
typedef unsigned   long    mem_ptr_t;

#define S16_F "hd"
#define U16_F "hu"
#define X16_F "hx"
#define S32_F "d"
#define U32_F "u"
#define X32_F "x"

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __PACKED
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#define LWIP_PLATFORM_DIAG(x) dprintf x
#define LWIP_PLATFORM_ASSERT(x) panic(x)

#if ARCH_ARM
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if 0
#define LWIP_DEBUG
//#define DHCP_DEBUG  DBG_ON
//#define PBUF_DEBUG  DBG_ON
#define DBG_TYPES_ON  DBG_TRACE
#endif

#endif

