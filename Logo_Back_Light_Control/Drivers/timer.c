/*
 * timer.c
 *
 *  Created on: 2016Äê7ÔÂ1ÈÕ
 *      Author: wangjiuling
 */

#include "timer.h"
/**
 * time = (TIM_Prescaler+1) * (TIM_Period+1) / FLK
 */
void timer3_init(uint16_t psc, uint16_t arr)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = arr - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = psc - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}
void pwm15_init()
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	uint16_t TimerPeriod = 0;
	uint16_t Channel1Pulse = 0;
//	uint16_t Channel2Pulse = 0;
//	uint16_t Channel3Pulse = 0;
//	uint16_t Channel4Pulse = 0;

	/* GPIOA Clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_0);
	/* TIM15 Configuration ---------------------------------------------------
	Generate 7 PWM signals with 4 different duty cycles:
	TIM1 input clock (TIM1CLK) is set to APB2 clock (PCLK2)
	 => TIM1CLK = PCLK2 = SystemCoreClock
	TIM1CLK = SystemCoreClock, Prescaler = 0, TIM1 counter clock = SystemCoreClock
	SystemCoreClock is set to 48 MHz for STM32F0xx devices

	The objective is to generate 7 PWM signal at 17.57 KHz:
	  - TIM1_Period = (SystemCoreClock / 17570) - 1
	The channel 1 and channel 1N duty cycle is set to 50%
	The channel 2 and channel 2N duty cycle is set to 37.5%
	The channel 3 and channel 3N duty cycle is set to 25%
	The channel 4 duty cycle is set to 12.5%
	The Timer pulse is calculated as follows:
	  - ChannelxPulse = DutyCycle * (TIM1_Period - 1) / 100

	Note:
	 SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f0xx.c file.
	 Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
	 function to update SystemCoreClock variable value. Otherwise, any configuration
	 based on this variable will be incorrect.
	----------------------------------------------------------------------- */
	/* Compute the value to be set in ARR regiter to generate signal frequency at 17.57 Khz */
	TimerPeriod = (SystemCoreClock / 17570 ) - 1;
	/* Compute CCR1 value to generate a duty cycle at 50% for channel 1 and 1N */
	Channel1Pulse = (uint16_t) (((uint32_t) 5 * (TimerPeriod - 1)) / 10);
	/* Compute CCR2 value to generate a duty cycle at 37.5%  for channel 2 and 2N */
//	Channel2Pulse = (uint16_t) (((uint32_t) 375 * (TimerPeriod - 1)) / 1000);
	/* Compute CCR3 value to generate a duty cycle at 25%  for channel 3 and 3N */
//	Channel3Pulse = (uint16_t) (((uint32_t) 25 * (TimerPeriod - 1)) / 100);
	/* Compute CCR4 value to generate a duty cycle at 12.5%  for channel 4 */
//	Channel4Pulse = (uint16_t) (((uint32_t) 125 * (TimerPeriod- 1)) / 1000);

	/* TIM15 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM15, ENABLE);

	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM15, &TIM_TimeBaseStructure);

	/* Channel 1, 2,3 and 4 Configuration in PWM mode */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
//	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
//	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

	TIM_OCInitStructure.TIM_Pulse = Channel1Pulse;
	TIM_OC1Init(TIM15, &TIM_OCInitStructure);

//	TIM_OCInitStructure.TIM_Pulse = Channel2Pulse;
//	TIM_OC2Init(TIM15, &TIM_OCInitStructure);

//	TIM_OCInitStructure.TIM_Pulse = Channel3Pulse;
//	TIM_OC3Init(TIM15, &TIM_OCInitStructure);

//	TIM_OCInitStructure.TIM_Pulse = Channel4Pulse;
//	TIM_OC4Init(TIM15, &TIM_OCInitStructure);

	/* TIM15 counter enable */
	TIM_Cmd(TIM15, ENABLE);

	/* TIM15 Main Output Enable */
	TIM_CtrlPWMOutputs(TIM15, ENABLE);
}
void pwm1_init(uint8_t duty)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	uint16_t TimerPeriod = 0;
	uint16_t Channel3Pulse = 0;

	/* GPIOA Clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_2);
	/* TIM1 Configuration ---------------------------------------------------
	Generate 7 PWM signals with 4 different duty cycles:
	TIM1 input clock (TIM1CLK) is set to APB2 clock (PCLK2)
	 => TIM1CLK = PCLK2 = SystemCoreClock
	TIM1CLK = SystemCoreClock, Prescaler = 0, TIM1 counter clock = SystemCoreClock
	SystemCoreClock is set to 48 MHz for STM32F0xx devices

	The objective is to generate 7 PWM signal at 17.57 KHz:
	  - TIM1_Period = (SystemCoreClock / 17570) - 1
	The channel 1 and channel 1N duty cycle is set to 50%
	The channel 2 and channel 2N duty cycle is set to 37.5%
	The channel 3 and channel 3N duty cycle is set to 25%
	The channel 4 duty cycle is set to 12.5%
	The Timer pulse is calculated as follows:
	  - ChannelxPulse = DutyCycle * (TIM1_Period - 1) / 100

	Note:
	 SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f0xx.c file.
	 Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
	 function to update SystemCoreClock variable value. Otherwise, any configuration
	 based on this variable will be incorrect.
	----------------------------------------------------------------------- */
	/* Compute the value to be set in ARR regiter to generate signal frequency at 17.57 Khz */
	TimerPeriod = (SystemCoreClock / 17570 ) - 1;
	/* Compute CCR1 value to generate a duty cycle at 50% for channel 1 and 1N */
//	Channel1Pulse = (uint16_t) (((uint32_t) 5 * (TimerPeriod - 1)) / 10);
	/* Compute CCR2 value to generate a duty cycle at 37.5%  for channel 2 and 2N */
//	Channel2Pulse = (uint16_t) (((uint32_t) 375 * (TimerPeriod - 1)) / 1000);
	/* Compute CCR3 value to generate a duty cycle at 25%  for channel 3 and 3N */
	Channel3Pulse = (uint16_t) (((uint32_t) duty * (TimerPeriod - 1)) / 100);
	/* Compute CCR4 value to generate a duty cycle at 12.5%  for channel 4 */
//	Channel4Pulse = (uint16_t) (((uint32_t) 125 * (TimerPeriod- 1)) / 1000);

	/* TIM1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);

	TIM_DeInit(TIM1);

	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	/* Channel 1, 2,3 and 4 Configuration in PWM mode */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
//	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
//	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

//	TIM_OCInitStructure.TIM_Pulse = Channel1Pulse;
//	TIM_OC1Init(TIM1, &TIM_OCInitStructure);

//	TIM_OCInitStructure.TIM_Pulse = Channel2Pulse;
//	TIM_OC2Init(TIM1, &TIM_OCInitStructure);

	TIM_OCInitStructure.TIM_Pulse = Channel3Pulse;
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);

//	TIM_OCInitStructure.TIM_Pulse = Channel4Pulse;
//	TIM_OC4Init(TIM1, &TIM_OCInitStructure);

	/* TIM1 counter enable */
	TIM_Cmd(TIM1, ENABLE);

	/* TIM1 Main Output Enable */
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
//	SystemCoreClockUpdate();
}
void pwm1_update(uint8_t duty)
{
	uint16_t TimerPeriod = 0;
	uint16_t Channel3Pulse = 0;

	TimerPeriod = (SystemCoreClock / 17570 ) - 1;
	Channel3Pulse = (uint16_t) (((uint32_t) duty * (TimerPeriod - 1)) / 100);
	TIM1->CCR3 = Channel3Pulse;
}
