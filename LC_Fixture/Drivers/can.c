/*
 * can.c
 *
 *  Created on: 2016��6��2��
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
	CAN_InitStructure.CAN_TTCM = DISABLE;	// ʱ�䴥��ģʽ
	CAN_InitStructure.CAN_ABOM = DISABLE;	// �Զ����߹���
	CAN_InitStructure.CAN_AWUM = DISABLE;	// �Զ�����ģʽ
	CAN_InitStructure.CAN_NART = DISABLE;	// ���Զ��ش�ģʽ
	CAN_InitStructure.CAN_RFLM = DISABLE;	// FIFO����ģʽ
	CAN_InitStructure.CAN_TXFP = DISABLE;	// ��FIFO���ȼ�
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;	// ����ģʽ
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;	// ����ͬ����Ծ���1��ʱ�䵥λ
	CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq;	// ʱ���1Ϊ3��ʱ�䵥λ
	CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;	// ʱ���2Ϊ2��ʱ�䵥λ
	CAN_InitStructure.CAN_Prescaler = 60;		// ʱ�䵥λ����Ϊ60
	/* baud 72M/2/60(1+3+2)=0.1M, 100K */

	CAN_Init(CAN1, &CAN_InitStructure);
}
static void filter_config()
{
	CAN_FilterInitTypeDef CAN_FilterInitStructure;

	// ����CAN���չ�����
	CAN_FilterInitStructure.CAN_FilterNumber = 0;						// ������0
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		// ��ʶ������λģʽ
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	// λ��32bit
	// ����4��Ϊ0����ʾ�����κ�����
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
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);	// FIFO0 ��Ϣ�Һ��ж�
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

