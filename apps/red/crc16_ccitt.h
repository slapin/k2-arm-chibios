/*
 * crc16_ccitt.h
 *
 *    Created on: 28.01.2010
 *        Author: vazic
 *   Modified by: afal
 */

#ifndef CRC16_CCITT_H_
#define CRC16_CCITT_H_

unsigned short crc16_ccitt(const unsigned char * pcBlock, unsigned short len);

unsigned short crc16_ccitt_upd(unsigned short crc, const unsigned char * pcBlock, unsigned short len);


#endif /* CRC16_CCITT_H_ */
