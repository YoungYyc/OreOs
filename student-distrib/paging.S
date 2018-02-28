# x86_paging.S - set up x86 paging

#define ASM     1
#include "paging.h"

.text
.globl loadPageDirectory, enablePaging, globalPaging


#loadPageDirectory
#this function loads the address of page directory nto CR3 
#input: address of page directory
#output: none
#return value: none
#side effects: cr3 is loaded with address of page directory
loadPageDirectory:
	pushl %ebp
	movl %esp, %ebp
	movl 8(%esp), %eax		
	movl %eax, %cr3 	#load address of page directory to cr3 
	movl %ebp, %esp
	popl %ebp
	ret

#enablePaging
#this function enables the kernel to use paging to map memory address 
#by enabling Protected Mode Enable(PE) and Paging(PG)
#input: none
#output: none
#side effects: paging is enabled to map memory address 
enablePaging:
	pushl %ebp
	movl %esp, %ebp
	movl %cr0, %eax
	orl $PE_ENABLE, %eax	#enable PE
	orl $PG_ENABLE, %eax 	#enable PG
	movl %eax, %cr0
	movl %ebp, %esp
	popl %ebp
	ret

#globalPaging
#function enables page size extension(PSE) and paging global enabled(PGE)
#PSE allows page size of 4 MB exist along with 4 KB 
#PGE allows address translation shared between address memories
#input: none 
#output: none 
#return value: none
#side effect: paging directory entry can point to 4 MB page
#paging entries are global that can be share between address memories 
globalPaging:
	pushl %ebp
	movl %esp, %ebp
	movl %cr4, %eax
	orl $PSE_ENABLE, %eax
	orl $PGE_ENABLE, %eax
	movl %eax, %cr4
	movl %ebp, %esp
	popl %ebp
	ret



