#ifndef __ZUTIL_H
#define __ZUTIL_H

/* necessary stuff to transplant crc32 and adler32 from zlib */
#include <inttypes.h>
#include <sys/types.h>

typedef ulong uLong;
typedef uint uInt;
typedef uint8_t Byte;
typedef Byte Bytef;
typedef off_t z_off_t;
typedef int64_t z_off64_t;
typedef unsigned long z_crc_t;

//#define Z_U4 uint32_t


#define Z_NULL NULL
#define OF(args) args
#define local static
#define ZEXPORT
#define FAR

#endif

