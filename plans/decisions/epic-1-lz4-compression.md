# Decision: LZ4 Compression Required

**Date:** 2026-01-27
**Status:** accepted
**Epic:** 1
**Affects Tickets:** 1-014, 1-006

## Context

Full state snapshots for reconnecting players can reach 1-5MB for mature worlds. Sending uncompressed data of this size over typical internet connections would cause:
- Long reconnection times (5MB at 1Mbps upload = 40+ seconds)
- Bandwidth costs for server operators
- Poor experience for players on slower connections

The original plan marked compression as "optional" but this is unrealistic for production use.

## Options Considered

### 1. LZ4 (Required from Start)
- Extremely fast compression/decompression
- Good compression ratios for game state data
- Header-only option available (lz4)
- BSD license
- Available via vcpkg

### 2. Optional (Add Later)
- Simpler initial implementation
- Risk of being deprioritized
- Harder to retrofit if not designed in from start

### 3. zstd
- Better compression ratios than LZ4
- Slower than LZ4
- More complex API

### 4. zlib/gzip
- Universal compatibility
- Slower than LZ4
- Adequate compression

## Decision

**LZ4 Required** - Make LZ4 compression mandatory for snapshots and large delta updates.

## Rationale

1. **Speed:** LZ4 decompresses at 4+ GB/s - effectively free
2. **Compression:** Typically 2-4x reduction for game state data
3. **Simplicity:** Simple API, easy to integrate
4. **Bandwidth:** Essential for reasonable reconnection times
5. **Early integration:** Easier to build in from start than retrofit

## Implementation Details

```cpp
#include <lz4.h>

// Compress snapshot before sending
std::vector<uint8_t> compress_snapshot(const std::vector<uint8_t>& raw) {
    int max_compressed = LZ4_compressBound(raw.size());
    std::vector<uint8_t> compressed(max_compressed + sizeof(uint32_t));

    // Store original size at start (for decompression buffer allocation)
    uint32_t original_size = static_cast<uint32_t>(raw.size());
    memcpy(compressed.data(), &original_size, sizeof(uint32_t));

    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(raw.data()),
        reinterpret_cast<char*>(compressed.data() + sizeof(uint32_t)),
        raw.size(),
        max_compressed
    );

    compressed.resize(sizeof(uint32_t) + compressed_size);
    return compressed;
}

// Decompress on client
std::vector<uint8_t> decompress_snapshot(const std::vector<uint8_t>& compressed) {
    uint32_t original_size;
    memcpy(&original_size, compressed.data(), sizeof(uint32_t));

    std::vector<uint8_t> raw(original_size);
    LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed.data() + sizeof(uint32_t)),
        reinterpret_cast<char*>(raw.data()),
        compressed.size() - sizeof(uint32_t),
        original_size
    );

    return raw;
}
```

## Consequences

- Add lz4 to vcpkg.json
- All FullStateMessage payloads are LZ4 compressed
- Large StateUpdateMessage payloads (>1KB) should also be compressed
- Message header includes flag indicating compression
- Minimal CPU overhead due to LZ4's speed

## References

- LZ4 GitHub: https://github.com/lz4/lz4
- vcpkg port: lz4
