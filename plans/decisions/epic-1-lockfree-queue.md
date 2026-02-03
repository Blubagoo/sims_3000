# Decision: Lock-Free Queue Library

**Date:** 2026-01-27
**Status:** accepted
**Epic:** 1
**Affects Tickets:** 1-004

## Context

Epic 1's network threading model requires lock-free single-producer single-consumer (SPSC) queues for communication between the network I/O thread and the main simulation thread. This is critical for performance - mutex contention would add latency to every network message.

## Options Considered

### 1. moodycamel::readerwriterqueue
- Header-only C++ library
- Specifically designed for SPSC scenarios
- Proven in production, extensively tested
- MIT license
- Available via vcpkg

### 2. Boost.Lockfree
- Part of the Boost library ecosystem
- Well-tested and documented
- Brings in Boost as a dependency (heavy)

### 3. Custom Implementation
- No external dependencies
- Full control
- Lock-free code is notoriously hard to get right
- High risk of subtle bugs

### 4. std::atomic with Ring Buffer
- Standard library only
- Requires manual implementation
- Less tested than dedicated libraries

## Decision

**moodycamel::readerwriterqueue** - Use this header-only library for SPSC queues.

## Rationale

1. **Header-only:** Easy integration, no linking issues
2. **Battle-tested:** Used in many production systems
3. **SPSC-optimized:** Designed exactly for our use case
4. **Performance:** Benchmarks show excellent throughput and latency
5. **MIT license:** Compatible with our project
6. **vcpkg available:** Consistent with our dependency management

## Consequences

- Add moodycamel-readerwriterqueue to vcpkg.json
- Use `moodycamel::ReaderWriterQueue<T>` for network message queues
- One queue for inbound messages (network thread → main thread)
- One queue for outbound messages (main thread → network thread)

## Usage Pattern

```cpp
#include "readerwriterqueue.h"

// In NetworkManager
moodycamel::ReaderWriterQueue<InboundMessage> inbound_queue;
moodycamel::ReaderWriterQueue<OutboundMessage> outbound_queue;

// Network thread (producer for inbound, consumer for outbound)
void network_thread_tick() {
    // Receive from network, enqueue for main thread
    if (auto msg = receive_from_socket()) {
        inbound_queue.enqueue(std::move(*msg));
    }

    // Dequeue from main thread, send to network
    OutboundMessage out;
    while (outbound_queue.try_dequeue(out)) {
        send_to_socket(out);
    }
}

// Main thread (consumer for inbound, producer for outbound)
void main_thread_process_network() {
    InboundMessage in;
    while (inbound_queue.try_dequeue(in)) {
        handle_message(in);
    }
}
```

## References

- GitHub: https://github.com/cameron314/readerwriterqueue
- vcpkg port: readerwriterqueue
