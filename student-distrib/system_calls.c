#include "system_calls.h"
#include "file_system.h"
#include "init_paging.h"
#include "paging_structure.h"
#include "lib.h"
#include "task.h"
#include "x86_desc.h"
#include "terminal.h"
#include "rtc.h"
#include "scheduler.h"

#define PCB_SUPPORT  			6
#define FILE_JUMP_SIZE 			4
#define DIR_JUMP_SIZE 			4
#define RTC_JUMP_SIZE 			4
#define STDIN_JUMP_SIZE 		4
#define STDOUT_JUMP_SIZE 		4
#define CMD_SIZE 				128
#define EXE_CHECK_SIZE 			4
#define REGULAR_FILE 			2
#define DIRECTORY 				1
#define IN_USE 					1
#define NOT_USE 				0
#define STDIN_IDX 				0
#define STDOUT_IDX 				1
#define STACK_BITMASK 			0xFFFFE000
#define SPACE_ASCII 			32
#define CLEAR_SIZE 				5
#define OFFSET_128MB			134217728
#define OFFSET_132MB			138412032
#define FAULT_STATUS			10
#define RETURN_VALUE_MAX		256
#define ENTRY_POINT_OFFSET		24
#define EXE_FIRST				0
#define EXE_SECOND				1
#define EXE_THIRD				2
#define EXE_FOURTH				3
#define COLOR_SIZE				5
#define VIDEO 					0xB8000

#define JUMP_OPEN				0
#define JUMP_CLOSE				1
#define JUMP_READ				2
#define JUMP_WRITE				3
#define ENTRY_POINT_READ_BYTES	4

#define COLOR_MAX_SIZE			7

// Uncomment this to test 
// #define PCB_TEST 				1
// #define FD_TEST					1


// Global variables
uint32_t file_position_offset;
uint32_t max_process = 0;
uint32_t process_cap = 500;
int cap_flag = 0;

// Private functions

static void initialize_pcb(pcb_t *);
static int valid_pointer(const void * pointer);


// Private variables 
static int exe_char[EXE_CHECK_SIZE] = {0x7F, 0x45, 0x4C, 0x46};	// exe char values to be compared with...

// order: open, close, read, write
generic_fp file_jumptable[FILE_JUMP_SIZE] = { (generic_fp)file_open, (generic_fp)file_close, (generic_fp)file_read, (generic_fp)file_write  };
generic_fp dir_jumptable[DIR_JUMP_SIZE] = { (generic_fp)dir_open, (generic_fp)dir_close, (generic_fp)dir_read, (generic_fp)dir_write  };
generic_fp rtc_jumptable[RTC_JUMP_SIZE] = { (generic_fp)open_rtc, (generic_fp)close_rtc, (generic_fp)read_rtc, (generic_fp)write_rtc  };
generic_fp stdin_jumptable[STDIN_JUMP_SIZE] = { (generic_fp)decoy_function, (generic_fp)decoy_function, (generic_fp)terminal_read, (generic_fp)decoy_function  };
generic_fp stdout_jumptable[STDOUT_JUMP_SIZE] = { (generic_fp)decoy_function, (generic_fp)decoy_function, (generic_fp)decoy_function, (generic_fp)terminal_write  };

//decoy function
//input: none
//output: none
//return value: none
//side effects: none
int32_t decoy_function() {
	return 0;
}

