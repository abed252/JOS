#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>
#include <inc/types.h>

extern volatile uint32_t *e1000;

#define E1000_STATUS 0x00008

#define E1000_TXD_STAT_DD 0x1 // Descriptor Done

// Intel 82540EM device identifiers
#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100e

// Transmit Ring Size (must be multiple of 8 and <= 64)
#define TX_RING_SIZE 64

// Register Offsets
#define E1000_TDBAL 0x03800 // TX Descriptor Base Low
#define E1000_TDBAH 0x03804 // TX Descriptor Base High
#define E1000_TDLEN 0x03808 // TX Descriptor Length
#define E1000_TDH 0x03810   // TX Descriptor Head
#define E1000_TDT 0x03818   // TX Descriptor Tail
#define E1000_TCTL 0x00400  // TX Control
#define E1000_TIPG 0x00410  // TX Inter Packet Gap

// TCTL bits
#define E1000_TCTL_EN 0x00000002  // Transmit Enable
#define E1000_TCTL_PSP 0x00000008 // Pad Short Packets
#define E1000_TCTL_COLD_FULL_DUPLEX (0x40 << 12)

// TIPG default values (IEEE 802.3)
#define E1000_TIPG_IPGT 10
#define E1000_TIPG_IPGR1 8
#define E1000_TIPG_IPGR2 6


//
#define E1000_TXD_CMD_EOP 0x1
#define E1000_TXD_CMD_RS 0x8

#define TX_PKT_SIZE 1518



///////////////////////////////////////
#define RX_RING_SIZE 128 // or more
#define RX_BUF_SIZE 2048 // see datasheet

// RX register offsets
#define E1000_RDBAL 0x02800
#define E1000_RDBAH 0x02804
#define E1000_RDLEN 0x02808
#define E1000_RDH   0x02810
#define E1000_RDT   0x02818
#define E1000_RCTL  0x00100
#define E1000_RAL0  0x05400
#define E1000_RAH0  0x05404
#define E1000_RAH_AV 0x80000000

// RCTL bits
#define E1000_RCTL_EN     0x00000002
#define E1000_RCTL_BAM    0x00008000
#define E1000_RCTL_SECRC  0x04000000
#define E1000_RCTL_BSIZE_2048 0x00000000 // 00 = 2048 bytes







// RX descriptor status bits
#define E1000_RXD_STAT_DD   0x01  // Descriptor Done
#define E1000_RXD_STAT_EOP  0x02  // End of Packet

// Interrupt registers and bits (from SDM)
#define E1000_ICR   0x000C0 // Interrupt Cause Read
#define E1000_IMS   0x000D0 // Interrupt Mask Set/Read
#define E1000_IMC   0x000D8 // Interrupt Mask Clear
#define E1000_ICS   0x000C8 // Interrupt Cause Set
#define E1000_ICR_RXT0 0x80 // RX timer interrupt (default)
#define E1000_ICR_RXO  0x40 // RX overrun
#define E1000_ICR_TXDW 0x1  // TX descriptor written back


#define E1000_EERD          0x00014   // EEPROM Read register
#define E1000_EERD_START    0x00000001
#define E1000_EERD_DONE     0x00000010
#define E1000_EERD_ADDR_SHIFT 8




int e1000_recv(void *dst, size_t maxlen); // prototype for receive syscall
void e1000_intr(void);
void e1000_get_mac(uint8_t dst[6]);

// RX Descriptor struct
struct rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t csum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));


// Transmit Descriptor
struct tx_desc
{
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));



int e1000_attach(struct pci_func *pcif);

int e1000_transmit(void *data, size_t len);

#endif	// JOS_KERN_E1000_H
