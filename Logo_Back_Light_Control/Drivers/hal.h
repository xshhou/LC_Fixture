/*
 * hal.h
 *
 *  Created on: 2016Äê6ÔÂ29ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_HAL_H_
#define DRIVERS_HAL_H_

#include "stm32f0xx.h"
#include <stdio.h>
#include "uart.h"

struct _key{
	uint8_t change;
	uint8_t delay;
	uint8_t time;
	uint8_t sta;
};
enum{DOWN, UP};

void hal_init(void);

#endif /* DRIVERS_HAL_H_ */
