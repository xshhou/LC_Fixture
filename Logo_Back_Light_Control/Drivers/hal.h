/*
 * hal.h
 *
 *  Created on: 2016Äê6ÔÂ29ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_HAL_H_
#define DRIVERS_HAL_H_

#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include <stdio.h>
#include "uart.h"

#define ADC_BUF_LENGTH	100
#define LED_FLASH_TIME	20
#define LED_FLASH_TIMES	300
#define ADDR_DATA		0x08003C00// page15, sector3, 15Kbyte
#define PWM_HIG			60
#define PWM_LOW			10
#define PWM_DEFAULT		30

struct _key{
	uint8_t change;
	uint8_t delay_enable;
	uint16_t time_cnt;
	uint8_t sta;
	uint16_t times;
};
struct _adc{
	uint8_t change;
	uint8_t delay;
	uint8_t pos;
	uint16_t buf[ADC_BUF_LENGTH];
	uint32_t sum;
	uint8_t times;
	uint8_t enable;
};
struct _battery{
	float value;
	uint8_t low;
	uint8_t pos;
	uint32_t sum;
};
enum{DOWN, UP};

void hal_init(void);
void key_test(void);
void adc_convert(void);
void led_control(void);
void long_press_pwr_on(void);

#endif /* DRIVERS_HAL_H_ */
