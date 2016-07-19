/*
 * tool.c
 *
 *  Created on: 2016Äê7ÔÂ19ÈÕ
 *      Author: wangjiuling
 */

#include "tool.h"
#include "crc16.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern u8 uart_pc_buf[100];

void uart_dut_putch(char ch)
{
	while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
	USART_SendData(USART3, ch);
}
void uart_dut_putln(const u8* buf, u8 len)
{
	while(len--){
		uart_dut_putch(*buf);
		buf++;
	}
}
void uart_pc_putch(char ch)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	USART_SendData(USART1, ch);
}
void uart_pc_putln(const u8* buf, u8 len)
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
int verify_data(const u8* buf, u8 len)
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
int compare(const u8* p1, const u8* p2, u8 len)
{
	u8 sta = 0;
	while(len--){
		if(*p1 != *p2){
			sta = 1;
		}
		p1++;
		p2++;
	}
	if(sta){
		return -1;
	}else{
		return 0;
	}
}
/**
 * @brief packet type of float data
 *
 * @param val value of float type
 * @param itg integer of value
 * @param dcm decimal of value
 *
 * @retval length of packet
 */
u8 packetf_pc(float val, u8 itg, u8 dcm)
{
	u16 crc16;
	char fmt[6];
	u8 len;

	memset(uart_pc_buf, 0, 100);
	uart_pc_buf[0] = 0xAA;

	fmt[0] = '%';
	fmt[1] = itg + '0';
	fmt[2] = '.';
	fmt[3] = dcm + '0';
	fmt[4] = 'f';
	fmt[5] = '\0';

	sprintf((char*)&uart_pc_buf[1], fmt, val);
	len = itg + dcm + 1;
	crc16 = crc16_calc(&uart_pc_buf[1], len);
	uart_pc_buf[len + 1] = (crc16 >> 8) & 0xff;
	uart_pc_buf[len + 2] = crc16 & 0xff;
	uart_pc_buf[len + 3] = 0xA5;

	return len + 4;
}
/**
 * @brief packet type of bool data
 *
 * @param val value of bool type
 *
 * @retval length of packet
 */
u8 packetb_pc(u8 val)
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
