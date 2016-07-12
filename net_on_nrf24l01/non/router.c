/*
 * node.c
 *
 *  Created on: 2016Äê6ÔÂ18ÈÕ
 *      Author: wangjiuling
 */

#include "non.h"

#ifdef ROUTER

#define EVENT_RECV_PACKET	(1 << 0)
#define EVENT_SEND_PACKET	(1 << 1)
#define EVENT_RECV_REG		(1 << 2)

static struct rt_event	event_non;
static struct _packet	recv_buf;
static struct _packet	send_buf;
static u8 router_addr = 0;

static void printf_packet(u8* buf);
static void send_packet(u8* buf);

static void regist_thread(void* parameter)
{
	rt_uint32_t e;
	int addr;

	while(1){
		rt_event_recv(&event_non,
					EVENT_RECV_REG,
					RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
					RT_WAITING_FOREVER,
					&e);
		if(recv_buf.cmd == 0x00){
			rt_kprintf("received register frame\r\n");
			send_buf.saddr = router_addr;
			send_buf.daddr = 0xFF;
			send_buf.pid = 0x01;
			send_buf.cmd = 0x01;
			memset(send_buf.data, 0, 28);
			memcpy(send_buf.data, recv_buf.data, 12);
			addr = get_addr(recv_buf.data);
			if(addr == -1){
				rt_kprintf("ERROR: get address failed!\r\n");
				continue;
			}
			rt_kprintf("address: %02X\r\n", addr);
			send_buf.data[12] = addr & 0xFF;

			rt_kprintf("send register acknowledge frame:\r\n");
			printf_packet((u8*)&send_buf);

			send_packet((u8*)&send_buf);
		}else if(recv_buf.cmd == 0x02){
			rt_kprintf("received register confirm frame\r\n");
		}

	}


}
static void net_manage_thread(void* parameter)
{
	rt_uint32_t e;


	rt_kprintf("net manage thread..\r\n");
	while(1){
		rt_event_recv(&event_non,
					EVENT_RECV_PACKET,
					RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
					RT_WAITING_FOREVER,
					&e);
		if(nrf24l01_rx_packet((u8*)&recv_buf) == RX_OK){
			rt_kprintf("receive packet\r\n");
			printf_packet((u8*)&recv_buf);
			if(recv_buf.daddr == 0xFF){
				if(recv_buf.saddr == 0xFF
				&& recv_buf.pid == 0x00
				&& recv_buf.cmd == 0x00){
					rt_event_send(&event_non, EVENT_RECV_REG);
				}
			}else if(recv_buf.daddr == router_addr){
				if(recv_buf.pid == 0x82
				&& recv_buf.cmd == 0x02){
					rt_event_send(&event_non, EVENT_RECV_REG);
				}
			}
		}
	}
}
void non_send(void* parameter)
{
	u8 buf[32];
	rt_uint32_t e;
	u8 i;
	u8 j;

	rt_thread_delay(DELAY_MS(100));

	while(1){
		rt_event_recv(
				&event_non,
				EVENT_SEND_PACKET,
				RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
				RT_WAITING_FOREVER,
				&e);
		for(i = 0;i < 32;i++){
			buf[i] = j++;
		}

		nrf24l01_tx_packet(buf);
		rt_kprintf("send packet\r\n");
		printf_packet(buf);
	}
}
void non_init(void* parameter)
{
	int sta;

	rt_thread_delay(DELAY_MS(10));
	rt_kprintf("\r\n");

	rt_event_init(&event_non, "non", RT_IPC_FLAG_FIFO);

//	router_addr = 0;

	sta = nrf24l01_init();
	if(sta != -1){
		rt_kprintf("NRF24L01 router initialize success\r\n");
	}
	dhcp_init();

	nrf24l01_rx_mode();

	rt_thread_startup(rt_thread_create("register",
					regist_thread,
					RT_NULL,
					2048,
					20,
					100));
	rt_thread_startup(rt_thread_create("nm",
					net_manage_thread,
					RT_NULL,
					2048,
					20,
					100));
}

void EXTI1_IRQHandler(void)
{
	rt_interrupt_enter();

	rt_event_send(&event_non, EVENT_RECV_PACKET);

	EXTI_ClearITPendingBit(EXTI_Line1);

	rt_interrupt_leave();
}

static void printf_packet(u8* buf)
{
	u8 i;

	rt_kprintf("packet: ");
	for(i = 0;i < 32;i++){
		rt_kprintf("%02X ", buf[i]);
	}
	rt_kprintf("\r\n");
}
static void send_packet(u8* buf)
{
	nrf24l01_tx_mode();
	nrf24l01_tx_packet(buf);
	nrf24l01_rx_mode();
}
#endif
