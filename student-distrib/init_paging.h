
#ifndef _INIT_PAGING_H
#define _INIT_PAGING_H

#include "types.h"

#define READ_AND_WRITE 0x2 	//bit 1 is R/W
#define PRESENT 0x1 		//bit 0 is present
#define USER 0x4 			//bit 2 is user/supervisor 
#define GLOBAL 0x100 		//bit 8 is global 
#define PAGE_4MB 0x80 		//bit 7 is enable 4 MB
#define OFFSET_4MB 0x00400000	//offset of 4 MB in hex
#define OFFSET_8MB 0x00800000	//offset of 8 MB in hex
#define OFFSET_4KB 0x00001000	//offset of 4 KB in hex 
#define PD_SIZE 1024		//size of paging directory
#define PT_SIZE 1024		//size of paging table 
#define VID_MEM 0xB8   		//page table of video memory: 0xB8000 >> 12

#define NW_PD 31

void init_paging(int idx);
void init_paging_without_load(int idx);

#endif

