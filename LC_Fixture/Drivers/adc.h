/*
 * adc.h
 *
 *  Created on: 2016��5��31��
 *      Author: wangjiuling
 */

#ifndef DRIVERS_ADC_H_
#define DRIVERS_ADC_H_

#include "stm32f10x.h"

void adc_init(void);
void adc_start(void);
void adc_stop(void);

#endif /* DRIVERS_ADC_H_ */
