#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
    int start = 0;
    if (curenv)
        start = ENVX(curenv->env_id);

    int best = -1;
    int best_pri = -1;
    // Scan all NENV slots, in round-robin order past “start”
	int cnt;
    for (cnt = 1; cnt <= NENV; cnt++) {
        int i = (start + cnt) % NENV;
        struct Env *e = &envs[i];
        if (e->env_status == ENV_RUNNABLE && e->env_pri > best_pri) {
            best_pri = e->env_pri;
            best = i;
        }
    }

    if (best >= 0) {
        // jump into the highest-priority runnable env
        env_run(&envs[best]);
    }

    // if nothing else, and the current one is still running, keep it
    if (curenv && curenv->env_status == ENV_RUNNING) {
        env_run(curenv);
    }

    // no work to do: halt
    sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_NOT_RUNNABLE || // Environment might be waiting for IPC or network
		     envs[i].env_status == ENV_DYING
		     ) && envs[i].env_type == ENV_TYPE_USER)
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

