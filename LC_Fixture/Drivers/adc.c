/*
 * adc.c
 *
 *  Created on: 2016��5��31��
 *      Author: wangjiuling
 */

#include "adc.h"
#include "hal.h"

volatile u16 ad_value[N][M];


static void rcc_config()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA
						| RCC_APB2Periph_GPIOB, ENABLE );
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	// ����ADC��Ƶ����6, 72M/8=12, ADC���ʱ�䲻�ܳ���14M
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
}
static void gpio_config()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0
								| GPIO_Pin_1
								| GPIO_Pin_2
								| GPIO_Pin_3
								| GPIO_Pin_4
								| GPIO_Pin_5
								| GPIO_Pin_6
								| GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
static void adc_config()
{
	ADC_InitTypeDef ADC_InitStructure;

	ADC_DeInit(ADC1);  //��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ
//	ADC_InitStructure.ADC_ScanConvMode = ENABLE;	//ģ��ת��������ɨ��ģʽ
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
//	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	//ģ��ת������������ת��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = M;	//˳����й���ת����ADCͨ������Ŀ
	ADC_Init(ADC1, &ADC_InitStructure);	//����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���

	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 5, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 6, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 7, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 8, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 9, ADC_SampleTime_239Cycles5);

	// ����ADC��DMA֧�֣�Ҫʵ��DMA���ܣ������������DMAͨ���Ȳ�����
	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1

	ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼
	while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����
	ADC_StartCalibration(ADC1);	 //����ADУ׼
	while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����
}
static void dma_config()
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_DeInit(DMA1_Channel1); //��DMA��ͨ��1�Ĵ�������Ϊȱʡֵ

	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->DR; //DMA����ADC����ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&ad_value; //DMA�ڴ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; //�ڴ���Ϊ���ݴ����Ŀ�ĵ�
	DMA_InitStructure.DMA_BufferSize = N*M; //DMAͨ����DMA����Ĵ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; //�����ַ�Ĵ�������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; //�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //���ݿ��Ϊ16λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //���ݿ��Ϊ16λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//DMA_Mode_Normal;DMA_Mode_Circular //������ѭ������ģʽ
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; //DMAͨ�� xӵ�и����ȼ�
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��

	DMA_Init(DMA1_Channel1, &DMA_InitStructure); //����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA��ͨ��
//	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE); //ʹ��DMA��������ж�
	DMA_Cmd(DMA1_Channel1, ENABLE); //����DMAͨ��
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
//static void nvic_config()
//{
//  NVIC_InitTypeDef NVIC_InitStructure;
//
//  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
//  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//}
void adc_init()
{
	rcc_config();
//	nvic_config();
	gpio_config();
	adc_config();
	dma_config();
}

void adc_start()
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
void adc_stop()
{
//	DMA_Cmd(DMA1_Channel1, DISABLE);
	ADC_SoftwareStartConvCmd(ADC1, DISABLE);
}


