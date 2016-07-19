/*
 * tool.h
 *
 *  Created on: 2016Äê7ÔÂ19ÈÕ
 *      Author: wangjiuling
 */

#ifndef APP_TOOL_H_
#define APP_TOOL_H_

#include "hal.h"

void uart_dut_putch(char ch);
void uart_dut_putln(const u8* buf, u8 len);
void uart_pc_putch(char ch);
void uart_pc_putln(const u8* buf, u8 len);
int verify_data(const u8* buf, u8 len);
int compare(const u8* p1, const u8* p2, u8 len);
u8 packetb_pc(u8 val);
u8 packetf_pc(float val, u8 itg, u8 dcm);



#endif /* APP_TOOL_H_ */
