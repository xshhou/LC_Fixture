/*
 * non.h
 *
 *  Created on: 2016年7月28日
 *      Author: wangjiuling
 */

#ifndef NON_NON_H_
#define NON_NON_H_

#define NODE

#include "stm32f10x.h"
#include <rtthread.h>
#include <rtconfig.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "24L01.h"
#include "flash.h"
/* 选项 */
#define NON_DEBUG_ENABLE
/* 变量定义 */
#define DELAY_MS(ms)	(RT_TIMER_TICK_PER_SECOND / 1000 * ms)
#define ADDR_SRC		0x01// 数据的源地址
#define ADDR_DST		0x00// 数据的目标地址
#define ADDR_MY			ADDR_SRC// 器件地址
#define LEN_DATA		29// _packet的数据长度
/* 事件定义 */
#define EV_RECV		(1 << 0)
#define EV_RECVE_F		(1 << 0) // victory, 成功
#define EV_SEND			(1 << 1)
#define EV_SEND_V		(1 << 2) // victory, 成功
#define EV_SEND_F		(1 << 3) // failure, 失败
/* 命令定义 */
enum{
	CMD_ACK,// 应答命令
};
/* 结构体定义 */
struct _packet{
	u8 addr_src;// source, 源地址
	u8 addr_dst;//　destination, 目标地址
	u8 cmd;
	u8 data[LEN_DATA];
};
/* 条件编译 */
#ifdef	NON_DEBUG_ENABLE
#define NON_DEBUG	rt_kprintf
#else
#define NON_DEBUG(...)
#endif
/* 函数声明 */
void non_init(void* parameter);

#endif /* NON_NON_H_ */
