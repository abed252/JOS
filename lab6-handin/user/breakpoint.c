// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	//cprintf("Breakpoint test program =========================\n");
	asm volatile("int $3");
}
