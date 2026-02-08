/**
 * @file EnergySerialization.cpp
 * @brief Implementation of energy component and pool serialization
 *        (Epic 5, tickets 5-034, 5-035)
 */

#include <sims3000/energy/EnergySerialization.h>
#include <stdexcept>

namespace sims3000 {
namespace energy {

// ============================================================================
// Little-endian helper functions
// ============================================================================

static void write_uint8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

static void write_uint32_le(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

static void write_int32_le(std::vector<uint8_t>& buf, int32_t value) {
    write_uint32_le(buf, static_cast<uint32_t>(value));
}

static uint8_t read_uint8(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

static uint32_t read_uint32_le(const uint8_t* data, size_t& offset) {
    uint32_t value = static_cast<uint32_t>(data[offset])
                   | (static_cast<uint32_t>(data[offset + 1]) << 8)
                   | (static_cast<uint32_t>(data[offset + 2]) << 16)
                   | (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return value;
}

static int32_t read_int32_le(const uint8_t* data, size_t& offset) {
    return static_cast<int32_t>(read_uint32_le(data, offset));
}

// ============================================================================
// EnergyComponent serialization (Ticket 5-034)
// ============================================================================

void serialize_energy_component(const EnergyComponent& comp, std::vector<uint8_t>& buffer) {
    // Version byte
    write_uint8(buffer, ENERGY_SERIALIZATION_VERSION);  // 1 byte

    // EnergyComponent is trivially copyable and 12 bytes - use memcpy
    const size_t prev_size = buffer.size();
    buffer.resize(prev_size + sizeof(EnergyComponent));
    std::memcpy(buffer.data() + prev_size, &comp, sizeof(EnergyComponent));
    // Total: 1 + 12 = 13 bytes
}

size_t deserialize_energy_component(const uint8_t* data, size_t size, EnergyComponent& comp) {
    constexpr size_t REQUIRED_SIZE = 1 + sizeof(EnergyComponent); // 13 bytes
    if (size < REQUIRED_SIZE) {
        throw std::runtime_error("EnergyComponent deserialization: buffer too small");
    }

    size_t offset = 0;
    /* uint8_t version = */ read_uint8(data, offset); // version byte (for future use)

    std::memcpy(&comp, data + offset, sizeof(EnergyComponent));
    offset += sizeof(EnergyComponent);

    return offset; // 13 bytes consumed
}

// ============================================================================
// Compact power-state bit packing (Ticket 5-034)
// ============================================================================

void serialize_power_states(const bool* states, uint32_t count, std::vector<uint8_t>& buffer) {
    // Write entity count as 4-byte LE
    write_uint32_le(buffer, count);

    // Pack 8 states per byte
    const uint32_t byte_count = (count + 7) / 8;
    for (uint32_t byte_idx = 0; byte_idx < byte_count; ++byte_idx) {
        uint8_t packed = 0;
        for (uint32_t bit = 0; bit < 8; ++bit) {
            const uint32_t entity_idx = byte_idx * 8 + bit;
            if (entity_idx < count && states[entity_idx]) {
                packed |= (1u << bit);
            }
        }
        buffer.push_back(packed);
    }
}

size_t deserialize_power_states(const uint8_t* data, size_t size, bool* states, uint32_t max_count) {
    // Need at least 4 bytes for the count
    if (size < 4) {
        throw std::runtime_error("Power states deserialization: buffer too small for count");
    }

    size_t offset = 0;
    const uint32_t count = read_uint32_le(data, offset);

    if (count > max_count) {
        throw std::runtime_error("Power states deserialization: count exceeds max_count");
    }

    const uint32_t byte_count = (count + 7) / 8;
    if (size < 4 + byte_count) {
        throw std::runtime_error("Power states deserialization: buffer too small for packed data");
    }

    // Unpack bits
    for (uint32_t byte_idx = 0; byte_idx < byte_count; ++byte_idx) {
        const uint8_t packed = data[offset++];
        for (uint32_t bit = 0; bit < 8; ++bit) {
            const uint32_t entity_idx = byte_idx * 8 + bit;
            if (entity_idx < count) {
                states[entity_idx] = (packed & (1u << bit)) != 0;
            }
        }
    }

    return offset; // 4 + byte_count bytes consumed
}

// ============================================================================
// EnergyPoolSyncMessage serialization (Ticket 5-035)
// ============================================================================

void serialize_pool_sync(const EnergyPoolSyncMessage& msg, std::vector<uint8_t>& buffer) {
    write_uint8(buffer, msg.owner);           // 1 byte
    write_uint8(buffer, msg.state);           // 1 byte
    write_uint8(buffer, 0);                   // padding byte 1
    write_uint8(buffer, 0);                   // padding byte 2
    write_uint32_le(buffer, msg.total_generated);  // 4 bytes
    write_uint32_le(buffer, msg.total_consumed);   // 4 bytes
    write_int32_le(buffer, msg.surplus);            // 4 bytes
    // Total: 16 bytes
}

size_t deserialize_pool_sync(const uint8_t* data, size_t size, EnergyPoolSyncMessage& msg) {
    constexpr size_t REQUIRED_SIZE = 16;
    if (size < REQUIRED_SIZE) {
        throw std::runtime_error("EnergyPoolSyncMessage deserialization: buffer too small");
    }

    size_t offset = 0;
    msg.owner = read_uint8(data, offset);
    msg.state = read_uint8(data, offset);
    msg._padding[0] = read_uint8(data, offset); // skip padding
    msg._padding[1] = read_uint8(data, offset); // skip padding
    msg.total_generated = read_uint32_le(data, offset);
    msg.total_consumed = read_uint32_le(data, offset);
    msg.surplus = read_int32_le(data, offset);

    return offset; // 16 bytes consumed
}

EnergyPoolSyncMessage create_pool_sync_message(const PerPlayerEnergyPool& pool) {
    EnergyPoolSyncMessage msg;
    msg.owner = pool.owner;
    msg.state = static_cast<uint8_t>(pool.state);
    msg._padding[0] = 0;
    msg._padding[1] = 0;
    msg.total_generated = pool.total_generated;
    msg.total_consumed = pool.total_consumed;
    msg.surplus = pool.surplus;
    return msg;
}

} // namespace energy
} // namespace sims3000
