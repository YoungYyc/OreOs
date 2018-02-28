#include "e1000.h"
#include "init_paging.h"

#define INTEL_VEND     0x8086  // Vendor ID for Intel 
#define E1000_DEV      0x100E  // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
 
 
// http://wiki.osdev.org/Intel_Ethernet_i217
 
#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014
#define REG_CTRL_EXT    0x0018
#define REG_IMASK       0x00D0
#define REG_RCTRL       0x0100
#define REG_RXDESCLO    0x2800
#define REG_RXDESCHI    0x2804
#define REG_RXDESCLEN   0x2808
#define REG_RXDESCHEAD  0x2810
#define REG_RXDESCTAIL  0x2818
 
#define REG_TCTRL       0x0400
#define REG_TXDESCLO    0x3800
#define REG_TXDESCHI    0x3804
#define REG_TXDESCLEN   0x3808
#define REG_TXDESCHEAD  0x3810
#define REG_TXDESCTAIL  0x3818
 
 
#define REG_RDTR         0x2820 // RX Delay Timer Register
#define REG_RXDCTL       0x3828 // RX Descriptor Control
#define REG_RADV         0x282C // RX Int. Absolute Delay Timer
#define REG_RSRPD        0x2C00 // RX Small Packet Detect Interrupt
 
 
 
#define REG_TIPG         0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU        0x40        //set link up
 
 
#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC
 
// Buffer Sizes
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))
 
 
// Transmit Command
 
#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable
 
 
// TCTL Register
 
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision
 
#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3) 

   // Transmit Underrun
#define E1000_NUM_RX_DESC 	32
#define E1000_NUM_TX_DESC 	8
#define BASE_RX 			(NW_PD * OFFSET_4MB)
#define BASE_TX				(BASE_RX + OFFSET_4MB/2)

uint32_t  mem_base = 0xfebc0000;   // BAR0
uint8_t mac [6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};      // A buffer for storing the mack address
struct e1000_rx_desc * * rx_descs = (struct e1000_rx_desc * *) (BASE_RX + 16 * (E1000_NUM_RX_DESC + 1)); // Receive Descriptor Buffers
struct e1000_tx_desc * * tx_descs = (struct e1000_tx_desc * *) (BASE_TX + 16 * (E1000_NUM_TX_DESC + 1)); // Transmit Descriptor Buffers
uint16_t rx_cur;      // Current Receive Descriptor Buffer
uint16_t tx_cur;      // Current Transmit Descriptor Buffer


void writeCommand(uint32_t p_address, uint32_t p_value){
	 (*((volatile uint32_t*)(p_address + mem_base)))=(p_value);
}

uint32_t readCommand(uint32_t p_address){
	return *((volatile uint32_t*)(p_address + mem_base));
}

void rxinit()
{
	int i;
    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
    for(i = 0; i < E1000_NUM_RX_DESC; i++)
    {
        rx_descs[i] = (struct e1000_rx_desc *)(BASE_RX + i * 16);
        rx_descs[i]->addr = (uint64_t)(BASE_RX + 16 * (E1000_NUM_RX_DESC + 1) + 8 * E1000_NUM_RX_DESC + (8192 + 16) * i);			
        rx_descs[i]->status = 0;
    }
 
    writeCommand(REG_TXDESCLO, BASE_RX);
    writeCommand(REG_TXDESCHI, 0);
 
    writeCommand(REG_RXDESCLO, BASE_RX);
    writeCommand(REG_RXDESCHI, 0);
 
    writeCommand(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);
 
    writeCommand(REG_RXDESCHEAD, 0);
    writeCommand(REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
    rx_cur = 0;
    writeCommand(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048);
}

void txinit()
{    
    int i;
    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
    for(i = 0; i < E1000_NUM_TX_DESC; i++)
    {
        tx_descs[i] = (struct e1000_tx_desc *)(BASE_TX + i * 16);
        tx_descs[i]->addr = 0;
        tx_descs[i]->cmd = 0;
        tx_descs[i]->status = TSTA_DD;
    }
 
    writeCommand(REG_TXDESCHI, 0);
    writeCommand(REG_TXDESCLO, BASE_TX);
 
 
    //now setup total length of descriptors
    writeCommand(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);
 
 
    //setup numbers
    writeCommand( REG_TXDESCHEAD, 0);
    writeCommand( REG_TXDESCTAIL, 0);
    tx_cur = 0;
    writeCommand(REG_TCTRL,  TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC);
}

void enableInterrupt()
{
    writeCommand(REG_IMASK ,0x1F6DC);
    writeCommand(REG_IMASK ,0xff & ~4);
    readCommand(0xc0);
}

void initialize_e1000()
{
	int i;

    // startLink();

    for(i = 0; i < 0x80; i++) {
        writeCommand(0x5200 + i*4, 0);
    }

    rxinit();
    txinit(); 
    enableInterrupt();
}


void fire ()
{
    uint32_t status = readCommand(0xc0);
    if(status & 0x04)
    {
        // startLink();
    }
    else if(status & 0x10)
    {
           // good threshold
    }
    else if(status & 0x80)
    {
            handleReceive();
    }
}
 
void handleReceive()
{
    uint16_t old_cur;
    int got_packet = 0;
 
    while((rx_descs[rx_cur]->status & 0x1))
    {
        got_packet = 1;
        // uint8_t *buf = (uint8_t *)(uint32_t)rx_descs[rx_cur]->addr;
        // uint16_t len = rx_descs[rx_cur]->length;
 
         // Here you should inject the received packet into your network stack
 
 
        rx_descs[rx_cur]->status = 0;
        old_cur = rx_cur;
        rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
        writeCommand(REG_RXDESCTAIL, old_cur);
    }    
}

int sendPacket(const void * p_data, uint16_t p_len)
{    
	uint8_t old_cur = tx_cur;
    tx_descs[tx_cur]->addr = (uint64_t)(uint32_t) p_data;
    tx_descs[tx_cur]->length = p_len;
    tx_descs[tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
    tx_descs[tx_cur]->status = 0;
    tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
    writeCommand(REG_TXDESCTAIL, tx_cur);   
    while(!(tx_descs[old_cur]->status & 0xff));    
    return 0;
}
