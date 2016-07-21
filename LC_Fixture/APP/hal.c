/*
 * hal.c
 *
 *  Created on: 2016��6��2��
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
struct _obj	can;
struct _obj	cmd_to_dut;
struct _obj	reply_to_pc;
struct _obj	power_on_delay;
struct _timer timer2;
float ad_filter[M];
u8 cmd_list_len;
CanRxMsg can_rxbuf;

extern volatile u16 ad_value[N][M];

struct _list cmd_list[] = {
	{"v5", test_v5},
	{"v3d3", test_v3d3},
	{"v24", test_v24},
	{"v6", test_v6},
	{"v12", test_v12},
	{"can", test_CAN},
	{"pwr_on", test_pwr_on},
	{"pwr_off", test_pwr_off},
	{"LCcurrent", test_current},
	{"barcode", test_barcode},
};

/**
 * @brief initiate hardware driver
 * 
 * @param  None
 * @retval None
 */
void hal_init()
{
	cmd_list_len = sizeof(cmd_list) / sizeof(struct _list);
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
 * @brief У���յ��������Ƿ���ȷ
 *
 * @param  None
 * @retval None
 */
void verify_pc_data()
{
	/* ��ʱ����ʱ������ʱʱ����û�յ����ݣ���ʾһ֡���ݽ������ */
	if(uart_pc.timer.state == TIME_OUT){
		uart_pc.timer.state = NORMAL;
		/* У������ */
		if(verify_data(uart_pc.recv_buf, uart_pc.recv_len) == 1){
			/* ����У����ȷ����λ��־λ������handle_pc_data()���� */
			uart_pc.state = RECEIVED;
		}else{
			/* ��ϴ���ջ��� */
//			memset(uart_pc.recv_buf, 0, uart_pc.recv_len);
			uart_pc.recv_len = 0;
		}
	}
}
/**
 * @brief ִ�н��յ�������
 *
 * @param  None
 * @retval None
 */
void handle_pc_data()
{
	u8 len;

	if(uart_pc.state == RECEIVED){
		/* ȥ��ͷ(1)��β(1)��У���(2) */
		len = uart_pc.recv_len - 4;
		/* �����ڴ� *///TODO �����Ż��ɲ������ڴ�
		uart_pc.cmd = NULL;
		uart_pc.cmd = (char*)malloc(len + 1);
		if(uart_pc.cmd == NULL){
			return;
		}
		/* ������Ч���ݵ�uart_pc.cmd */
		memcpy(uart_pc.cmd, uart_pc.recv_buf + 1, len);
		*(uart_pc.cmd + len) = '\0';
		/* ִ������ */
		execute_cmd(uart_pc.cmd, len);
		/* �ͷ��ڴ� */
		free(uart_pc.cmd);
		/* ��ϴ���ջ��� */
//		memset(uart_pc.recv_buf, 0, uart_pc.recv_len);
		uart_pc.recv_len = 0;

		uart_pc.state = NORMAL;
	}
}
void calc_ad_value()
{
	if(adc.enable == DISABLE){
		return;
	}

	for(adc.i = 0;adc.i < M;adc.i++){
		adc.sum = 0;
		for(adc.j = 0;adc.j < N;adc.j++){
			adc.sum += ad_value[adc.j][adc.i];
		}
		adc.sum = adc.sum / N;
		ad_filter[adc.i] = ((float)adc.sum) / 4096.0 * 3.3;
	}
	adc_val = (struct _adc_val*)ad_filter;
	adc_val->v24 *= 7.8;
	adc_val->v6 *= 2.0;
	adc_val->v5 *= 2.0;
	adc_val->v3d3 *= 1.1;
	adc_val->v12 *= 4.09;
	adc_val->v6m *= 2.0;
	adc_val->cur /= 2.0;
	adc_val->cur *= 1000; // mA
	adc_val->tmp = (13.582 - sqrt(13.582*13.582 + 4*0.00433*(2230.8 - adc_val->tmp*1000))) / (2*-0.00433) + 30;

	adc.state = 0;
	if(adc_val->v24 > 24*1.2
	|| adc_val->v24 < 24*0.8){
		adc.state = 1;
	}
	if(adc_val->v6 > 6*1.05
	|| adc_val->v6 < 6*0.95){
		adc.state = 1;
	}
	if(adc_val->v5 > 5*1.02
	|| adc_val->v5 < 5*0.92){
		adc.state = 1;
	}
	if(adc_val->v3d3 > 3.3*1.02
	|| adc_val->v3d3 < 3.3*0.92){
		adc.state = 1;
	}
	if(adc_val->cur > 300){
		adc.state = 1;
	}
	if(adc.state){// TODO
//		packet_bool(0);
//		uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
//		DUT_PWR_OFF;// DUT power off
//		return;
	}

}
void handle_flag()
{
	if(power_on_delay.enable == ENABLE){
		/* ��ʱ������ */
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
			packet_bool(0);
		}else{
			packet_bool(1);
		}
		uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
	}
}
void part_of_power_on()
{
	/* ����Ƿ����ִ�д˺��� */
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
	delay_ms(10);
	/********* open motor *********/
	handle_dut_data(DUT_MOTOR_ON, sizeof(DUT_MOTOR_ON));
	GPIO_SetBits(GPIOB, GPIO_Pin_5);// �򿪶������
	delay_ms(2);
	/* ���������  */
	calc_ad_value();
	if((adc_val->v6 - adc_val->v6m) < 0.02){
		DUT_PWR_OFF;// DUT power off
//		packet_bool(0);
//		uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
		return;
	}
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);// �Ͽ��������
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
	if(timer2.enable == DISABLE){
		timer2.start();
		timer2.enable = ENABLE;
	}
	/* waiting for receiving data or time out */
	while((uart_dut.timer.state == NORMAL) || timer2.state == NORMAL);
	if(uart_dut.timer.state != NORMAL){// �յ�����
//		compare(uart_dut.recv_buf, DUT_OLED_ACK, uart_dut.recv_len);
		timer2.stop();
		// TODO
		return 0;
	}else{// δ�յ����ݣ���ʱ100mS
		// TODO
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
void test_CAN(char* parameter)
{
	packet_bool(1);
	uart_pc_putln(uart_pc.send_buf, uart_pc.send_len);
}
void test_pwr_on(char* parameter)
{
	/* Ŀ����ϵ� */
	DUT_PWR_ON;
	delay_ms(200);
	/* ����ADC�ɼ� */
	adc.enable = ENABLE;
	/* ��λ��־λ������handle_flag()����  */
	power_on_delay.enable = ENABLE;
	power_on_delay.tmp = 0;

	/********* open beep *********/
//	uart_dut_putln(DUT_BEEP_ON, sizeof(DUT_BEEP_ON));
}
void test_pwr_off(char* parameter)
{
	adc.enable = DISABLE;
	memset(ad_filter, M, 0);
	memset((void*)&ad_value[0], M*N, 0);
	DUT_PWR_OFF;

	packet_bool(1);
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
		/* ��λ��־λ */
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
		/* ״̬���ԣ�������һ֡����δ�����꣬��ֹ���� */
		if(uart_dut.state != NORMAL){
			return;// TODO ����
		}
		/* �������� */
		uart_dut.recv_buf[uart_dut.recv_len] = USART3->DR & 0xFF;
		uart_dut.recv_len++;
		/* ��ֹ������� */
		if(uart_dut.recv_len >= MAX_LEN){
			uart_dut.recv_len = 0;// TODO ����
		}
		/* �����ʱ������ֵ�������ʱ���رգ���������ʱ�� */
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
		/* ��λ��־λ������ verify_pc_data()���� */
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
		/* ״̬���ԣ�������һ֡����δ�����꣬��ֹ���� */
		if(uart_pc.state != NORMAL){
			return;// TODO ����
		}
		/* �������� */
		uart_pc.recv_buf[uart_pc.recv_len] = USART1->DR & 0xFF;
		uart_pc.recv_len++;
		/* ��ֹ������� */
		if(uart_pc.recv_len >= MAX_LEN){
			uart_pc.recv_len = 0;// TODO ����
		}
		/* �����ʱ������ֵ�������ʱ���رգ���������ʱ�� */
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
	can.enable = RECEIVED;
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_Receive(CAN1, CAN_FIFO0, &can_rxbuf);
}
