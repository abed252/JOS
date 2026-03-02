/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */


#define TRAPHANDLER(name, num)\
.data;\
	.long name;\
.text;\
	.globl name;\
	.type name, @function;\
	.align 2;\
name:\
	pushl $(num);\
	jmp _alltraps

#define TRAPHANDLER_NOEC(name, num)\
.data;\
	.long name;\
.text;\
	.globl name;\
	.type name, @function;\
	.align 2;\
name:\
	pushl $0;\
	pushl $(num);\
	jmp _alltraps

#define ZERO()\
.data;\
	.long 0

.data
	.p2align 2
	.globl funs
funs:
.text
/*
 * Challenge: my code here
 */
	TRAPHANDLER_NOEC(th0, 0)
	TRAPHANDLER_NOEC(th1, 1)
	ZERO()
	TRAPHANDLER_NOEC(th3, 3)
	TRAPHANDLER_NOEC(th4, 4)
	TRAPHANDLER_NOEC(th5, 5)
	TRAPHANDLER_NOEC(th6, 6)
	TRAPHANDLER_NOEC(th7, 7)
	TRAPHANDLER(th8, 8)
	TRAPHANDLER_NOEC(th9, 9)
	TRAPHANDLER(th10, 10)
	TRAPHANDLER(th11, 11)
	TRAPHANDLER(th12, 12)
	TRAPHANDLER(th13, 13)
	TRAPHANDLER(th14, 14)
	ZERO()
	TRAPHANDLER_NOEC(th16, 16)
	.data
	.space 60
	.text
	/////////////////
	TRAPHANDLER_NOEC(th32, 32)
    TRAPHANDLER_NOEC(th33, 33)
    TRAPHANDLER_NOEC(th34, 34)
    TRAPHANDLER_NOEC(th35, 35)
    TRAPHANDLER_NOEC(th36, 36)
    TRAPHANDLER_NOEC(th37, 37)
    TRAPHANDLER_NOEC(th38, 38)
    TRAPHANDLER_NOEC(th39, 39)
    TRAPHANDLER_NOEC(th40, 40)
    TRAPHANDLER_NOEC(th41, 41)
    TRAPHANDLER_NOEC(th42, 42)
    TRAPHANDLER_NOEC(th43, 43)
    TRAPHANDLER_NOEC(th44, 44)
    TRAPHANDLER_NOEC(th45, 45)
    TRAPHANDLER_NOEC(th46, 46)
    TRAPHANDLER_NOEC(th47, 47)
	TRAPHANDLER_NOEC(th48, 48)

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
     # — Build the Trapframe exactly as the C struct expects —
    # Push data-segment registers as 32-bit values
    pushl  %ds
    pushl  %es
    # Push all general-purpose regs
    pushal

    # — Switch to the kernel data segment —
    movw   $(GD_KD), %ax
    movw   %ax,      %ds
    movw   %ax,      %es

    # — Pass &TF to trap(tf) —
    
    pushl  %esp
    call   trap
