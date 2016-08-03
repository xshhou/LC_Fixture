/*
 * main.c
 *
 *  Created on: 2016年6月29日
 *      Author: wangjiuling
 */

#include "hal.h"
#include "timer.h"

int main()
{
	hal_init();
	/* 长按开机 */
	long_press_pwr_on();

	while(1){
		key_test();
		adc_convert();
		led_control();
	}
}
