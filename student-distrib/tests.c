#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "paging_structure.h"
#include "terminal.h"
#include "types.h"
#include "rtc.h"
#include "file_system.h"
#include "system_calls.h"

#define PASS 1
#define FAIL 0

#define KEYBOARD_BUFFER_MAX_SIZE	128
#define MAX_FILENAME_SIZE 			32
#define EXE_FILE_TEST_RANGE 		16
#define BUNCH_OF_ONES 				1111111
#define RTC_TEST_RANGE 				20
#define RTC_NUM_TEST_FREQ 			10

// RTC test frequencies
#define NEGFREQ 		-1 
#define ZEROFREQ 		0
#define ONEFREQ 		1
#define ODDFREQONE 		3
#define ODDFREQTWO 		1023
#define ODDFREQTHREE 	500
#define LARGEFREQONE 	1025
#define LARGEFREQTWO 	2048
#define INITIALFREQ 	2


// Read data corner cases variable
#define TEST_BUFFER_LENGTH 	32
#define FAVORITE_OFFSET 	5260
#define SMALL_LENGTH 		10 
#define LARGE_LENGTH 		1024
#define OVERFLOW_LENGTH 	20


// Input to system calls constant 
#define KERNEL_PTR			0x400000
#define NONEXISTEN_PTR		0xC00000
#define USER_PTR			0x8040000

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static int32_t power_of_two[11] = {1, 2, 4, 8 ,16, 32, 64, 128, 256, 512, 1024};
static int8_t exe_signal[4] = {0x7F, 0x45, 0x4C, 0x46 };
 
static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}



/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */




/**



//unit test for exception division error
//input: none
//output: none
//return value: none
//side effects: create division error
void test_divide_zero() {
	TEST_HEADER;

	printf("Divide by zero error!");
	int number = 5/0;
}
//unit test for idt
//input:none 
//output: result of idt test
//return value: PASS when success, FAIL when fail to create idt
//side effects: none
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

//return the idt address
//input: none
//ouput: none
//return value: none
//side effect: print idt address of screen
void test_idt_values(){
	clear();

	TEST_HEADER;

	int i;

	printf("IDT[0]: 0x%x%x\n", idt[0].offset_31_16, idt[0].offset_15_00);

	for (i = 0x20; i <= 0x2F; i++)
		printf("IDT[0x%x]: 0x%x%x\n",i, idt[i].offset_31_16, idt[i].offset_15_00);

}

//unit test for page fault
//input: none
//output: none
//return value: none
//side effect: create page fault 
void test_null_ptr() {
	TEST_HEADER;

	int * ptr = NULL;
	int number = *ptr;
}

//unit test for video address
//input: none
//output: none
//return value: none
//side effect: none 
void test_video_address() {
	TEST_HEADER;

	int * ptr = (int*) 0xB8000 + 5;
	int number = * ptr;
}

//unit test for kernel address
//input: none
//output: none
//return value: none
//side effect: none 
void test_kernel_address() {
	TEST_HEADER;

	int * ptr = (int*) 0x400000 + 5;
	int number = * ptr;
}

//unit test for keyboard
//input: none
//output: none
//return value: none
//side effect: enable keyboard 
void test_keyboard() {
	TEST_HEADER;

	enable_irq(1);
}

//unit test for rtc
//input: none
//output: none
//return value: none
//side effect: enable rtc
void test_rtc() {
	TEST_HEADER;

	enable_irq(8);
}

//unit test for paging structure
//input: none
//output: none
//return value: none
//side effect: show paging entries on screen
void test_paging_structures() {
	clear();

	TEST_HEADER;

	int i = 0;

	for (i = 0; i < 10; i++) {
		printf("Page Directory [%d]: %x\n",i,page_directory[i]);
	}

	printf("First Page Table [%d] (video memory page): %x\n",0xB8,first_page_table[0xB8]);
}

*/

// add more tests here

/* Checkpoint 2 tests */

//unit test for terminal read/write syscall
//input: none
//output: none
//return value: none
//side effect: terminal read from and write to terminal
void test_hello_world() {
	while (1) {
		int i;	// for loop var

		// only reason not replaced with constant: easier to change here to test: input arg. is number of bytes to read.
		terminal_write(1,"Hi, what's your name?\n",22);	// this 22 number is arbitrary (not magic): change to any to test terminal_write

		char buf[KEYBOARD_BUFFER_MAX_SIZE];	// buffer to store in 

		int32_t numBytesRead = terminal_read(1,buf,KEYBOARD_BUFFER_MAX_SIZE);	// actually read from terminal

		printf("Hi ");
		for (i = 0; i < numBytesRead; i++) {	// for every byte read
			putc(buf[i]);	// print this char
		}
	}
}

