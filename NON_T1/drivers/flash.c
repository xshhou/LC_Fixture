/*
 * flash.c
 *
 *  Created on: 2016Äê5ÔÂ15ÈÕ
 *      Author: WangJiuLing
 */

#include "flash.h"
#include <string.h>
#include <stdlib.h>

void flash_read(u32 addr, u8* p, u16 len)
{
	while(len--){
		*(p++)=*((u8*)addr++);
	}
}
int flash_write(u32 addr,u8 *p,u16 len)
{
	u16 i;
	FLASH_Status sta;
	u8* buf = NULL;

	if(FLASH_GetStatus() == FLASH_COMPLETE){
		FLASH_Unlock();
		FLASH_ErasePage(addr);
		for(i = 0;i < len;i += 4){
			FLASH_ProgramWord(addr + i, *((u32*)(p + i)));
		}
		do{
			sta = FLASH_WaitForLastOperation(3);
		}while(sta != FLASH_COMPLETE && FLASH_TIMEOUT != sta);
		FLASH_Lock();
		buf = (u8*)malloc(len);
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
