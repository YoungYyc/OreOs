#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25


#define CAPS_LOCK_PRESS 0x3A
#define CAPS_LOCK_RELEASE 0xBA
#define LEFT_SHIFT_PRESS 0x2A
#define RIGHT_SHIFT_PRESS 0x36
#define LEFT_SHIFT_RELEASE 0xAA
#define RIGHT_SHIFT_RELEASE 0xB6
#define LEFT_CTL_PRESS 0x1D
#define LEFT_CTL_RELEASE 0x9D
#define BACKSPACE_PRESS 0x0E
#define ENTER_PRESS 0x1C
#define L_PRESS 0x26
#define PRESS_RELEASE_BOUNDARY 0x81
#define NULL_CHAR 0
#define BUFFER_MAX_SIZE 128
#define SCANCODE_MAX 128
#define HISTORY_MAX_SIZE 100
#define UP_ARROW_PRESS 0x48
#define DOWN_ARROW_PRESS 0x50
#define ALT_PRESS 0x38
#define ALT_RELEASE 0xB8
#define KERNEL_OFFSET 0x800000
#define OFFSET_4KB 0x00001000

extern uint8_t attrib;
extern int screen_x;
extern int screen_y;
extern int cap_flag;
void add_to_buffer(char c);

int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_open (const uint8_t* filename);
int32_t terminal_close (int32_t fd);

void keyboard_handler();
void save_terminal();

void initialize_terminal();

typedef struct terminal_instance
{
    int terminal_cursor_x;
    int terminal_cursor_y;
    int terminal_opened;
    char terminal_screen[NUM_ROWS*NUM_COLS*2];
    char command_buffer[128];
    char command_history[HISTORY_MAX_SIZE][BUFFER_MAX_SIZE];    // history of commands...
	int command_history_bytes[HISTORY_MAX_SIZE];
    int buffer_capacity;
    int historyIndex;
	int numSavedCommands;
	int indexWithinHistory;
	int userIndexWithinHistory;
	int firstTimeAccessingHistory;
	int userNumCommandsLeftUp;
	int userNumCommandsLeftDown;
	volatile int allow_terminal_to_read;

	int execute_flag_shell;
	uint8_t attrib;
} terminal_t;

extern terminal_t terminals[3];
terminal_t terminals[3];


#endif 


