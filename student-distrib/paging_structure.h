#ifndef _PAGING_STRUCT_H
#define _PAGING_STRUCT_H

// First process paging structure
uint32_t page_directory0[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4GB of space, align to 4KB as per spec
uint32_t first_page_table0[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4MB of spaec, align to 4FKB as per spec 
uint32_t vidmap_page_table0[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4MB of spaec, align to 4FKB as per spec

// Second process paging structure
uint32_t page_directory1[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4GB of space, align to 4KB as per spec
uint32_t first_page_table1[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4MB of spaec, align to 4FKB as per spec 
uint32_t vidmap_page_table1[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4MB of spaec, align to 4FKB as per spec 

// Third process paging structure
uint32_t page_directory2[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4GB of space, align to 4KB as per spec
uint32_t first_page_table2[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4MB of spaec, align to 4FKB as per spec 
uint32_t vidmap_page_table2[1024] __attribute__((aligned(4096)));//there are 1024 entries to fill up 4MB of spaec, align to 4FKB as per spec 

// Pack up and make global 
extern uint32_t * page_directory[3];
extern uint32_t * first_page_table[3];
extern uint32_t * vidmap_page_table[3];

#endif
