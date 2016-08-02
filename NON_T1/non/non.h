/*
 * non.h
 *
 *  Created on: 2016��7��28��
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
/* ѡ�� */
#define NON_DEBUG_ENABLE
/* �������� */
#define DELAY_MS(ms)	(RT_TIMER_TICK_PER_SECOND / 1000 * ms)
#define ADDR_SRC		0x01// ���ݵ�Դ��ַ
#define ADDR_DST		0x00// ���ݵ�Ŀ���ַ
#define ADDR_MY			ADDR_SRC// ������ַ
#define LEN_DATA		29// _packet�����ݳ���
/* �¼����� */
#define EV_RECV		(1 << 0)
#define EV_RECVE_F		(1 << 0) // victory, �ɹ�
#define EV_SEND			(1 << 1)
#define EV_SEND_V		(1 << 2) // victory, �ɹ�
#define EV_SEND_F		(1 << 3) // failure, ʧ��
/* ����� */
enum{
	CMD_ACK,// Ӧ������
};
/* �ṹ�嶨�� */
struct _packet{
	u8 addr_src;// source, Դ��ַ
	u8 addr_dst;//��destination, Ŀ���ַ
	u8 cmd;
	u8 data[LEN_DATA];
};
/* �������� */
#ifdef	NON_DEBUG_ENABLE
#define NON_DEBUG	rt_kprintf
#else
#define NON_DEBUG(...)
#endif
/* �������� */
void non_init(void* parameter);

#endif /* NON_NON_H_ */
