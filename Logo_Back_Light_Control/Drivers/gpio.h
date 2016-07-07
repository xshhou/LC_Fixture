/*
 * gpio.h
 *
 *  Created on: 2016Äê7ÔÂ1ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_

#include "stm32f0xx.h"

void exti_init(void);
void nvic_init(void);
void gpio_init(void);

#endif /* DRIVERS_GPIO_H_ */
