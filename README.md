# JOS Operating System — x86 Kernel Implementation

A teaching operating system developed as part of the Technion Operating Systems Engineering (OSE) course.

This project implements core OS components including:
- Virtual memory & paging
- User-level environments
- Preemptive scheduling
- File system
- Networking stack

~10K lines of C and x86 assembly.
---

## 🚀 Features Implemented

- x86 boot process & protected mode
- Kernel initialization
- Physical page allocator
- Virtual memory & paging
- User environments (process abstraction)
- System calls
- Preemptive multitasking
- File system
- Basic networking stack

---

## 🧠 High-Level Architecture

Bootloader  
→ Kernel Entry (protected mode)  
→ Memory Management (paging, allocator)  
→ Environment / Process Management  
→ System Calls & Traps  
→ File System & Networking  

---

## 📂 Repository Structure

| Folder | Description |
|--------|------------|
| boot/  | Bootloader and early CPU initialization |
| kern/  | Kernel core (memory, traps, scheduler, syscalls) |
| lib/   | Shared libraries |
| user/  | User-level programs |
| fs/    | File system implementation |
| net/   | Networking stack |
| inc/   | Shared header files |

---

## 🛠 Build & Run

### Requirements
- Linux / WSL2
- gcc
- make
- qemu-system-x86
- gdb

### Build
```bash
make
```

### Run (Graphical)
```bash
make qemu
```

### Run (Terminal only)
```bash
make qemu-nox
```

### Debug Mode
```bash
make qemu-gdb
```

In another terminal:
```bash
gdb
target remote :26000
```

---

## 📚 Labs Overview

### Lab 1 — Boot & Console
- Enter protected mode
- Basic kernel printing & debugging

### Lab 2 — Memory Management
- Physical page allocator
- Page tables and virtual memory mapping

### Lab 3 — Environments & System Calls
- User environments
- Trap handling
- Syscall dispatcher

### Lab 4 — Preemptive Scheduling
- Timer interrupts
- Round-robin scheduling

### Lab 5 — File System
- Block cache
- File operations
- User-space file server

### Lab 6 — Networking
- Network driver integration
- IPC-based communication
- Packet handling

---

## 📌 Academic Integrity

This repository is provided for documentation and learning purposes.
If you are currently taking the course, ensure you follow your university’s academic integrity policies.

---

## 👨‍💻 Authors

Abed Alhady Egbaria, Kareem Abo Gazala
B.Sc Computer Science — Technion
