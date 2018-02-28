#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

#define MAX_BASE    3

extern uint32_t total_base; 
extern uint32_t screen_process;

extern uint32_t sloppy_1;
extern uint32_t sloppy_2; 


// Structure of the stack when interrupt is called
typedef struct registers
{
    uint32_t esp;
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;
    uint32_t flags;

    uint32_t ret_addr_iret;
    uint32_t cs_iret;
    uint32_t eflags_iret;
    uint32_t esp_iret;
    uint32_t ss_iret; 
} registers_t;

// Per-base information
typedef struct process_struct
{
    uint32_t        pid;
    registers_t     regs;
    uint32_t * 		cr3;
    int terminal_id;
    struct process_struct *next;
} process_t;

// Other linked structure
process_t pqueue[MAX_BASE];
extern process_t *current_proc;



// Functions
void initialize_scheduler();
void add_base(int terminal_id);
void switch_task(registers_t *);


#endif

