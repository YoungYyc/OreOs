////////////control mouse//////////////
#include "mouse.h"
#include "idt.h"
#include "lib.h"
#include "types.h"
#include "i8259.h"

#define X_MAX 79
#define Y_MAX 24
uint8_t mouse_bytes[3];
// uint32_t x;
// uint32_t y;
uint16_t char_buf;
uint16_t select_buf;
// int select_flag;
// int left_but_pre;
int select_x_pos;
int select_y_pos;

void mouse_handler() {
    // from: http://f.osdev.org/viewtopic.php?f=1&t=27984

    send_eoi(12);

    // printf("cycle:%d\n",m_cycle);
     mouse_bytes[m_cycle] = inb(0x60);
     m_cycle++;
    if (m_cycle == 3){
        m_cycle = 0;
        //printf("mouse 3 bytes get\n");
        // printf("byte1:0x%x\n",(unsigned int)mouse_bytes[0] );
        // printf("byte2:0x%x\n",(unsigned int)mouse_bytes[1] );
        // printf("byte3:0x%x\n",(unsigned int)mouse_bytes[2] );
        // if (!(mouse_bytes[1] & 0x20)){
        //     y |= 0xFFFFFF00;
        //     //printf("y is negative\n");
        // }
        // if (!(mouse_bytes[1] & 0x10)){
        //     x |= 0xFFFFFF00;
        //     //printf("x is negative\n");
        // }
        // if (mouse_bytes[1] & 0x4)
        //     puts("Middle button is pressed!\n");
        // if (mouse_bytes[1] & 0x2)
        //     puts("Right button is pressed!\n");
        //  if (mouse_bytes[1] & 0x1)
        //     puts("Left button is pressed!\n");
          // m_pos_x++;
          // m_pos_y++;
          // if(m_pos_x >= X_MAX)  m_pos_x = 0;
          // if(m_pos_y >= Y_MAX)  m_pos_y = 0;
          //  WriteCharacter(7, 8, m_pos_x, m_pos_y);
          update_mouse(mouse_bytes[1], mouse_bytes[2], mouse_bytes[0]);   
    //       if(mouse_bytes[1] & 0x1){
    //           unselectCharacter( select_buf, select_x_pos, select_y_pos);

    //           select_y_pos = m_pos_y;
    //           select_x_pos = m_pos_x;

    //           selectCharacter(0, 6, select_x_pos, select_y_pos);

    //       }
    }
}

void update_mouse(uint8_t status, uint8_t delta_x, uint8_t delta_y){
    //update x position

        // printf("status: 0x%x\n", (int)status);
        // printf("delta_y_bef: 0x%x\n", (int)delta_y);
      if(!(status&0x1)){
        // select_x_pos = m_pos_x;
        // select_y_pos = m_pos_y;
        restoreCharacter( char_buf,m_pos_x, m_pos_y);
      }
      else{

        restoreCharacter( char_buf,select_x_pos, select_y_pos);
        select_x_pos = m_pos_x;
        select_y_pos = m_pos_y;

      }
        //update y postion
        if (status & 0x20){
            delta_y = 0xFF - delta_y + 1;
            if(delta_y > 1 && delta_y <= 10)
              m_pos_y += 1;
            else if(delta_y > 10 && delta_y <= 25)
              m_pos_y += 2;
            else if(delta_y > 25 && delta_y <=127)
              m_pos_y += 3;
        //     //printf("y is negative\n");
        }
        else{  
            if(delta_y > 1 && delta_y <= 10)
              m_pos_y -= 1;
            else if(delta_y > 10 && delta_y <= 25)
              m_pos_y -= 2;
            else if(delta_y > 25 && delta_y <= 127)
              m_pos_y -= 3;
        }
        // printf("delta_y_aft: 0x%x\n", (int)delta_y);

        // printf("delta_x_bef: 0x%x\n", (int)delta_x);
    //update x postion
        if (status & 0x10){
            delta_x = 0xFF - delta_x + 1;
            if(delta_x > 1 && delta_x <= 10)
              m_pos_x -= 1;
            else if(delta_x > 10 && delta_x <= 25)
              m_pos_x -= 4;
            else if(delta_x > 25 && delta_x <= 127)
              m_pos_x -= 7;
    //         //printf("x is negative\n");
        }
        else{   
            if(delta_x > 1 && delta_x <= 10)
              m_pos_x += 1;
            else if(delta_x > 10 && delta_x <= 25)
              m_pos_x += 4;
            else if(delta_x > 25 && delta_x <= 127)
              m_pos_x += 7;
        }
    //     printf("delta_x_aft: 0x%x\n", (int)delta_x);

          // m_pos_x++;
          // m_pos_y++;
          if(m_pos_x >= X_MAX)  m_pos_x = X_MAX;
          if(m_pos_x <= 0)      m_pos_x = 0;
          if(m_pos_y >= Y_MAX)  m_pos_y = Y_MAX;
          if(m_pos_y <= 0)      m_pos_y = 0;
          WriteCharacter(0, 6, m_pos_x, m_pos_y);   
}

