/*
 * hal.c
 *
 *  Created on: 2016年6月2日
 *      Author: wangjiuling
 */

#include "hal.h"
#include "adc.h"
#include "can.h"
#include "delay.h"
#include "flash.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tool.h"

struct _uart		uart_pc;
struct _uart		uart_dut;
struct _adc_val*	adc_val;
struct _adc			adc;
struct _timer		timer2;
struct _obj	cmd_to_dut;
struct _obj	reply_to_pc;
struct _obj	power_on_delay;
struct _obj	can;
struct _obj	motor;
struct _obj	hall;
struct _obj	temp;
struct _obj	exti_motor;
float ad_filter[M];
u8 cmd_list_len;
CanRxMsg can_rxbuf;
u8 can_data[] = {0,1,2,3,4,5,6,7};

extern volatile u16 ad_value[N][M];

struct _cmd_list cmd_list[] = {
	{"v5", test_v5},
	{"v3d3", test_v3d3},
	{"v24", test_v24},
	{"v6", test_v6},
	{"v12", test_v12},
	{"pwr_on", test_pwr_on},
	{"pwr_off", test_pwr_off},
	{"LCcurrent", test_current},
	{"barcode", test_barcode},
	{"can", test_can},
	{"motor", test_motor},
	{"hall", test_hall},
	{"temp", test_temp},
};

/**
 * @brief initiate hardware driver
 * 
 * @param  None
 * @retval None
 */
void hal_init()
{
	cmd_list_len = sizeof(cmd_list) / sizeof(struct _cmd_list);
	timer2.stop = timer2_stop;
	timer2.start = timer2_start;
	timer2.clear = timer2_clear;
	uart_pc.timer.stop = timer3_stop;
	uart_pc.timer.start = timer3_start;
	uart_pc.timer.clear = timer3_clear;
	uart_dut.timer.stop = timer4_stop;
	uart_dut.timer.start = timer4_start;
	uart_dut.timer.clear = timer4_clear;

	delay_init();
	adc_init();
	can_init();
	gpio_init();
	uart1_init();
	uart3_init();
	timer2_init(7200, 1000);//10KHz, 100mS
	timer3_init(7200, 20);// 10KHz, 2mS
	timer4_init(7200, 20);// 10KHz, 2mS
}

/**
 * @brief if find command in cmd_list, then execute command
 *
 * @param buf command buffer
 * @param len length of command buffer
 * 
 * @retval not used yet
 */
static int execute_cmd(const char* buf, u8 len)
{
	u8 i;
	u8 pos;
	char* cmd = NULL;
	char* tmp = NULL;
	char param[11] = {0};

	tmp = strchr(buf, ':');
	if(tmp != NULL){
		pos = tmp - buf;
		cmd = (char*)malloc(pos + 1);
		*(cmd + pos) = '\0';
		memcpy(cmd, buf, pos);
		memcpy(param, tmp + 1, 10);
		param[10] = '\0';

		for(i = 0;i < cmd_list_len;i++){
			if(strcmp(cmd_list[i].str, cmd) == 0){
				cmd_list[i].cmd(param);
				break;
			}
		}
		free(cmd);
	}else{
		for(i = 0;i < cmd_list_len;i++){
			if(strcmp(cmd_list[i].str, buf) == 0){
				cmd_list[i].cmd(NULL);
				break;
			}
		}
	}

	return 0;
}
/**
 * @brief 校验收到的数据是否正确
 *
 * @param  None
 * @retval None
 */
void verify_pc_data()
{
	/* 定时器超时，代表定时时间内没收到数据，表示一帧数据接收完成 */
	if(uart_pc.timer.state == TIME_OUT){
		uart_pc.timer.state = NORMAL;
		/* 校验数据 */
		if(verify_data(uart_pc.recv_buf, uart_pc.recv_len) == 1){
			/* 数据校验正确，置位标志位，交由handle_pc_data()处理 */
			uart_pc.state = RECEIVED;
		}else{
			/* 清洗接收缓存 */
//			memset(uart_pc.recv_buf, 0, uart_pc.recv_len);
			uart_pc.recv_len = 0;
		}
	}
}
/**
 * @brief 执行接收到的命令
 *
 * @param  None
 * @retval None
 */