//unit test for rtc change rate and corner cases
//input: none
//output: none
//return value: none
//side effect: change rtc rate and print to terminal
void test_rtc(){
	int i = 0, j = 0;
	uint8_t ignore = 0;
	int32_t buf = 0;
	int32_t * null_ptr = NULL;

	// Initialize rtc 
	open_rtc(&ignore);

	for (i = 1 ; i < RTC_NUM_TEST_FREQ + 1; i++) {
		// Clear the screen
		clear();

		// Change rtc rate
		buf = power_of_two[i];	// buffer to pass in for change freq.
		printf("Current frequency is %d\n", buf);
		write_rtc(ignore, &buf, 4);	// actually change freq.: 4 bytes
		
		// Print out at some period
		for(j = 0; j < RTC_TEST_RANGE; j++){	// how many times to print out to terminal
			read_rtc(ignore, &buf, 4);	// 4 bytes to read
			printf("%d", BUNCH_OF_ONES);	// some stuff to write to terminal screen
		}
	}

	// Test corner cases
	clear();
	printf("%s\n", "All the following calls should return -1");

	// Test wrong buffer size passed
	buf = INITIALFREQ;
	printf("Pass odd buffer size of 2 %d\n", write_rtc(ignore, &buf, 2));

	// Test null pointer
	printf("Pass null pointer %d\n", write_rtc(ignore, null_ptr, 2));

	// Test negative frequency 
	buf = NEGFREQ; 
	printf("Pass negative freq %d\n", write_rtc(ignore, &buf, 4));

	// Test zero frequency
	buf = ZEROFREQ; 
	printf("Pass 0 freq %d\n", write_rtc(ignore, &buf, 4));

	// Test frequency of 1 (which is not allowed by rtc hardware)
	buf = ONEFREQ; 
	printf("Pass freq = 1 %d\n", write_rtc(ignore, &buf, 4));

	// Test odd small frquency
	buf = ODDFREQONE; 
	printf("Pass odd freq of 3 %d\n", write_rtc(ignore, &buf, 4));

	// Test odd large frequency
	buf = ODDFREQTWO; 
	printf("Pass odd freq of 1023 %d\n", write_rtc(ignore, &buf, 4));

	// Test frequency that is not power of 2
	buf = ODDFREQTHREE; 
	printf("Pass odd freq of 500 %d\n", write_rtc(ignore, &buf, 4));

	// Test larger than range frequency
	buf = LARGEFREQONE; 
	printf("Pass large freq of 1025 %d\n", write_rtc(ignore, &buf, 4));

	// Test power of 2 larger than range
	buf = LARGEFREQTWO; 
	printf("Pass large freq of 2048 %d\n", write_rtc(ignore, &buf, 4));

	// End
	printf("%s\n", "End of test");
}

//unit test for rtc change rate
//input: none
//output: none
//return value: none
//side effect: change rtc rate and print to terminal
void test_read_dir(){
	int i = 0;
	int j = 0;

	// Clear stuff
	clear();	// clear screen

	// Print everything out 
	for (i = 0; i < filesys_start-> dir_entry; i++){	// for every dir entry
		printf("%s", "File name: ");	// print file name
		for(j = 0; j < MAX_FILENAME_SIZE; j++){
			printf("%c", filesys_start->dentry_arr[i].file_name[j]);	// file name char
		}
		printf("%s", " File type: ");	// print file type
		printf("%d", filesys_start->dentry_arr[i].file_type);	// print file type
		printf("%s", " File size: ");	// print file size in B
		printf("%d", inode_arr[filesys_start->dentry_arr[i].inode_num].length);	// print file size
		printf("\n");

		printf("inod %d",filesys_start->dentry_arr[i].inode_num);
	}
}


