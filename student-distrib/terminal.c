#define KEYBOARD_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#include "terminal.h"
#include "idt.h"
#include "lib.h"
#include "types.h"
#include "i8259.h"
#include "x86_desc.h"
#include "paging.h"
#include "init_paging.h"
#include "scheduler.h"
#include "system_calls.h"
#include "paging_structure.h"

#define WINDOWS 1

#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25
#define MAGIC_SLOPPY 10

#define MAGIC_ERASE 7

#define WHITE 7

#define NUM_TERMINALS 3

#define VIDMEM 0xB8

#if (WINDOWS)
    #define F1_PRESS 0x3B   // arrow left: 0x4B
    #define F1_RELEASE 0xBB // arrow left: 0xCB
    #define F2_PRESS 0x3C   // tab: 0x0F
    #define F2_RELEASE 0xBC // tab: 0x8F
    #define F3_PRESS 0x3D // arrow right: 0x4D
    #define F3_RELEASE 0xBD // arrow right: 0xCD
#else
// MAC:
    #define F1_PRESS 0x4B   // left
    #define F1_RELEASE 0xCB     // left
    #define F2_PRESS 0x0F   // tab
    #define F2_RELEASE 0x8F // tab
    #define F3_PRESS 0x4D   // right
    #define F3_RELEASE 0xCD  // right
#endif

// char command_buffer[BUFFER_MAX_SIZE];
// char command_history[HISTORY_MAX_SIZE][BUFFER_MAX_SIZE];    // history of commands...
// int command_history_bytes[HISTORY_MAX_SIZE];

// int buffer_capacity = 0;


static char* video_mem = (char *)VIDEO;

int capsLockPressed = 0;
int capsLock = 0;
int shiftPressed = 0;
int ctlPressed = 0;
int altPressed = 0;
int f1Pressed = 0;
int f2Pressed = 0;
int f3Pressed = 0;

int counter = 0;

int active_terminal_num = 0;

int calledCounter = 0;

int sloppy_1_counter = 7; 
int sloppy_2_counter = 7; 


uint8_t tempVid[3][80*25*2];
int tempFlag[3];
int tempCursorX[3];
int tempCursorY[3];
// int historyIndex = 0;
// int numSavedCommands = 0;
// int indexWithinHistory = 0;
// int userIndexWithinHistory = 0;
// int firstTimeAccessingHistory = 1;
// int userNumCommandsLeftUp = 0;
// int userNumCommandsLeftDown = 0;

// int opened_terminals[3] = {1,0,0};

// int terminal_cursor_x[3] = {0,0,0};
// int terminal_cursor_y[3] = {0,0,0};

// char terminal_screen[3][NUM_ROWS*NUM_COLS*2];


// keyboard mapping and array from: http://www.osdever.net/bkerndev/Docs/keyboard.htm reference site
unsigned char keyboard_map[SCANCODE_MAX] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
  '9', '0', '-', '=', '\b', /* Backspace */
  0,         /* Tab */
  'q', 'w', 'e', 'r',   /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
    0,          /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
 '\'', '`',   0,        /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
  'm', ',', '.', '/',   0,              /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

// scan code array for caps lock
unsigned char keyboard_map_caps[SCANCODE_MAX] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
  '9', '0', '-', '=', '\b', /* Backspace */
  0,         /* Tab */
  'Q', 'W', 'E', 'R',   /* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', /* Enter key */
    0,          /* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', /* 39 */
 '\'', '`',   0,        /* Left shift */
 '\\', 'Z', 'X', 'C', 'V', 'B', 'N',            /* 49 */
  'M', ',', '.', '/',   0,              /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

