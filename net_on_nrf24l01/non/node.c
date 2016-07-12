/*
 * node.c
 *
 *  Created on: 2016Äê6ÔÂ18ÈÕ
 *      Author: wangjiuling
 */

#include "non.h"

#ifdef NODE

#define EVENT_RECV_PACKET	(1 << 0)
#define EVENT_TIMEOUT_NM	(1 << 1)
#define	EVENT_RECV_REG		(1 << 2)
#define EVENT_REG_SUCCESS	(1 << 3)
#define EVENT_SEND_PACKET	(1 << 4)

static struct rt_event	event_non;
static struct _packet	recv_buf;
static struct _packet	send_buf;
//static struct rt_timer	timer_nm;
static struct _addr		node_addr;
static struct _addr		router_addr;
u8 cpu_id[12];

static void printf_packet(u8* buf);
static void addr_init(void);
static void send_packet(void);
static void get_cpu_id(void);
static void heart_beat_thread(void* parameter);

static void regist_therad(void* parameter)
{
	rt_uint32_t e;
	u8 i;
	u8 sta;

	send_buf.saddr = 0xFF;
	send_buf.daddr = 0xFF;
	send_buf.pid = 0x0;
	send_buf.cmd = 0;
	memset(send_buf.data, 28, 0);
	memcpy(send_buf.data, cpu_id, 12);

	rt_kprintf("send register frame:\r\n");
	printf_packet((u8*)&send_buf);
//	send_packet((u8*)&send_buf);
	rt_event_send(&event_non, EVENT_SEND_PACKET);

	rt_event_recv(&event_non,
				EVENT_RECV_REG,
				RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
				RT_WAITING_FOREVER,
				&e);
	if(recv_buf.daddr == 0xFF
	&& recv_buf.pid == 0x01
	&& recv_buf.cmd == 0x01){
		sta = 0;
		for(i = 0;i < 12;i++){
			if(recv_buf.data[i] != cpu_id[i]){
				sta = 1;
				break;
			}
		}
		if(sta == 0){
			node_addr.addr = recv_buf.data[12];
			node_addr.flag = 'Y';
			router_addr.addr = recv_buf.saddr;
			router_addr.flag = 'Y';
			rt_kprintf("router address: 0x%02X\r\n", router_addr.addr);
			rt_kprintf("received register acknowledge frame\r\n");
//			flash_write(BASE_ADDR + ROUTER_ADDR_SHIFT, (u8*)&router_addr, 2);
//			flash_write(BASE_ADDR + NODE_ADDR_SHIFT, (u8*)&node_addr, 2);

			send_buf.saddr = 0xFF;
			send_buf.daddr = router_addr.addr;
			send_buf.pid = 0x82;
			send_buf.cmd = 2;
			memset(send_buf.data, 0, 28);
			memcpy(send_buf.data, cpu_id, 12);
			rt_kprintf("send register confirm frame\r\n");
			printf_packet((u8*)&send_buf);

//			send_packet((u8*)&send_buf);
			rt_event_send(&event_non, EVENT_SEND_PACKET);

			rt_event_send(&event_non, EVENT_REG_SUCCESS);
		}else{
			rt_kprintf("this register acknowledge frame was not sent to me\r\n");
		}
	}
}

static void net_manage_thread(void* parameter)
{
	rt_uint32_t e;

	while(1){
		rt_event_recv(&event_non,
					EVENT_RECV_PACKET | EVENT_SEND_PACKET,
					RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
					RT_WAITING_FOREVER,
					&e);
		if(e == EVENT_RECV_PACKET){
			if(nrf24l01_rx_packet((u8*)&recv_buf) == RX_OK){
				rt_kprintf("receive packet\r\n");
				printf_packet((u8*)&recv_buf);

				if(node_addr.flag == 'Y'){
					if(recv_buf.daddr == node_addr.addr){

					}
				}else{
					rt_event_send(&event_non, EVENT_RECV_REG);
				}
			}
		}else{
			send_packet();
		}
	}
}
//static void timeout_nm(void* parameter)
//{
//	rt_event_send(&event_non, EVENT_TIMEOUT_NM);
//}
void non_init(void* parameter)
{
	int sta;
	u8 i = 0;
	rt_uint32_t e;

	rt_thread_delay(DELAY_MS(10));
	rt_kprintf("\r\n");

	addr_init();
	get_cpu_id();

	rt_kprintf("cpu id: ");
	for(i = 0;i < 12;i++){
		rt_kprintf("%02x ", cpu_id[i]);
	}
	rt_kprintf("\r\n");
	rt_kprintf("node address: 0x%02X\r\n", node_addr.addr);
	rt_kprintf("router address: 0x%02X\r\n", router_addr.addr);

	rt_event_init(&event_non, "non", RT_IPC_FLAG_FIFO);

	sta = nrf24l01_init();
	if(sta != -1){
		rt_kprintf("NRF24L01 node initialize success\r\n");
	}

	nrf24l01_rx_mode();
	nrf24l01_rx_packet(RT_NULL);

	rt_thread_startup(rt_thread_create("nm",
								net_manage_thread,
								RT_NULL,
								2048,
								20,
								100));
//	rt_timer_init(&timer_nm,
//				"nm",
//				&timeout_nm,
//				RT_NULL,
//				DELAY_MS(20),
//				RT_TIMER_FLAG_ONE_SHOT);
//	rt_timer_start(&timer_nm);

	if(node_addr.flag != 'Y'){
		rt_kprintf("node don't have address, start register thread...\r\n");
		rt_thread_startup(rt_thread_create("reg",
						regist_therad,
						RT_NULL,
						2048,
						20,
						100));
		rt_event_recv(&event_non,
						EVENT_REG_SUCCESS,
						RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
						RT_WAITING_FOREVER,
						&e);
		rt_kprintf("node regist success\r\n");
	}
	rt_thread_startup(rt_thread_create("heart beat",
					heart_beat_thread,
					RT_NULL,
					512,
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
static void addr_init()
{
	flash_read(BASE_ADDR + NODE_ADDR_SHIFT, (u8*)&node_addr, 2);
	flash_read(BASE_ADDR + ROUTER_ADDR_SHIFT, (u8*)&router_addr, 2);
}
static void send_packet()
{
	nrf24l01_tx_mode();
	nrf24l01_tx_packet((u8*)&send_buf);
	nrf24l01_rx_mode();
}
static void get_cpu_id()
{
	u8 i;

	for(i = 0;i < 12;i++){
		cpu_id[i] = *(volatile u8*)(0x1ffff7e8 + i);
	}
}
static void heart_beat_thread(void* parameter)
{
//	while(1){
//		rt_thread_delay(DELAY_MS(10000));
//		send_buf.saddr = node_addr.addr;
//		send_buf.daddr = router_addr.addr;
//		send_buf.pid = 0x80;
//		send_buf.cmd = 3;
//		memset(send_buf.data, 0, 28);
//		memcpy(send_buf.data, cpu_id, 12);
//		rt_kprintf("send heart beat packet\r\n");
//		printf_packet((u8*)&send_buf);
//
//		rt_event_send(&event_non, EVENT_SEND_PACKET);
//	}
}
#endif