//function: halt
//input: status -- return value for execute
//output: none
//return value: none
//side effects: terminate a process, jump to return of execute 
int32_t halt (uint8_t status) {
	pcb_t * cur_pcb = get_pcb(); // get previous pcb
	uint32_t i;
	int flags;

	// printf("Halt syscall\n");
	///////////////
	cap_flag = 0;
	//////////////
	// Mark all files as not being open and make pcb available
	for(i = 0; i < OPEN_FILE_NUM-2; i++) {	// 0 to 5 so -2
		cur_pcb->file_array_array[i+2].flags = 0;	// actually mark files to be not opened; +2 b/c first two always open
	}
	cur_pcb->available = 1;

	// If base shell is halting then open another shell (need to rewrite this for multi-proc)

	// if(cur_pcb->my_process_id == 0) execute((uint8_t*)"shell");
	
	if (cur_pcb->isBaseShellInATerminal)  {	
		terminals[screen_process].execute_flag_shell = 0;

		execute((uint8_t*)"shell");
	};	

	// If the halt is called from an interrupt handler then store i (recycled) as the proper return value
	i = (status == FAULT_STATUS) ? RETURN_VALUE_MAX : status;
	// printf("my parent no namae wa %d\n", cur_pcb->parent_process_id);

	// Critical section starts
	cli_and_save(flags);

#ifdef PCB_TEST
    /* Run tests */

    printf("Closing pcb number %d\n", cur_pcb->my_process_id);

#endif   

	// Paging and logistics 
	new_page(cur_pcb->parent_process_id);
	current_proc->pid = cur_pcb->parent_process_id;

	tss.ss0 = KERNEL_DS;
    tss.esp0 = KERNEL_OFFSET - (OFFSET_4KB * 2 * (cur_pcb->parent_process_id + 1));		//setup stack pointer 

    // Inline to jump back to parents execute (actually returning from execute occurs here as well) and sti and restore flags
	asm volatile(" 						\n\
		movl  	%%edi, %%esp 			\n\
		movl 	%%esi, %%ebp 			\n\
		pushl	%%ecx 					\n\
		popf 							\n\
		xorl	%%eax, %%eax 			\n\
		movl	%%ebx, %%eax 			\n\
		leal	-12(%%ebp), %%esp 		\n\
		popl	%%edi 					\n\
		popl	%%esi 					\n\
		popl 	%%ebx 					\n\
		popl	%%ebp 					\n\
		sti 							\n\
		ret 							\n\
    "
    :
    :	"D" (cur_pcb->parent_esp), "S" (cur_pcb->parent_ebp), "b" (i), "c" (flags)
    :	"memory"
    );	

	// Never reached 
	printf("End of Halt syscall\n");
	return 0;
}

void changeColors() {
	int32_t i;
	char* video_mem = (char *)VIDEO;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1) + 1) = terminals[screen_process].attrib;
    }

    update_cursor(screen_x,screen_y);
}

// 1: blue
// 2: green
// 3: cyan
// 4: red
// 5: magenta
// 6: brown
// 7: white
// E: yellow
void printColorTextErrorMessage() {
	printf("Not valid argument to color text...\n");

	printf("Valid arguments to color text are:\n");
	printf("blue\n");
	printf("brown\n");
	printf("cyan\n");
	printf("green\n");
	printf("magenta\n");
	printf("red\n");
	printf("white\n");
	printf("yellow\n");
}

