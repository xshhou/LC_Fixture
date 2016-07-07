/*
 * timer.h
 *
 *  Created on: 2016Äê6ÔÂ2ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_TIMER_H_
#define DRIVERS_TIMER_H_

#include "stm32f10x.h"

void timer2_init(u16 psc, u16 arr);
void timer3_init(u16 psc, u16 arr);
void timer4_init(u16 psc, u16 arr);

#endif /* DRIVERS_TIMER_H_ */
