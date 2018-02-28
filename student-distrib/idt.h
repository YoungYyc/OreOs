#ifndef ISR_H
#define ISR_H

#include "scheduler.h"

void divide_c_handler();	// divide by zero handler
void page_fault_handler();	// page fault handler
void rtc_handler();	// rtc handler
void other_handler();	// any other handler

void initialize_idt();	// set up idt table
void initialize_pit();	// set up idt table

registers_t * get_iret_struc();


#endif 
