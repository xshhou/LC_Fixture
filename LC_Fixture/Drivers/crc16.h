/*
 * crc16.h
 *
 *  Created on: 2016��5��12��
 *      Author: WangJiuLing
 */

#ifndef CRC16_H_
#define CRC16_H_

int crc16_calc(const unsigned char* buf, unsigned char len);
unsigned int crc32_calc(const unsigned char *buf, unsigned int size);

#endif /* CRC16_H_ */
