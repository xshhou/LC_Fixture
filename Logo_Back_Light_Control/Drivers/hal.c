/*
 * hal.c
 *
 *  Created on: 2016年6月29日
 *      Author: wangjiuling
 */

#include "hal.h"
#include "gpio.h"
#include "timer.h"
#include "adc.h"
#include "flash.h"

struct _key key;
struct _key led;
struct _adc adc;
struct _battery battery;
struct _key pwr_on;
struct _key breathing;

uint16_t pwm = 0;
uint8_t key_sta = 0;

void hal_init(void)
{
	gpio_init();
	timer3_init(48000, 10);//48E6 / (48e3*10) = 100Hz, 10mS
	adc_init();
//	flash_read(ADDR_DATA, &pwm, 1);
//	if(pwm > PWM_HIG || pwm < PWM_LOW){
//		pwm = PWM_DEFAULT;
//	}
//	flash_write(ADDR_DATA, &pwm, sizeof(pwm));
	pwm1_init(0);
	breathing.time_cnt = BREATHING_TIME;
	breathing.delay_enable = 1;
	breathing.change = 1;
}

void EXTI0_1_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line1) != RESET)
	{
		key.time_cnt = 5;
		key.delay_enable = 1;
		while(key.delay_enable == 1);// delay 50mS
		key.sta = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
		key.change = 1;

		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

		if(key.delay_enable){
			key.time_cnt--;
			if(key.time_cnt == 0){
				key.delay_enable = 0;
			}
		}
		if(led.delay_enable){
			led.time_cnt--;
			if(led.time_cnt == 0){
				led.time_cnt = LED_FLASH_TIME;
				led.change = 1;
			}
		}
		/* 这个pwr_on去掉，会无法关机（灯灭了，电断不掉），WTF */
		if(pwr_on.delay_enable){
			pwr_on.time_cnt--;
			if(pwr_on.time_cnt == 0){
				pwr_on.delay_enable = 0;
				pwr_on.change = 1;
			}
		}
		if(breathing.delay_enable){
			breathing.time_cnt--;
			if(breathing.time_cnt == 0){
				breathing.time_cnt = BREATHING_TIME;
				breathing.change = 1;
			}
		}
	}
}
void key_test()
{
	if(key.change){
		key.change = 0;
		if(key.sta == DOWN){
			breathing.delay_enable = 0;// 禁止呼吸效果，直接跳到指定的亮度
			if(key_sta == 0){
				pwm1_update(PWM_2);
				key_sta++;
			}else if(key_sta == 1){
				pwm1_update(PWM_3);
				key_sta++;
			}else if(key_sta == 2){
				key_sta++;
				pwm1_update(0);
			}
		}else{
			if(key_sta == 3){
				GPIO_SetBits(GPIOA, GPIO_Pin_6);
				while(1);
			}
		}
	}
}
void adc_convert()
{
	uint8_t i;

	/* Test EOC flag */
//	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET){
		return;
	}

	/* Get ADC1 converted data */
	adc.buf[adc.pos] = ADC_GetConversionValue(ADC1);
	adc.pos++;
	if(adc.pos >= ADC_BUF_LENGTH){
		adc.pos = 0;
		adc.sum = 0;

		/* in a little time while power on, disable ADC,
		 * cause of the capacity of ADC hanvn't full charged */
		if(adc.enable == 0){
			adc.times++;
			if(adc.times > 100){
				adc.enable = 1;
			}else{
				return;
			}
		}

		for(i = 0; i < ADC_BUF_LENGTH; i++){
			adc.sum += adc.buf[i];
		}

		/* Compute the voltage */
		battery.value = (float) adc.sum / ADC_BUF_LENGTH / 800;

		/* test adapter whether present */
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == 0){
			battery.low = 0;
		}

		/* if battery voltage is too low */
		if(battery.low == 0){
			if(battery.value < 3.0){
				/* close led */
				pwr_on.delay_enable = 0;
				pwm1_update(0);
				GPIO_ResetBits(GPIOA, GPIO_Pin_10);

				/* enable led timer to flash led */
				led.time_cnt = LED_FLASH_TIME;
				led.delay_enable = 1;
				/* disable adc_convert function */
				battery.low = 1;
			}
		}
	}
}
void led_control()
{
	/* test led timer change flag */
	if(led.change){
		led.change = 0;
		led.sta = !led.sta;
		if(led.sta){
			pwm1_update(5);
		}else{
			pwm1_update(0);
		}
		led.times++;

		if(led.times > LED_FLASH_TIMES){
			GPIO_SetBits(GPIOA, GPIO_Pin_6);
			while(1);
		}
	}
}
void led_breathing()
{
	if(breathing.change){
		breathing.change = 0;
		pwm += (PWM_1 >> 4);
		if(pwm > PWM_1){
			breathing.delay_enable = 0;
		}
		pwm1_update(pwm);
	}
}
