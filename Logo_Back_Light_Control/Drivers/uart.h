#ifndef __UART_H_
#define __UART_H_

#include "stm32f0xx.h"
#include <stdio.h>

void uart_init(void);
void uart1_putchar(uint8_t ch);
void uart1_txstring(char *string);

#endif
