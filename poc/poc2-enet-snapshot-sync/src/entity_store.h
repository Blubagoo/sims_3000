#pragma once

#include "snapshot_types.h"
#include <vector>
#include <cstdint>

class EntityStore {
public:
    explicit EntityStore(uint32_t count = ENTITY_COUNT);

    // Accessors
    uint32_t count() const { return count_; }
    PositionComponent& position(uint32_t id) { return positions_[id]; }
    DataComponent& data(uint32_t id) { return data_[id]; }
    const PositionComponent& position(uint32_t id) const { return positions_[id]; }
    const DataComponent& data(uint32_t id) const { return data_[id]; }

    // Raw array access for bulk serialization
    const PositionComponent* positions() const { return positions_.data(); }
    const DataComponent* data_components() const { return data_.data(); }
    PositionComponent* positions() { return positions_.data(); }
    DataComponent* data_components() { return data_.data(); }

    // Dirty tracking
    uint8_t dirty(uint32_t id) const { return dirty_[id]; }
    void mark_dirty(uint32_t id, uint8_t fields);
    void clear_dirty();
    uint32_t dirty_count() const;

    // Get list of dirty entity IDs and their field masks
    std::vector<EntityDelta> get_deltas() const;

    // FNV-1a checksum over all entity data
    uint64_t compute_checksum() const;

    // Initialize entities with deterministic data
    void initialize_deterministic(uint32_t seed);

    // Quantize dirty entities to match network serialization precision.
    // Call after simulation tick to ensure server checksum matches client.
    void quantize_dirty();

private:
    uint32_t count_;
    std::vector<PositionComponent> positions_;
    std::vector<DataComponent> data_;
    std::vector<uint8_t> dirty_; // per-entity field bitmask
};
