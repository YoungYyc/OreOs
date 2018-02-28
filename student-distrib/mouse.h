////////////control mouse//////////////

#ifndef MOUSE_H
#define MOUSE_H

#include "idt.h"
#include "lib.h"
#include "types.h"
#include "i8259.h"

extern int m_pos_x;
extern int m_pos_y;
extern uint8_t m_cycle;
extern int left_but_pre;

void mouse_handler();
void mouse_wait(unsigned char t);
void initialize_mouse();
void WriteCharacter(unsigned char forecolour, unsigned char backcolour, int x, int y);
void update_mouse(uint8_t status, uint8_t delta_x, uint8_t delta_y);
void restoreCharacter( uint16_t prev_char, int x, int y);
// void selectCharacter( unsigned char forecolour, unsigned char backcolour, int x, int y);
// void unselectCharacter( uint16_t prev_char, int x, int y);

#endif

