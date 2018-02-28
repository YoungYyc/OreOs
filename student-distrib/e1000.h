/*Spec:
	user.0: index = 0, type = user, net = 10.0.2.2
	e1000.0: index = 0, type = nic, model = e1000, macaddr = 52:54:00:12:34:56
*/

/*dev: e1000, id ""
irq = 11
mac = 52.54.00.12.34.56
vlan = 0 
netdev = 0 
bootindex = -1
autonegotiation = on 
addr = 03.0
romfile = "efi-e1000.rom"
rombar = 1
multifunction = off
command_serr_enable = on
class Ethernet controller, addr 00:03.0, pci id 8086:100e(sub 1af4:1100)
bar 0: mem at 0xfebc0000 [0xfebdffff]
bar 1: i/o at 0xc00 [0xc03f]
bar 6: mem at 0xffffffffffffffff [0x3fffe]
*/

// http://wiki.osdev.org/Intel_Ethernet_i217
#include "types.h"

struct e1000_rx_desc {
        volatile uint64_t addr;
        volatile uint16_t length;
        volatile uint16_t checksum;
        volatile uint8_t status;
        volatile uint8_t errors;
        volatile uint16_t special;
} __attribute__((packed));
 
struct e1000_tx_desc {
        volatile uint64_t addr;
        volatile uint16_t length;
        volatile uint8_t cso;
        volatile uint8_t cmd;
        volatile uint8_t status;
        volatile uint8_t css;
        volatile uint16_t special;
} __attribute__((packed));

// Send Commands and read results From NICs either using MMIO
void writeCommand(uint32_t p_address, uint32_t p_value);
uint32_t readCommand(uint32_t p_address);


// void starttLink ();           // Start up the network
void rxinit();               // Initialize receive descriptors an buffers
void txinit();               // Initialize transmit descriptors an buffers
void enableInterrupt();      // Enable Interrupts
void handleReceive();        // Handle a packet reception.

void initialize_e1000();                             // perform initialization tasks and starts the driver
void fire();  // This method should be called by the interrupt handler 
int sendPacket(const void * p_data, uint16_t p_len);  // Send a packet

