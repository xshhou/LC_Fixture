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
struct _key dimming;

uint8_t pwm = 0;
uint8_t key_sta = 0;
uint8_t power = 0;

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
	pwm1_init(PWM_1);
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
		if(pwr_on.delay_enable){
			pwr_on.time_cnt--;
			if(pwr_on.time_cnt == 0){
				pwr_on.delay_enable = 0;
				pwr_on.change = 1;
			}
		}
		if(dimming.delay_enable){
			dimming.time_cnt--;
			if(dimming.time_cnt == 0){
				dimming.delay_enable = 0;
				dimming.change++;
			}
		}
	}
}
void key_test()
{
	if(key.change){
		key.change = 0;
		if(power == 0){
//			pwm1_init(PWM_1);
			key_sta++;
			power = 1;
			return;
		}
		if(key.sta == DOWN){
			if(key_sta == 1){
				pwm1_update(PWM_2);
				key_sta++;
			}else if(key_sta == 2){
				pwm1_update(PWM_3);
				key_sta++;
			}else if(key_sta == 3){
				key_sta++;
				pwm1_update(0);
			}
		}else{
			if(key_sta == 4){
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
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

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
void long_press_pwr_on()
{
	pwr_on.time_cnt = 100;// 100*10mS = 1S
	pwr_on.delay_enable = 1;

	while(1){
		/* 开关状态变化，key.change会置1 */
		if(key.change){
			key.change = 0;
			GPIO_SetBits(GPIOA, GPIO_Pin_6);
		}
		/* 一定时间内开关状态未变化 */
		if(pwr_on.change){
			pwr_on.change = 0;
			pwm1_init(pwm);
			break;
		}
	}
}

