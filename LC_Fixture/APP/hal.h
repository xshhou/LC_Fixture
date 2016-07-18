/*
 * hal.h
 *
 *  Created on: 2016年6月2日
 *      Author: wangjiuling
 */

#ifndef APP_HAL_H_
#define APP_HAL_H_

#include "stm32f10x.h"

#define MAX_RECV_LEN	32

#define CS_PWR_HIGH		GPIO_SetBits(GPIOA, GPIO_Pin_8)
#define	CS_PWR_LOW		GPIO_ResetBits(GPIOA, GPIO_Pin_8)
#define N 		10	//每通道采10次
#define M 		9	//为9个通道

struct _uart{
	u8 sta;
	u8 timer;
	u8 buf[MAX_RECV_LEN];
	u8 len;
	char* str;
};
struct _adc_val{
	float v24;
	float v6;
	float v5;
	float v3d3;
	float v12;
	float v6m;
	float tmp;
	float mic;
	float cur;
};
struct _adc{
	u8 start;
	u8 count;
	u8 sta;
	u8 i;
	u8 j;
	u32 times;
	u32 sum;
};
struct _list{
	char* str;
	void (*cmd)(char*);
};
enum{
	TIME_NORMALLY,
	LEN_OVERFLOW,
	TIME_OVERFLOW,
	TIMER_ON,
	TIMER_OFF
};

void hal_init(void);
void handle_pc_data(void);
void cal_ad_value(void);
void part_of_power_on(void);

#endif /* APP_HAL_H_ */
