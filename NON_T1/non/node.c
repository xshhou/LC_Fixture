/*
 * node.c
 *
 *  Created on: 2016��7��28��
 *      Author: wangjiuling
 */

#include "non.h"

struct rt_event	e_non;
struct _packet	buf_recv;
struct _packet	buf_send;
//u8 buf_recv[32];
//u8 buf_send[32];

/* ��ӡ���� */
void data_print(u8* buf)
{
	u8 i;

	rt_kprintf("data: ");
	for(i = 0;i < 32;i++){
		rt_kprintf("%2X ", buf[i]);
	}
	rt_kprintf("\r\n");
}
/* ���������ж� */
void EXTI1_IRQHandler(void)
{
	rt_interrupt_enter();


//	if(buf_recv.addr_to == ADDR_MY){
		rt_event_send(&e_non, EV_RECV);
//	}

	EXTI_ClearITPendingBit(EXTI_Line1);

	rt_interrupt_leave();
}
/* �������� */
void data_send()
{
	nrf24l01_tx_mode();
	nrf24l01_tx_packet((u8*)&buf_send);
	nrf24l01_rx_mode();
}
/* ����Ӧ���ź� */
void ack_send()
{
}
u8 wait_ack = 0;
u8 num_send = 0;
void (*recv_indicate)(void);
void default_recv_indicate()
{
	NON_DEBUG("�յ�����\r\n");
	data_print((u8*)&buf_recv);
	data_send();
}
void thread_recv(void* parameter)
{
	rt_uint32_t e;

	while(1){
		rt_event_recv(&e_non,
				EV_RECV,
				RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
				RT_WAITING_FOREVER,
				&e);
		if(nrf24l01_rx_packet((u8*)&buf_recv) == RX_OK){
			/* �����ջ����е�Ŀ���ַ���ǲ��Ǹ��ҵ� */
			if(buf_recv.addr_dst == ADDR_MY){
				if(wait_ack == 1){
					/* ���ͳɹ� */
					rt_event_send(&e_non, EV_SEND_V);
				}else{
					recv_indicate();
				}
			}
		}
	}
}
int non_send()
{
	rt_uint32_t e;

	num_send = 0;
	wait_ack = 1;
	while(1){
		data_send();
		/* �ȴ����ͳɹ� */
		rt_event_recv(&e_non,
					EV_SEND_V,
					RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
					DELAY_MS(100),
					&e);
		if(e == EV_SEND_V){
			/* �յ� �����ͳɹ������¼� */
			NON_DEBUG("���ͳɹ�\r\n");
			wait_ack = 0;
			return 0;
		}else{
			/* ��ʱδ�յ������ͳɹ������¼� */
			num_send++;
			if(num_send > 3){
				/* �ط��������࣬�������� */
				return -1;
			}
			NON_DEBUG("����ʧ�ܣ�%d\r\n", num_send);
			/* �¸�ѭ���У��ٴη��� */
		}
	}
}

void thread_test(void* parameter)
{
	while(1){
		if(non_send() == 0){
			/* ���ͳɹ� */
			data_print((u8*)&buf_recv);
		}
		rt_thread_delay(DELAY_MS(1000));
	}
}
/* non��ʼ�� */
void non_init(void* parameter)
{
	int sta;
	rt_thread_t init_thread;

	rt_thread_delay(DELAY_MS(10));
	rt_kprintf("\r\n");

	rt_event_init(&e_non, "non", RT_IPC_FLAG_FIFO);

	sta = nrf24l01_init();
	if(sta != -1){
		rt_kprintf("NRF24L01 initialize success\r\n");
	}

	recv_indicate = default_recv_indicate;
	nrf24l01_rx_mode();
	nrf24l01_rx_packet((u8*)&buf_recv);
	init_thread = rt_thread_create("non recv",
							thread_recv, RT_NULL,
							2048, 20, 100);
	if(init_thread != RT_NULL){
		rt_thread_startup(init_thread);
	}

	buf_send.addr_dst = ADDR_DST;
	buf_send.addr_src = ADDR_SRC;
	buf_send.cmd = 2;
	buf_send.data[0] = 3;

	/* �����ڵ㣬һ�����ͣ�һ������ */
//	init_thread = rt_thread_create("non test",
//							thread_test, RT_NULL,
//							2048, 20, 100);
//	if(init_thread != RT_NULL){
//		rt_thread_startup(init_thread);
//	}
}
