#include "task.h"
#include "paging.h"
#include "init_paging.h"
#include "paging_structure.h"
#include "lib.h"
#include "scheduler.h"

//function: new_page
//input: entry index
//output: none
//return value: none
//side effects: change page and flush tlb
void new_page(int entry_index){
	// create new page for new process based on entry index (process index)
	// 32 * 4 = 128 (remember, 4 MB enabled, so pde directly points to 4 MB page in phys. memory)
	// make this entry of page dir. point to appropriate physical address (leftover bits used for other purposes)

	page_directory[current_proc->terminal_id][USER_PD] =  OFFSET_4MB*(entry_index + 2) | PAGE_4MB | PRESENT | READ_AND_WRITE | USER;

    //first tlb
	asm volatile ("                             \n\
            movl	%%cr3, %%eax             	\n\
            movl	%%eax, %%cr3				\n\
            "
            : 
            :
            : "memory", "eax"
    );	


    // TLB flushing: http://wiki.osdev.org/TLB

	// printf("page 32 is %#x\n",  page_directory[32]);
}

