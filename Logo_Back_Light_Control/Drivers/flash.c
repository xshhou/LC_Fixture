/*
 * flash.c
 *
 *  Created on: 2016Äê7ÔÂ27ÈÕ
 *      Author: wangjiuling
 */

#include "flash.h"
#include <string.h>
#include <stdlib.h>

void flash_read(uint32_t addr, uint8_t* p, uint16_t len)
{
	while(len--){
		*(p++) = *((uint8_t*)addr++);
	}
}
int flash_write(uint32_t addr, uint8_t* p, uint16_t len)
{
	uint16_t i;
	FLASH_Status sta;
	uint8_t* buf = NULL;

	if(FLASH_GetStatus() == FLASH_COMPLETE){
		FLASH_Unlock();
		FLASH_ErasePage(addr);
		for(i = 0;i < len;i += 4){
			FLASH_ProgramWord(addr + i, *((uint32_t*)(p + i)));
		}
		do{
			sta = FLASH_WaitForLastOperation(3);
		}while(sta != FLASH_COMPLETE && FLASH_TIMEOUT != sta);
		FLASH_Lock();
		buf = (uint8_t*)malloc(len);
		if(buf == NULL){
			return -2;
		}
		flash_read(addr, buf, len);
		if(strncmp((char*)buf, (char*)p, len) == 0){
			return 0;
		}else{
			return -3;
		}
	}else{
		return -1;
	}

}
