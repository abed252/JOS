// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
    void *addr = (void *) utf->utf_fault_va;
    uint32_t err = utf->utf_err;
    pte_t    pte = uvpt[PGNUM(addr)];
    int      r;

    // If this was NOT a write fault on a COW page, just kill ourselves
    // (i.e. this child process).  That prevents “panic: not a COW at 0x74”.
    if (!(err & FEC_WR) || !(pte & PTE_COW)) {
        // Any read‐fault or any write‐fault on a page that isn't PTE_COW
        // is not our responsibility.  Bail out instead of panic.
        exit();
    }

    // At this point, we know (err & FEC_WR) and (pte & PTE_COW).  Do the usual COW:
    // 1) Allocate a new page at PFTEMP
    if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
        panic("pgfault: sys_page_alloc: %e", r);

    // 2) Copy the old contents into PFTEMP
    void *aligned = ROUNDDOWN(addr, PGSIZE);
    memmove(PFTEMP, aligned, PGSIZE);

    // 3) Remap PFTEMP at the original VA with PTE_P|PTE_U|PTE_W
    if ((r = sys_page_map(0, PFTEMP, 0, aligned, PTE_P | PTE_U | PTE_W)) < 0)
        panic("pgfault: sys_page_map: %e", r);

    // 4) Unmap the temporary page
    if ((r = sys_page_unmap(0, PFTEMP)) < 0)
        panic("pgfault: sys_page_unmap: %e", r);
}
//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
    void *va  = (void *)(pn * PGSIZE);
    pte_t pte = uvpt[pn];
    int r;

    // 1) If it’s a shared page, just copy its original perms.
    if (pte & PTE_SHARE) {
        if ((r = sys_page_map(0, va, envid, va,
                              pte & PTE_SYSCALL)) < 0)
            panic("duppage share: %e", r);
    }

    // 2) If it’s writable (or already COW), then map both parent
    //    and child as (read‐only + PTE_COW).
    else if ((pte & PTE_W) || (pte & PTE_COW)) {
        // first, map into the child as COW
        if ((r = sys_page_map(0, va, envid, va,
                              PTE_P | PTE_U | PTE_COW)) < 0)
            panic("duppage child COW: %e", r);

        // then remap the parent’s own page at ‘va’ to be read‐only+COW
        if ((r = sys_page_map(0, va, 0,      va,
                              PTE_P | PTE_U | PTE_COW)) < 0)
            panic("duppage parent COW: %e", r);
    }

    // 3) Otherwise it’s a read-only page; just copy the mapping.
    else {
        if ((r = sys_page_map(0, va, envid, va,
                              PTE_P | PTE_U)) < 0)
            panic("duppage readonly: %e", r);
    }

    return 0;
}


//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
    envid_t child;
    uintptr_t va;
    int r;

    // 1) Install pgfault handler
    set_pgfault_handler(pgfault);

    // 2) Create child environment
    if ((child = sys_exofork()) < 0)
        panic("fork: sys_exofork: %e", child);
    if (child == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    // —— print PTE[0] “present, write, user, cow” bits ——
    // ————————————————————————————————————————————

    // 3) In parent: duplicate page mappings
    for (va = 0; va < USTACKTOP; va += PGSIZE) {
        unsigned pn = PGNUM(va);
        if (!(uvpd[PDX(va)] & PTE_P))
            continue;
        if (!(uvpt[pn] & PTE_P)) {
            // also re‐print page 0 here, if pn==0
            continue;
        }
        duppage(child, pn);
    }

    // 4) Allocate a fresh exception stack for the child
    if ((r = sys_page_alloc(child,
                            (void *)(UXSTACKTOP - PGSIZE),
                            PTE_P | PTE_U | PTE_W)) < 0)
        panic("fork: uxstack alloc: %e", r);

    // 5) Tell the child to use the same pgfault upcall
    sys_env_set_pgfault_upcall(child, thisenv->env_pgfault_upcall);

    // 6) Mark child runnable
    if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
        panic("fork: sys_env_set_status: %e", r);

    return child;
}
envid_t
pfork(int pr)
{
	set_pgfault_handler(pgfault);

	envid_t envid;
	uint32_t addr;
	envid = sys_exofork();
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		sys_set_priority(pr);
		return 0;
	}

	if (envid < 0)
		panic("sys_exofork: %e", envid);

	for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
		if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P)
			&& (uvpt[PGNUM(addr)] & PTE_U)) {
			duppage(envid, PGNUM(addr));
		}

	if (sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P) < 0)
		panic("1");
	extern void _pgfault_upcall();
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
		panic("sys_env_set_status");

	return envid;
	panic("fork not implemented");
}
// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
