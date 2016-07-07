/*
 * hal.c
 *
 *  Created on: 2016Äê6ÔÂ29ÈÕ
 *      Author: wangjiuling
 */

#include "hal.h"
#include "uart.h"
#include "gpio.h"
#include "timer.h"
#include "adc.h"

struct _key key;
struct _key led;

void hal_init(void)
{
	uart_init();
	gpio_init();
	timer3_init(48000, 10);//48E6 / (48e3*10) = 100Hz, 10mS
	pwm1_init(30);
	adc_init();
}

void EXTI0_1_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line1) != RESET)
	{
		key.time = 5;
		key.delay = 1;
		while(key.delay == 1);// delay 50mS
		key.change = 1;

		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{

		if(key.delay){
			key.time--;
			if(key.time == 0){
				key.delay = 0;
			}
		}
		if(led.delay){
			led.time--;
			if(led.time == 0){
				led.time = 20;
				led.change = 1;
			}
		}

		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}
