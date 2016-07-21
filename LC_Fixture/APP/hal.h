/*
 * hal.h
 *
 *  Created on: 2016年6月2日
 *      Author: wangjiuling
 */

#ifndef APP_HAL_H_
#define APP_HAL_H_

#include "stm32f10x.h"

#define MAX_LEN	32
#define DUT_PWR_ON		GPIO_SetBits(GPIOA, GPIO_Pin_8)
#define	DUT_PWR_OFF		GPIO_ResetBits(GPIOA, GPIO_Pin_8)
#define N 		10	// 每通道采10次
#define M 		9	// 为9个通道

struct _timer{
	u8 enable;	// 运行状态, ENABLE, DISABLE
	u8 state;	// 定时状态, NORMAL, TIME_OUT
	void (*stop)(void); // 停止定时器
	void (*start)(void);// 开启定时器
	void (*clear)(void);// 清楚定时器的计数值
};
struct _uart{
	u8 state;				// 串口的状态
	u8 recv_buf[MAX_LEN];	// 接收缓存
	u8 recv_len;			// 缓存长度
	u8 send_buf[MAX_LEN];	// 发送缓存
	u8 send_len;			// 缓存长度
	char* cmd;				// 解析出的命令
	struct _timer timer;
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
	u8 enable;
	u8 state;
	u8 i;
	u8 j;
	u32 sum;
};
struct _list{
	char* str;
	void (*cmd)(char*);
};
struct _obj{
	u8 enable;
	u8 state;
	u32 tmp;
};
enum{
	NORMAL		= 0,
	TIME_OUT	= !NORMAL,
	RECEIVED	= !NORMAL,
//	STOP		= 0,
//	START		= !STOP,
//	RUN			= !STOP,
	GOOD		= 0,
	BAD			= !GOOD,
	OFF			= 0,
	ON			= !OFF,
};

void hal_init(void);
void verify_pc_data(void);
void handle_pc_data(void);
void calc_ad_value(void);
void handle_flag(void);
void part_of_power_on(void);
int handle_dut_data(const u8 *p, u8 len);

void test_v3d3(char* parameter);
void test_v5(char* parameter);
void test_v24(char* parameter);
void test_v6(char* parameter);
void test_v12(char* parameter);
void test_CAN(char* parameter);
void test_pwr_on(char* parameter);
void test_pwr_off(char* parameter);
void test_current(char* parameter);
void test_barcode(char* parameter);


#endif /* APP_HAL_H_ */