// scancode array for shift
unsigned char keyboard_map_shift[SCANCODE_MAX] =
{
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', /* 9 */
  '(', ')', '_', '+', '\b', /* Backspace */
  0,         /* Tab */
  'Q', 'W', 'E', 'R',   /* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', /* Enter key */
    0,          /* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 39 */
 '\"', '~',   0,        /* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',            /* 49 */
  'M', '<', '>', '?',   0,              /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

// scancode array for shift
unsigned char keyboard_map_shift_caps[SCANCODE_MAX] =
{
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', /* 9 */
  '(', ')', '_', '+', '\b', /* Backspace */
  0,         /* Tab */
  'q', 'w', 'e', 'r',   /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n', /* Enter key */
    0,          /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', /* 39 */
 '\"', '~',   0,        /* Left shift */
 '|', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
  'm', '<', '>', '?',   0,              /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};


/* void initialize_terminal();
 *   Inputs: none
 *   Return Value: none
 *    Function: initialize each of three terminals*/
void initialize_terminal() {
    int i,j;

    for (i = 0; i < NUM_TERMINALS; i++) {
        tempFlag[i] = 0;

        terminals[i].terminal_cursor_x = 0;
        terminals[i].terminal_cursor_y = 0;
        terminals[i].terminal_opened = 0;

        terminals[i].buffer_capacity = 0;
        terminals[i].historyIndex = 0;
        terminals[i].numSavedCommands = 0;
        terminals[i].indexWithinHistory = 0;
        terminals[i].userIndexWithinHistory = 0;
        terminals[i].firstTimeAccessingHistory = 1;
        terminals[i].userNumCommandsLeftUp = 0;
        terminals[i].userNumCommandsLeftDown = 0;

        terminals[i].allow_terminal_to_read = 0;

        terminals[i].execute_flag_shell = 0;

        terminals[i].attrib = WHITE;    // white text color for terminal


        for (j = 0; j < NUM_ROWS * NUM_COLS * 2; j++) {
            if (j % 2 == 0)
                terminals[i].terminal_screen[j] = ' ';
            else
                terminals[i].terminal_screen[j] = attrib;
        }
    }

    terminals[0].terminal_opened = 1;

    tempFlag[0] = 1;
}

/* void update_history();
 *   Inputs: none
 *   Return Value: none
 *    Function: Updates the history of command after newline char*/
void update_history() {
    int i;

    if (terminals[screen_process].numSavedCommands == HISTORY_MAX_SIZE-1) {
        for (i = 0; i < BUFFER_MAX_SIZE; i++) {
            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
        }

        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

        terminals[screen_process].indexWithinHistory = (terminals[screen_process].indexWithinHistory + 1) % HISTORY_MAX_SIZE;
        terminals[screen_process].userIndexWithinHistory = terminals[screen_process].indexWithinHistory;

        terminals[screen_process].numSavedCommands++;
    }else if (terminals[screen_process].numSavedCommands == HISTORY_MAX_SIZE) {
        for (i = 0; i < BUFFER_MAX_SIZE; i++) {
            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
        }

        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

        terminals[screen_process].indexWithinHistory = (terminals[screen_process].indexWithinHistory + 1) % HISTORY_MAX_SIZE;
        terminals[screen_process].userIndexWithinHistory = terminals[screen_process].indexWithinHistory;
    }else {
        for (i = 0; i < BUFFER_MAX_SIZE; i++) {
            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
        }

        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

        terminals[screen_process].indexWithinHistory++;
        terminals[screen_process].userIndexWithinHistory = terminals[screen_process].indexWithinHistory;
        if (terminals[screen_process].firstTimeAccessingHistory)
            terminals[screen_process].numSavedCommands++;
    }

    terminals[screen_process].firstTimeAccessingHistory = 1;
    terminals[screen_process].userNumCommandsLeftUp = terminals[screen_process].numSavedCommands;
    terminals[screen_process].userNumCommandsLeftDown = 0;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putd(uint8_t c, int id) {
    // if(c == '\n' || c == '\r') {
    //     screen_y++;
    //     screen_x = 0;
    // } else {
    //     *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
    //     *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib;
    //     screen_x++;
    //     screen_x %= NUM_COLS;
    //     screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
    // }
    int i,j; // for loop variables
    if(c == '\n' || c == '\r') {    // if new line char
        if (terminals[id].terminal_cursor_y == NUM_ROWS-1) {   // if last line
            for (i = 0; i <= NUM_ROWS-1; i++) { // over rows
                for (j = 0; j < NUM_COLS; j++) {    // over cols
                    if (i == NUM_ROWS-1) {  // if last row
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1)) = ' '; // new line so fill in nothing
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1) + 1) = terminals[screen_process].attrib;
                    }else {
                // copy next line to this line to scroll
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * (i+1) + j) << 1));
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1) + 1) = terminals[screen_process].attrib;
                    }
                }
            }

            terminals[id].terminal_cursor_x  = 0;      // scroll screen
        }else {
            terminals[id].terminal_cursor_y++;
            terminals[id].terminal_cursor_x = 0;
        }
    }else {
        *(uint8_t *)(video_mem + ((NUM_COLS * terminals[id].terminal_cursor_y + terminals[id].terminal_cursor_x) << 1)) = c;  // set it to this char
        *(uint8_t *)(video_mem + ((NUM_COLS * terminals[id].terminal_cursor_y + terminals[id].terminal_cursor_x) << 1) + 1) = terminals[screen_process].attrib; // second byte is weird constant


        if (terminals[id].terminal_cursor_x == NUM_COLS-1 && terminals[id].terminal_cursor_y == NUM_ROWS-1) { // if abs. last char of screen
            for (i = 0; i <= NUM_ROWS-1; i++) { // over rows
                for (j = 0; j < NUM_COLS; j++) {    // over cols
                    if (i == NUM_ROWS-1) {  // if last row
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1)) = ' '; // new line so fill in nothing
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1) + 1) = terminals[screen_process].attrib;
                    }else {
                        // copy next line to this line to scroll
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * (i+1) + j) << 1));
                        *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1) + 1) = terminals[screen_process].attrib;
                    }      
                }
            }

            terminals[id].terminal_cursor_x  = 0;      // scroll screen
        }else if (terminals[id].terminal_cursor_x == NUM_COLS-1) { // if last col
            terminals[id].terminal_cursor_x = 0;   // wrap...
            terminals[id].terminal_cursor_y++; // next row
        }else {
            terminals[id].terminal_cursor_x++;
        }
    }
}

