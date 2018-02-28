#ifndef TASK_H
#define TASK_H

#include"types.h"

// #define OFFSET_8MB 0x00800000	//offset of 8 MB in hex
// #define OFFSET_12MB 0x01000000	//offset of 12 MB in hex
#define PROGRAM_OFFSET 0x48 //offset of program image
#define VIRTUAL_ADDRESS 0x08000000	//virtual memory address of program   

#define USER_PD 32

void new_page(int entry_index);

#endif