// from http://forum.osdev.org/viewtopic.php?t=10247
void mouse_wait(unsigned char t) //unsigned char
{
  int timeout=100000; //unsigned int
  if(t==0)
  {
    while(timeout--) //Data
    {
      if((inb(0x64) & 1)==1)
      {
        return;
      }
    }
    return;
  }
  else
  {
    while(timeout--) //Signal
    {
      if((inb(0x64) & 2)==0)
      {
        return;
      }
    }
    return;
  }
}

// from http://forum.osdev.org/viewtopic.php?t=10247 and http://f.osdev.org/viewtopic.php?f=1&t=27984
void initialize_mouse() {


    // outb(0xA8,0x64);    // added.................

    // outb(0xD4,0x64);
    // // Write it
    // outb(0xF6,0x60);

    // while (inb(0x60) != 0xFA); // Wait for ACK from mouse...

    // outb(0xD4,0x64);
    // // Write it
    // outb(0xF4,0x60);

    // while (inb(0x60) != 0xFA); // Wait for ACK from mouse...
   
    // // Tell mouse to enable interrupts (IRQ12)
    // outb(0x20,0x64);
       
    // unsigned char res = inb(0x60);
    // res |= 1 << 1;
    // res &= 0xDF;
       
    // outb(0x60,0x64);
    // outb(res,0x60);

    mouse_wait(1);
    outb(0xA8,0x64);
    // while (inb(0x60) != 0xFA);

    mouse_wait(1);
    outb(0x20,0x64);
    // while (inb(0x60) != 0xFA);

    mouse_wait(0);
    unsigned char res = (inb(0x60) /**& 0xDF*/) | 2;

    mouse_wait(1);
    outb(0x60,0x64);

    mouse_wait(1);
    outb(res,0x60);

    // mouse write
    mouse_wait(1);
    outb(0xD4,0x64);

    mouse_wait(1);
    outb(0xF6,0x60);
    while (inb(0x60) != 0xFA);

    mouse_wait(0);
    inb(0x60);

    // mouse write
    mouse_wait(1);
    outb(0xD4,0x64);

    mouse_wait(1);
    outb(0xF4,0x60);
    while (inb(0x60) != 0xFA);

    mouse_wait(0);
    inb(0x60);

    //initialize mouse parameter
    m_pos_x = 20;
    m_pos_y = 20;
    m_cycle = 0;
    left_but_pre = 0;
    // select_flag = 0;
    select_buf = (0x0 << 4) | (0x6 & 0x0F);
}

//code source: http://wiki.osdev.org/Text_UI
void WriteCharacter( unsigned char forecolour, unsigned char backcolour, int x, int y)
{
      // printf("write x,y:%d %d\n", m_pos_x, m_pos_y );
     uint16_t attrib = (backcolour << 4) | (forecolour & 0x0F);
     volatile uint16_t * where;
     where = (volatile uint16_t *)0xB8000 + (y * 80 + x) ;
     // printf("0x%x\n",(int)((*where) >> 8) );
     char_buf = *where;
     char_buf &= 0xFF00;
     // if(left_but_pre == 1){
     //    select_y_pos = y;
     //    select_x_pos = x;
     //    select = *where;
     //    select &= 0xFF00;
     //  }
     // printf("char_bef_wri:0x%x\n", char_buf);
     *where |= (attrib << 8);
     // printf("char_aft_wri:0x%x\n", (int)*where);
}

void restoreCharacter( uint16_t prev_char, int x, int y)
{
    // printf("restore x,y:%d %d\n", m_pos_x, m_pos_y );
     volatile uint16_t * where;
     where = (volatile uint16_t *)0xB8000 + (y * 80 + x) ;
     // printf("0x%x\n",(int)((*where) >> 8) );
     *where &= 0x00FF;
     *where |= prev_char;
     // printf("char_aft_res:0x%x\n", (int)*where);
}

// void selectCharacter( unsigned char forecolour, unsigned char backcolour, int x, int y)
// {
//       // printf("write x,y:%d %d\n", m_pos_x, m_pos_y );
//      uint16_t attrib = (backcolour << 4) | (forecolour & 0x0F);
//      volatile uint16_t * where;
//      where = (volatile uint16_t *)0xB8000 + (y * 80 + x) ;
//      // printf("0x%x\n",(int)((*where) >> 8) );
//      select_buf = *where;
//      select_buf &= 0xFF00;
//      // if(left_but_pre == 1){
//      //    select_y_pos = y;
//      //    select_x_pos = x;
//      //    select = *where;
//      //    select &= 0xFF00;
//      //  }
//      // printf("char_bef_wri:0x%x\n", char_buf);
//      *where |= (attrib << 8);
//      // printf("char_aft_wri:0x%x\n", (int)*where);
// }

// void unselectCharacter( uint16_t prev_char, int x, int y)
// {
//     // printf("restore x,y:%d %d\n", m_pos_x, m_pos_y );
//      volatile uint16_t * where;
//      where = (volatile uint16_t *)0xB8000 + (y * 80 + x) ;
//      // printf("0x%x\n",(int)((*where) >> 8) );
//      *where &= 0x00FF;
//      *where |= prev_char;
//      // printf("char_aft_res:0x%x\n", (int)*where);
// }