/* void add_to_buffer(char c);
 *   Inputs: char to add to line buffer
 *   Return Value: none
 *    Function: updates line buffer appropriately based on char and size of buffer*/
void add_to_buffer(char c) {
    if (c == '\n') {    // if newline char
        terminals[screen_process].command_buffer[terminals[screen_process].buffer_capacity] = c; // could have made this one line but whatever
        terminals[screen_process].buffer_capacity++;  // increment capacity

        if (terminals[screen_process].buffer_capacity > 1) {
            update_history();   // if not newline char only in command, update history of command
        }

    }else {
        if (terminals[screen_process].buffer_capacity < BUFFER_MAX_SIZE-1) {    // if buffer is not "full" yet
            terminals[screen_process].command_buffer[terminals[screen_process].buffer_capacity] = c; // could have made this one line but whatever
            terminals[screen_process].buffer_capacity++;  // increment capacity
        }
    }
}

/* int32_t terminal_read (int32_t fd, char* buf, int32_t nbytes)
 *   Inputs: file id, buffer array to copy to, num of bytes to read
 *   Return Value: number of bytes read
 *    Function: copies command to parameter buf and updates buffer*/
int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes) {
    int i;  // for loop var
    int bytesRead = 0;  // bytes read

    // printf("Terminal read reached\n");

    while (!terminals[current_proc->terminal_id].allow_terminal_to_read);    // wait until allowed to read

    // printf("Got Here %d\n",counter++);
    // loadPageDirectory(current_proc->cr3);

    for (i = 0; i < nbytes; i++) {
        if (terminals[current_proc->terminal_id ].buffer_capacity == 0)   // if no more char to read
            break;

        ((char*)buf)[i] = terminals[current_proc->terminal_id ].command_buffer[i]; // copy to buf array
        bytesRead++;    // inc. bytes read
        terminals[current_proc->terminal_id ].buffer_capacity--;  // dec. size of buffer
    }

    if (terminals[current_proc->terminal_id ].buffer_capacity < 0)    // probably won't reach here, but just in case...
        terminals[current_proc->terminal_id ].buffer_capacity = 0;    // min val is 0 for size
    else if (terminals[current_proc->terminal_id ].buffer_capacity > 0) { // if not all of command buffer was read, shift remaining contents to left
        for (i = 0; i < terminals[current_proc->terminal_id ].buffer_capacity; i++) { // shift remain. contents to left
            terminals[current_proc->terminal_id ].command_buffer[i] = terminals[current_proc->terminal_id ].command_buffer[i+bytesRead];
        }
    }

    terminals[current_proc->terminal_id ].allow_terminal_to_read = 0; // don't allow terminal to read anymore, since we're done for now

    return bytesRead;
}