void handle_pc_data()
{
	u8 len;

	if(uart_pc.state == RECEIVED){
		/* 去掉头(1)、尾(1)、校验和(2) */
		len = uart_pc.recv_len - 4;
		/* 申请内存 *///TODO 可以优化成不申请内存
		uart_pc.cmd = NULL;
		uart_pc.cmd = (char*)malloc(len + 1);
		if(uart_pc.cmd == NULL){
			return;
		}
		/* 复制有效数据到uart_pc.cmd */
		memcpy(uart_pc.cmd, uart_pc.recv_buf + 1, len);
		*(uart_pc.cmd + len) = '\0';
		/* 执行命令 */
		execute_cmd(uart_pc.cmd, len);
		/* 释放内存 */
		free(uart_pc.cmd);
		/* 清洗接收缓存 */
//		memset(uart_pc.recv_buf, 0, uart_pc.recv_len);
		uart_pc.recv_len = 0;

		uart_pc.state = NORMAL;
	}
}
void calc_ad_value()
{
	u8 i;
	u8 j;

	if(adc.enable == DISABLE){
		return;
	}
	/* ad_value[N][M]
	 * {ch0, ch1, ch2, ch3, ...}, N0
	 * {ch0, ch1, ch2, ch3, ...}, N1
	 * {ch0, ch1, ch2, ch3, ...}, N2
	 * ...
	 * {M0, M1, M2, M3}
	 */
	/* 计算平均值（M个通道, 每个通道N个采集值）*/
	for(i = 0;i < M;i++){
		adc.sum = 0;
		for(j = 0;j < N;j++){
			adc.sum += ad_value[j][i];
		}
		adc.sum = adc.sum / N;
		ad_filter[i] = ((float)adc.sum) / 4096.0 * 3.3;
	}
	/* 转换成实际的值 */
	adc_val = (struct _adc_val*)ad_filter;
	adc_val->v24 *= 7.8;
	adc_val->v6 *= 2.0;
	adc_val->v5 *= 2.0;
	adc_val->v3d3 *= 1.1;
	adc_val->v12 *= 4.09;
	adc_val->v6m *= 2.0;
	adc_val->cur /= 2.0;
	adc_val->cur *= 1000; // mA
//	adc_val->tmp = (13.582 - sqrt(13.582*13.582 + 4*0.00433*(2230.8 - adc_val->tmp*1000))) / (2*-0.00433) + 30;

	adc.state = 0;
	if(adc_val->v24 > 25.5
	|| adc_val->v24 < 21.5){// 21.5~25.5
		adc.state = 1;
	}
	if(adc_val->v12 > 13.5
	|| adc_val->v12 < 11.5){// 11.5~13.5
		adc.state = 1;
	}
	if(adc_val->v6 > 6.3
	|| adc_val->v6 < 5.7){// 5.7~6.3
		adc.state = 1;
	}
	if(adc_val->v5 > 5.1
	|| adc_val->v5 < 4.9){// 4.9~5.1
		adc.state = 1;
	}
	if(adc_val->v3d3 > 3.4
	|| adc_val->v3d3 < 3.2){// 3.2~3.4
		adc.state = 1;
	}
	/* LC刚上电，未打开外设，电流应该很小 */
	if(cmd_to_dut.enable == DISABLE){
		if(adc_val->cur > 100){
			adc.state = 1;
		}
	}else{
		if(adc_val->cur > 300){
			adc.state = 1;
		}
	}
	if(adc.state){
		DUT_PWR_OFF;// DUT power off
	}
}
void handle_flag()
{
	if(power_on_delay.enable == ENABLE){
		/* 延时的作用 */
		power_on_delay.tmp++;
		if(power_on_delay.tmp >= 5e3){
			power_on_delay.tmp = 0;
			power_on_delay.enable = DISABLE;
			cmd_to_dut.enable = ENABLE;
		}
	}
	if(reply_to_pc.enable == ENABLE){
		reply_to_pc.enable = DISABLE;
		if(cmd_to_dut.state == BAD){
			packet_hex(0);
		}else{
			packet_hex(1);
		}
		uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
	}
}
void part_of_power_on()
{
	/* 检测是否可以执行此函数 */
	if(cmd_to_dut.enable == DISABLE){
		return;
	}
	cmd_to_dut.enable = DISABLE;
	cmd_to_dut.state = GOOD;
	/********* open OLED *********/
	handle_dut_data(DUT_OLED_ON, sizeof(DUT_OLED_ON));
	delay_ms(50);
	/********* open LED *********/
	handle_dut_data(DUT_LED_ON, sizeof(DUT_LED_ON));
	delay_ms(10);
	/********* close beep *********/
	handle_dut_data(DUT_BEEP_OFF, sizeof(DUT_BEEP_OFF));

	reply_to_pc.enable = ENABLE;
}
int handle_dut_data(const u8 *p, u8 len)
{
	uart_dut.recv_len = 0;
	uart_dut.timer.state = NORMAL;
	uart_dut.state = NORMAL;
	uart_dut_putln(p, len);
	timer2.state = NORMAL;
	timer2.clear();
	timer2.start();
	/* waiting for receiving data or time out */
	while((uart_dut.timer.state == NORMAL) && (timer2.state == NORMAL));
	if(uart_dut.timer.state != NORMAL){// 收到数据
		timer2.stop();
		return 0;
	}else{// 未收到数据，超时100mS
		/* 下面两句话不应该在这个底层函数里执行，应该通过返回值判断处理 */
		DUT_PWR_OFF;// DUT power off
		cmd_to_dut.state = BAD;
		return -1;
	}
}
void test_v3d3(char* parameter)
{
	packet_float(adc_val->v3d3, 1, 2);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_v5(char* parameter)
{
	packet_float(adc_val->v5, 1, 2);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_v24(char* parameter)
{
	packet_float(adc_val->v24, 2, 2);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_v6(char* parameter)
{
	packet_float(adc_val->v6, 1, 2);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_v12(char* parameter)
{
	packet_float(adc_val->v12, 2, 2);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_pwr_on(char* parameter)
{
	/* 目标板上电 */
	DUT_PWR_ON;
	delay_ms(200);
	/* 允许ADC采集 */
	adc.enable = ENABLE;
	/* 置位标志位，交由handle_flag()处理  */
	power_on_delay.enable = ENABLE;
	power_on_delay.tmp = 0;

	/********* open beep *********/
	uart_dut_putln(DUT_BEEP_ON, sizeof(DUT_BEEP_ON));
}
void test_pwr_off(char* parameter)
{
	adc.enable = DISABLE;
	memset(ad_filter, M, 0);
	memset((void*)&ad_value[0], M*N, 0);
	DUT_PWR_OFF;

	packet_hex(1);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_current(char* parameter)
{
	packet_float(adc_val->cur, 4, 0);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_barcode(char* parameter)
{
	packet_float(adc_val->cur, 4, 0);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_can(char* parameter)
{
	can.state = NORMAL;
	timer2.state = NORMAL;
	timer2.clear();
	timer2.start();
	if(handle_dut_data(DUT_CAN_CHECK, sizeof(DUT_CAN_CHECK)) == 0
	&& compare(uart_dut.recv_buf, DUT_CAN_ACK, uart_dut.recv_len) == 0){
		/* waiting for receiving data or time out */
		while((can.state == NORMAL) || timer2.state == NORMAL);
		if(can.state != NORMAL){// 收到数据
			timer2.stop();
			if(compare(can_rxbuf.Data, can_data, 8) == 0){
				packet_hex(GOOD);
			}else{
				packet_hex(BAD);
			}
		}else{// 未收到数据，超时100mS
			DUT_PWR_OFF;// DUT power off
			packet_hex(BAD_COM);
		}
	}else{
		packet_hex(BAD_COM);
	}
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_motor(char* parameter)
{
	float cur1, cur2;
	u16 tmp;

	/* 未打开舵机前，此IO应该为低电平 */
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 0){
		exti_motor.state = NORMAL;
		GPIO_SetBits(GPIOB, GPIO_Pin_5);// 打开舵机负载
		delay_ms(2);
		calc_ad_value();
		/* 此时的电压应该很低 */
		if(adc_val->v6m > 0.2){
			packet_hex(BAD);
		}else if(handle_dut_data(DUT_MOTOR_ON, sizeof(DUT_MOTOR_ON)) == 0){
			tmp = uart_dut.recv_buf[6] | uart_dut.recv_buf[7] << 8;
			cur1 = tmp * 3.3 / 4096.0;// LC上传的电流值
			delay_ms(2);
			/* 检测舵机电流  */
			calc_ad_value();
			cur2 = adc_val->v6 - adc_val->v6m;// 工装板自己计算的电流值
			cur1 = cur2 / cur1;
			/* 电流相差在范围内，并且IO的电平变化了 */
			if(cur1 > 0.5 && cur1 < 1.5 && exti_motor.state != NORMAL){
				packet_hex(GOOD);
			}else{
				packet_hex(BAD);
			}
		}else{
			packet_hex(BAD_COM);
		}
	}else{
		packet_hex(BAD);
	}
	handle_dut_data(DUT_MOTOR_OFF, sizeof(DUT_MOTOR_OFF));
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);// 断开舵机负载
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_hall(char* parameter)
{

}
void test_temp(char* parameter)
{
	if(handle_dut_data(DUT_TMP_CHECK, sizeof(DUT_TMP_CHECK)) == 0){
		packet_float(uart_dut.recv_buf[6], 1, 0);// 发送温度值
	}else{
		packet_hex(BAD_COM);
	}
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
/**
 * @brief when timer2 interrupt occurs, it indicate the transmission
 *        of this round is completed
 *
 * @param  None
 * @retval None
 */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		timer2.stop();
		timer2.enable = DISABLE;
		timer2.state = TIME_OUT;
	}
}
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
		TIM4->SR = ~TIM_IT_Update;
		/* 置位标志位 */
		uart_dut.timer.state = TIME_OUT;
		/* disable timer */
		uart_dut.timer.stop();
		uart_dut.timer.enable = DISABLE;
	}
}
/**
 * @brief write the received data to buffer,
 *        and clear timer4 counter value
 * @param  None
 * @retval None
 */
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET){
		/* 状态不对，代表上一帧数据未处理完，禁止接收 */
		if(uart_dut.state != NORMAL){
			return;// TODO 处理
		}
		/* 接收数据 */
		uart_dut.recv_buf[uart_dut.recv_len] = USART3->DR & 0xFF;
		uart_dut.recv_len++;
		/* 防止数据溢出 */
		if(uart_dut.recv_len >= MAX_LEN){
			uart_dut.recv_len = 0;// TODO 处理
		}
		/* 清除定时器计数值，如果定时器关闭，则启动定时器 */
		uart_dut.timer.clear();
		if(uart_dut.timer.enable == DISABLE){
			/* enable timer */
			uart_dut.timer.start();
			uart_dut.timer.enable = ENABLE;
		}
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}
/**
 * @brief when timer3 interrupt occurs, it indicate the USART1
 *          transmission of this round is completed
 * @param  None
 * @retval None
 */
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM3->SR = ~TIM_IT_Update;
		/* 置位标志位，交由 verify_pc_data()处理 */
		uart_pc.timer.state = TIME_OUT;
		/* disable timer */
		uart_pc.timer.stop();
		uart_pc.timer.enable = DISABLE;
	}
}
/**
 * @brief write the received data to buffer,
 *          and clear timer3 counter value
 * @param  None
 * @retval None
 */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		/* 状态不对，代表上一帧数据未处理完，禁止接收 */
		if(uart_pc.state != NORMAL){
			return;// TODO 处理
		}
		/* 接收数据 */
		uart_pc.recv_buf[uart_pc.recv_len] = USART1->DR & 0xFF;
		uart_pc.recv_len++;
		/* 防止数据溢出 */
		if(uart_pc.recv_len >= MAX_LEN){
			uart_pc.recv_len = 0;// TODO 处理
		}
		/* 清除定时器计数值，如果定时器关闭，则启动定时器 */
		uart_pc.timer.clear();
		if(uart_pc.timer.enable == DISABLE){
			/* enable timer */
			uart_pc.timer.start();
			uart_pc.timer.enable = ENABLE;
		}
	}
}
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	can.state = RECEIVED;
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_Receive(CAN1, CAN_FIFO0, &can_rxbuf);
}
void EXTI4_IRQHandler(void)
{
	exti_motor.state = RECEIVED;
	EXTI_ClearITPendingBit(EXTI_Line4);
}
