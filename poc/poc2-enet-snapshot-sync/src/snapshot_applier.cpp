#include "snapshot_applier.h"
#include "message_header.h"
#include "compression.h"
#include "network_buffer.h"

SnapshotApplier::SnapshotApplier(EntityStore& store)
    : store_(store)
{
}

ApplyResult SnapshotApplier::apply_full(const uint8_t* data, size_t size) {
    ApplyResult result;
    NetworkBuffer buf(data, size);

    MessageHeader header;
    if (!header.deserialize(buf)) return result;
    if (header.type != MessageType::FullSnapshot) return result;

    result.tick = header.sequence;

    const uint8_t* payload_data = data + MessageHeader::HEADER_SIZE;
    size_t payload_size = header.payload_length;

    std::vector<uint8_t> decompressed;
    NetworkBuffer payload_buf(static_cast<size_t>(0));

    if (header.is_compressed()) {
        decompressed = Compression::decompress(payload_data, payload_size);
        payload_buf = NetworkBuffer(decompressed.data(), decompressed.size());
    } else {
        payload_buf = NetworkBuffer(payload_data, payload_size);
    }

    uint32_t entity_count = payload_buf.read_u32();
    if (entity_count != store_.count()) return result;

    payload_buf.read_bytes(store_.positions(), entity_count * sizeof(PositionComponent));
    payload_buf.read_bytes(store_.data_components(), entity_count * sizeof(DataComponent));

    result.success = true;
    result.entities_updated = entity_count;
    return result;
}

ApplyResult SnapshotApplier::apply_delta(const uint8_t* data, size_t size) {
    ApplyResult result;
    NetworkBuffer buf(data, size);

    MessageHeader header;
    if (!header.deserialize(buf)) return result;
    if (header.type != MessageType::DeltaSnapshot) return result;

    result.tick = header.sequence;

    const uint8_t* payload_data = data + MessageHeader::HEADER_SIZE;
    size_t payload_size = header.payload_length;

    std::vector<uint8_t> decompressed;
    NetworkBuffer payload_buf(static_cast<size_t>(0));

    if (header.is_compressed()) {
        decompressed = Compression::decompress(payload_data, payload_size);
        payload_buf = NetworkBuffer(decompressed.data(), decompressed.size());
    } else {
        payload_buf = NetworkBuffer(payload_data, payload_size);
    }

    // Compact format: checksum(u64) + delta_count(u16) + [entity_id(u16) + mask(u8) + fields...]
    result.server_checksum = payload_buf.read_u64();
    uint16_t delta_count = payload_buf.read_u16();

    for (uint16_t d = 0; d < delta_count; ++d) {
        uint16_t entity_id = payload_buf.read_u16();
        uint8_t mask = payload_buf.read_u8();

        if (entity_id >= store_.count()) return result;

        auto& pos = store_.position(entity_id);
        auto& dat = store_.data(entity_id);

        if (mask & FIELD_POS_X) pos.x = payload_buf.read_float();
        if (mask & FIELD_POS_Y) pos.y = payload_buf.read_float();
        if (mask & FIELD_POS_Z) pos.z = payload_buf.read_float();
        if (mask & FIELD_TYPE_ID) dat.type_id = payload_buf.read_u32();
        if (mask & FIELD_FLAGS) dat.flags = payload_buf.read_u32();
        if (mask & FIELD_VALUE) dat.value = payload_buf.read_float();
    }

    result.entities_updated = delta_count;

    // Verify checksum
    uint64_t local_checksum = store_.compute_checksum();
    result.checksum_match = (local_checksum == result.server_checksum);

    result.success = true;
    return result;
}
