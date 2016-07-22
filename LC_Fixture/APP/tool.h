/*
 * tool.h
 *
 *  Created on: 2016Äê7ÔÂ19ÈÕ
 *      Author: wangjiuling
 */

#ifndef APP_TOOL_H_
#define APP_TOOL_H_

#include "hal.h"

/* command to DUT */
extern u8 DUT_LED_ON	[10];
extern u8 DUT_BEEP_ON	[10];
extern u8 DUT_BEEP_OFF	[10];
extern u8 DUT_TMP_CHECK	[10];
extern u8 DUT_HALL_CHECK[10];
extern u8 DUT_OLED_ON	[10];
extern u8 DUT_MOTOR_ON	[10];
extern u8 DUT_MOTOR_OFF	[10];
extern u8 DUT_CAN_CHECK	[10];
/* ACK from DUT */
extern u8 DUT_LED_ACK	[10];
extern u8 DUT_BEEP_ACK	[10];
extern u8 DUT_TMP_ACK	[12];
extern u8 DUT_HALL_OPEN	[12];
extern u8 DUT_HALL_CLOSE[12];
extern u8 DUT_OLED_ACK	[10];
extern u8 DUT_MOTOR_ACK	[12];
extern u8 DUT_CAN_ACK	[10];

void uart_dut_putch(char ch);
void uart_dut_putln(const u8* buf, u8 len);
void uart_pc_putch(char ch);
void uart_pc_putln(const u8* buf, u8 len);
int verify_data(const u8* buf, u8 len);
int compare(const u8* p1, const u8* p2, u8 len);
u8 packet_hex(u8 val);
u8 packet_float(float val, u8 itg, u8 dcm);
void timer2_stop(void);
void timer2_start(void);
void timer2_clear(void);
void timer3_stop(void);
void timer3_start(void);
void timer3_clear(void);
void timer4_stop(void);
void timer4_start(void);
void timer4_clear(void);

#endif /* APP_TOOL_H_ */
