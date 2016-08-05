/*
 * flash.h
 *
 *  Created on: 2016Äê7ÔÂ27ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_FLASH_H_
#define DRIVERS_FLASH_H_

#include "stm32f0xx.h"

void flash_read(uint32_t addr, uint8_t* p, uint16_t len);
int flash_write(uint32_t addr, uint8_t* p, uint16_t len);

#endif /* DRIVERS_FLASH_H_ */
