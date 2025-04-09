# Custom File System Simulator

A multi-stage file system simulator implemented in C.  
This project gradually builds a complete file system from scratch—starting with queue management, moving through virtual disk and buffer layers, and ending with a working inode-based file system.

---

## Features

### Stage 1: Queue Implementation (`hw1`)
- Basic FIFO queue for internal scheduling and buffering
- Unit-tested with sample cases

### Stage 2: Virtual Disk & Buffer Manager (`hw2`)
- Simulated disk via file I/O (`MY_DISK`)
- Block-level read/write using a custom buffer manager
- LRU (Least Recently Used) buffer replacement policy
- Utilities for memory and block debugging

### Stage 3: Full File System (`hw3`)
- Inode-based file system with:
  - File and directory creation
  - Open, read, write, and close operations
  - Path parsing and hierarchical navigation
- File descriptor table, directory entries, block allocation logic
- Modular structure (`fs.c`, `buf.c`, `disk.c`, etc.)

---

## Core Concepts

- Block-level I/O with direct access to disk-like file
- Buffer cache with replacement strategy
- Inode structure and directory management
- Abstracted filesystem API (e.g. `CreateFile`, `ReadFile`, `OpenFile`)
- Manual memory and disk block management

---

## Getting Started

### Build

```bash
cd hw1 && make
cd ../hw2 && make
cd ../hw3 && make
```

### Run testcases

## hw1
```bash
cd hw1
./hw1 [testcase number(1~3)]
```


## hw2
```bash
cd hw2
./hw2 [testcase number(1~3)]
```

## hw3
```bash
cd hw3
./hw3 format 1 #testcase 1
./hw3 readwrite 2 #testcase 2
./hw3 readwrite 3 #testcase 3
./hw3 readwrite 4 #testcase 4
```

> Note: `MY_DISK` will be generated/used in hw2 and hw3 as the virtual storage device.

---

## 📂 Project Structure

```bash
FileSystem/
├── hw1/               # Queue implementation
├── hw2/               # Disk and buffer layer
├── hw3/               # Full file system
```
