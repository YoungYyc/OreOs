//paging.h
//define paging diectory and paging table


#ifndef _PAGING_H
#define _PAGING_H

#define PE_ENABLE 0x1 		//bit 0 of cr0 is PE 
#define PG_ENABLE 0x80000000	//bit 31 of cr0 is PG 
#define PSE_ENABLE 0x10		//bit 4 of cr4 is PSE 
#define PGE_ENABLE 0x80 	//bit 7 of cr4 is PGE 

#ifndef ASM

extern void loadPageDirectory(unsigned int*);
extern void enablePaging();
extern void globalPaging();


#endif
#endif

