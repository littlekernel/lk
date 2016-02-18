#ifndef __CKSUM_H
#define __CKSUM_H

#include <compiler.h>

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

unsigned long adler32(unsigned long adler, const unsigned char *buf, unsigned int len);

__END_CDECLS

#endif

