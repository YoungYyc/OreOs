#include "file_system.h"
#include "types.h"
#include "multiboot.h"
#include "lib.h"
#include "scheduler.h"
#include "system_calls.h"


#define BUFFER_SIZE 1024 //used as magic constant to identify overshooting of offset


//init_file_system
//this function initialize the file system by assigning pointer of inode array and data array
//input: start address of file system
//output: none
//return value: none
//side effects: assigning address of inode array and data array
void init_file_system(boot_block_t * start){
	//printf("start file system");
	inode_arr = (inode_block_t*) (filesys_start + 1);	//initialize index node address of file system
	data_arr = (data_block_t*) (filesys_start + (filesys_start->inodes_num +1));	//initialize data address of file system
}

//file_open
//this function handles operation of opening file
//input: name of file suppose to open
//output: return value from routine
//return value: 0
//side effects: initialize any temporary struct
int32_t file_open(const uint8_t* file_name){
	// dentry_t * file_open_dentry;
	// read_dentry_by_name(file_name, file_open_dentry);
	return 0;
}

//file_close
//this function undo file_open, current do nothing
//input: none
//output: none
//return value: none
//side effects: undo file_open
int32_t file_close(int32_t fd){
	return 0;	// do nothing, return 0
}

//file_write
//this function should not do anything because its read-only file system
//input:none
//output: none
//return value: -1
//side effects: none
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
	return -1;	// do nothing, return -1
}

//file_read
//this function read counts bytes of data into buf
//input: count, buf
//ouput: none
//return value: none
//side effects: counts bytes is written into buf
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
	int32_t retVal = read_data(get_pcb()->file_array_array[fd].inode, get_pcb()->file_array_array[fd].file_position, buf, nbytes);
	get_pcb()->file_array_array[fd].file_position += retVal;
	return retVal;
}

//dir_open
//this function opens directory file
//input: dir_name
//output: none
//return value: 0
//side effects: opens the dir we want to open
int32_t dir_open(const uint8_t* dir_name){
	curent_file_index = 0;		//reset the index of the current file in the loop to 0
	return 0;
}

//dir_close
//this function probably does nothing 
//input: none
//output: none
//return value: 0
//side effects: none
int32_t dir_close(int32_t fd){
	return 0;	// do nothing, return 0 (stuff are done by the syscals)
}

//dir_write
//this function should not do anything because its read-only file system
//input: none
//output: none
//reurn value: -1
//side effects: none 
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){
	return -1;	// do nothing, return -1
}

//dir_read
//this function  read files filename by filename
//input: file name
//output: read file matching file name
//return value: number of bytes read
//side effects: none
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){

	// printf("DIR READ\n");

	int j = 0;//counter 
	uint8_t * tmp = buf;

	// Check for bad parameters 
	if(buf == NULL) return -1;  

	// Return 0 eventually
	if(curent_file_index > filesys_start->dir_entry) return 0;

	// Write file name to the buffer

	// Copy to user stuff
	for(j = 0; j < nbytes || j < FILE_NAME_SIZE; j++){
		tmp[j] = filesys_start->dentry_arr[curent_file_index].file_name[j];
		if (tmp[j] == '\0') break;
	}

	// Go to next file
	curent_file_index++;

	return j;
}

// 0
// verylargetextwithverylongname.tx$
// verylargetextwithverylongname.tx

//read_dentry_by_name
//this routine reads dir entry by file name and copy the data into dentry 
//input: file name, empty dir entry
//output: data copyed to dentry
//return value: 0 when success , -1 when fail
//side effects: none
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry){
	int i = 0, j = 0, k = 0;  //counters
	int flag = 1;
	if(dentry == NULL || fname[0] == '\0')	return -1;	//dentry is invalid

	for(i = 0; i < filesys_start->inodes_num; i++){
		for(j = 0; j < FILE_NAME_SIZE + 1; j++){
			if (fname[j] == 0 && filesys_start->dentry_arr[i].file_name[j] == 0) {
				// printf("broke here: %d\n",j);
				break;
			}else if (j == FILE_NAME_SIZE && fname[j] == 0){
				break;
			}
			else if (fname[j] == 0 || filesys_start->dentry_arr[i].file_name[j] == 0){
				flag = 0;
				break;
			} 
			if(fname[j] != filesys_start->dentry_arr[i].file_name[j]){
				flag = 0;
				break;
			}
		}								//check if whole name matches
		if(flag){

			// printf("Success\n");
			for(k = 0; k < FILE_NAME_SIZE; k++){
				dentry->file_name[k] = filesys_start->dentry_arr[i].file_name[k];
			}

			dentry->file_type = filesys_start->dentry_arr[i].file_type;
			dentry->inode_num = filesys_start->dentry_arr[i].inode_num;
			return 0;
		}								//if mathces, copy the data
		flag = 1;
	}

	return -1;						//else fail
}

// read_dentry_by_index
// this function reads dir entry by index and copy the data into dentry
// input: index, dentry
// ouput: data copy into dentry
// return value: o when success, -1 when fail
// side effects: none
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry){
	int i = 0;	//counter

	if(dentry == NULL || index > filesys_start->inodes_num)	return -1;	//dentry is invalid or index is invalid

	for(i = 0; i < FILE_NAME_SIZE; i++){		//copy file name
		dentry->file_name[i] = filesys_start->dentry_arr[index].file_name[i];
	}
	dentry->file_type = filesys_start->dentry_arr[index].file_type;
	dentry->inode_num = filesys_start->dentry_arr[index].inode_num;		//copy other data 
	return 0;
}

// read_data
// this function read data and copy to buf
// input: index of node, offset, buffer, length to read
// output: data copied into buffer
// return value: -1 when fail. If reads length bytes data, return 0, else return number of data read
// side effects: none
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){

	uint32_t i = offset / DATA_ARR_SIZE;		//index of inodes array start to read
	uint32_t j = offset % DATA_ARR_SIZE;  		//index of data array start to read

	if(buf == NULL || (int32_t) length < 0 || (int32_t) offset < 0)	return -1;		//fail in buf or length is invalid

	while((i * DATA_ARR_SIZE + j) < inode_arr[inode].length && (i * DATA_ARR_SIZE + j - offset) < length){
		buf[i * DATA_ARR_SIZE + j - offset] = data_arr[inode_arr[inode].d_blocks_arr[i]].data_blocks_arr[j];
		j++;
		if (j == DATA_ARR_SIZE) {
			i++;
			j = 0;
		}
	}

	return i * DATA_ARR_SIZE + j - offset;
}



