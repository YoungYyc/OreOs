#include "rtc.h"
#include "lib.h"
#include "i8259.h"

#define CLEAR_NMI(reg) (0x80 | reg)
#define RTC_OUTPORT 0x70        //default port for rtc output
#define RTC_INPORT 0x71         //default port for rtc input
#define SELECT_A 0x0A           //0x0A selects register B of rtc
#define SELECT_B 0x0B           //0x0B selects register B of rtc
#define SELECT_C 0x0C           //0x0C selects register C of rtc
#define UNMASK_NMI 0x7F         //0x70 set the bit that unmask nmi, 0x0F preserves previous bits

#define INITIAL_RATE 15         //initial rate of 2Hz converted to rate as per OSdev conversion
#define MAX_FREQ 1024           //max frequency as specified in spec 
#define MIN_FREQ  2             //min frequency as specified in OSdev
#define MAX_RATE 15             //max rate as speciied in OSdev

#define RTC_IRQ 8

//Private function, look at header for more details
static int32_t change_rate_with_check(int32_t freq);

//Initialize rtc's interval irq 8 
//input: none
//output: none
//return value: none
//side effect: read and write from rtc's ports, masking nmi in the process, turn on rtc interrupt  
void initialize_rtc(){
    //This piece of code is adapted from http://wiki.osdev.org/RTC
    int prev; 

    outb(CLEAR_NMI(SELECT_B), RTC_OUTPORT);       	// select register B, and disable NMI
    prev = inb(RTC_INPORT);    						// read the current value of register B
    outb(CLEAR_NMI(SELECT_B), RTC_OUTPORT);       	// another read will reset the index to register D
 
    outb(prev | 0x40, RTC_INPORT); 					// OR with 0x40 turns on bit 6 of register B

    // This code is recommended for Bosch; but why not
    outb(SELECT_C, RTC_OUTPORT);   					// select register C
    inb(RTC_INPORT); 								// discard content

    // This piece of code is adapted from http://wiki.osdev.org/NMI
    outb(inb(RTC_OUTPORT) & UNMASK_NMI, RTC_OUTPORT); 	//unmask nmi
}


//Change rtc's interval irq 8 
//input: rate into any power of 2 from 1 to 1024
//output: none
//return value: none
//side effect: read and write from rtc's ports, masking nmi in the process, change rtc rate  
void change_rate_rtc(int rate) {
    //This piece of code is adapted from http://wiki.osdev.org/RTC
    char prev;

    rate &= 0x0F;                                   // avoid bad input using bitmask
    disable_irq(RTC_IRQ);                                 // disable interrupt

    outb(CLEAR_NMI(SELECT_A), RTC_OUTPORT);         // set index to register A, disable NMI

    prev=inb(RTC_INPORT);                           // get initial value of register A
    outb(CLEAR_NMI(SELECT_A), RTC_OUTPORT);         // reset index to A
    outb((prev & RTC_OUTPORT) | rate, RTC_INPORT);  //write only our rate to A. Note, rate is the bottom 4 bits.

    // This piece of code is adapted from http://wiki.osdev.org/NMI
    outb(inb(RTC_OUTPORT) & UNMASK_NMI, RTC_OUTPORT);   //unmask nmi
    enable_irq(RTC_IRQ);  // rtc is irq 8
}

//This function waits till the next rtc interrupt and then return, acting as a timer program
//input: 
//  fd -- file descriptor (ignored)
//  buf -- buffer (ignored)
//  nbytes -- (ignored)
//output: none
//return value: 0 upon success
//side effect: may force the process to wait 
int32_t read_rtc (int32_t fd, void* buf, int32_t nbytes){
    // Set the flag 
    interuptted = 0; 

    // Wait for handler to change the flag and return
    while(interuptted == 0);
    return 0;
}

//This function changes the interrupt rate of the rtc
//input: 
//  fd -- file descriptor (ignored)
//  buf -- buffer storing the new frequency
//  nbytes -- must be 4, otherwise return error
//output: none
//return value: 0 on successfully change the rate, -1 otherwise
//side effect: change the rate of the rtc interrupt
int32_t write_rtc (int32_t fd, const void* buf, int32_t nbytes){
    // We are only accepting signed integers (32 bits) as input
    if (nbytes != 4 || buf == NULL) return -1;

    // Typecasting
    const int32_t * tmp = buf;

    // Change rate
    return change_rate_with_check(tmp[0]);
}

//This function initializes the rtc and set it to 2 Hz
//input: 
//  filename -- ignored
//output: none
//return value: 0 on success
//side effect: initialize rtc and set the rate to 2hz
int32_t open_rtc (const uint8_t* filename){
    // Init
    initialize_rtc();

    // Change rate
    change_rate_rtc(INITIAL_RATE);
    return 0;
}

//This function closes the rtc
//input: 
//  fd -- ignored
//output: none
//return value: 0 on success
//side effect: none
int32_t close_rtc (int32_t fd){
    return 0;
}

//This function wraps the change rate with checking for invalid input
//input: 
//  freq -- the new frequency to be set
//output: none
//return value: 0 on success, - 1 on failure
//side effect: change the interrupt rate if it is valid
static int32_t change_rate_with_check(int32_t freq){
    int32_t tmp = freq; 
    int32_t power = -1;//offset to 1 since 0-th power exists and is 1

    // If outside of the acceptable range (specified in spec) then fail
    if (freq < MIN_FREQ || freq > MAX_FREQ) return -1;

    // Find the power of 2 currently used
    while(tmp > 0) {
        if (tmp % 2 == 1 && tmp != 1) return -1;
        tmp = tmp/2;
        power += 1; 
    }

    change_rate_rtc(MAX_RATE - power + 1); //frequency calculation (refer to OSdev)
    return 0;
}

