/*
 * can.h
 *
 *  Created on: 2016Äê6ÔÂ2ÈÕ
 *      Author: wangjiuling
 */

#ifndef DRIVERS_CAN_H_
#define DRIVERS_CAN_H_

#include "stm32f10x.h"

typedef struct
{
	uint8_t dataNum;
	uint32_t extId;  //4byte
	uint8_t Data[8];
} CAN_DATA;

typedef struct
{
	uint8_t direction;
	uint8_t revFlag;
	uint8_t boxID;
	uint8_t hostID;
	uint8_t Instruction;
	uint8_t Index;
	uint8_t SegPolo;
	uint8_t SegNum;
	uint8_t dataNum;
	uint8_t Data[8];
} CAN_LOGIC_DATA;

void can_init(void);

#endif /* DRIVERS_CAN_H_ */
