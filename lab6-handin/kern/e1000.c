#include <kern/e1000.h>
#include <kern/pmap.h> // for mmio_map_region
#include <inc/x86.h>
#include <inc/string.h>
#include <kern/console.h> // for cprintf
#include <kern/env.h> // for envid_t    
//#include <kern/lapic.h> // for lapic_eoi
#include <inc/error.h>
#include <kern/picirq.h> // for irq_eoi
// LAB 6: Your driver code here
/* LAPIC end‑of‑interrupt routine lives in kern/lapic.c; declare manually */
extern void lapic_eoi(void);

/* -------------------------------------------------------------------------
 * Local error codes (not present in inc/error.h)
 * -------------------------------------------------------------------------*/
#ifndef E_AGAIN
#define E_AGAIN     1000
#endif
#ifndef E_NOT_SUPP
#define E_NOT_SUPP  1001
#endif

/* -------------------------------------------------------------------------
 * Globals & forward decls
 * -------------------------------------------------------------------------*/

// Alias kept for legacy skeleton accesses (e1000[REG/4])
volatile uint32_t *e1000;

// Canonical pointer used by the new helpers
static volatile uint32_t *e1000_regs;

// Forward decl – helper to read one 16‑bit word from the NIC EEPROM
static uint16_t e1000_eeprom_read(uint16_t idx);

// Six‑byte copy of the NIC's factory MAC (filled in e1000_attach)
static uint8_t e1000_mac[6];

/* -------------------------------------------------------------------------
 * Tiny MMIO helpers (always go through e1000_regs)
 * -------------------------------------------------------------------------*/
static inline uint32_t _e1000_rr(uint32_t reg)
{
    assert(e1000_regs);
    return e1000_regs[reg >> 2];
}

static inline void _e1000_wr(uint32_t reg, uint32_t val)
{
    assert(e1000_regs);
    e1000_regs[reg >> 2] = val;
}

#define REG(r)     _e1000_rr(r)
#define REGW(r,v)  _e1000_wr((r),(v))

/* -------------------------------------------------------------------------
 * Rings & buffers (unchanged from original skeleton)
 * -------------------------------------------------------------------------*/
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(128)));
static char          tx_bufs[TX_RING_SIZE][TX_PKT_SIZE];

static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(128)));
static char          rx_bufs[RX_RING_SIZE][RX_BUF_SIZE];

/* -------------------------------------------------------------------------
 * Driver entry – runs once during pci_init()
 * -------------------------------------------------------------------------*/
int
e1000_attach(struct pci_func *pcif)
{
    /* Enable device & map BAR0 */
    pci_func_enable(pcif);
    e1000_regs = (volatile uint32_t *) mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    e1000      = e1000_regs;   // keep old name working

    /* Print status just to confirm MMIO works */
    cprintf("E1000 status register = 0x%08x\n", REG(E1000_STATUS));

    /* -------------------------------------------------------------
     * Read MAC from EEPROM *after* MMIO is valid
     * -----------------------------------------------------------*/
    uint8_t mac[6];
    int i;
    for (i = 0; i < 3; i++) {
        uint16_t w = e1000_eeprom_read(i);
        mac[i*2]     =  w & 0xFF;
        mac[i*2 + 1] = (w >> 8) & 0xFF;
    }
    memcpy(e1000_mac, mac, 6);
    cprintf("e1000: detected MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* -------------------------------------------------------------
     * TX ring initialisation (stock lab code)
     * -----------------------------------------------------------*/
    for (i = 0; i < TX_RING_SIZE; i++) {
        tx_ring[i].addr   = PADDR(&tx_bufs[i]);
        tx_ring[i].status = E1000_TXD_STAT_DD;
    }

    REGW(E1000_TDBAL, PADDR(tx_ring) & 0xFFFFFFFF);
    REGW(E1000_TDBAH, 0);
    REGW(E1000_TDLEN, sizeof(tx_ring));
    REGW(E1000_TDH,   0);
    REGW(E1000_TDT,   0);

    REGW(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | (0x40 << 12));
    REGW(E1000_TIPG, 10 | (4 << 10) | (6 << 20));

    /* -------------------------------------------------------------
     * RX ring initialisation
     * -----------------------------------------------------------*/
     int j;
    for (j = 0; j < RX_RING_SIZE; j++) {
        rx_ring[j].addr = PADDR(&rx_bufs[j]);
        rx_ring[j].status = 0;
    }

    REGW(E1000_RDBAL, PADDR(rx_ring) & 0xFFFFFFFF);
    REGW(E1000_RDBAH, 0);
    REGW(E1000_RDLEN, sizeof(rx_ring));
    REGW(E1000_RDH,   0);
    REGW(E1000_RDT,   RX_RING_SIZE - 1);

    REGW(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_BSIZE_2048);

    /* Interrupts */
    REGW(E1000_IMS, E1000_ICR_RXT0 | E1000_ICR_RXO | E1000_ICR_TXDW);
    irq_setmask_8259A(irq_mask_8259A & ~(1 << 11));

    return 0;
}

/* -------------------------------------------------------------------------
 * Transmit – simple polled TX used in the lab
 * -------------------------------------------------------------------------*/
int
e1000_transmit(void *data, size_t len)
{
    uint32_t tail = REG(E1000_TDT);
    struct tx_desc *desc = &tx_ring[tail];

    if (!(desc->status & E1000_TXD_STAT_DD))
        return -E_AGAIN;                       // queue full – try later

    memcpy(tx_bufs[tail], data, len);
    desc->length = len;
    desc->cmd    = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
    desc->status = 0;

    REGW(E1000_TDT, (tail + 1) % TX_RING_SIZE);
    return 0;
}

/* -------------------------------------------------------------------------
 * Receive – polled, copies into user buffer supplied by syscall
 * -------------------------------------------------------------------------*/
int
e1000_recv(void *dst, size_t maxlen)
{
    static uint32_t tail = 0;
    struct rx_desc *desc = &rx_ring[tail];

    user_mem_assert(curenv, dst, maxlen, PTE_U | PTE_W);

    if (!(desc->status & E1000_RXD_STAT_DD))
        return -E_AGAIN;          // no packet yet
    if (!(desc->status & E1000_RXD_STAT_EOP))
        return -E_NOT_SUPP;       // jumbo frames not supported
    if (desc->length > maxlen)
        return -E_NO_MEM;         // caller buffer too small

    memcpy(dst, rx_bufs[tail], desc->length);
    desc->status = 0;

    uint32_t old_tail = tail;
    tail = (tail + 1) % RX_RING_SIZE;
    REGW(E1000_RDT, old_tail);

    return desc->length;
}

/* -------------------------------------------------------------------------
 * Interrupt handler – prints cause then EOIs local APIC
 * -------------------------------------------------------------------------*/
void
e1000_intr(void)
{
    uint32_t cause = REG(E1000_ICR);
    cprintf("[INTERRUPT] E1000 ICR = 0x%08x\n", cause);
    lapic_eoi();
}

/* -------------------------------------------------------------------------
 * EEPROM helper – poll EERD register for 1 word
 * -------------------------------------------------------------------------*/
static uint16_t
e1000_eeprom_read(uint16_t idx)
{
    REGW(E1000_EERD, (idx << E1000_EERD_ADDR_SHIFT) | E1000_EERD_START);
    while (!(REG(E1000_EERD) & E1000_EERD_DONE))
        /* spin */;
    return (uint16_t)(REG(E1000_EERD) >> 16);
}

/* Public accessor called by sys_getmac() */
void
e1000_get_mac(uint8_t dst[6])
{
    memcpy(dst, e1000_mac, 6);
}

