# High-Performance Multi-Process File Encryptor & Decryptor

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=flat&logo=c%2B%2B)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20WSL-orange.svg)
![IPC](https://img.shields.io/badge/Concurrency-POSIX%20Shared%20Memory%20%26%20Semaphores-green.svg)
![Security](https://img.shields.io/badge/Domain-Cybersecurity%20%26%20Systems-red.svg)

A concurrent batch file encryption and decryption engine written in modern C++ (C++17). The project demonstrates low-level operating system concepts, including **POSIX Inter-Process Communication (IPC)**, **bounded shared-memory queues**, **named semaphore synchronization**, and **parallel process execution via `fork()`**.

---

## Architecture Overview

The system implements a concurrent **Producer-Consumer pattern** running across separate operating system processes:

```
+-----------------------------------------------------------------------------------+
|                                  MAIN PROCESS                                     |
|  1. Recursively traverses directory (std::filesystem::recursive_directory_iterator) |
|  2. Serializes tasks ("<filePath>,<ENCRYPT|DECRYPT>")                              |
+-----------------------------------------+-----------------------------------------+
                                          |
                               sem_wait(emptySlots)
                                          v
+-----------------------------------------------------------------------------------+
|                        POSIX SHARED MEMORY RING BUFFER                            |
|                            (/my_queue via mmap)                                   |
|   [Task 0] [Task 1] [Task 2] ... [Task 999]                                       |
|   - Synchronized with std::mutex & POSIX Semaphores                               |
+-----------------------------------------+-----------------------------------------+
                                          |
                                sem_post(itemAvailable)
                                          v
+-----------------------------------------------------------------------------------+
|                             WORKER PROCESSES (fork)                               |
|  - Consumer dequeues task from ring buffer                                        |
|  - Deserializes task arguments                                                    |
|  - Performs in-place binary stream cryption                                       |
+-----------------------------------------------------------------------------------+
```

### Key Technical Concepts

- **POSIX Shared Memory (`shm_open`, `ftruncate`, `mmap`)**: High-speed zero-copy ring buffer shared across independent processes.
- **Counting Semaphores (`sem_open`, `sem_wait`, `sem_post`)**: Handles bounded-buffer synchronization (`/item_semaphore` and `/empty_slots_semaphore`) to prevent buffer overflow and race conditions.
- **Mutual Exclusion (`std::mutex`, `std::unique_lock`)**: Protects ring buffer front/rear pointers and atomic size updates within shared memory.
- **Process Orchestration (`fork()`, `exit()`)**: Decouples directory scanning from CPU-intensive cryptographic I/O tasks.
- **In-Place File Streaming (`std::fstream`, `seekp`)**: Encrypts and decrypts files in-place using binary I/O without requiring duplicate disk footprint.

---

## Project Structure

```text
├── .env.example                       # Template for cipher configuration
├── .gitignore                         # Security rules (ignores binaries, .o, and .env)
├── Makefile                           # Build automation for both targets
├── main.cpp                           # Entry point: directory walker & task producer
├── makeDirs.py                        # Test fixture generator script
├── src/
│   └── app/
│       ├── encryptDecrypt/
│       │   ├── Cryption.hpp           # Cryptographic worker interface
│       │   ├── Cryption.cpp           # In-place stream transformation engine
│       │   └── CryptionMain.cpp       # Standalone CLI binary for single-task execution
│       ├── fileHandling/
│       │   ├── IO.hpp / IO.cpp        # RAII wrapper for binary file streams
│       │   └── ReadEnv.cpp            # Key extraction utility from environment
│       └── processes/
│           ├── ProcessManagement.hpp  # Shared memory & semaphore queue manager
│           ├── ProcessManagement.cpp  # Forking & IPC lifecycle implementation
│           └── Task.hpp               # Task abstraction & string serialization
```

---

## Prerequisites

- **Compiler**: GCC / Clang with **C++17** support (`g++ >= 8.0`)
- **OS Environment**: Linux, macOS, or Windows with **WSL** / **MSYS2** (POSIX headers `<sys/mman.h>`, `<semaphore.h>`, `<unistd.h>` required)
- **Make**: GNU Make
- **Python**: Python 3.x (optional, for generating test dummy files)

---

## Quick Start

### 1. Clone & Configure

```bash
git clone https://github.com/<YOUR_USERNAME>/<YOUR_REPO>.git
cd "File ENC&DEC using cpp"

# Create your private environment key
cp .env.example .env
```

Ensure `.env` contains your encryption key (e.g., `8717`).

### 2. Build the Project

Compile both the main batch runner (`encrypt_decrypt`) and the standalone worker (`cryption`):

```bash
make
```

### 3. Generate Sample Test Files

Generate dummy data files inside a `test/` directory to test batch processing:

```bash
python3 makeDirs.py
```

### 4. Run Batch Encryption & Decryption

Execute the main program:

```bash
./encrypt_decrypt
```

Follow the interactive prompts:
```text
Enter the directory path: test
Enter the action (encrypt/decrypt): encrypt
```

To reverse the process and recover the original plaintext:
```bash
./encrypt_decrypt
# Enter 'test' and 'decrypt'
```

---

## Standalone Task Execution

The project also builds a standalone CLI worker (`cryption`) to execute targeted tasks independently:

```bash
# Syntax: ./cryption "<file_path>,<ENCRYPT|DECRYPT>"
./cryption "test/test1.txt,ENCRYPT"
./cryption "test/test1.txt,DECRYPT"
```

---

## Future Roadmap

- [ ] **AEAD Modern Cryptography**: Upgrade Caesar cipher to AES-256-GCM / ChaCha20-Poly1305.
- [ ] **Key Derivation (KDF)**: Integrate PBKDF2/Argon2id for passphrase-based keys with cryptographic salts.
- [ ] **Worker Pool Architecture**: Replace per-task `fork()` with a fixed-size pre-forked worker pool based on CPU core count.
- [ ] **Data Integrity**: Pre- and post-operation SHA-256 checksum verification.
- [ ] **Secure File Shredding**: Optional DoD 5220.22-M 3-pass overwrite mode.
- [ ] **Live CLI Dashboard**: Real-time throughput (MB/s) and multi-process progress bars.

---

## License

This project is licensed under the [MIT License](LICENSE).
