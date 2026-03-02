#include "ns.h"
#include <inc/lib.h>
extern union Nsipc nsipcbuf;


void input(envid_t ns_envid)
{
    binaryname = "ns_input";
    int r;

    while (1) {
        // Allocate a new page for each incoming packet
        void *pktpg = (void*)UTEMP; // or any unused user VA
        if (sys_page_alloc(0, pktpg, PTE_U | PTE_W | PTE_P) < 0)
            panic("page alloc fail");

        // Receive packet into pktpg's jp_data field
        r = sys_e1000_receive(pktpg + offsetof(struct jif_pkt, jp_data), 2048);
        if (r < 0)
            continue; // nothing ready

        ((struct jif_pkt*)pktpg)->jp_len = r;

        // Send to network server (ns) and hand off the page
        ipc_send(ns_envid, NSREQ_INPUT, pktpg, PTE_U | PTE_P);


        // Don't reuse the same page immediately; let ns free it
        sys_yield();
    }
}