//unit test for reading by file name
//input: file name
//output: none
//return value: none
//side effect: change rtc rate and print to terminal
void test_read_by_name(const uint8_t * filename){
	clear();

	dentry_t my_dentry;	// dir entry struct
	uint8_t cur;
	int i;
	int flag = 0; // exe or not

	printf("%s\n", filename);	// print file name to terminal

	read_dentry_by_name(filename, &my_dentry);	// read by file name
	for (i = 0; i < 4; i++){	// only four signals in exe; doesn't really make sense to replace with magic constant
		read_data(my_dentry.inode_num, i, &cur, 1);
		if (cur != exe_signal[i]){
			flag = 1;	// this is not exe
			break;
		}
	}

	if (!flag){	// is exe
		//read_dentry_by_name(filename, &my_dentry);	// read by filename
		for (i = 0; i < EXE_FILE_TEST_RANGE; i++){	//
			read_data(my_dentry.inode_num, i, &cur, 1);	// read one byte data from this offset
			if(cur <= 0xF) printf("0x0%x ", cur);
			else printf("0x%x ", cur);
		}
	} else {	// not exe
		for (i = 0; i < inode_arr[my_dentry.inode_num].length; i++){
			read_data(my_dentry.inode_num, i, &cur, 1);	// read one byte data from this offset
			printf("%c", cur);
		}
	}
}

//unit test read file by index
//input: index number to file
//output: none
//return value: none
//side effect: test read by index
void test_read_by_index(const uint32_t index){
	test_read_by_name(filesys_start->dentry_arr[index].file_name);	// read file with this filename
}



//unit test read data corner cases
//input: none
//output: none
//return value: none
//side effect: test read data
void test_read_data_corner(){
	// Local variable for testing
	int i; 
	uint8_t buf[TEST_BUFFER_LENGTH];
	dentry_t temp;

	// Initial prompt
	printf("Number of inodes is %d\n", filesys_start->inodes_num);

	// Test valid range, valid offset, non-overflowing length
	printf("%s\n", "The following test should print out 0, followed by the last 10 char of verylargetextwithverylongname.txt");
    read_dentry_by_name((uint8_t * )"verylargetextwithverylongname.tx", &temp);
    printf("%d\n",read_data(temp.inode_num, FAVORITE_OFFSET, buf, SMALL_LENGTH));

    for(i = 0; i < SMALL_LENGTH ; i++){
        printf("%c", buf[i]);
    }

    printf("\n");

    // Test valid range, valid offset, overflowing length
    printf("%s\n", "The following test should print out 17, followed by the last 17 char of verylargetextwithverylongname.txt then 3 trash");
    printf("%d\n",read_data(temp.inode_num, FAVORITE_OFFSET, buf, OVERFLOW_LENGTH));

    for(i = 0; i < OVERFLOW_LENGTH ; i++){
        printf("%c", buf[i]);
    }

    printf("\n");

    // Test valid range, valid offset, large overflowing length
    printf("%s\n", "The following test should print out 17, followed by the last 17 char of verylargetextwithverylongname.txt without corrupting dentry");
    printf("%d\n",read_data(temp.inode_num, FAVORITE_OFFSET, buf, LARGE_LENGTH));

    for(i = 0; i < TEST_BUFFER_LENGTH ; i++){
        printf("%c", buf[i]);
    }

    printf("\n");

    // Test whether the last code corrupts local variable by over-writing them
    printf("Checking that dentry is not corrupted:\n");
    printf("File type is %d\n", temp.file_type);
    printf("Inode number is %d\n", temp.inode_num);

    // Test invalid range, invalid offset and invaid length
    printf("%s\n", "The following test should fail and print out -1");
    printf("Passing null pointer %d\n",read_data(temp.inode_num, FAVORITE_OFFSET, NULL, OVERFLOW_LENGTH));
    printf("Passing negative nbytes %d\n",read_data(temp.inode_num, FAVORITE_OFFSET, buf, -1));
    printf("Passing negative offset %d\n",read_data(temp.inode_num, -1, buf, 1));
}


/* Checkpoint 3 tests */