/* int32_t terminal_write (int32_t fd, char* buf, int32_t nbytes)
 *   Inputs: file id, buffer array to copy from, num of bytes to write
 *   Return Value: number of bytes written
 *    Function: writes to terminal from buf*/
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes) {
    int i;  // for loop var
    int flags;
    int bytesWritten = 0;
    loadPageDirectory(current_proc->cr3);
    cli_and_save(flags);
    // loadPageDirectory(page_directory[screen_process]);

    // char * buf2 = (char *) buf;

    // printf("helllo \n");
    // printf("%#x\n", buf2[0]);
    // printf("%#x\n", buf2[1]);

    // printf("%#x\n", buf2[2]);

    // printf("%#x\n", buf2[3]);


    for (i = 0; i < nbytes; i++) {  // over buffer...
        // if (((char*)buf)[i] == 0)
        //     break;


        if (current_proc -> terminal_id == sloppy_1) {
            sloppy_1 = MAGIC_SLOPPY;
            break;
        }
        if (current_proc -> terminal_id == sloppy_2) {
            sloppy_2 = MAGIC_SLOPPY;
            break;
        }


        if (screen_process == current_proc->terminal_id) putc(((char*)buf)[i]);   // write to terminal
        else putd(((char*)buf)[i], current_proc->terminal_id); 
        bytesWritten++; // increment bytes written
    }
    // loadPageDirectory(current_proc->cr3);
    restore_flags(flags);
    sti();
    // printf("Terminal Write Success: %d\n", bytesWritten);

    return bytesWritten;
}

/* int32_t terminal_open (const uint8_t* filename)
 *   Inputs: file name as int
 *   Return Value: 0 on success
 *    Function: initializes terminal driver*/
int32_t terminal_open (const uint8_t* filename){
    // do nothing... (maybe screen_x, screen_y? probably not needed...)
    // printf("Terminal Open!!!!\n");
    terminals[current_proc->terminal_id].allow_terminal_to_read = 0; // don't allow terminal to read yet

    return 0;
}

/* int32_t terminal_close (int32_t fd)
 *   Inputs: file name as int
 *   Return Value: 0 on success
 *    Function: closes terminal driver*/
int32_t terminal_close (int32_t fd){
    terminals[screen_process].buffer_capacity = 0;    // clear buffer

    terminals[screen_process].allow_terminal_to_read = 0; // set to 0

    return 0;
}


void save_terminal() {
    uint32_t tmp = first_page_table[screen_process][VIDMEM + 1]; 

    terminals[screen_process].terminal_cursor_x = screen_x;
    terminals[screen_process].terminal_cursor_y = screen_y;

    memcpy((void *)(VIDEO + OFFSET_4KB), (void *) VIDEO,NUM_COLS*NUM_ROWS*2);
    first_page_table[screen_process][VIDMEM + 1] = first_page_table[screen_process][VIDMEM];
    first_page_table[screen_process][VIDMEM] = tmp;
    vidmap_page_table[screen_process][0] = ((VID_MEM + screen_process + 1) * OFFSET_4KB) | READ_AND_WRITE | PRESENT | USER;
}

void restore_terminal() {
    uint32_t tmp = first_page_table[screen_process][VIDMEM]; 
    memcpy((void *)(VIDEO + OFFSET_4KB), (void *)VIDEO,NUM_COLS*NUM_ROWS*2);

    screen_x = terminals[screen_process].terminal_cursor_x;
    screen_y = terminals[screen_process].terminal_cursor_y;


    first_page_table[screen_process][VIDMEM] =  first_page_table[screen_process][VIDMEM + 1];
    vidmap_page_table[screen_process][0] = (VID_MEM * OFFSET_4KB) | READ_AND_WRITE | PRESENT | USER;
    first_page_table[screen_process][VIDMEM + 1] = tmp;
    update_cursor(screen_x,screen_y);
}

void restoreTempVid(int num) {
    int32_t i;
    for (i = 0; i < NUM_COLS*NUM_ROWS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = tempVid[num][i << 1];
        *(uint8_t *)(video_mem + (i << 1) + 1) = terminals[screen_process].attrib;
    }
}

void copyToTempVid(int num) {
    int32_t i;
    for (i = 0; i < NUM_COLS*NUM_ROWS; i++) {
        tempVid[num][i << 1] = *(uint8_t *)(video_mem + (i << 1));
        tempVid[num][(i << 1)+1] = terminals[screen_process].attrib;
    }
}

