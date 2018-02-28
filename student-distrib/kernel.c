/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"
#include "idt.h"
#include "init_paging.h"
#include "rtc.h"
#include "paging_structure.h"
#include "file_system.h"
#include "task.h"
#include "system_calls.h"
#include "scheduler.h"
#include "terminal.h"
#include "e1000.h"
#include "mouse.h"


// #define RUN_TESTS

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))
#define RTC_VARIABLE 1024

// Global variable to regulate acess of all 3 processes
uint32_t * page_directory[3]  = {page_directory0, page_directory1, page_directory2};
uint32_t * first_page_table[3]  = {first_page_table0, first_page_table1, first_page_table2};
uint32_t * vidmap_page_table[3]  = {vidmap_page_table0, vidmap_page_table1, vidmap_page_table2};

// Multi-proc global variables
uint32_t screen_process;
process_t *current_proc;
uint32_t total_base;


uint32_t sloppy_1;
uint32_t sloppy_2; 


//parameter for mouse
int m_pos_x;
int m_pos_y;
uint8_t m_cycle;
int left_but_pre;

 // Check if MAGIC is valid and print the Multiboot information structure
 //   pointed by ADDR. 
void entry(unsigned long magic, unsigned long addr) {
    multiboot_info_t *mbi;
    // int8_t * c = NULL;
    // int32_t d = 1024;
    // uint8_t * buf = 0x0068A001;

    // int * c;
    int i = RTC_VARIABLE; 

    /* Clear the screen. */
    // clear();

    initialize_terminal();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);
    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
            filesys_start = (boot_block_t*) mod->mod_start;
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
     
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char*)(mod->mod_start+i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
                (unsigned)elf_sec->num, (unsigned)elf_sec->size,
                (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
                (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (memory_map_t *)mbi->mmap_addr;
                (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
            printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
                    (unsigned)mmap->size,
                    (unsigned)mmap->base_addr_high,
                    (unsigned)mmap->base_addr_low,
                    (unsigned)mmap->type,
                    (unsigned)mmap->length_high,
                    (unsigned)mmap->length_low);
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }
    
    init_file_system(filesys_start);
    init_paging(0);
    
    /* Init the PIC */
    i8259_init();    

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */
    initialize_idt();
    initialize_rtc();
    initialize_pit();
    initialize_scheduler();
    initialize_terminal();
    // initialize_e1000();
    // initialize_mouse();

    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    printf("Enabling Interrupts\n");
    sti();
    enable_irq(1);
    enable_irq(8);
    enable_irq(0);
    // enable_irq(11);
    // enable_irq(12);

    // Initialize rtc to slow rate (development of multi-proc in progress)
    open_rtc((uint8_t*)"garbage");
    write_rtc (0, &i, 4); 


#ifdef RUN_TESTS
    /* Run tests */

    launch_tests();

#endif
    /* Execute the first program ("shell") ... */
        // printf("%s\n", "awdaw");
    execute((uint8_t*)"shell");
    printf("%s\n", "welcome back");
    
    
    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
