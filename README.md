# Persistent Hash Table C

## About

Persistent Hash Table is an advanced C-language system programming project developed as a first-year Master's (M1 IMPAIRS) project at Université Paris Cité. Built entirely from scratch, this project demonstrates a deep understanding of advanced systems concepts, including shared memory management (`mmap`), fine-grained file-locking synchronization (`fcntl`), and concurrency control via POSIX threads (`pthread`).

The project features a robust multi-versioned hash table supporting both 32-bit checksums (`.cks`) and SHA-1 (`.sha`) based hashing algorithms to efficiently store and manage strings and file objects.

*Note: As this was originally an academic project evaluated on systems architecture and robustness, the codebase emphasizes low-level memory handling, rigorous system call error management, and multi-process concurrency safety. The source code and comments might contain some French.*

## Tech Stack

- **Language:** C
- **System Calls & APIs:** POSIX Threads (`pthread`), Memory Mapping (`mmap`), File Locking (`fcntl`)
- **Cryptography:** OpenSSL (`libcrypto`) for SHA-1 hashing
- **Build System:** GNU Make

## Key Features

- **Shared Memory Backing:** Persistent storage and inter-process communication using `mmap`.
- **Concurrency & Locking:** Robust multi-process safety utilizing fine-grained file-locking mechanisms (`fcntl`).
- **Dynamic Resizing:** Automated extension and shrinkage of the hash table structure based on load factors.
- **Parallel Processing:** Multi-threaded operations using `pthread` to optimize bulk data insertion and querying.
- **Integrity & Diagnostics:** Built-in verification utilities and corruption-checking commands to ensure data consistency.
- **Flexible Hashing:** Support for multiple hashing algorithms including 32-bit custom checksums (`.cks`) and SHA-1 (`.sha`).

## Limitations & Known Issues

- **Concurrency:** While fine-grained file locking (`fcntl`) and mutex mechanisms are implemented, certain high-stress parallel execution scenarios insertions may encounter race conditions or synchronization edge cases.

## Projet Structure

```text
📁 hachement
│
├── 📁 docs/
│   └── 📄 Rapport.pdf              # Technical architecture report
│
├── 📁 include/                     # Header files (.h)
├── 📁 src/                         # C source files (.c)
│   ├── bloc.c                      # Block management, memory mapping and locking
│   ├── cmd.c                       # Command parsing and action handlers (make, insert, lookup, etc.)
│   ├── hachage.c                   # Hashing algorithms (SHA-1 and 32-bit checksums)
│   ├── main.c                      # Program entry point and argument dispatching
│   └── utils.c                     # General helper functions and utilities
│
├── 📄 Makefile                     # Build configuration
└── 📄 README.md                    # Project documentation
```

## Getting Started

### Prerequisites

* UNIX-like operating system (Linux / macOS)
* GCC compiler supporting C99
* GNU Make
* OpenSSL development libraries (libssl-dev)

### Installation & Compilation

1. **Clone the repository:**
```bash
git clone https://github.com/SuyuKuka/persistent-hash-table-c
cd persistent-hash-table-c
```

2. **Compile the project:**
```bash
make
```

3. **Cleaning build files:**
```bash
make clean
```

### Usage

The program is executed via command-line parameters specifying the action, the table filename (.cks or), a delay parameter (slowns) to be able to see better concurrency, and specific arguments depending on the command:

* **Create a new table:**
```bash
./hachement make table.sha 0 100
```

* **Display table information and statistics:**
```bash
./hachement free table.sha 0
```

* **Insert a string:**
```bash
./hachement insertstring table.sha 0 "Hello World"
```

* **Insert a file:**
```bash
./hachement insertfile table.sha 0 myfile.txt
```

* **Lookup an entry:**
```bash
./hachement lookup table.sha 0 "Hello World"
```

* **Delete an entry with hashcode:**
```bash
./hachement deletehash table.sha 0 hashcode
```

* **Delete an entry:**
```bash
./hachement delete table.sha 0 "Hello World"
```

* **Check table integrity:**
```bash
./hachement check table.sha 0
```

* **Extend table size:** (double the capacity of the table)
```bash
./hachement extend table.sha 0
```

* **Shrink table size:** (halves the capacity of the table)
```bash
./hachement shrink table.sha 0
```

* **Adjust table size based on load factor:** (automatically extends if the fill rate exceeds 70%, shrinks if it drops below 10%, and does nothing otherwise)
```bash
./hachement ajust table.sha 0
```

## Team

This project was developed collaboratively as a 2-person team:

- **CHEN Michel** - System Architecture & C Implementation
  [![GitHub](https://img.shields.io/badge/GitHub-Profile-181717?logo=github)](https://github.com/SuyuKuka)
  [![LinkedIn](https://img.shields.io/badge/LinkedIn-Profile-0A66C2?logo=linkedin)](https://linkedin.com/in/chen-michel)

- **MA Qian En** - System Architecture & C Implementation

## License

This project is licensed under the GPL 3.0 License — see [LICENSE](LICENSE) for details.