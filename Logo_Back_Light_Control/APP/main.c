/*
 * main.c
 *
 *  Created on: 2016Äê6ÔÂ29ÈÕ
 *      Author: wangjiuling
 */

#include "hal.h"
#include "timer.h"

extern struct _key key;


uint8_t flag = 0;

int main()
{
	hal_init();

	printf("power on\r\n");

	while(1){
		if(key.change == 1){
			key.change = 0;
			printf("key up\r\n");

			flag++;// first key up
			if(flag == 2){
				flag = 0;
				printf("power off\r\n");
				GPIO_SetBits(GPIOA, GPIO_Pin_6);
				while(1);
			}
		}
		adc_convert();
		led_control();
		debug_print();
	}
}
