/*
 * main.c
 *
 *  Created on: 2016Äê5ÔÂ31ÈÕ
 *      Author: wangjiuling
 */

#include "hal.h"

int main()
{
	hal_init();
	while(1){
		handle_pc_data();
		cal_ad_value();
	}
}
