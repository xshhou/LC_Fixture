/*
 * main.c
 *
 *  Created on: 2016��6��29��
 *      Author: wangjiuling
 */

#include "hal.h"
#include "timer.h"

int main()
{
	hal_init();

	printf("power on\r\n");

	while(1){
		key_test();
		adc_convert();
		led_control();
		debug_print();
	}
}
