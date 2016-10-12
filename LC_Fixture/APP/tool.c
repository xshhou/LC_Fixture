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

extern struct _uart uart_pc;

/* command to DUT */
u8 DUT_LED_ON	[] 	= {0x55, 0xAA, 0x00, 0x06, 0x01, 0x01, 0xC8, 0x9E, 0x60, 0x3D};
u8 DUT_BEEP_ON	[] 	= {0x55, 0xAA, 0x00, 0x06, 0x02, 0x01, 0x5F, 0x36, 0x77, 0xE6};
u8 DUT_BEEP_OFF	[] 	= {0x55, 0xAA, 0x00, 0x06, 0x02, 0x00, 0xE8, 0x2B, 0xB6, 0xE2};
u8 DUT_TMP_CHECK[] 	= {0x55, 0xAA, 0x00, 0x06, 0x03, 0xFF, 0xD1, 0x0C, 0x4C, 0x1A};
u8 DUT_HALL_CHECK[] = {0x55, 0xAA, 0x00, 0x06, 0x04, 0xFF, 0xC5, 0x27, 0xAF, 0xE1};
u8 DUT_OLED_ON	[] 	= {0x55, 0xAA, 0x00, 0x06, 0x05, 0x01, 0x4B, 0x1D, 0x94, 0x1D};
u8 DUT_MOTOR_ON	[]	= {0x55, 0xAA, 0x00, 0x06, 0x06, 0x01, 0xDC, 0xB5, 0x83, 0xC6};
u8 DUT_MOTOR_OFF[]	= {0x55, 0xAA, 0x00, 0x06, 0x06, 0x00, 0x6B, 0xA8, 0x42, 0xC2};
u8 DUT_CAN_CHECK[]	= {0x55, 0xAA, 0x00, 0x06, 0x07, 0xFF, 0x52, 0x8F, 0xB8, 0x3A};
/* ACK from DUT */
u8 DUT_LED_ACK	[]	= {0x55, 0xAA, 0x01, 0x06, 0x01, 0x00, 0x7A, 0xD5, 0x05, 0xD1};
u8 DUT_BEEP_ACK	[]	= {0x55, 0xAA, 0x01, 0x06, 0x02, 0x00, 0xED, 0x7D, 0x12, 0x0A};
u8 DUT_TMP_ACK	[] 	= {0x55, 0xAA, 0x01, 0x08, 0x03, 0x00, 0x1D, 0x00, 0x22, 0x31, 0x63, 0x16};
u8 DUT_HALL_OPEN[] 	= {0x55, 0xAA, 0x01, 0x08, 0x04, 0x00, 0x01, 0x00, 0xDE, 0x22, 0x53, 0x66};
u8 DUT_HALL_CLOSE[] = {0x55, 0xAA, 0x01, 0x08, 0x04, 0x00, 0x00, 0x00, 0x53, 0x45, 0x5E, 0x2F};
u8 DUT_OLED_ACK	[]	= {0x55, 0xAA, 0x01, 0x06, 0x05, 0x00, 0xF9, 0x56, 0xF1, 0xF1};
//u8 DUT_MOTOR_ACK[]	= {0x55, 0xAA, 0x01, 0x08, 0x06, 0x00, 0x5C, 0x00, 0x39, 0x7D, 0xD2, 0x1E};
u8 DUT_MOTOR_ACK[]	= {0x55, 0xAA, 0x01, 0x08, 0x06, 0x00, 0x3F, 0x00, 0x5F, 0xD7, 0x6A, 0xC0};
u8 DUT_CAN_ACK	[]	= {0x55, 0xAA, 0x01, 0x06, 0x07, 0x00, 0xE3, 0x99, 0xEB, 0x63};

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
void uart_to_pc(const u8* buf, u8 len)
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
u8 packet_float(float val, u8 itg, u8 dcm)
{
	u16 crc16;
	char fmt[6];
	u8 len;

	memset(uart_pc.send_buf, 0, MAX_LEN);
	uart_pc.send_buf[0] = 0xAA;

	fmt[0] = '%';
	fmt[1] = itg + '0';
	fmt[2] = '.';
	fmt[3] = dcm + '0';
	fmt[4] = 'f';
	fmt[5] = '\0';

	sprintf((char*)&uart_pc.send_buf[1], fmt, val);
	len = itg + dcm + 1;
	crc16 = crc16_calc(&uart_pc.send_buf[1], len);
	uart_pc.send_buf[len + 1] = (crc16 >> 8) & 0xff;
	uart_pc.send_buf[len + 2] = crc16 & 0xff;
	uart_pc.send_buf[len + 3] = 0xA5;

	uart_pc.send_len = len + 4;
	return len + 4;
}
/**
 * @brief packet type of bool data
 *
 * @param val value of bool type
 *
 * @retval length of packet
 */
