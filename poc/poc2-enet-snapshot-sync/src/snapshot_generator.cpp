#include "snapshot_generator.h"
#include "message_header.h"
#include "compression.h"

SnapshotGenerator::SnapshotGenerator(const EntityStore& store)
    : store_(store)
{
}

std::vector<uint8_t> SnapshotGenerator::generate_full(uint32_t tick) {
    NetworkBuffer payload(store_.count() * 24 + 4);
    payload.write_u32(store_.count());
    payload.write_bytes(store_.positions(), store_.count() * sizeof(PositionComponent));
    payload.write_bytes(store_.data_components(), store_.count() * sizeof(DataComponent));

    auto compressed = Compression::compress(payload.data(), payload.size());

    MessageHeader header;
    header.type = MessageType::FullSnapshot;
    header.set_compressed(true);
    header.payload_length = static_cast<uint32_t>(compressed.size());
    header.sequence = tick;

    NetworkBuffer msg;
    header.serialize(msg);
    msg.write_bytes(compressed.data(), compressed.size());
    return std::move(msg.raw());
}

// Compact delta format with 16-bit entity IDs:
// Format: checksum(u64) + delta_count(u16) + [entity_id(u16) + changed_fields(u8) + field_data...]
// Always LZ4 compressed.
static std::vector<uint8_t> serialize_delta_compact(
    const EntityStore& store,
    const uint8_t* dirty_mask,
    uint32_t count,
    uint32_t tick,
    uint64_t checksum) {

    uint32_t dirty_count = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (dirty_mask[i] != 0) ++dirty_count;
    }
    if (dirty_count == 0) return {};

    NetworkBuffer payload;
    payload.write_u64(checksum);
    payload.write_u16(static_cast<uint16_t>(dirty_count));

    for (uint32_t i = 0; i < count; ++i) {
        uint8_t mask = dirty_mask[i];
        if (mask == 0) continue;

        payload.write_u16(static_cast<uint16_t>(i));
        payload.write_u8(mask);

        const auto& pos = store.position(i);
        const auto& dat = store.data(i);

        if (mask & FIELD_POS_X) payload.write_float(pos.x);
        if (mask & FIELD_POS_Y) payload.write_float(pos.y);
        if (mask & FIELD_POS_Z) payload.write_float(pos.z);
        if (mask & FIELD_TYPE_ID) payload.write_u32(dat.type_id);
        if (mask & FIELD_FLAGS) payload.write_u32(dat.flags);
        if (mask & FIELD_VALUE) payload.write_float(dat.value);
    }

    auto compressed = Compression::compress(payload.data(), payload.size());

    MessageHeader header;
    header.type = MessageType::DeltaSnapshot;
    header.sequence = tick;
    header.set_compressed(true);
    header.payload_length = static_cast<uint32_t>(compressed.size());

    NetworkBuffer msg;
    header.serialize(msg);
    msg.write_bytes(compressed.data(), compressed.size());
    return std::move(msg.raw());
}

std::vector<uint8_t> SnapshotGenerator::generate_delta(uint32_t tick, uint64_t checksum) {
    std::vector<uint8_t> mask(store_.count());
    for (uint32_t i = 0; i < store_.count(); ++i) {
        mask[i] = store_.dirty(i);
    }
    return serialize_delta_compact(store_, mask.data(), store_.count(), tick, checksum);
}

std::vector<uint8_t> SnapshotGenerator::generate_delta_from_mask(
    uint32_t tick, uint64_t checksum,
    const uint8_t* dirty_mask, uint32_t count) {
    return serialize_delta_compact(store_, dirty_mask, count, tick, checksum);
}
