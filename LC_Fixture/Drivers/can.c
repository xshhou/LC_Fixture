/*
 * can.c
 *
 *  Created on: 2016年6月2日
 *      Author: wangjiuling
 */

#include "can.h"

static void rcc_config()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO ,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
}
static void nvic_config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable CAN1 RX0 interrupt IRQ channel */
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
static void gpio_config()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_PinRemapConfig(GPIO_Remap1_CAN1 , ENABLE);

	/* Configure CAN pin: RX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/* Configure CAN pin: TX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
static void can_config(void)
{
	CAN_InitTypeDef CAN_InitStructure;

	/* CAN register init */
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);

	/* CAN cell init */
	CAN_InitStructure.CAN_TTCM = DISABLE;	// 时间触发模式
	CAN_InitStructure.CAN_ABOM = DISABLE;	// 自动离线管理
	CAN_InitStructure.CAN_AWUM = DISABLE;	// 自动唤醒模式
	CAN_InitStructure.CAN_NART = DISABLE;	// 非自动重传模式
	CAN_InitStructure.CAN_RFLM = DISABLE;	// FIFO锁定模式
	CAN_InitStructure.CAN_TXFP = DISABLE;	// 自FIFO优先级
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;	// 正常模式
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;	// 重新同步跳跃宽度1个时间单位
	CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq;	// 时间段1为3个时间单位
	CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;	// 时间段2为2个时间单位
	CAN_InitStructure.CAN_Prescaler = 60;		// 时间单位长度为60
	/* baud 72M/2/60(1+3+2)=0.1M, 100K */

	CAN_Init(CAN1, &CAN_InitStructure);
}
static void filter_config()
{
	CAN_FilterInitTypeDef CAN_FilterInitStructure;

	// 设置CAN接收过滤器
	CAN_FilterInitStructure.CAN_FilterNumber = 0;						// 过滤器0
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		// 标识符屏蔽位模式
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	// 位宽32bit
	// 以下4个为0，表示接收任何数据
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);
}
void can_init()
{
	rcc_config();
	nvic_config();
	gpio_config();
	can_config();
	filter_config();
	/* CAN FIFO0 message pending interrupt enable */
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);	// FIFO0 消息挂号中断
}
int can_send_data(CanTxMsg *TxMessage)
{
	uint8_t TransmitMailbox;
	uint32_t TimeOut = 0;

	TransmitMailbox = CAN_Transmit(CAN1, TxMessage);
	while (CAN_TransmitStatus(CAN1, TransmitMailbox) != CAN_TxStatus_Ok) {
		TimeOut++;
		if (TimeOut > 10000000) {
			return -1;
		}
	}
	return 0;
}

