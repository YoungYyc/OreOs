#include "paging.h"
#include "init_paging.h"
#include "paging_structure.h"


/*  Init_paging without load
 *  this function initialize paging directory and first page table
 *  and enables paging to be used 
 *  Input: address of page directory and page table 
 *  Output: none
 *  Return value: none
 *  Side effects: paging directory and first page table is initialized but not loaded to cr3
 */
void init_paging_without_load(int idx){
    int i; //counter 
    //map kernel to physical memory
    for(i = 0; i < PD_SIZE; i++)
        page_directory[idx][i] = 0x0 | READ_AND_WRITE;
    
    //map second entry to 4 MB page which is the kernel 
    page_directory[idx][1] = OFFSET_4MB | PRESENT | READ_AND_WRITE | GLOBAL | PAGE_4MB;
    page_directory[idx][NW_PD] = (NW_PD * OFFSET_4MB) | PRESENT | READ_AND_WRITE | GLOBAL | PAGE_4MB;


    for(i = 0; i < PT_SIZE; i++)
        first_page_table[idx][i] = (i * OFFSET_4KB) | READ_AND_WRITE; 

    //map video memory to physical memory
    first_page_table[idx][VID_MEM] = (VID_MEM * OFFSET_4KB) | PRESENT;
    first_page_table[idx][VID_MEM + 1] = ((VID_MEM + idx + 1) * OFFSET_4KB) | PRESENT;

    //map virtual address of video memory to physcial memory in page table  
    page_directory[idx][0] = ((unsigned int)first_page_table[idx]) | PRESENT | READ_AND_WRITE;
}

/*  Init_paging wrapper
 *  wrapper function that load cr3 with the initialized PD and PT
 *  Input: address of page directory and page table
 *  Output: none
 *  Return value: none
 *  Side effects: paging directory and first page table is initialized and loaded to cr3 with all settings
 */
void init_paging(int idx){
    // Initialize PD and PT 
    init_paging_without_load(idx);

    // Load to cr3
    loadPageDirectory(page_directory[idx]);

    // Enable global paging
    globalPaging();

    // Turn on paging
    enablePaging();
}
