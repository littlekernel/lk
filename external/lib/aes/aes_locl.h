/* aes_locl.h
 * Jason Holt
 */

#ifndef AES_LOCL_H
#define AES_LOCL_H

#define GETU32(p) (((u32)(p)[3]) ^ ((u32)(p)[2] << 8) ^ ((u32)(p)[1] << 16) ^ ((u32)(p)[0] << 24))

#define PUTU32(c, s) { (c)[3] = (u8)(s); (c)[2] = (u8)((s)>>8); (c)[1] = (u8)((s)>>16); (c)[0] = (u8)((s)>>24); }

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define MAXKC   (256/32)
#define MAXKB   (256/8)
#define MAXNR   14

#undef FULL_UNROLL

#endif