u8 packet_hex(int val)
{
	memset(uart_pc.send_buf, 0, MAX_LEN);

	uart_pc.send_buf[0] = 0xAA;
	if(val == BAD){
		uart_pc.send_buf[1] = '0';
		uart_pc.send_buf[2] = 0x14;
		uart_pc.send_buf[3] = 0x00;
		uart_pc.send_buf[4] = 0xA5;
		uart_pc.send_len = 5;
		return 5;
	}else if(val == GOOD){
		uart_pc.send_buf[1] = '1';
		uart_pc.send_buf[2] = 0xD4;
		uart_pc.send_buf[3] = 0xC1;
		uart_pc.send_buf[4] = 0xA5;
		uart_pc.send_len = 5;
		return 5;
	}else if(val == ERROR_COM){
		uart_pc.send_buf[1] = '-';
		uart_pc.send_buf[2] = '1';
		uart_pc.send_buf[3] = 0x84;
		uart_pc.send_buf[4] = 0xDC;
		uart_pc.send_buf[5] = 0xA5;
		uart_pc.send_len = 6;
		return 6;
	}else if(val == ERROR_VOL){
		uart_pc.send_buf[1] = '-';
		uart_pc.send_buf[2] = '2';
		uart_pc.send_buf[3] = 0x85;
		uart_pc.send_buf[4] = 0x9C;
		uart_pc.send_buf[5] = 0xA5;
		uart_pc.send_len = 6;
		return 6;
	}else{
		uart_pc.send_buf[1] = '-';
		uart_pc.send_buf[2] = '3';
		uart_pc.send_buf[3] = 0x45;
		uart_pc.send_buf[4] = 0x5D;
		uart_pc.send_buf[5] = 0xA5;
		uart_pc.send_len = 6;
		return 6;
	}
}
void timer2_stop()
{
	TIM2->CR1 &= ~TIM_CR1_CEN;
}
void timer2_start()
{
	TIM2->CR1 |= TIM_CR1_CEN;
}
void timer2_clear()
{
	TIM2->CNT = 0;
}
void timer3_stop()
{
	TIM3->CR1 &= ~TIM_CR1_CEN;
}
void timer3_start()
{
	TIM3->CR1 |= TIM_CR1_CEN;
}
void timer3_clear()
{
	TIM3->CNT = 0;
}
void timer4_stop()
{
	TIM4->CR1 &= ~TIM_CR1_CEN;
}
void timer4_start()
{
	TIM4->CR1 |= TIM_CR1_CEN;
}
void timer4_clear()
{
	TIM4->CNT = 0;
}

/********************************************************
  * @brief: Computes the 32-bit CRC of a given data word(32-bit).
  * @param: pBuffer: pointer to the buffer containing the data to be computed
  * @param: BufferLength: length of the buffer to be computed
  * @retval: 32-bit CRC
********************************************************/
uint32_t u8CRC_CalcBlockCRC(uint8_t pBuffer[], uint8_t BufferLength)
{
  uint32_t index = 0;
  CRC_ResetDR();
  for(index = 0; index < BufferLength; index++)
  {
    CRC->DR = pBuffer[index];
  }
  return (CRC->DR);
}
