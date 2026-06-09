# Redis Key Light

A high-performance, lightweight Redis clone built from scratch in modern C++ (C++17).

By applying the $\mathcal{O}(1)$ algorithmic efficiency to network architecture, this server handles concurrent clients, custom protocol parsing and persistant storage - all on a single multiplexewd thread.

## Core features

* **Custom TCP Server:** Built directly on top of POSIX sockets('<sys/socket.h>')
* **Protocol Parser:** Implements the official **REdis Serialization Protocol** using zero-copy 'string_view' for high-output string parsing
* **I/O Multiplexing:** Utilizes Linux's 'epoll' and non-blocking file descriptors ('<fcntl.h>') to handle thousands of concurrent client connections asynchronously without thread blocking.
* **Smart LRU Caching with TTL:** * Combines an 'unordered_map' with a doubly linked_list to achieve $\mathcal{O}(1)$ Least Recently Used (LRU) evictions.
* Implements time based "lazy-deletion" (Time-To-Live) using '<chrono>'.
* **AOF Persistance:** Implements Append-Only File(AOF) disk logging to rebuild the exact state of the database after a system restarts or crashes.

## Architecture
* **Language:** C++17
* **Build System:** CMake
* **Environment:** Linux(Ubuntu)
* **Concurrency Model:** Single-threaded Event Loop (I/O Multiplexing)

## Getting Started
* A Linux environment (or WSL)
* A C++17 compatible compiler(GCC/Clang)
* 'redis-tools' (for testing via the official 'redis-cli')
