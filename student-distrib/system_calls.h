#ifndef SYSTEM_CALLS_H
#define SYSTEM_CALLS_H

#include "types.h"

#define KERNEL_OFFSET 			0x800000
#define PROGRAM_IMAGE_OFFSET 	0x08048000
#define KEYBOARD_BUF_SIZE 		128
#define OPEN_FILE_NUM 			8

// For userplebs programs
int32_t sys_halt (uint8_t status);
int32_t sys_execute (const uint8_t* command);
int32_t sys_read (int32_t fd, void* buf, int32_t nbytes);
int32_t sys_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t sys_open (const uint8_t* filename);
int32_t sys_close (int32_t fd);
int32_t sys_getargs (uint8_t* buf, int32_t nbytes);	
int32_t sys_vidmap (uint8_t** sreen_start);
int32_t sys_set_handler (int32_t signum, void* handler_address);
int32_t sys_sigreturn (void);


// For masterkernel programs
int32_t halt (uint8_t status);
int32_t execute (const uint8_t* command);
int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
int32_t open (const uint8_t* filename);
int32_t close (int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap (uint8_t** sreen_start);
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);

int32_t vidmap_prev (uint8_t** sreen_start);

int32_t decoy_function();

// https://stackoverflow.com/questions/7670766/c-how-can-i-use-a-single-function-pointer-array-for-functions-with-variable-par
typedef int32_t (*generic_fp)(void);	// generic fn ptr
typedef int32_t (*read_fp)(int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_fp) (int32_t fd, const void* buf, int32_t nbytes);
typedef int32_t (*open_fp) (const uint8_t* filename);
typedef int32_t (*close_fp) (int32_t fd);

typedef struct file_array 		//struct for directory entries
{
	generic_fp * file_jumptable;
	uint32_t inode;
	uint32_t file_position;
	uint32_t flags;
} file_array_t;

typedef struct pcb 		//struct for directory entries
{
	file_array_t file_array_array[OPEN_FILE_NUM];		// 24 B reserved 
	uint8_t args[KEYBOARD_BUF_SIZE];	// Idk what it actually should be, but considering terminal buffer capped at 128, makes sense.
	uint32_t argumentSizeBytes;
	uint32_t my_process_id;
	uint32_t parent_process_id; 
	uint32_t parent_esp;
	uint32_t parent_ebp;
	uint32_t available;


	int isBaseShellInATerminal;
} pcb_t;

typedef int (*operation_fp)(int argc, int* argv);

pcb_t * get_pcb();

#endif