void forceErase() {
    int i;
    for (i = screen_y*NUM_COLS+MAGIC_ERASE;i < screen_y*NUM_COLS+NUM_ROWS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = terminals[screen_process].attrib;
    }
}

/* void keyboard_handler()
 *   Inputs: none
 *   Return Value: none
 *    Function: handles keyboard interrupt*/
// btw, most of the info came from reference sites
// http://arjunsreedharan.org/post/99370248137/kernel-201-lets-write-a-kernel-with-keyboard
// and
// http://wiki.osdev.org/PS/2_Keyboard
void keyboard_handler(){
    
    int temp_buffer_capacity = 0;
    int i;

    send_eoi(1);    // send end of interr.
    int flags;

    loadPageDirectory(page_directory[screen_process]);

    uint32_t keyboardStatus = inb(KEYBOARD_STATUS_PORT);    // read from keyboard port the status

    if ((keyboardStatus & 0x01) == 0x01) {    // check keyboard status; 0x01 is mask: only reason not replaced by #define constant is it would be less intuitive
        //printf("Key: ");
        uint32_t inputCode = inb(KEYBOARD_PORT);     // read the input char

        if (inputCode == CAPS_LOCK_PRESS) {    // pressed caps lock
            if (!capsLockPressed) {
                capsLockPressed = 1;    // caps lock has been pressed

                if (capsLock)
                    capsLock = 0;   // uncheck caps lock
                else
                    capsLock = 1;   // check caps lock
            }
            
        }else if (inputCode == CAPS_LOCK_RELEASE) {  // released caps lock
            capsLockPressed = 0;    // caps lock released
        }else if (inputCode == LEFT_SHIFT_PRESS || inputCode == RIGHT_SHIFT_PRESS) { // left or right shift pressed
            shiftPressed = 1;
        }else if (inputCode == LEFT_SHIFT_RELEASE || inputCode == RIGHT_SHIFT_RELEASE) {  // left or right shift released
            shiftPressed = 0;
        }else if (inputCode == LEFT_CTL_PRESS)  {    // left or right ctl pressed 
            ctlPressed = 1;
        }else if (inputCode == LEFT_CTL_RELEASE)  {    // left or right ctl released
            ctlPressed = 0;
        }

        else if (altPressed && f1Pressed) {

            // altPressed = 0;
            f1Pressed = 0;

            // if (screen_process != 0) {
            //     save_terminal();
            //     screen_process = 0;
            //     restore_terminal();
            // }

            // printf("PREV???\n");

            
            if (screen_process != 0) {
                if(cap_flag){
                    printf("maximum process reached, new shell is not allowed\n");
                    printf("391OS> ");
                    return;
                }
                if (terminals[0].terminal_opened == 0) {
                    terminals[0].terminal_opened = 1;
                    cli_and_save(flags);

                    copyToTempVid(screen_process);
                    
                    tempCursorX[screen_process] = screen_x;
                    tempCursorY[screen_process] = screen_y;

                    tempFlag[0] = 1;

                    add_base(0);
                    asm volatile("                      \n\
                        xorl    %%eax, %%eax            \n\
                        leal    -12(%%ebp), %%esp       \n\
                        popl    %%edi                   \n\
                        popl    %%esi                   \n\
                        popl    %%ebx                   \n\
                        popl    %%ebp                   \n\
                        ret                             \n\
                     "
                    :
                    :
                    :   "memory"
                    );  
                }

                // printf("middle\n");

                save_terminal();
                screen_process = 0;
                loadPageDirectory(page_directory[screen_process]);
                restore_terminal();

                // printf("Yo\n");

                if (tempFlag[0] == 1 && 0) {

                    // printf("Here 0\n");

                    tempFlag[0] = 0;

                    restoreTempVid(0);

                    screen_x = tempCursorX[0];
                    screen_y = tempCursorY[0];
                    update_cursor(screen_x,screen_y);

                    // putc('c');

                    // forceErase();
                    
                }

            }

            // printf("WHAT???\n");

        }else if (altPressed && f2Pressed) {

            // altPressed = 0;
            f2Pressed = 0;

            // if (screen_process != 1) {
            //     save_terminal();
            //     screen_process = 1;
            //     restore_terminal();
            // }
            // printf("PREV???\n");

            if (screen_process != 1) {
                if(cap_flag){
                    printf("maximum process reached, new shell is not allowed\n");
                    printf("391OS> ");
                    return;
                }
                if (terminals[1].terminal_opened == 0) {
                    terminals[1].terminal_opened = 1;
                    cli_and_save(flags);

                    copyToTempVid(screen_process);
                    tempCursorX[screen_process] = screen_x;
                    tempCursorY[screen_process] = screen_y;

                    tempFlag[1] = 1;

                    add_base(1);
                    asm volatile("                      \n\
                        xorl    %%eax, %%eax            \n\
                        leal    -12(%%ebp), %%esp       \n\
                        popl    %%edi                   \n\
                        popl    %%esi                   \n\
                        popl    %%ebx                   \n\
                        popl    %%ebp                   \n\
                        ret                             \n\
                     "
                    :
                    :
                    :   "memory"
                    );  

                    
                }

                // printf("middle\n");

                save_terminal();
                screen_process = 1;
                loadPageDirectory(page_directory[screen_process]);
                restore_terminal();

                if (tempFlag[1] == 1 && 0) {

                    // printf("Here 1\n");

                    tempFlag[1] = 0;

                    restoreTempVid(1);

                    screen_x = tempCursorX[1];
                    screen_y = tempCursorY[1];
                    update_cursor(screen_x,screen_y);

                    // forceErase();

                }
            }

            // printf("WHAT???\n");

        }else if (altPressed && f3Pressed) {

            // altPressed = 0;
            f3Pressed = 0;

            // if (screen_process != 2) {
            //     save_terminal();
            //     screen_process = 2;
            //     restore_terminal();
            // }  

            // printf("PREV???\n");

            if (screen_process != 2) {
                if(cap_flag){
                    printf("maximum process reached, new shell is not allowed\n");
                    printf("391OS> ");
                    return;
                }
                if (terminals[2].terminal_opened == 0) {
                    terminals[2].terminal_opened = 1;
                    cli_and_save(flags);

                    copyToTempVid(screen_process);

                    tempCursorX[screen_process] = screen_x;
                    tempCursorY[screen_process] = screen_y;

                    tempFlag[2] = 1;

                    add_base(2);
                    asm volatile("                      \n\
                        xorl    %%eax, %%eax            \n\
                        leal    -12(%%ebp), %%esp       \n\
                        popl    %%edi                   \n\
                        popl    %%esi                   \n\
                        popl    %%ebx                   \n\
                        popl    %%ebp                   \n\
                        ret                             \n\
                     "
                    :
                    :
                    :   "memory"
                    );  
                }

                // printf("middle\n");

                save_terminal();
                screen_process = 2;
                loadPageDirectory(page_directory[screen_process]);
                restore_terminal();

                if (tempFlag[2] == 1 && 0) {

                    // printf("Here 2\n");

                    tempFlag[2] = 0;

                    restoreTempVid(2);

                    screen_x = tempCursorX[2];
                    screen_y = tempCursorY[2];
                    update_cursor(screen_x,screen_y);

                    // forceErase();
                }
            }          

            // printf("WHAT???\n");

        }else if (inputCode == ALT_PRESS) {
            altPressed = 1;
        }else if (inputCode == ALT_RELEASE) {
            altPressed = 0;
        }else if (inputCode == F1_PRESS) {
            f1Pressed = 1;
        }else if (inputCode == F1_RELEASE) {
            f1Pressed = 0;
        }else if (inputCode == F2_PRESS) {
            f2Pressed = 1;
        }else if (inputCode == F2_RELEASE) {
            f2Pressed = 0;
        }else if (inputCode == F3_PRESS) {
            f3Pressed = 1;
        }else if (inputCode == F3_RELEASE) {
            f3Pressed = 0;
        }else if (inputCode == BACKSPACE_PRESS) {    // backspace pressed
            if (terminals[screen_process].buffer_capacity > 0) {

                if (screen_x == 0 && screen_y == 0) {   // if very first char
                    // do nothing
                }else if (screen_x == 0) {  // if first col but not first row
                    screen_x = NUM_COLS-1;  // wrap around
                    screen_y--; // last row
                }else {
                    screen_x--; // go back one char
                }

                char* video_mem = (char *)VIDEO;    // vid mem ptr

                *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';    // clear prev char
                *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = terminals[screen_process].attrib;

                update_cursor(screen_x,screen_y);   // update cursor

                terminals[screen_process].buffer_capacity--;  // min is 0 size
            }
        }else if (inputCode == ENTER_PRESS) {  // enter pressed
            putc('\n');

            add_to_buffer('\n');    // add newline to buffer

            terminals[screen_process].allow_terminal_to_read = 1; // allow terminal to read (syscall)
        }else if (inputCode == UP_ARROW_PRESS) {    // up pressed

            if (terminals[screen_process].numSavedCommands > 0 && terminals[screen_process].userNumCommandsLeftUp >= 1) {
                if (terminals[screen_process].firstTimeAccessingHistory) {    // if this is first time accessing history

                    terminals[screen_process].firstTimeAccessingHistory = 0;

                    if (terminals[screen_process].numSavedCommands == HISTORY_MAX_SIZE) { // if max size commands

                        for (i = 0; i < terminals[screen_process].buffer_capacity; i++) {     // copy to command history
                            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
                        }
                        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

                        terminals[screen_process].userNumCommandsLeftUp--;
                        // userNumCommandsLeftDown++;

                    }else {
                        for (i = 0; i < terminals[screen_process].buffer_capacity; i++) { // copy
                            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
                        }
                        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

                        terminals[screen_process].numSavedCommands++;
                    }
                    
                }else if (terminals[screen_process].userNumCommandsLeftDown == 0) {   // if this was latest command
                    if (terminals[screen_process].numSavedCommands == HISTORY_MAX_SIZE) {

                        for (i = 0; i < terminals[screen_process].buffer_capacity; i++) {
                            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
                        }
                        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

                    }else {
                        for (i = 0; i < terminals[screen_process].buffer_capacity; i++) {
                            terminals[screen_process].command_history[terminals[screen_process].indexWithinHistory][i] = terminals[screen_process].command_buffer[i];
                        }
                        terminals[screen_process].command_history_bytes[terminals[screen_process].indexWithinHistory] = terminals[screen_process].buffer_capacity;

                    }
                }

                terminals[screen_process].userIndexWithinHistory = (terminals[screen_process].userIndexWithinHistory - 1 == -1) ? HISTORY_MAX_SIZE-1 : (terminals[screen_process].userIndexWithinHistory - 1);
                terminals[screen_process].userNumCommandsLeftUp--;
                terminals[screen_process].userNumCommandsLeftDown++;

                temp_buffer_capacity = terminals[screen_process].buffer_capacity;

                // erase...
                while (temp_buffer_capacity > 0) {
                    if (screen_x == 0 && screen_y == 0) {   // if very first char
                        // do nothing
                    }else if (screen_x == 0) {  // if first col but not first row
                        screen_x = NUM_COLS-1;  // wrap around
                        screen_y--; // last row
                    }else {
                        screen_x--; // go back one char
                    }

                    char* video_mem = (char *)VIDEO;    // vid mem ptr

                    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';    // clear prev char
                    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = terminals[screen_process].attrib;

                    update_cursor(screen_x,screen_y);   // update cursor

                    temp_buffer_capacity--;  // min is 0 size
                }

                // copy previous command to screen
                for (i = 0; i < terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory]; i++) {
                    if (terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][i] == '\n') {
                        break;
                    }

                    putc(terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][i]);

                    terminals[screen_process].command_buffer[i] = terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][i];
                }

                // see if newline char is present in command history
                if (terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory]-1] == '\n')
                    terminals[screen_process].buffer_capacity = terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory]-1;
                else
                    terminals[screen_process].buffer_capacity = terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory];
            }
        }else if (inputCode == DOWN_ARROW_PRESS) {  // down pressed
            if (terminals[screen_process].numSavedCommands > 0 && terminals[screen_process].userNumCommandsLeftDown >= 1) { // if able to process down arrow

                terminals[screen_process].userIndexWithinHistory = (terminals[screen_process].userIndexWithinHistory + 1 == HISTORY_MAX_SIZE) ? 0 : (terminals[screen_process].userIndexWithinHistory + 1);
                terminals[screen_process].userNumCommandsLeftUp++;
                terminals[screen_process].userNumCommandsLeftDown--;

                temp_buffer_capacity = terminals[screen_process].buffer_capacity;

                // erase...
                while (temp_buffer_capacity > 0) {
                    if (screen_x == 0 && screen_y == 0) {   // if very first char
                        // do nothing
                    }else if (screen_x == 0) {  // if first col but not first row
                        screen_x = NUM_COLS-1;  // wrap around
                        screen_y--; // last row
                    }else {
                        screen_x--; // go back one char
                    }

                    char* video_mem = (char *)VIDEO;    // vid mem ptr

                    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';    // clear prev char
                    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = terminals[screen_process].attrib;

                    update_cursor(screen_x,screen_y);   // update cursor

                    temp_buffer_capacity--;  // min is 0 size
                }

                // copy to screen the next command
                for (i = 0; i < terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory]; i++) {
                    if (terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][i] == '\n') {
                        break;
                    }

                    putc(terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][i]);

                    terminals[screen_process].command_buffer[i] = terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][i];
                }

                // see if newline char is present in command history
                if (terminals[screen_process].command_history[terminals[screen_process].userIndexWithinHistory][terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory]-1] == '\n')
                    terminals[screen_process].buffer_capacity = terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory]-1;
                else
                    terminals[screen_process].buffer_capacity = terminals[screen_process].command_history_bytes[terminals[screen_process].userIndexWithinHistory];
            }
        }else {
            if (shiftPressed == 1) {
                if (capsLock == 1) {
                    if (inputCode < PRESS_RELEASE_BOUNDARY && keyboard_map_shift_caps[inputCode] != NULL_CHAR && terminals[screen_process].buffer_capacity < BUFFER_MAX_SIZE-1)   {// if not release key
                        putc(keyboard_map_shift_caps[inputCode]);  // write to terminal
                        add_to_buffer(keyboard_map_shift_caps[inputCode]);   // add to buffer
                    }
                }else {
                    if (inputCode < PRESS_RELEASE_BOUNDARY && keyboard_map_shift[inputCode] != NULL_CHAR && terminals[screen_process].buffer_capacity < BUFFER_MAX_SIZE-1)   {// if not release key
                        putc(keyboard_map_shift[inputCode]);  // write to terminal
                        add_to_buffer(keyboard_map_shift[inputCode]);   // add to buffer
                    }
                }
            }else if (ctlPressed == 1) {
                if (inputCode == L_PRESS) {    // (ctl) L: clear screen
                    clear();    // clear screen
                    screen_x = 0;
                    screen_y = 0;
                }

                terminals[screen_process].buffer_capacity = 0;    // buffer has nothing in it now

                update_cursor(screen_x,screen_y);   // update cursor

            }else if (capsLock == 1) {
                if (inputCode < PRESS_RELEASE_BOUNDARY && keyboard_map_caps[inputCode] != NULL_CHAR && terminals[screen_process].buffer_capacity < BUFFER_MAX_SIZE-1)   {// if not release key
                    putc(keyboard_map_caps[inputCode]);  // write to terminal
                    add_to_buffer(keyboard_map_caps[inputCode]);
                }

                
            }else {
                if (inputCode < PRESS_RELEASE_BOUNDARY && keyboard_map[inputCode] != NULL_CHAR && terminals[screen_process].buffer_capacity < BUFFER_MAX_SIZE-1)   {// if not release key
                    putc(keyboard_map[inputCode]);  // write to terminal
                    add_to_buffer(keyboard_map[inputCode]);

/*                        if (keyboard_map[inputCode] == 'j') {
                            if (screen_process != 0) {
                                save_terminal();
                                screen_process = 0;
                                loadPageDirectory(page_directory[screen_process]);
                                restore_terminal();
                            }
                        }

                        if (keyboard_map[inputCode] == 'k') {
                            if (screen_process != 1) {
                                save_terminal();
                                screen_process = 1;
                                loadPageDirectory(page_directory[screen_process]);
                                restore_terminal();
                            }
                        }

                        if (keyboard_map[inputCode] == 'l') {                    
                            if (screen_process != 2) {
                                save_terminal();
                                screen_process = 2;
                                loadPageDirectory(page_directory[screen_process]);
                                restore_terminal();
                            }
                        }
*/
                }
            }
        }
    }

    loadPageDirectory(current_proc->cr3);


    // restore_flags(flags);
    // sti();
}

