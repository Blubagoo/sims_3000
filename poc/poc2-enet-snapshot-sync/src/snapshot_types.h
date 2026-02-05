#pragma once

#include <cstdint>

// 12 bytes
struct PositionComponent {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// 12 bytes
struct DataComponent {
    uint32_t type_id = 0;
    uint32_t flags = 0;
    float value = 0.0f;
};

// Total: 24 bytes per entity (packed contiguously)

// Field bitmask for delta encoding (6 fields)
enum FieldBits : uint8_t {
    FIELD_POS_X   = 0x01,
    FIELD_POS_Y   = 0x02,
    FIELD_POS_Z   = 0x04,
    FIELD_TYPE_ID = 0x08,
    FIELD_FLAGS   = 0x10,
    FIELD_VALUE   = 0x20,
    FIELD_ALL     = 0x3F,
};

// Delta entry for a single entity
struct EntityDelta {
    uint32_t entity_id;
    uint8_t changed_fields; // bitmask of FieldBits
};

static constexpr uint32_t ENTITY_COUNT = 50000;
static constexpr float TICK_RATE = 20.0f;
static constexpr float TICK_INTERVAL = 1.0f / TICK_RATE;