//function: execute
//input: command
//output: sucess or fail
//return value: 0 if success, -1 if fail
//side effects: attempts to load and exeute a new program, handing off the 
// proessor to the new program until it terminates
int32_t execute (const uint8_t* command) {
	// The whole thing is critical
	int flags;  // flags to save
    cli_and_save(flags);

	// File system and pcb variable 
	dentry_t dentry;
	uint32_t pcb_address;
	pcb_t * my_pcb = get_pcb(); // get previous pcb
	pcb_t * child_pcb;
	int32_t my_pid;

	// Loop variable 
	int i;
	uint8_t currentChar;

	// Argument parsing variable
	uint8_t cmd[CMD_SIZE]; 	// will store file name (command)
	int commandIndex = 0;
	int cmdIndex = 0;
	int argsIndex = 0;			///variables to parse the cmd 
	int noArgs = 0;

	// Exe variable
	uint8_t exe_buffer[EXE_CHECK_SIZE] = {0,0,0,0};	//used to check magic numbers in executable file 

	// Misc to put to inline
	int32_t user_cs = USER_CS;
	int32_t user_ds = USER_DS;	
	int32_t user_esp = PROGRAM_IMAGE_OFFSET - 1;
	int32_t entry_point;		 //variables of segment selectors and registers

	// Special command variables
	int noCharsYet = 1;
	int argsStartedFlag = 0;
	int lastArgWordIndex = -1;
	int clearScreen = 1;
	char clearArray[CLEAR_SIZE] = {'c','l','e','a','r'};
	char colorArray[COLOR_SIZE] = {'c','o','l','o','r'};
	int whenClearNoFurtherChars = 1;
	int clearIndex = 0;

	int changeTextColor = 1;
	int colorTextIndex = 0;

	char colorTextArgs[COLOR_MAX_SIZE];
	int colorArgsIndex = 0;
	// int colorArgsIndex2 = 0;
	int colorTextArgsStarted = 0;
	int checkForExtraColorArgs = 0;



	// Debug print statements
	// uint8_t * file_data_buffer;
	// printf("In execute: PCB: %#x\n", my_pcb);
	// printf("Execute syscall\n");

	// I don't think we have stdio lib file so we can't use strtok() function, etc. for parsing

	// assume command is null terminated.
	// get command (not args just yet)
	while (1) {
		currentChar = command[commandIndex];

		if (noCharsYet && currentChar == SPACE_ASCII) {
			commandIndex++;
			continue;
		}

		if (currentChar == 0) {
			cmd[cmdIndex] = 0;
			noArgs = 1;	// no arguments!
			break;
		}else if (currentChar == SPACE_ASCII) {
			cmd[cmdIndex] = 0;
			commandIndex++;
			break;
		}else {
			noCharsYet = 0;
			cmd[cmdIndex] = currentChar;
		}

		commandIndex++;
		cmdIndex++;
	}

	// Recognizing if command is exactly 'clear'
	if (commandIndex < CLEAR_SIZE) {
		clearScreen = 0;
	}else {
		for (i = 0; i < CLEAR_SIZE; i++) {
			if (cmd[i] != clearArray[i]) {
				clearScreen = 0;
				break;
			}
		}
	}

	clearIndex = commandIndex;
	while (command[clearIndex] != 0) {
		if (command[clearIndex] != SPACE_ASCII) {
			whenClearNoFurtherChars = 0;
			break;
		}
		clearIndex++;
	}


	// If 'clear' then clear screen
	if (clearScreen && whenClearNoFurtherChars) {
		clear();
		return 0;
	}


	if (commandIndex < COLOR_SIZE) {
		changeTextColor = 0;
	}else {
		for (i = 0; i < COLOR_SIZE; i++) {
			if (cmd[i] != colorArray[i]) {
				changeTextColor = 0;
				break;
			}
		}
	}

	colorTextIndex = commandIndex;

	while (changeTextColor && command[colorTextIndex] != 0) {

		if (command[colorTextIndex] == SPACE_ASCII && colorTextArgsStarted) {
			checkForExtraColorArgs = 1;
		}else if (command[colorTextIndex] != SPACE_ASCII) {
			colorTextArgsStarted = 1;

			if (colorArgsIndex >= COLOR_MAX_SIZE) {
				printColorTextErrorMessage();
				return -1;
			}

			colorTextArgs[colorArgsIndex++] = command[colorTextIndex];
		}

		colorTextIndex++;
	}

	colorTextArgs[colorArgsIndex] = 0;

	if (changeTextColor && !colorTextArgsStarted) {
		changeTextColor = 0;
		printColorTextErrorMessage();
		return -1;
	}

	if (changeTextColor && checkForExtraColorArgs) {
		while (command[colorTextIndex] != 0) {
			if (command[colorTextIndex++] != SPACE_ASCII) {
				printColorTextErrorMessage();
				return -1;
			}
		}
	}

	if (changeTextColor) {

		if (strncmp(colorTextArgs,"blue",4) == 0 && strncmp(colorTextArgs,"blue",colorArgsIndex) == 0) {	// blue is 4 letters
			terminals[screen_process].attrib = 1;	// blue color
		}else if (strncmp(colorTextArgs,"green",5) == 0 && strncmp(colorTextArgs,"green",colorArgsIndex) == 0) {	// green is 5 letters
			terminals[screen_process].attrib = 2;	// green color
		}else if (strncmp(colorTextArgs,"cyan",4) == 0 && strncmp(colorTextArgs,"cyan",colorArgsIndex) == 0) {	// cyan is 4 letters
			terminals[screen_process].attrib = 3;	// cyan color
		}else if (strncmp(colorTextArgs,"red",3) == 0 && strncmp(colorTextArgs,"red",colorArgsIndex) == 0){	// red is 3 letters
			terminals[screen_process].attrib = 4;	// red color
		}else if (strncmp(colorTextArgs,"magenta",7) == 0 && strncmp(colorTextArgs,"magenta",colorArgsIndex) == 0){	// magenta is 7 letters
			terminals[screen_process].attrib = 5;	// magenta color
		}else if (strncmp(colorTextArgs,"brown",5) == 0 && strncmp(colorTextArgs,"brown",colorArgsIndex) == 0) {	// brown is 5 letters
			terminals[screen_process].attrib = 6;	// brown color
		}else if (strncmp(colorTextArgs,"white",5) == 0 && strncmp(colorTextArgs,"white",colorArgsIndex) == 0){	// white is 5 letters
			terminals[screen_process].attrib = 7;	// white color
		}else if (strncmp(colorTextArgs,"yellow",6) == 0 && strncmp(colorTextArgs,"yellow",colorArgsIndex) == 0){	// yellow is 6 letters
			terminals[screen_process].attrib = 0xE;	// yellow color
		}else {
			printColorTextErrorMessage();
			return -1;
		}


		changeColors();

		return 0;
	}


	// Check if file exists, if not then return 
	if(read_dentry_by_name(cmd, &dentry) == -1){
		printf("Command not found error: entered cmd is %s\n", cmd);
		restore_flags(flags);
		sti();
		return -1;
	} 

	// Check if file is executable, if not then return
	if(read_data(dentry.inode_num, 0, exe_buffer, EXE_CHECK_SIZE) != EXE_CHECK_SIZE) {
		printf("%s\n", "File is not executable");
		restore_flags(flags);
		sti();
		return -1; 
	}

	// if this is not executable file...
	if (!(exe_buffer[EXE_FIRST] == exe_char[EXE_FIRST] && exe_buffer[EXE_SECOND] == exe_char[EXE_SECOND]		//check the magic numbers of executable file 
		&& exe_buffer[EXE_THIRD] == exe_char[EXE_THIRD] && exe_buffer[EXE_FOURTH] == exe_char[EXE_FOURTH])) {

		printf("%s\n", "File is not executable");
	 	restore_flags(flags);
		sti();
		return -1; 
	}

	// Look for available pcb 
	for (my_pid = 0; my_pid < max_process + 1; my_pid++){	
		pcb_address = OFFSET_8MB - (OFFSET_4KB * 2 * (my_pid + 2));	// + 2 b/c skip bottom one
		child_pcb = (pcb_t *) pcb_address;
		if (child_pcb->available == 1 || my_pid == max_process){	// if can be used although not null (b/c it was used before)
			break;
		} 
	}


	if (terminals[screen_process].execute_flag_shell == 0) {
		terminals[screen_process].execute_flag_shell = 1;

		child_pcb->isBaseShellInATerminal = 1;
	}else {
		child_pcb->isBaseShellInATerminal = 0;
	}


	// If none of the pcb is available then create a new pcb
	if (my_pid == max_process) {
		if(my_pid == process_cap){
			printf("maximum process reached\n");
			cap_flag = 1;
			return my_pid;
		}
		max_process++;
	}
	// Logistics
	child_pcb->argumentSizeBytes = 0;

	// get args
	while (!noArgs && 1) {	// if there are arguments to command
		currentChar = command[commandIndex];

		if (currentChar == SPACE_ASCII && !argsStartedFlag) {
			commandIndex++;
			continue;
		}

		// if (currentChar == SPACE_ASCII && argsStartedFlag) {
		// 	lastArgWordIndex = commandIndex;
		// }

		argsStartedFlag = 1;

		// args[argsIndex] = currentChar;
		child_pcb->args[argsIndex] = currentChar;

		child_pcb->argumentSizeBytes++;

		if (currentChar != SPACE_ASCII && currentChar != 0) {
			lastArgWordIndex = argsIndex;
		}

		if (currentChar == 0) {
			
			if (lastArgWordIndex != -1) {
				child_pcb->args[lastArgWordIndex+1] = 0;

				child_pcb->argumentSizeBytes = lastArgWordIndex+2;	// 2 = 1 for null char + 1 for size = last index
			}


			break;
		}

		argsIndex++;
		commandIndex++;
	}

	// Init pcb (open 1, 2, file fields)
	initialize_pcb(child_pcb);

	child_pcb->my_process_id = my_pid;
	current_proc->pid = my_pid;

	// Paging (create new page, mark present, r/w, global, etc.)
	new_page(my_pid);


	// Load file to new page
	// remember that buffer needs to start at + 0x48000 offset from 128 MB in virt. mem. (will be mapped to correct phys. addr.)
	if(read_data(dentry.inode_num, 0, (uint8_t *) (PROGRAM_IMAGE_OFFSET), inode_arr[dentry.inode_num].length) == OFFSET_4MB) {
		printf("%s\n", "Program too large");

		// Make pcb avaliable
		child_pcb->available = 1;
		if (my_pid == max_process) max_process --;

		// Enable int
		restore_flags(flags);
		sti();
		return -1; 
	}


	// Context switch 
	if(read_data(dentry.inode_num, ENTRY_POINT_OFFSET, (void *) &entry_point, ENTRY_POINT_READ_BYTES) != ENTRY_POINT_READ_BYTES) {	//
		printf("%s\n", "Fail to read entry point");
		
		// Make pcb avaliable
		child_pcb->available = 1;
		if (my_pid == max_process) max_process --;

		// Enable int
		restore_flags(flags);
		sti();
		return -1;
	}

	// Store the parent esp and ebp to jump back during halt (after halt, we need to come back to "return" statement of execute)
	asm volatile ("                             \n\
            movl   %%esp, %0             		\n\
            "
            : "=r" (child_pcb->parent_esp)
            :
            : "memory"
    );	
    asm volatile ("                             \n\
            movl   %%ebp, %0             		\n\
            "
            : "=r" (child_pcb->parent_ebp)
            :
            : "memory"
    );	

#ifdef PCB_TEST
    /* Run tests */

    printf("Opening pcb number %d\n", my_pid);

#endif    

    // Prolly have to rewrite this for multi-proc
    if(my_pid != 0) // if not first process (suppose ls is run from shell right away; shell's pid: 0, so ls's parent pid is 0, ls has pid 1)
    	child_pcb->parent_process_id = my_pcb->my_process_id;	// set parent of new process to prev. process
    // else // if first user process (first user process is always shell)
    	// pcb_tasklist[my_pid]->parent_process_id = 13;		// set parent of new process to 13

    // Jump with interrupts enabled
	asm volatile(" 					\n\
		movw  	%%ax, %%ds 			\n\
		movw 	%%ax, %%es 			\n\
     	movw 	%%ax, %%fs 			\n\
     	movw 	%%ax, %%gs 			\n\
     	pushl  	%%eax				\n\
		pushl  	%%edi		 		\n\
		pushf						\n\
		popl	%%ecx 				\n\
		orl	 	$0x200, %%ecx		\n\
		pushl	%%ecx 				\n\
		pushl 	%%esi				\n\
		pushl  	%%ebx				\n\
     "
    :
    :	"S" (user_cs), "a" (user_ds), "D" (user_esp), "b" (entry_point)
    :	"memory", "ecx"
    );	

	//tss registers (for when going from user back to kernel; for going from level 3 to level 0)
	tss.ss0 = KERNEL_DS;
    tss.esp0 = KERNEL_OFFSET - (OFFSET_4KB * 2 * (my_pid + 1));	// 8 KB...


	asm volatile("iret");

	return 0;
}

