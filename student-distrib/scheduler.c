#include "scheduler.h"
#include "paging_structure.h"
#include "init_paging.h"
#include "lib.h"
#include "paging.h"
#include "system_calls.h"
#include "x86_desc.h"
#include "system_calls.h"
#include "terminal.h"


#define VIDMEM 0xB8
#define MAGIC_BASE_CONSTANT 10

/*  initialize_scheduler: intialize scheduler
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: set up global variables and linked list used to track processes
 */
void initialize_scheduler(){
	screen_process = 0;
	total_base = 1; 

    sloppy_1 = MAGIC_BASE_CONSTANT;
    sloppy_2 = MAGIC_BASE_CONSTANT;

    // Set up linked list
	pqueue[0].next = pqueue;
	current_proc = pqueue;

    // Paging structure for first process
	current_proc->cr3 = page_directory[0];

    pqueue[0].terminal_id = -1;
    pqueue[1].terminal_id = -1;
    pqueue[2].terminal_id = -1;
    current_proc->terminal_id = 0;

    // printf("id : %d \n",pqueue[0].terminal_id);
}


/*  add_base: add another initial shell to the process
 *  Input: none 
 *  Output: none
 *  Return value: none
 *  Effect: set up new return point to after this function, execute shell and never return properly
 */
void add_base(int terminal_id){
    uint32_t tmp;
    // Reach max
	if (total_base == MAX_BASE) return;

    if (total_base == 1) sloppy_1 = current_proc -> terminal_id;
    if (total_base == 2) sloppy_2 = current_proc -> terminal_id;
    // printf("%d\n", sloppy_1);
    // printf("%d\n", sloppy_2);


    // Re-link the list
	pqueue[total_base - 1].next = &(pqueue[total_base]);
	pqueue[total_base].next = pqueue;

    // printf("DEBUG 1\n");

    tmp = first_page_table[screen_process][VIDMEM + 1]; 

    terminals[screen_process].terminal_cursor_x = screen_x;
    terminals[screen_process].terminal_cursor_y = screen_y;

    memcpy((void *)(VIDEO + OFFSET_4KB), (void *)VIDEO,NUM_COLS*NUM_ROWS*2);
    first_page_table[screen_process][VIDMEM + 1] = first_page_table[screen_process][VIDMEM];
    first_page_table[screen_process][VIDMEM] = tmp;
    vidmap_page_table[screen_process][0] = ((VIDMEM + screen_process + 1) * OFFSET_4KB) | READ_AND_WRITE | PRESENT | USER;

    // Set up paging for the new process
	init_paging(terminal_id);
	pqueue[total_base].cr3 = page_directory[terminal_id];

    // printf("DEBUG 2\n");

    screen_process = terminal_id;
    // clear();
    memcpy((void *)VIDEO, (void *)(VIDEO + OFFSET_4KB),NUM_COLS*NUM_ROWS*2);

    screen_x = terminals[screen_process].terminal_cursor_x;
    screen_y = terminals[screen_process].terminal_cursor_y;

    update_cursor(screen_x,screen_y);

    printf("welcome to t%d\n", screen_process + 1);


    // Modify return value when the next process is called by saving the return address and ebp
	asm volatile ("                             	\n\
            movl   4(%%ebp), %0                    	\n\
            "
            : "=r" (current_proc->regs.ret_addr_iret)
            :
            : "memory"
    );  
    asm volatile ("                                 \n\
            pushfl                       \n\
            popl %%eax         \n\
            movl %%eax, %0  \n\
            "
            : "=r" (current_proc->regs.eflags_iret)
            :
            : "memory", "eax"
    );  		
    current_proc->regs.eflags_iret &= 0xFDFF;
    asm volatile ("                             	\n\
            movl   (%%ebp), %0                    	\n\
            "
            : "=r" (current_proc->regs.ebp)
            :
            : "memory"
    );

    // Go to next process  	
	current_proc = &(pqueue[total_base]);

    current_proc->terminal_id = terminal_id;
    
    // Logistics
	total_base ++; 

    // Execute the base shell
	execute((uint8_t *)"shell");
}

/*  switch_task: go to the next task in the queue every interrupt
 *  Input: regs -- registers structure (IRET and pusha) when interrupt is called 
 *  Output: none
 *  Return value: none
 *  Effect: jump to the next task where it left it BEFORE the interrupt, never return properly
 */
void switch_task(registers_t * regs)
{

   	// copy the saved registers into the current_proc structure (osdev)
   	memcpy(&current_proc->regs, regs, sizeof(registers_t));
 
   	// now go onto the next task - if there isn't one, go back to the start of the queue. (osdev)
   	current_proc = current_proc->next;
    // printf("%d\n", current_proc->pid);

   	// perform context switch
   	loadPageDirectory(current_proc->cr3);

    tss.ss0 = KERNEL_DS;
    tss.esp0 = KERNEL_OFFSET - (OFFSET_4KB * 2 * (current_proc->pid + 1));		//setup stack pointer 

    // Jump to the next process (32 is the offset of return address in the IRET structure from esp)
    asm volatile ("                             \n\
    	movl %%eax, %%esp 						\n\
    	movl %%ebx, %%ebp 						\n\
    	movl %%ecx, 32(%%esp) 					\n\
        movl %%edx, 40(%%esp)                   \n\
    	popl %%edi								\n\
        popl %%edi								\n\
		popl %%esi								\n\
		popl %%edx								\n\
		popl %%ecx 								\n\
		popl %%ebx								\n\
		popl %%eax 								\n\
		popfl 									\n\
		iret 									\n\
        "
        : 
        : "a" (current_proc->regs.esp), "b" (current_proc->regs.ebp), "c" (current_proc->regs.ret_addr_iret), "d" (current_proc->regs.eflags_iret)
        : "memory"
    );  	
 };

