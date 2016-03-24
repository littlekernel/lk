/*
 * Computes the CRC for transmitted and received data using
 * the CCITT 16bit algorithm (X^16 + X^12 + X^5 + 1) with
 * a 0xFFFF initialization vector.
 */

#define CRC16_INIT_VALUE  0xFFFF

 /*
  * Computes an updated version of the CRC from existing CRC.
  * crc: the previous values of the CRC
  * buf: the data on which to apply the checksum
  * length: the number of bytes of data in 'buf' to be calculated.
  */
unsigned short update_crc16(unsigned short crc, const unsigned char *buf,
			   unsigned int length)
{
	unsigned int i;
	for (i = 0; i < length; i++) {
		crc = (unsigned char) (crc >> 8) | (crc << 8);
		crc ^= buf[i];
		crc ^= (unsigned char) (crc & 0xff) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}
	return crc;
}

 /*
  * Computes a CRC, starting with an initialization value.
  * buf: the data on which to apply the checksum
  * length: the number of bytes of data in 'buf' to be calculated.
  */
unsigned short crc16(const unsigned char *buf, unsigned int length)
{
	unsigned short crc = CRC16_INIT_VALUE;
	return update_crc16(crc, buf, length);
}
