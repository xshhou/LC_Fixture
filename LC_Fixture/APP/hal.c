/*
 * hal.c
 *
 *  Created on: 2016Äê6ÔÂ2ÈÕ
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
float ad_filter[M];
u8 cmd_list_len;
u8 uart_pc_buf[100] = {0};
u8 time2_flag = 0;
u8 flag_poweron = 0;

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
/* command to DUT */
u8 DUT_LED_ON[] 	= {0x55, 0xAA, 0x00, 0x06, 0x01, 0x01, 0xC8, 0x9E, 0x60, 0x3D};
u8 DUT_BEEP_ON[] 	= {0x55, 0xAA, 0x00, 0x06, 0x02, 0x01, 0x5F, 0x36, 0x77, 0xE6};
u8 DUT_BEEP_OFF[] 	= {0x55, 0xAA, 0x00, 0x06, 0x02, 0x00, 0xE8, 0x2B, 0xB6, 0xE2};
u8 DUT_TMP_CHECK[] 	= {0x55, 0xAA, 0x00, 0x06, 0x03, 0xFF, 0xD1, 0x0C, 0x4C, 0x1A};
u8 DUT_HALL_CHECK[] = {0x55, 0xAA, 0x00, 0x06, 0x04, 0xFF, 0xC5, 0x27, 0xAF, 0xE1};
u8 DUT_OLED_ON[] 	= {0x55, 0xAA, 0x00, 0x06, 0x05, 0x01, 0x4B, 0x1D, 0x94, 0x1D};
u8 DUT_MOTOR_ON[]	= {0x55, 0xAA, 0x00, 0x06, 0x06, 0x01, 0xDC, 0xB5, 0x83, 0xC6};
u8 DUT_MOTOR_OFF[]	= {0x55, 0xAA, 0x00, 0x06, 0x06, 0x00, 0x6B, 0xA8, 0x42, 0xC2};
/* ACK from DUT */
u8 DUT_LED_ACK[]	= {0x55, 0xAA, 0x01, 0x06, 0x01, 0x00, 0x7A, 0xD5, 0x05, 0xD1};
u8 DUT_BEEP_ACK[]	= {0x55, 0xAA, 0x01, 0x06, 0x02, 0x00, 0xED, 0x7D, 0x12, 0x0A};
u8 DUT_TMP_ACK[] 	= {0x55, 0xAA, 0x01, 0x08, 0x03, 0x00, 0x1D, 0x00, 0x22, 0x31, 0x63, 0x16};
u8 DUT_HALL_OPEN[] 	= {0x55, 0xAA, 0x01, 0x08, 0x04, 0x00, 0x01, 0x00, 0xDE, 0x22, 0x53, 0x66};
u8 DUT_HALL_CLOSE[] = {0x55, 0xAA, 0x01, 0x08, 0x04, 0x00, 0x00, 0x00, 0x53, 0x45, 0x5E, 0x2F};
u8 DUT_OLED_ACK[]	= {0x55, 0xAA, 0x01, 0x06, 0x05, 0x00, 0xF9, 0x56, 0xF1, 0xF1};
u8 DUT_MOTOR_ACK[]	= {0x55, 0xAA, 0x01, 0x08, 0x06, 0x00, 0x5C, 0x00, 0x39, 0x7D, 0xD2, 0x1E};
/**
 * @brief initiate hardware driver
 * 
 * @param  None
 * @retval None
 */
