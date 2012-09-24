#ifndef __CKSUM_H
#define __CKSUM_H

unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
unsigned long adler32(unsigned long adler, const unsigned char *buf, unsigned int len);

#endif

