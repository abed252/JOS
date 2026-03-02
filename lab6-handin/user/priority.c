#include <inc/lib.h>
#include <inc/env.h>

// Dump all non-FREE envs: index, envid, status, priority
static void
dump_envs(void) {
    cprintf("=== ENV LIST ===\n");
    int i;
    for (i = 0; i < NENV; i++) {
        if (envs[i].env_status != ENV_FREE) {
            cprintf("slot %d: envid %08x status %d pri %d\n",
                    i,
                    envs[i].env_id,
                    envs[i].env_status,
                    envs[i].env_pri);
        }
    }
    cprintf("================\n");
}

void
umain(int argc, char **argv) {
    envid_t me = sys_getenvid();
    cprintf("PID %08x: starting priority test\n", me);

    // 1) Show initial env table
    dump_envs();

    // 2) Spawn three children at pri=1,3,5
    int prios[3] = {1, 3, 5};
    envid_t kids[3];
    int i;
    for (i = 0; i < 3; i++) {
        if ((kids[i] = pfork(prios[i])) == 0) {
            // child
            envid_t self = sys_getenvid();
            cprintf("  child %08x: priority %d\n", self, prios[i]);
            // loop and yield a few times
            int iter;
            for (iter = 0; iter < 5; iter++) {
                cprintf("    child %08x (pri %d) iter %d → yielding\n",
                        self, prios[i], iter);
                sys_yield();
            }
            cprintf("    child %08x: exiting\n", self);
            exit();
        }
    }

    // parent
    cprintf("parent %08x: all children created\n", me);
    dump_envs();

    // 3) Let them run for a while
    for (i = 0; i < 20; i++) {
        cprintf("parent %08x: yield %d\n", me, i);
        sys_yield();
    }

    cprintf("parent %08x: test done, exiting\n", me);
    exit();
}