void hal_init()
{
	delay_init();
	adc_init();
	can_init();
	gpio_init();
	uart1_init();
//	uart2_init();
	uart3_init();
	timer2_init(7200, 1000);//10KHz, 100mS
	timer3_init(7200, 20);// 10KHz, 2mS
	timer4_init(7200, 20);// 10KHz, 2mS

	uart_pc.timer = TIMER_OFF;
	uart_pc.sta = TIME_NORMALLY;
	cmd_list_len = sizeof(cmd_list) / sizeof(struct _list);
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
	char barcode[11] = {0};

	tmp = strchr(buf, ':');
	if(tmp != NULL){
		pos = tmp - buf;
		cmd = (char*)malloc(pos + 1);
		*(cmd + pos) = '\0';
		memcpy(cmd, buf, pos);
		memcpy(barcode, tmp + 1, 10);
		barcode[10] = '\0';

		for(i = 0;i < cmd_list_len;i++){
			if(strcmp(cmd_list[i].str, cmd) == 0){
				cmd_list[i].cmd(barcode);
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
 * @brief if received new PC data, then verify it, execute it
 *
 * @param  None
 * @retval None
 */
void handle_pc_data()
{
	int tmp;
	u8 len;

	if(uart_pc.sta == TIME_OVERFLOW){
		/* disable uart1 */
//		USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		uart_pc.sta = TIME_NORMALLY;

		tmp = verify_data(uart_pc.buf, uart_pc.len);
		if(tmp == 1){
			len = uart_pc.len - 4;
			uart_pc.str = NULL;
			uart_pc.str = (char*)malloc(len + 1);
			if(uart_pc.str == NULL){
				return;
			}
			memcpy(uart_pc.str, uart_pc.buf + 1, len);
			*(uart_pc.str + len) = '\0';
			execute_cmd(uart_pc.str, len);
			free(uart_pc.str);
		}

		memset(uart_pc.buf, 0, uart_pc.len);
		uart_pc.len = 0;
		uart_pc.sta = TIME_NORMALLY;

		/* enable uart1 */
//		USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	}
}
void cal_ad_value()
{
	if(adc.start == 0){
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

	adc.sta = 0;
	if(adc_val->v24 > 24*1.2
	|| adc_val->v24 < 24*0.8){
		adc.sta = 1;
	}
	if(adc_val->v6 > 6*1.05
	|| adc_val->v6 < 6*0.95){
		adc.sta = 1;
	}
	if(adc_val->v5 > 5*1.02
	|| adc_val->v5 < 5*0.92){
		adc.sta = 1;
	}
	if(adc_val->v3d3 > 3.3*1.02
	|| adc_val->v3d3 < 3.3*0.92){
		adc.sta = 1;
	}
	if(adc_val->cur > 300){
		adc.sta = 1;
	}
	if(adc.sta){
		packetb_pc(0);
		uart_pc_putln(uart_pc_buf, 5);
		CS_PWR_LOW;// DUT power off
		return;
	}
	if(adc.count == 1){
		adc.times++;
		if(adc.times >= 1e4){
			adc.count = 0;
			adc.times = 0;
			flag_poweron = 1;
		}
	}
}

void part_of_power_on()
{
	if(flag_poweron == 0){
		return;
	}
	flag_poweron = 0;

	/********* open OLED *********/
	uart_dut.len = 0;
	uart_dut.sta = TIME_NORMALLY;
	uart_dut_putln(DUT_OLED_ON, sizeof(DUT_OLED_ON));
	time2_flag = 0;
	TIM_Cmd(TIM2, ENABLE);
	while((uart_dut.sta == TIME_NORMALLY) || time2_flag == 0);
	if(uart_dut.sta != TIME_NORMALLY){
		compare(uart_dut.buf, DUT_OLED_ACK, uart_dut.len);
	}else{
		CS_PWR_LOW;// DUT power off
		packetb_pc(0);
		uart_pc_putln(uart_pc_buf, 5);
		return;
	}
	/********* open LED *********/
	uart_dut.len = 0;
	uart_dut.sta = TIME_NORMALLY;
	uart_dut_putln(DUT_LED_ON, sizeof(DUT_LED_ON));
	time2_flag = 0;
	TIM_Cmd(TIM2, ENABLE);
	while((uart_dut.sta == TIME_NORMALLY) || time2_flag == 0);
	if(uart_dut.sta != TIME_NORMALLY){
		compare(uart_dut.buf, DUT_LED_ACK , uart_dut.len);
	}else{
		CS_PWR_LOW;// DUT power off
		packetb_pc(0);
		uart_pc_putln(uart_pc_buf, 5);
		return;
	}
	/********* open motor *********/
	uart_dut.len = 0;
	uart_dut.sta = TIME_NORMALLY;
	uart_dut_putln(DUT_MOTOR_ON, sizeof(DUT_MOTOR_ON));
	time2_flag = 0;
	TIM_Cmd(TIM2, ENABLE);
	while((uart_dut.sta == TIME_NORMALLY) || time2_flag == 0);
	if(uart_dut.sta != TIME_NORMALLY){
		compare(uart_dut.buf, DUT_LED_ACK , uart_dut.len);
	}else{
		CS_PWR_LOW;// DUT power off
		packetb_pc(0);
		uart_pc_putln(uart_pc_buf, 5);
		return;
	}
	delay_ms(10);
	cal_ad_value();
	GPIO_SetBits(GPIOB, GPIO_Pin_5);
	delay_ms(2);
	cal_ad_value();
	if((adc_val->v6 - adc_val->v6m) < 0.02){
		CS_PWR_LOW;// DUT power off
		packetb_pc(0);
		uart_pc_putln(uart_pc_buf, 5);
		return;
	}
	/********* close beep *********/
	uart_dut_putln(DUT_BEEP_OFF, sizeof(DUT_BEEP_OFF));
	/********* response to DUT *********/
	packetb_pc(1);
	uart_pc_putln(uart_pc_buf, 5);
}
void test_v3d3(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v3d3, 1, 2);

	uart_pc_putln(uart_pc_buf, len);
}
void test_v5(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v5, 1, 2);

	uart_pc_putln(uart_pc_buf, len);
}
void test_v24(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v24, 2, 2);

	uart_pc_putln(uart_pc_buf, len);
}
void test_v6(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v6, 1, 2);

	uart_pc_putln(uart_pc_buf, len);
}
void test_v12(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v12, 2, 2);

	uart_pc_putln(uart_pc_buf, len);
}
void test_CAN(char* parameter)
{
	packetb_pc(1);

	uart_pc_putln(uart_pc_buf, 5);
}
void test_pwr_on(char* parameter)
{
	CS_PWR_HIGH;
	delay_ms(200);
	adc.start = 1;
	adc.count = 1;
	adc.times = 0;

	/********* open beep *********/
	uart_dut_putln(DUT_BEEP_ON, sizeof(DUT_BEEP_ON));
}
void test_pwr_off(char* parameter)
{
	packetb_pc(1);

	adc.start = 0;
	memset(ad_filter, M, 0);
	memset((void*)&ad_value[0], M*N, 0);
	CS_PWR_LOW;

	uart_pc_putln(uart_pc_buf, 5);
}
void test_current(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->cur, 4, 0);

	uart_pc_putln(uart_pc_buf, len);
}
void test_barcode(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->cur, 4, 0);

	uart_pc_putln(uart_pc_buf, len);
}
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
		TIM4->SR = ~TIM_IT_Update;

		uart_dut.sta = TIME_OVERFLOW;
		/* disable timer */
		TIM4->CR1 &= ~TIM_CR1_CEN;
		uart_dut.timer = TIMER_OFF;
	}
}
/**
 * @brief write the received data to buffer,
 *        and clear timer4 counter value
 *
 * @param  None
 * @retval None
 */
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET){
		uart_dut.buf[uart_dut.len] = USART3->DR & 0xFF;
		uart_dut.len++;

		if(uart_dut.len >= MAX_RECV_LEN){
			uart_dut.len = 0;
		}
		/* clear timer counter value */
		TIM4->CNT = 0;
		if(uart_dut.timer == TIMER_OFF){
			/* enable timer */
			TIM4->CR1 |= TIM_CR1_CEN;
			uart_dut.timer = TIMER_ON;
		}
	}
}
/**
 * @brief when timer3 interrupt occurs, it indicate the transmission
 *        of this round is completed
 *
 * @param  None
 * @retval None
 */
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM3->SR = ~TIM_IT_Update;

		uart_pc.sta = TIME_OVERFLOW;
		/* disable timer */
		TIM3->CR1 &= ~TIM_CR1_CEN;
		uart_pc.timer = TIMER_OFF;
	}
}
/**
 * @brief write the received data to buffer,
 *        and clear timer3 counter value
 *
 * @param  None
 * @retval None
 */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		uart_pc.buf[uart_pc.len] = USART1->DR & 0xFF;
		uart_pc.len++;

		if(uart_pc.len >= MAX_RECV_LEN){
			uart_pc.len = 0;
		}
		/* clear timer counter value */
		TIM3->CNT = 0;
		if(uart_pc.timer == TIMER_OFF){
			/* enable timer */
			TIM3->CR1 |= TIM_CR1_CEN;
			uart_pc.timer = TIMER_ON;
		}
	}
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

		time2_flag = 1;
		/* disable timer */
		TIM2->CR1 &= ~TIM_CR1_CEN;
		uart_dut.timer = TIMER_OFF;
	}
}
