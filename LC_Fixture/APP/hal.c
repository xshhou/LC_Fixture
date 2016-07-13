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
#include "crc16.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

struct _uart		uart_pc;
struct _uart		uart_dut;
struct _adc_val*	adc_val;
struct _adc			adc;
float ad_filter[M];
u8 cmd_list_len;
u8 uart_pc_buf[100] = {0};
u8 uart_dut_buf[10] = {0};

extern volatile u16 ad_value[N][M];

static u8 packetb_pc(u8 val);
static void test_v5(char* parameter);
static void test_v3d3(char* parameter);
static void test_v24(char* parameter);
static void test_v6(char* parameter);
static void test_CAN(char* parameter);
static void test_pwr_on(char* parameter);
static void test_pwr_off(char* parameter);
static void test_current(char* parameter);
static void test_barcode(char* parameter);
static void test_led(char* parameter);

struct _list cmd_list[] = {
	{"v5", test_v5},
	{"v3d3", test_v3d3},
	{"v24", test_v24},
	{"v6", test_v6},
	{"can", test_CAN},
	{"pwr_on", test_pwr_on},
	{"pwr_off", test_pwr_off},
	{"LCcurrent", test_current},
	{"barcode", test_barcode},
	{"led", test_led},
};
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
 * @addtogroup handle DUT data
 */
static void uart_dut_putch(char ch)
{
	while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
	USART_SendData(USART3, ch);
}
static void uart_dut_putln(const u8* buf, u8 len)
{
	while(len--){
		uart_dut_putch(*buf);
		buf++;
	}
}
/**
 * @brief packet data to DUT
 *
 * @param index index
 * @param cmd command
 *
 * @retval length of packet
 */
static void send_packet_dut(u8 index, u8 cmd)
{
	u32 crc32;

	uart_dut_buf[0] = 0x55;
	uart_dut_buf[1] = 0xAA;	// header
	uart_dut_buf[2] = 0;	// id
	uart_dut_buf[3] = 6;	// length
	uart_dut_buf[4] = index;
	uart_dut_buf[5] = cmd;

	crc32 = crc32_calc(&uart_dut_buf[2], 4);
	uart_dut_buf[6] = (crc32 >> 24) & 0xff;
	uart_dut_buf[7] = (crc32 >> 16) & 0xff;
	uart_dut_buf[8] = (crc32 >> 8) & 0xff;
	uart_dut_buf[9] = crc32 & 0xff;

	uart_dut_putln(uart_dut_buf, 10);
}
/**
 * @brief when timer4 interrupt occurs, it indicate the transmission
 *        of this round is completed
 *
 * @param  None
 * @retval None
 */
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
  * @}
  */
/**
 * @addtogroup handle PC data
 */
static void uart_pc_putch(char ch)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	USART_SendData(USART1, ch);
}
static void uart_pc_putln(const u8* buf, u8 len)
{
	while(len--){
		uart_pc_putch(*buf);
		buf++;
	}
}
/**
 * @brief verify data weather correct
 *
 * @param buf data buffer
 * @param len length of data buffer
 * 
 * @retval 1 indicate verify correctly
 *         2 indicate CRC error
 *         3 indicate header error
 *         4 indicate length error
 */
static int verify_data(const u8* buf, u8 len)
{
	if(len > 4){ /* header + crc16 + ending */
		if(buf[0] == 0xAA && buf[len - 1] == 0xA5){
			if(crc16_calc(buf + 1, len - 3)
			== (u16)(buf[len - 3] << 8) | buf[len - 2]){
				return 1;
			}
			return 2;
		}
		return 3;
	}

	return -1;
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

//		uart_pc_putln(uart_pc.buf, uart_pc.len);

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
  * @}
  */
/**
 * @addtogroup ADC convert
 */