// test_bad_ptr()
// Description: test how sys calls react to bad pointers
//input: none
//output: none
//return value: none
//side effect: none
void test_bad_ptr() {
	const int32_t * null_ptr = NULL;
	int32_t buf = 0;
	uint32_t ignore = 0;

	// Test pointer based input: null ptr
	printf("Pass null pointer to sys_execute %d\n", sys_execute((uint8_t *)null_ptr));
	printf("Pass null pointer to sys_read %d\n", sys_read(ignore, (void *) null_ptr, ignore));
	printf("Pass null pointer to sys_write %d\n", sys_write(ignore, (void *) null_ptr, ignore));
	printf("Pass null pointer to sys_open %d\n", sys_open((uint8_t *)null_ptr));
	printf("Pass null pointer to sys_getargs %d\n", sys_getargs((uint8_t *)null_ptr, ignore));
	printf("Pass null pointer to sys_vidmap %d\n", sys_vidmap((uint8_t**)null_ptr));

	// Test pointer based input: kernel ptr
	buf = KERNEL_PTR; 
	printf("Pass kernel pointer to sys_execute %d\n", sys_execute((uint8_t *)&buf));
	printf("Pass kernel pointer to sys_read %d\n", sys_read(ignore, (void *) &buf, ignore));
	printf("Pass kernel pointer to sys_write %d\n", sys_write(ignore, (void *) &buf, ignore));
	printf("Pass kernel pointer to sys_open %d\n", sys_open((uint8_t *)&buf));
	printf("Pass kernel pointer to sys_getargs %d\n", sys_getargs((uint8_t *)&buf, ignore));
	printf("Pass kernel pointer to sys_vidmap %d\n", sys_vidmap((uint8_t**)&buf));

	// Test pointer based input: nonexistent page 
	buf = NONEXISTEN_PTR; 
	printf("Pass nonexistent page address to sys_execute %d\n", sys_execute((uint8_t *)&buf));
	printf("Pass nonexistent page address to sys_read %d\n", sys_read(ignore, (void *) &buf, ignore));
	printf("Pass nonexistent page address to sys_write %d\n", sys_write(ignore, (void *) &buf, ignore));
	printf("Pass nonexistent page address to sys_open %d\n", sys_open((uint8_t *)&buf));
	printf("Pass nonexistent page address to sys_getargs %d\n", sys_getargs((uint8_t *)&buf, ignore));
	printf("Pass nonexistent page address to sys_vidmap %d\n", sys_vidmap((uint8_t**)&buf));

}

// test_bad_input()
// Description: test how sys calls react to bad inputs
//input: none
//output: none
//return value: none
//side effect: none
void test_bad_input() {
	int i = 0, j = 0;
	uint32_t ignore = 0;
	int32_t buf = 0;
	
	// Test fd input: negative fd
	buf = USER_PTR;
	i = -1; 
	printf("Pass negative fd to sys_read %d\n", sys_read(i, (void *) &buf, ignore));
	printf("Pass negative fd to sys_write %d\n", sys_write(i, (void *) &buf, ignore));
	printf("Pass negative fd to sys_close %d\n", sys_close(i));

	// Test fd input: large fd
	i = 10; 
	printf("Pass large fd to sys_read %d\n", sys_read(i, (void *) &buf, ignore));
	printf("Pass large fd to sys_write %d\n", sys_write(i, (void *) &buf, ignore));
	printf("Pass large fd to sys_close %d\n", sys_close(i));

	// Test nbytes: negative nbytes
	i = 3;
	j = -3;
	printf("Pass negative nbytes to sys_read %d\n", sys_read(i, (void *) &buf, j));
	printf("Pass negative nbytes to sys_write %d\n", sys_write(i, (void *) &buf, j));
	printf("Pass negative nbytes to sys_getargs %d\n", sys_getargs((uint8_t *)&buf, i));

}
// test_testprint()
// Description: test the shell
//input: none
//output: none
//return value: none
//side effect: test read data
void test_shell_execute() {
	clear();

	execute((uint8_t*)"shell");
}

// test_testprint()
// Description: test the testprint program
//input: none
//output: none
//return value: none
//side effect: test read data
void test_testprint() {
	clear();

	execute((uint8_t*)"testprint");
}

/* Checkpoint 4 tests */

// test_bad_input and test_bad_ptr as above

/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){

	//TEST_OUTPUT("idt_test", idt_test());

	// launch your tests here

	// test_divide_zero();

	// test_keyboard();

	// test_rtc();

	//test_null_ptr();

	//test_video_address();

	//test_kernel_address();

	//test_idt_values();

	// test_paging_structures();

	// test_hello_world();

	// test_rtc();

	// test_read_dir();

	// test_read_by_name("frame0.txt");

	// test_read_by_index(12);

	// test_read_data_corner();


	/** Checkpoint 3 Tests **/
	// test_bad_input();

	test_bad_ptr();

	// test_shell_execute();

	// test_testprint();
}
