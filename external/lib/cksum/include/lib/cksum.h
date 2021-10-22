#ifndef __CKSUM_H
#define __CKSUM_H

#include <stdint.h>
#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

/*
 * Computes the CRC-CCITT, starting with an initialization value.
 * buf: the data on which to apply the checksum
 * length: the number of bytes of data in 'buf' to be calculated.
 */
unsigned short crc16(const unsigned char *buf, unsigned int length);

/*
 * Computes an updated version of the CRC-CCITT from existing CRC.
 * crc: the previous values of the CRC
 * buf: the data on which to apply the checksum
 * length: the number of bytes of data in 'buf' to be calculated.
 */
unsigned short update_crc16(unsigned short crc, const unsigned char *buf, unsigned int len);

unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
unsigned long crc32_combine(unsigned long, unsigned long, off_t len2);
unsigned long crc32_combine64(unsigned long, unsigned long, int64_t len2);

unsigned long adler32(unsigned long adler, const unsigned char *buf, unsigned int len);
unsigned long adler32_combine(unsigned long adler1, unsigned long adler2, off_t len2);
unsigned long adler32_combine64(unsigned long adler1, unsigned long adler2, int64_t len2);

__END_CDECLS

#endif