void cal_ad_value()
{
	u8 i, j;
	u32 sum;

	if(adc.start == 0){
		return;
	}

	for(i = 0;i < M;i++){
		sum = 0;
		for(j = 0;j < N;j++){
			sum += ad_value[j][i];
		}
		sum = sum / N;
		ad_filter[i] = ((float)sum) / 4096.0 * 3.3;
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

	if(adc_val->v24 > 24*1.2
	|| adc_val->v24 < 24*0.8){
	}
	if(adc_val->v6 > 6*1.05
	|| adc_val->v6 < 6*0.95){
	}
	if(adc_val->v5 > 5*1.02
	|| adc_val->v5 < 5*0.92){
	}
	if(adc_val->v3d3 > 3.3*1.02
	|| adc_val->v3d3 < 3.3*0.92){
	}

	if(adc.count == 1){
		adc.times++;
		if(adc.times >= 1e4){
			adc.count = 0;
			adc.times = 0;
			packetb_pc(1);
			uart_pc_putln(uart_pc_buf, 5);
		}
	}
}
void DMA1_Channel1_IRQHandler()
{
	if(DMA_GetITStatus(DMA1_IT_TC1)){
		cal_ad_value();
		DMA_ClearITPendingBit(DMA1_IT_TC1);
	}
}
/**
  * @}
  */
/**
 * @addtogroup packet data
 */
/**
 * @brief packet type of float data
 *
 * @param val value of float type
 * @param itg integer of value
 * @param dcm decimal of value
 *
 * @retval length of packet
 */
static u8 packetf_pc(float val, u8 itg, u8 dcm)
{
	u16 crc16;
	char* fmt;
	u8 len;

	memset(uart_pc_buf, 0, 100); //TODO
	uart_pc_buf[0] = 0xAA;

	fmt = (char*)malloc(6);// "%1.2f"
	*(fmt + 0) = '%';
	*(fmt + 1) = itg + '0';
	*(fmt + 2) = '.';
	*(fmt + 3) = dcm + '0';
	*(fmt + 4) = 'f';
	*(fmt + 5) = '\0';

	sprintf((char*)&uart_pc_buf[1], fmt, val);
	len = itg + dcm + 1;
	crc16 = crc16_calc(&uart_pc_buf[1], len);
	uart_pc_buf[len + 1] = (crc16 >> 8) & 0xff;
	uart_pc_buf[len + 2] = crc16 & 0xff;
	uart_pc_buf[len + 3] = 0xA5;

	free(fmt);

	return len + 4;
}
/**
 * @brief packet type of bool data
 *
 * @param val value of bool type
 *
 * @retval length of packet
 */
static u8 packetb_pc(u8 val)
{
	memset(uart_pc_buf, 0, 100);

	uart_pc_buf[0] = 0xAA;
	if(val == 0){
		uart_pc_buf[1] = '0';
		uart_pc_buf[2] = 0x14;
		uart_pc_buf[3] = 0x00;
	}else{
		uart_pc_buf[1] = '1';
		uart_pc_buf[2] = 0xD4;
		uart_pc_buf[3] = 0xC1;
	}
	uart_pc_buf[4] = 0xA5;

	return 5;
}
/**
  * @}
  */
/**
 * @addtogroup test command
 */
static void test_v3d3(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v3d3, 1, 2);

	uart_pc_putln(uart_pc_buf, len);
}
static void test_v5(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v5, 1, 2);

	uart_pc_putln(uart_pc_buf, len);
}
static void test_v24(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v24, 2, 2);

	uart_pc_putln(uart_pc_buf, len);
}
static void test_v6(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->v6, 1, 2);

	uart_pc_putln(uart_pc_buf, len);
}
static void test_CAN(char* parameter)
{
	packetb_pc(1);

	uart_pc_putln(uart_pc_buf, 5);
}
static void test_pwr_on(char* parameter)
{
	CS_PWR_HIGH;
	delay_ms(200);
	adc.start = 1;
	adc.count = 1;
	adc.times = 0;
}
static void test_pwr_off(char* parameter)
{
	packetb_pc(1);

	adc.start = 0;
	memset(ad_filter, M, 0);
	memset((void*)&ad_value[0], M*N, 0);
	CS_PWR_LOW;

	uart_pc_putln(uart_pc_buf, 5);
}
static void test_current(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->cur, 4, 0);

	uart_pc_putln(uart_pc_buf, len);
}
static void test_barcode(char* parameter)
{
	u8 len;

	len = packetf_pc(adc_val->cur, 4, 0);

	uart_pc_putln(uart_pc_buf, len);
}
/**
 * @brief when timer2 interrupt occurs, it indicate the transmission
 *        of this round is completed
 *
 * @param  None
 * @retval None
 */
u8 time2_flag = 0;
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		time2_flag = 1;
		/* disable timer */
		TIM4->CR1 &= ~TIM_CR1_CEN;
		uart_dut.timer = TIMER_OFF;
	}
}
static void test_led(char* parameter)
{
	/* open led */
	send_packet_dut(1, 1);
	uart_dut.sta = TIME_NORMALLY;
	time2_flag = 0;
	/* enable timer2 */
	TIM_Cmd(TIM2, ENABLE);
	while((uart_dut.sta == TIME_NORMALLY) || time2_flag == 0);
	if(uart_dut.sta != TIME_NORMALLY){

	}else{

	}
}
/**
  * @}
  */