// Redo this function for next cp

//function: read
//input: fd(file descriptor), buf(buffer to keep file data), nbytes(bytes read)
//output: byte read
//return value: number of bytes read if success, -1 if fail
//side effects: read a file 
int32_t read (int32_t fd, void* buf, int32_t nbytes) {


	// printf("Read syscall\n");

	// int bytesRead;

	// pcb_t * my_pcb = get_pcb();

	if (get_pcb()->file_array_array[fd].flags == 0)	// if not even opened...
		return -1;


	//just for testing initially: return ((read_fp)(file_jumptable[2]))(fd,buf,nbytes);

	// printf("Read syscall fd number: %d\n",fd);
	
	//get file position 
	// file_position_offset = my_pcb->file_array_array[fd].file_position;

	// printf("read\n");
	sti();

	return ((read_fp)(get_pcb()->file_array_array[fd].file_jumptable[JUMP_READ]))(fd,buf,nbytes);
	// bytesRead = ((read_fp)(my_pcb->file_array_array[fd].file_jumptable[2]))(fd,buf,nbytes);

	// //update position
	// my_pcb->file_array_array[fd].file_position += bytesRead;
	// file_position_offset = my_pcb->file_array_array[fd].file_position;

	// printf("REQUESTED: %d\n",nbytes);
	// printf("Bytes Read: %d\n",bytesRead);

	// printf()
	// return bytesRead;
}

