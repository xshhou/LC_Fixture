/*
 * dhcp.c
 *
 *  Created on: 2016Äê6ÔÂ18ÈÕ
 *      Author: wangjiuling
 */

#include "non.h"
#include <finsh.h>

#ifdef ROUTER

static struct _addr_pool* addr_pool = NULL;
u8 cpu_id[12];
static void get_cpu_id(void);

void dhcp_init()
{
	struct _addr_pool* node;

	get_cpu_id();

	node = (struct _addr_pool*)malloc(sizeof(struct _addr_pool));
	if(node == NULL){
		rt_kprintf("ERROR: malloc failed!\r\n");
		return;
	}
	memcpy(node->cpu_id, cpu_id, 12);
	node->addr = ROUTER_ADDRESS;
	node->next = NULL;
	addr_pool = node;
}

void print_addr_pool()
{

}

int get_addr(u8* id)
{
	struct _addr_pool* node;
	struct _addr_pool* tmp;
	u8 i;
	u8 flag;
	u8 addr = 0;

	tmp = addr_pool;
	while(tmp != NULL){
		/* find whether this id is already in address pool */
		addr++;
		flag = 0;
		for(i = 0;i < 12;i++){
			if(tmp->cpu_id[i] != *(id + i)){
				flag = 1;
				break;
			}
		}
		if(flag == 0){
			/* is in address pool */
			return tmp->addr;
		}
		if(tmp->next == NULL){
			break;
		}else{
			tmp = tmp->next;
		}
	}
	node = (struct _addr_pool*)malloc(sizeof(struct _addr_pool));
	if(node == NULL){
		rt_kprintf("ERROR: malloc failed!\r\n");
		return -1;
	}
	memcpy(node->cpu_id, id, 12);
	node->addr = addr;
	node->next = NULL;

	tmp->next = node;

	return addr;
}
static void get_cpu_id()
{
	u8 i;

	for(i = 0;i < 12;i++){
		cpu_id[i] = *(volatile u8*)(0x1ffff7e8 + i);
	}
}
void list_node(void* parameter)
{
	struct _addr_pool* tmp = addr_pool;
	u8 i;

	while(tmp != NULL){
		if(tmp->addr == ROUTER_ADDRESS){
			rt_kprintf("(router)");
		}
		rt_kprintf("\taddress: 0x%02X\r\n\t", tmp->addr);
		for(i = 0;i< 12;i++){
			rt_kprintf("%02X ", tmp->cpu_id[i]);
		}
		rt_kprintf("\r\n");
		tmp = tmp->next;
	}
}
FINSH_FUNCTION_EXPORT(list_node, list node that registed)

#endif
