#pragma once

#include "entity_store.h"
#include "network_buffer.h"
#include <vector>
#include <cstdint>

class SnapshotGenerator {
public:
    explicit SnapshotGenerator(const EntityStore& store);

    // Generate a full snapshot (all entities, always compressed)
    // Returns complete message (header + payload)
    std::vector<uint8_t> generate_full(uint32_t tick);

    // Generate a delta snapshot (dirty entities only, compressed if >1KB)
    // Returns complete message (header + payload), or empty if no changes
    std::vector<uint8_t> generate_delta(uint32_t tick, uint64_t checksum);

    // Generate a delta snapshot using an external dirty mask (for per-client accumulated state)
    std::vector<uint8_t> generate_delta_from_mask(uint32_t tick, uint64_t checksum,
                                                   const uint8_t* dirty_mask, uint32_t count);

private:
    const EntityStore& store_;
};