//function: write
//input: fd(file descriptor), buf(buffer to keep file data), nbytes(bytes write)
//output: bytes write
//return value: number of bytes write if success, -1 if fail
//side effects: write to terminal 
int32_t write (int32_t fd, const void* buf, int32_t nbytes) {


	// printf("Write syscall\n");
	//get pcb address
	//pcb_t * my_pcb = ;
	if (get_pcb()->file_array_array[fd].flags == NOT_USE)	// if not even opened...
		return -1;

	// printf("Write Success. fd: %d\n",fd);

	// int i;

	// for (i = 0; i < 100; i++)
	// 	printf("%c",((char*)buf)[i]);

	sti();
	return ((write_fp)(get_pcb()->file_array_array[fd].file_jumptable[JUMP_WRITE]))(fd,buf,nbytes);
}


// Redo this function for next cp

//function: open
//input: filename
//output: file descriptor
//return value: file descriptor index if success, -1 if fail
//side effects: open a file
int32_t open (const uint8_t* filename) {

	// printf("Open syscall\n");

	int i = 0; 
	int fd = -1; 
	int flag = 0;
	dentry_t dentry;
	pcb_t * my_pcb = get_pcb();

	if(read_dentry_by_name(filename, &dentry) == -1) return -1;
	//not needed at all for open: read_data(dentry.inode_num, 0, (uint8_t *) (OFFSET_4MB * 32), inode_arr[dentry.inode_num].length);


	// Assign fd
	for (i = 0; i < OPEN_FILE_NUM; i ++){	// really can start at 2 not 0
		if (my_pcb->file_array_array[i].flags == 0) {
			flag = 1;
			fd = i;

			if (dentry.file_type == 0) {	// rtc file

				if (open_rtc(filename) == -1)
					return -1;

				my_pcb->file_array_array[i].flags = IN_USE;
				my_pcb->file_array_array[i].inode = 0;  // inode only for reg file
				my_pcb->file_array_array[i].file_position = 0;  
				my_pcb->file_array_array[i].file_jumptable = rtc_jumptable;
			}else if (dentry.file_type == DIRECTORY) {	// directory

				if (dir_open(filename) == -1)
					return -1;

				my_pcb->file_array_array[i].flags = IN_USE;
				my_pcb->file_array_array[i].inode = 0;  // inode only for reg file
				my_pcb->file_array_array[i].file_position = 0;  
				my_pcb->file_array_array[i].file_jumptable = dir_jumptable;
			}else if (dentry.file_type == REGULAR_FILE) {	// regular file

				if (file_open(filename) == -1)
					return -1;

				my_pcb->file_array_array[i].flags = IN_USE;
				my_pcb->file_array_array[i].inode = dentry.inode_num;  // inode only for reg file
				my_pcb->file_array_array[i].file_position = 0;  
				my_pcb->file_array_array[i].file_jumptable = file_jumptable;
			}else {	// invalid file type
				return -1;
			}


			break;
		}
	}

	if (!flag) return -1; // file array full!
	//printf("%d\n", fd);
#ifdef PCB_TEST
    /* Run tests */

    printf("Opening fd number %d\n", fd);

#endif 
    sti();
	return fd;
}

