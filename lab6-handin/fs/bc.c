
#include "fs.h"

// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P);
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (uvpt[PGNUM(va)] & PTE_D) != 0;
}

// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr    = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint32_t) addr - DISKMAP) / BLKSIZE;
	int r;

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	// Hint: first round addr to page boundary. fs/ide.c has code to read
	// the disk.
	//
	// LAB 5: you code here:

	void *blk_base = ROUNDDOWN(addr, BLKSIZE);          /* start of block page   */

	if ((r = sys_page_alloc(0, blk_base, PTE_SYSCALL)) < 0)
		panic("bc_pgfault: sys_page_alloc: %e", r);

	/* Read BLKSECTS (= BLKSIZE / SECTSIZE) sectors that form this block */
	if ((r = ide_read(blockno * BLKSECTS, blk_base, BLKSECTS)) < 0)
		panic("bc_pgfault: ide_read: %e", r);
	
		// debug prints ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// cprintf("bc_pgfault: loaded block %u at %p\n", blockno, blk_base);

	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	if ((r = sys_page_map(0, blk_base, 0, blk_base,
	                       uvpt[PGNUM(blk_base)] & PTE_SYSCALL)) < 0)
		panic("in bc_pgfault, sys_page_map: %e", r);

	// Check that the block we read was allocated. (exercise for
	// the reader: why do we do this *after* reading the block
	// in?)
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);
}

/* -------------------------------------------------------------------- */

void
flush_block(void *addr)
{
	uint32_t blockno = ((uint32_t) addr - DISKMAP) / BLKSIZE;

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);

	// LAB 5: Your code here.

	void *blk_base = ROUNDDOWN(addr, BLKSIZE);

	/* If the block is not mapped or not dirty, nothing to do */
	if (!va_is_mapped(blk_base) || !va_is_dirty(blk_base))
		return;

	int r;
	if ((r = ide_write(blockno * BLKSECTS, blk_base, BLKSECTS)) < 0)
		panic("flush_block: ide_write: %e", r);

	/* Clear the dirty bit (PTE_D) now that disk and memory are in sync */
	if ((r = sys_page_map(0, blk_base, 0, blk_base,
	                       uvpt[PGNUM(blk_base)] & PTE_SYSCALL)) < 0)
		panic("flush_block: sys_page_map: %e", r);
}


// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	struct Super super;
	set_pgfault_handler(bc_pgfault);
	check_bc();

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof super);
}

