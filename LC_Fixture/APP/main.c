/*
 * main.c
 *
 *  Created on: 2016��5��31��
 *      Author: wangjiuling
 */

#include "hal.h"

int main()
{
	hal_init();
	while(1){
		verify_pc_data();
		handle_pc_data();
		calc_ad_value();
		handle_flag();
		part_of_power_on();
	}
}