// Redo this function for next cp

//function: close
//input: file descriptor
//output: none
//return value: 0 if success, -1 if fail
//side effects: close a file
int32_t close (int32_t fd) {

	// printf("Close syscall\n");

	pcb_t * my_pcb = get_pcb();

	if (fd < 0 || fd > OPEN_FILE_NUM - 1)	// if file descriptor is invalid
		return -1;

	if (fd == STDIN_IDX || fd == STDOUT_IDX)	// if user tries to close stdin or stdout
		return -1;

	if (my_pcb->file_array_array[fd].flags == NOT_USE) {	// if user is trying to close already closed file
		return -1;
	}else {	// if file is open and can be closed
		if ( ((close_fp)(my_pcb->file_array_array[fd].file_jumptable[1]))(fd) == -1 )	// if cannot close file
			return -1;

		// if can close file:
		my_pcb->file_array_array[fd].flags = NOT_USE;
		my_pcb->file_array_array[fd].inode = 0;
		my_pcb->file_array_array[fd].file_position = 0;
		my_pcb->file_array_array[fd].file_jumptable = NULL;
	}
#ifdef PCB_TEST
    /* Run tests */

    printf("Closing fd number %d\n", fd);

#endif 
    sti();
	return 0;
}

//function: getargs
//input: buf(buffer to keep file data), nbytes(bytes write)
//output: none
//return value: 0 if success, -1 if fail
//side effects: get argument
int32_t getargs (uint8_t* buf, int32_t nbytes) {
	// printf("Getargs syscall\n");

	int i;

	pcb_t * my_pcb = get_pcb();

	// printf("nBytes: %d savedBytes: %d\n", nbytes, my_pcb->argumentSizeBytes);
	//check fail

	if (buf == NULL)
		return -1;

	// if (buf < (uint8_t*)OFFSET_128MB || buf > (uint8_t*)OFFSET_132MB)	// if out of range for user space program
	// 	return -1;

	//check if too many bytes
	if (my_pcb->argumentSizeBytes > nbytes || my_pcb->argumentSizeBytes <= 0)
		return -1;

	for (i = 0; i < my_pcb->argumentSizeBytes; i++) {
		buf[i] = my_pcb->args[i];
	}

	// printf("got here START\n");
	// for (i = 0; i < my_pcb->argumentSizeBytes; i++)
	// 	printf("%c ",buf[i]);
	// printf("\n");
	// printf("got here END\n");
	sti();
	return 0;
}

