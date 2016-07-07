/*
 * timer.h
 *
 *  Created on: 2016Äê7ÔÂ1ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_TIMER_H_
#define DRIVERS_TIMER_H_

#include "stm32f0xx.h"

void timer3_init(uint16_t psc, uint16_t arr);
void pwm15_init(void);
void pwm1_init(uint8_t time);

#endif /* DRIVERS_TIMER_H_ */
