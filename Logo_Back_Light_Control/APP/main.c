/*
 * main.c
 *
 *  Created on: 2016Äê6ÔÂ29ÈÕ
 *      Author: wangjiuling
 */

#include "hal.h"
#include "timer.h"

extern struct _key key;

#define NUM	1000

uint8_t flag = 0;
uint16_t adc_value[NUM];
uint32_t value_sum;
uint16_t position = 0;
float battery_voltage = 0;
extern struct _key led;
uint8_t low_power = 0;

void gpio_a0_init()
{
//	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOA Clock enable */
//	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
//
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_10);
}

int main()
{
	uint16_t i;

	hal_init();

	printf("power on\r\n");

	while(1){
		if(key.change == 1){
			key.change = 0;
			printf("key up\r\n");

			flag++;// first key up
			if(flag == 2){
				flag = 0;
				printf("power off\r\n");
				GPIO_SetBits(GPIOA, GPIO_Pin_6);
				while(1);
			}
		}
		/* Test EOC flag */
		while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

		/* Get ADC1 converted data */
		adc_value[position] = ADC_GetConversionValue(ADC1);
		position++;
		if(position >= NUM){
			position = 0;
			value_sum = 0;
			for(i = 0;i < NUM;i++){
				value_sum += adc_value[i];
			}

			/* Compute the voltage */
			battery_voltage = (float)value_sum / NUM / 800;
			printf("voltage: %.3f\r\n", battery_voltage);
			if(low_power == 0){
				if(battery_voltage < 3.85){
					printf("low power!!!\r\n");
					gpio_a0_init();
					TIM_Cmd(TIM1, DISABLE);
					led.time = 20;
					led.delay = 1;
					low_power = 1;
				}
			}
		}
		if(led.change){
			led.change = 0;
			led.sta = !led.sta;
			if(led.sta){
				pwm1_init(10);
			}else{
				TIM_Cmd(TIM1, DISABLE);
				if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 1){
					GPIO_ResetBits(GPIOA, GPIO_Pin_10);
				}
			}
		}
	}
}
