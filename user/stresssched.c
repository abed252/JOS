#include <inc/lib.h>

volatile int counter;

void
umain(int argc, char **argv)
{
	int i, j;
	int seen;
	envid_t parent = sys_getenvid();
	
	// Fork several environments
	for (i = 0; i < 20; i++)
		if (fork() == 0)
			break;
	if (i == 20) {
		sys_yield();
		return;
	}

	// Wait for the parent to finish forking
	while (envs[ENVX(parent)].env_status != ENV_FREE)
		asm volatile("pause");

	// Check that one environment doesn't run on two CPUs at once
	for (i = 0; i < 10; i++) {
		sys_yield();
	
		for (j = 0; j < 10000; j++){
				//cprintf("[%08x] working on on CPU %d\n", thisenv->env_id, thisenv->env_cpunum);
			counter++;
		}
	}
	//cprintf("[%08x] counter = %d\n", thisenv->env_id, counter);
	if (counter != 10*10000)
		panic("ran on two CPUs at once (counter is %d)", counter);

	// Check that we see environments running on different CPUs
	cprintf("[%08x] stresssched on CPU %d\n", thisenv->env_id, thisenv->env_cpunum);
	cprintf("[%08x] stresssched on CPU %d\n", thisenv->env_id, 1);
	cprintf("[%08x] stresssched on CPU %d\n", thisenv->env_id, 2);
	cprintf("[%08x] stresssched on CPU %d\n", thisenv->env_id, 3);

}

