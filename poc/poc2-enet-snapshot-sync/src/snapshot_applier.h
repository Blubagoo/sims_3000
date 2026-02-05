#pragma once

#include "entity_store.h"
#include <vector>
#include <cstdint>

struct ApplyResult {
    bool success = false;
    uint32_t tick = 0;
    uint32_t entities_updated = 0;
    uint64_t server_checksum = 0;
    bool checksum_match = true;
};

class SnapshotApplier {
public:
    explicit SnapshotApplier(EntityStore& store);

    // Apply a full snapshot message (header + payload)
    ApplyResult apply_full(const uint8_t* data, size_t size);

    // Apply a delta snapshot message (header + payload)
    ApplyResult apply_delta(const uint8_t* data, size_t size);

private:
    EntityStore& store_;
};
