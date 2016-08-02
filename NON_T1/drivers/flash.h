/*
 * flash.h
 *
 *  Created on: 2016��5��15��
 *      Author: WangJiuLing
 */

#ifndef DRIVER_FLASH_H_
#define DRIVER_FLASH_H_

#include "stm32f10x.h"

void flash_read(u32 addr, u8* p, u16 len);
int flash_write(u32 addr,u8* p,u16 n);

#endif /* DRIVER_FLASH_H_ */
