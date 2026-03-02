#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	while (1) {
		// Receive a packet from the network server
		int r = ipc_recv(&ns_envid, &nsipcbuf, NULL);
		if (r < 0) {
			cprintf("output: ipc_recv failed: %e\n", r);
			continue;
		}

		// Check if the received message is for us
		if (nsipcbuf.pkt.jp_len > 0) {
			// Transmit the packet using the e1000 driver
			r = sys_e1000_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
			if (r < 0) {
				cprintf("output: e1000_transmit failed: %e\n", r);
			}
		}
	}
}
