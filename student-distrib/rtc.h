#ifndef RTC_H
#define RTC_H


#include "types.h"

// Volatile variable for the read function
volatile int interuptted; 

// Look at function header for more details
void initialize_rtc();
void change_rate_rtc(int rate);
int32_t read_rtc (int32_t fd, void* buf, int32_t nbytes);
int32_t write_rtc (int32_t fd, const void* buf, int32_t nbytes); 
int32_t open_rtc (const uint8_t * filename);
int32_t close_rtc (int32_t fd);

#endif 

