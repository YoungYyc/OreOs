#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H
#include "types.h"
#include "multiboot.h"

#define DATA_ARR_SIZE 		4096
#define FILE_NAME_SIZE 		32
#define DENTRY_ARR_SIZE 	63
#define BLOCK_NUM 			1024
#define RESERVED_NUM_INFO 	13
#define RESERVED_NUM_DENTRY 6

typedef struct dentry 		//struct for directory entries
{
	uint8_t file_name[FILE_NAME_SIZE];
	uint32_t file_type;
	uint32_t inode_num;	
	uint32_t reserved_dentry[RESERVED_NUM_DENTRY];		// 24 B reserved 
} dentry_t;

typedef struct boot_block 	//struct for boot blocks
{
	uint32_t dir_entry;
	uint32_t inodes_num;
	uint32_t d_blocks_num;
	uint32_t reserved_boot_block[RESERVED_NUM_INFO];	//52 B reserved 
	dentry_t dentry_arr[DENTRY_ARR_SIZE];
} boot_block_t;


typedef struct inode_block 		//struct for index node blocks
{
	uint32_t length;
	uint32_t d_blocks_arr[BLOCK_NUM - 1];
} inode_block_t;


typedef struct data_block 		//struct for data blocks
{
	uint8_t data_blocks_arr[DATA_ARR_SIZE];
} data_block_t;


boot_block_t * filesys_start; 	//start address of file system	
inode_block_t * inode_arr; 		//start address of index node array
data_block_t * data_arr;		//start address of data array

uint32_t curent_file_index; 


//initialize file system in kernel.c
void init_file_system(boot_block_t * start);
//handle file operation
int32_t file_open(const uint8_t* file_name);
int32_t file_close(int32_t fd);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
//handle directory operation
int32_t dir_open(const uint8_t * filename);
int32_t dir_close(int32_t fd);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
//routines of file syetem
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


#endif

