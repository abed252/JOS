# JOS Architecture Overview

## Boot Process
1. BIOS loads bootloader
2. Bootloader switches to protected mode
3. Kernel entry initializes memory

## Memory Management
- Physical page allocator
- Page directory + page tables
- Kernel virtual memory mapping

Key file: `kern/pmap.c`

## Environments (Process Abstraction)
- struct Env
- Trapframe storage
- Context switching

Key file: `kern/env.c`

## Scheduling
- Round-robin scheduler
- Timer interrupt driven preemption

Key file: `kern/sched.c`

## File System
- Block cache
- IPC-based file server

Key files:
- `fs/`
- `user/fs/`

## Networking
- Network driver integration
- Packet handling
- IPC communication
