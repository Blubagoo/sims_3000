#include "entity_store.h"
#include <cstring>

EntityStore::EntityStore(uint32_t count)
    : count_(count)
    , positions_(count)
    , data_(count)
    , dirty_(count, 0)
{
}

void EntityStore::mark_dirty(uint32_t id, uint8_t fields) {
    dirty_[id] |= fields;
}

void EntityStore::clear_dirty() {
    std::memset(dirty_.data(), 0, dirty_.size());
}

uint32_t EntityStore::dirty_count() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < count_; ++i) {
        if (dirty_[i] != 0) ++count;
    }
    return count;
}

std::vector<EntityDelta> EntityStore::get_deltas() const {
    std::vector<EntityDelta> deltas;
    deltas.reserve(count_ / 50); // ~2% expected
    for (uint32_t i = 0; i < count_; ++i) {
        if (dirty_[i] != 0) {
            deltas.push_back({i, dirty_[i]});
        }
    }
    return deltas;
}

uint64_t EntityStore::compute_checksum() const {
    // FNV-1a over all position and data arrays
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET;

    auto hash_bytes = [&](const void* data, size_t size) {
        auto bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= FNV_PRIME;
        }
    };

    hash_bytes(positions_.data(), count_ * sizeof(PositionComponent));
    hash_bytes(data_.data(), count_ * sizeof(DataComponent));

    return hash;
}

// Quantization constants (must match snapshot_generator/applier)
static constexpr float POSITION_MAX = 100.0f;
static constexpr float VALUE_MAX = 100.0f;

static inline uint16_t quantize_pos(float v) {
    float clamped = v < 0.0f ? 0.0f : (v > POSITION_MAX ? POSITION_MAX : v);
    return static_cast<uint16_t>((clamped / POSITION_MAX) * 65535.0f);
}

static inline float dequantize_pos(uint16_t v) {
    return (static_cast<float>(v) / 65535.0f) * POSITION_MAX;
}

static inline uint16_t quantize_value(float v) {
    float clamped = v < 0.0f ? 0.0f : (v > VALUE_MAX ? VALUE_MAX : v);
    return static_cast<uint16_t>((clamped / VALUE_MAX) * 65535.0f);
}

static inline float dequantize_value(uint16_t v) {
    return (static_cast<float>(v) / 65535.0f) * VALUE_MAX;
}

void EntityStore::quantize_dirty() {
    for (uint32_t i = 0; i < count_; ++i) {
        uint8_t mask = dirty_[i];
        if (mask == 0) continue;

        auto& pos = positions_[i];
        auto& dat = data_[i];

        // Apply quantize/dequantize round-trip to match network precision
        if (mask & FIELD_POS_X) pos.x = dequantize_pos(quantize_pos(pos.x));
        if (mask & FIELD_POS_Y) pos.y = dequantize_pos(quantize_pos(pos.y));
        if (mask & FIELD_POS_Z) pos.z = dequantize_pos(quantize_pos(pos.z));
        if (mask & FIELD_TYPE_ID) dat.type_id = dat.type_id & 0xFF;
        if (mask & FIELD_FLAGS) dat.flags = dat.flags & 0xFF;
        if (mask & FIELD_VALUE) dat.value = dequantize_value(quantize_value(dat.value));
    }
}

void EntityStore::initialize_deterministic(uint32_t seed) {
    // Simple xorshift32 for deterministic initialization
    uint32_t state = seed;
    auto next = [&]() -> uint32_t {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };

    auto next_float = [&]() -> float {
        return static_cast<float>(next() % 10000) / 100.0f; // 0..99.99
    };

    for (uint32_t i = 0; i < count_; ++i) {
        positions_[i].x = next_float();
        positions_[i].y = next_float();
        positions_[i].z = next_float();
        data_[i].type_id = next() % 16;
        data_[i].flags = next() % 256;
        data_[i].value = next_float();
    }
}