//function: vidmap
//input: pointer of start of screen
//output: none
//return value: 0 if success, -1 if fail
//side effects: change video map address
int32_t vidmap (uint8_t** screen_start) {
	int i; 	 // counter

	// printf("vidmap: %d\n",screen_start);
	// Set up video map table
	for(i = 0; i < PT_SIZE; i++){
      vidmap_page_table[current_proc->terminal_id][i] = (i * OFFSET_4KB) | READ_AND_WRITE | USER; 
  	}

  	// Point the first entry to video memory
  	vidmap_page_table[current_proc->terminal_id][0] = (VID_MEM * OFFSET_4KB) | READ_AND_WRITE | PRESENT | USER;

  	// Put the page directory in the page table
  	page_directory[current_proc->terminal_id][USER_PD + 1 + current_proc->terminal_id] = ((unsigned int)vidmap_page_table[current_proc->terminal_id]) | PRESENT | READ_AND_WRITE | USER;

  	// Write to user's address
  	screen_start[0] = (uint8_t*) ((USER_PD + 1 + current_proc->terminal_id) * OFFSET_4MB);

  	clear();

  	// printf("vidmap\n");
  	sti();
	return 0;
}

//function: vidmap
//input: pointer of start of screen
//output: none
//return value: 0 if success, -1 if fail
//side effects: change video map address
// int32_t vidmap_prev (uint8_t** screen_start) {
// 	int i; 	 // counter

// 	// printf("vidmap: %d\n",screen_start);
// 	// Set up video map table
// 	for(i = 0; i < PT_SIZE; i++){
//       vidmap_page_table[current_proc->id][i] = (i * OFFSET_4KB) | READ_AND_WRITE | USER; 
//   	}

//   	// Point the first entry to video memory
//   	vidmap_page_table[current_proc->id][0] = (VID_MEM * OFFSET_4KB) | READ_AND_WRITE | PRESENT | USER;

//   	// Put the page directory in the page table
//   	page_directory[current_proc->id][USER_PD + 1 + current_proc->id] = ((unsigned int)vidmap_page_table[current_proc->id]) | PRESENT | READ_AND_WRITE | USER;

//   	// Write to user's address
//   	screen_start[0] = (uint8_t*) ((USER_PD + 1 + current_proc->id) * OFFSET_4MB);

//   	clear();

//   	// printf("vidmap\n");
//   	sti();
// 	return 0;
// }




//function: set handler
int32_t set_handler (int32_t signum, void* handler_address) {
	return 0;
}

//function: sigreturn
int32_t sigreturn (void) {
	return 0;
}

/* 
 * sys_halt: wrapper function to check for userspace inputs validity and pass to halt
 */
int32_t sys_halt (uint8_t status) {
	// All params accepted
	return(halt(status));
};

