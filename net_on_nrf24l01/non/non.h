/*
 * non.h
 *
 *  Created on: 2016Äê6ÔÂ18ÈÕ
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

#define DELAY_MS(ms)	(RT_TIMER_TICK_PER_SECOND / 1000 * ms)
#define BASE_ADDR	((u32)0x0800F000)
#define NODE_ADDR_SHIFT		0
#define ROUTER_ADDR_SHIFT		2
#define ROUTER_ADDRESS	0x00

struct _packet{
	u8 saddr;
	u8 daddr;
	u8 pid;
	u8 cmd;
	u8 data[28];
};
struct _addr{
	char flag;
	u8 addr;
};

struct _addr_pool{
	u8 addr;
	u8 cpu_id[12];
	struct _addr_pool* next;
};

#ifndef NODE
#define ROUTER
#endif

void non_init(void* parameter);
void dhcp_init(void);
int get_addr(u8* id);

#endif /* NON_NON_H_ */
