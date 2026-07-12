# Redis Key Light 

A high-throughput, lightweight Redis clone engineered from scratch in modern C++ (C++17).

This project implements a single-threaded, event-driven architecture mirroring the reactor patterns used in low-latency backend systems. It handles concurrent clients, custom protocol parsing, and persistent storage entirely on a multiplexed thread without context-switching overhead.

##  Performance Benchmarks
*Tested on standard consumer hardware (Ubuntu) with 500 concurrent connections over 500,000 requests.*

* **Reads (GET):** 220,000+ ops/sec (p50 latency < 0.2ms)
* **Writes (SET):** 144,000+ ops/sec (with concurrent AOF disk logging)

##  Core Architecture

* **Asynchronous Networking:** Built directly on POSIX sockets utilizing Linux `epoll` and non-blocking I/O to comfortably manage 10,000+ concurrent connections on a single thread.
* **Zero-Copy Protocol Parser:** Implements the REdis Serialization Protocol (RESP) using `std::string_view` to eliminate hot-path heap allocations and latency jitter.
* **Smart Storage Engine:** * Features an `O(1)` LRU eviction cache combining `std::unordered_map` with a doubly-linked `std::list`.
  * Implements time-based "lazy deletion" (Time-To-Live) for efficient memory management.
* **Crash Recovery:** Utilizes Append-Only File (AOF) disk persistence to reconstruct exact database state upon server initialization.

##  Build & Run

### Prerequisites
* Linux environment (or WSL)
* CMake (3.10+)
* GCC/Clang (C++17 compatible)
* `redis-tools` (for testing via `redis-cli` and `redis-benchmark`)

### Compilation
```bash
git clone [https://github.com/goyal-chaitanya/redis-key-light.git](https://github.com/goyal-chaitanya/redis-key-light.git)
cd redis-key-light
mkdir build && cd build
cmake ..
make
./redis_server
```

### Usage
Open a new terminal and connect using the official Redis CLI:
```bash
redis-cli
127.0.0.1:6379> SET engine C++ EX 10
OK
127.0.0.1:6379> GET engine
"C++"
```

### Run the stress test
Validate the event loop throughput on your own machine:
```bash
redis-benchmark -q -n 500000 -c 500 -t set,get
```

## Author
Chaitanya Goyal (Computer Science Undergraduate, BITS Pilani)
