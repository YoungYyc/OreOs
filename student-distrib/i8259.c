/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

static unsigned char cached_8259[2] = { 0xff, 0xff };	// active low, so initially mask all interrupts
#define cached_A1 (cached_8259[0])
#define cached_21 (cached_8259[1])
////////////come bac

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/**
void set_irq_mask() {
	outb(cached_A1,0xA1);
	outb(cached_21,0x21);
}
*/

/* Initialize the 8259 PIC */
void i8259_init(void) {
	//int flags;

	outb(0xFF,SLAVE_8259_PORT+1);	// mask all interr.; 0xFF is just all 1 for active low
	outb(0xFF,MASTER_8259_PORT+1);	// mask all interr.; 0xFF is just all 1 for active low

	outb(ICW1,MASTER_8259_PORT);	// send icw1
	outb(ICW2_MASTER,MASTER_8259_PORT+1);	// send icw2
	outb(ICW3_MASTER,MASTER_8259_PORT+1);	// send icw3
	outb(ICW4,MASTER_8259_PORT+1);	// send icw4

	outb(ICW1,SLAVE_8259_PORT);	// send icw1
	outb(ICW2_SLAVE,SLAVE_8259_PORT+1);	// send icw2
	outb(ICW3_SLAVE,SLAVE_8259_PORT+1);	// send icw3
	outb(ICW4,SLAVE_8259_PORT+1);	// send icw4

	//udelay(100);
	int i;	// for loop var
	for (i = 0; i < 1000000; i++);	// wait for pic to init. 1000000 is just an arb. number to force waiting
///*
	outb(cached_A1, SLAVE_8259_PORT+1);	// mask all interr.
	outb(cached_21, MASTER_8259_PORT+1);	// mask all interr
//*/

}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
	int flags;	// flags to save
	
	cli_and_save(flags);	// clear interr., save flags

	if (irq_num < 8)	// if master; 8 is b/c there are 8 irqs for each pic
		cached_21 &= ~(1 << irq_num);	// set this irq for master
	else{
		cached_A1 &= ~(1 << (irq_num-8));	// set this irq for slave; 8 is b/c there are 8 irqs for each pic
		cached_21 &= ~(1 << 2);	// set this irq for master
	}

	outb(cached_A1, SLAVE_8259_PORT+1);	// send to slave pic
	outb(cached_21, MASTER_8259_PORT+1);	// send to master pic

	restore_flags(flags);	// restore flags
	sti();	// set interr.
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {

	int flags;	// flags to save
	
	cli_and_save(flags);	// clear interr. and save flags

	if (irq_num < 8)	// if master; 8 is b/c there are 8 irqs for each pic
		cached_21 |= 1 << irq_num;	// disable this irq for master
	else{
		cached_A1 |= 1 << (irq_num-8);	// diable this irq for slave
		cached_21 |= 1 << 2;	// diable this irq for master
	}

	outb(cached_A1, SLAVE_8259_PORT+1);	// send to slave pic
	outb(cached_21, MASTER_8259_PORT+1);	// send to master pic

	restore_flags(flags);	// restore flags
	sti();	// set interr.
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
	// int flags;	// flags to save
	
	// cli_and_save(flags);	// clear interr.

	if (irq_num < 8)	// master; 8 is b/c there are 8 irqs for each pic
		outb(EOI | irq_num, MASTER_8259_PORT);
	else	{// slave
		outb(EOI | 2, MASTER_8259_PORT);	// send eoi to master
		outb(EOI | (irq_num-8), SLAVE_8259_PORT);	// send eoi to slave; 8 is b/c there are 8 irqs for each pic
	}

	// restore_flags(flags);	// restore flags
	// sti();	// set interr.
}