/* 
 * sys_execute: wrapper function to check for userspace inputs validity and pass to execute
 */
int32_t sys_execute (const uint8_t* command){
	if (!valid_pointer((void *)command)) return -1;
	return(execute(command));
}

/* 
 * sys_read: wrapper function to check for userspace inputs validity and pass to read
 */
int32_t sys_read (int32_t fd, void* buf, int32_t nbytes){
	if (!valid_pointer(buf)) return -1;

	if (fd < 0 || fd > OPEN_FILE_NUM - 1 || buf == NULL || nbytes < 0)	// if invalid file descriptors or buffer is null
		return -1;

	if (fd == STDOUT_IDX)	// if stdout, doesn't make sense to read
		return -1;

	return(read(fd, buf, nbytes));
}

/* 
 * sys_write: wrapper function to check for userspace inputs validity and pass to write
 */
int32_t sys_write (int32_t fd, const void* buf, int32_t nbytes){
	if (!valid_pointer(buf)) return -1;

	if (fd < 0 || fd > OPEN_FILE_NUM - 1 || buf == NULL || nbytes < 0)	// if invalid file descriptors or buffer is null
		return -1;
	if (fd == STDIN_IDX)	// if stdin, doesn't make sense to write
		return -1;

	return(write(fd, buf, nbytes));
}

/* 
 * sys_open: wrapper function to check for userspace inputs validity and pass to open
 */
int32_t sys_open (const uint8_t* filename){
	if (!valid_pointer((void *)filename)) return -1;
	return(open(filename));
}

/* 
 * sys_close: wrapper function to check for userspace inputs validity and pass to close
 */
int32_t sys_close (int32_t fd) {
	if (fd < 0 || fd > OPEN_FILE_NUM - 1)	// if invalid file descriptors or buffer is null
		return -1;
	return(close(fd));
}

/* 
 * sys_getargs: wrapper function to check for userspace inputs validity and pass to getargs
 */
int32_t sys_getargs (uint8_t* buf, int32_t nbytes){
	if (!valid_pointer((void *)buf) || nbytes < 0) return -1;
	return(getargs(buf, nbytes));
}

/* 
 * sys_vidmap: wrapper function to check for userspace inputs validity and pass to vidmap
 */
int32_t sys_vidmap (uint8_t** sreen_start){
	if (!valid_pointer((void *)sreen_start)) return -1;
	return(vidmap(sreen_start));
}

/* 
 * sys_set_handler: wrapper function to check for userspace inputs validity and pass to set_handler
 */
int32_t sys_set_handler (int32_t signum, void* handler_address){
	return -1;
}

/* 
 * sys_sigreturn: wrapper function to check for userspace inputs validity and pass to sigreturn
 */
int32_t sys_sigreturn (void){
	return -1;
}

//function: get pcb
//input: none
//output: stack pointer
//return value: stack pointer
//side effects: none
pcb_t * get_pcb(){
	uint32_t esp;
	asm volatile ("                             \n\
            movl   %%esp, %0            		\n\
            "
            : "=r" (esp)
            :
            : "memory"
    );	
    esp &= STACK_BITMASK;	// lower 13 bits zeroed to get top
    return (pcb_t *) esp;
}

//function: initialize pcb
//input: my process id
//ouput: none
//return value: none
//side effects:  initialize pcb write to kernel space and some shit
static void initialize_pcb(pcb_t * child_pcb){
	// always open stdin, stdout
	child_pcb->file_array_array[STDIN_IDX].flags = IN_USE; 
	child_pcb->file_array_array[STDOUT_IDX].flags = IN_USE; 

	child_pcb->file_array_array[STDIN_IDX].file_jumptable = stdin_jumptable;
	child_pcb->file_array_array[STDOUT_IDX].file_jumptable = stdout_jumptable;

	// pcb_tasklist[my_pid]->	args[3];
	child_pcb->available = 0; 
}

//function: check_pointer
//input: a pointer
//ouput: boolean value for whether it is a valid pointer
//return value: none
//side effects:  none 
static int valid_pointer(const void * pointer) {
	uint32_t tmp = (uint32_t) pointer;
	if (USER_PD * OFFSET_4MB <= tmp && (USER_PD + 1) * OFFSET_4MB > tmp )
		return 1;
	return 0;
}


