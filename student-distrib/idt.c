#include "idt.h"
#include "lib.h"
#include "types.h"
#include "i8259.h"
#include "x86_desc.h"
#include "wrapper.h"
#include "system_calls.h"
#include "scheduler.h"
#include "rtc.h"
#include "e1000.h"
#include "mouse.h"

// Vector to IDT
#define KEYBOARD_INTERRUPT_NUM  0x21
#define RTC_INTERRUPRT_NUM      0x28
#define PAGE_FAULT_NUM          0x0E
#define DIVIDE_ZERO_ERROR_NUM   0x00
#define SYSTEM_CALL_NUM         0x80
#define PIT_INTERRUPRT_NUM      0x20
#define NIC_INTERRUPRT_NUM      0x2B
#define MOUSE_INTERRUPT_NUM     0x2C

// RTC sepecific constants
#define SELECT_C 0x0C           //0x0C selects register C of rtc
#define RTC_OUTPORT 0x70        //default port for rtc output
#define RTC_INPORT 0x71         //default port for rtc input
#define RTC_IRQ 8

// Other constants
#define EXCEPTION_RETURN_DECOY  10
#define OFFSET_TO_IRET          40

#define KEYBOARD_PORT 0x60

// Private function
static void attach_handler(int idx, int sys_call, int trap, void (* func_ptr));
// uint8_t cycle;
// uint8_t mouse_bytes[3];
// uint32_t x;
// uint32_t y;

/*  Private function to get the iret structure (and the saved registers from the stack) 
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: access the stack and obtain registers
 */
registers_t * get_iret_struc(){
    int tmp; //local variable for inline 

    // Get the ebp 
    asm volatile ("                             \n\
            movl   %%ebp, %0                    \n\
            "
            : "=r" (tmp)
            :
            : "memory"
    );  

    // Get the base of the iret structure 
    tmp += OFFSET_TO_IRET;

    return (registers_t *) tmp;
}

/*  Handler for divide by zero exception
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: print to screen, jump to halt syscal with magic return number (converted to 256 in halt)
 */
void divide_c_handler() {
    printf("divide_by_zero\n");
    halt(EXCEPTION_RETURN_DECOY);
}

/*  Handler for page fault exception
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: print to screen, jump to halt syscal with magic return number (converted to 256 in halt)
 */
void page_fault_handler(){
    printf("we_got_page_fault\n");
    // while(1);
    halt(EXCEPTION_RETURN_DECOY);
}

/*  Handler for rtc interrupts
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: jump to task switching in scheduler.c (currently used to switch task, probably change to PIT)
 */
void rtc_handler(){
    // Unmask interrupts on PIC
    send_eoi(RTC_IRQ);

    // Set up flag for read_rtc
    interuptted = 1; 
    //test_interrupts();

    // Flush register C of RTC (source: osdev)
    outb(SELECT_C, RTC_OUTPORT);       //0x0C selects register C, writing to 
    inb(RTC_INPORT); 
}

/*  pit_handler: handle PIT interrupt and switch task
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: access the stack and obtain registers
 */
void pit_handler(){
    registers_t * tmp =  get_iret_struc(); //tmp structure to hold all saved registers
    // Unmask interrupts on PIC
    send_eoi(0);

    // Switch task
    switch_task(tmp);
}

/*  nic_handler: handle network interrupt
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: handle packages 
 */
void nic_handler(){
    // Unmask interrupts on PIC
    send_eoi(11);

    fire();
    // Switch task
}


/*  Handler for all other exceptions
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: print to screen, jump to halt syscal with magic return number (converted to 256 in halt)
 */
void other_handler(){
    printf("we_got_others\n");
    halt(EXCEPTION_RETURN_DECOY);
}



/*  Private function to set up the handlers by writing to appropriate field of the struct set up in x86_desc.h
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: set up handlers
 */
static void attach_handler(int idx, int sys_call, int trap, void (* func_ptr)) {
    SET_IDT_ENTRY(idt[idx], func_ptr);
    idt[idx].seg_selector = KERNEL_CS; 
    idt[idx].reserved4 = 0; 
    idt[idx].reserved3 = trap;
    idt[idx].reserved2 = 1;
    idt[idx].reserved1 = 1;
    idt[idx].size      = 1;
    idt[idx].reserved0 = 0;
    idt[idx].dpl       = sys_call;  // 0 or 3 based on privilege
    idt[idx].present   = 1;
}

/*  Public function to initialize the idt
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: set up idt
 */
void initialize_idt(){
    int i;  //counter for loop

    // Attach known handlers
    attach_handler(DIVIDE_ZERO_ERROR_NUM, 0, 0, &divide_c_handler);
    attach_handler(PAGE_FAULT_NUM, 0, 0, &page_fault_handler);
    attach_handler(KEYBOARD_INTERRUPT_NUM, 0, 0, &keyboard_handler_wrapper);
    attach_handler(RTC_INTERRUPRT_NUM, 0, 0, &rtc_handler_wrapper);
    attach_handler(PIT_INTERRUPRT_NUM, 0, 0, &pit_handler_wrapper);
    attach_handler(NIC_INTERRUPRT_NUM, 0, 0, &nic_handler);
    attach_handler(SYSTEM_CALL_NUM, 3, 0, &system_call_handler_wrapper); //set privilege to 3 so that users can call syscalls

    attach_handler(MOUSE_INTERRUPT_NUM,0,0,&mouse_handler_wrapper);


    // Attach all other handlers
    for (i = 0; i < NUM_VEC; ++i){
        if ((idt[i].offset_15_00 == NULL) && // Check if the entry has already had a handler attached
            (idt[i].offset_31_16 == NULL)){
            attach_handler(i, 0, 0 , &other_handler);
        }
    }
}

/*  Private function to initialize the PIT to slowest frequency 
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: initialize PIT for interrupt on IRQ0
 */
void initialize_pit(int freq){
    asm volatile ("                             \n\
            movb $36, %%al                       \n\
            outb %%al, $0x43                        \n\
            xorl %%eax, %%eax                       \n\
            movl $11931, %%eax                           \n\
            outb %%al, $0x40                         \n\
            movb %%ah, %%al                         \n\
            outb %%al, $0x40                         \n\
            "
            : 
            : 
            : "memory", "eax"
    );  
}

