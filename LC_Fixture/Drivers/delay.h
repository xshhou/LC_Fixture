/*
 * delay.h
 *
 *  Created on: 2016Äê5ÔÂ11ÈÕ
 *      Author: WangJiuLing
 */

#ifndef DELAY_H_
#define DELAY_H_

#include "stm32f10x.h"

void delay_init(void);
void delay_ms(u16 nms);
void delay_us(u32 nus);

#endif /* DELAY_H_ */
