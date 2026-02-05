#pragma once

#include <cstdint>
#include "network_buffer.h"

// Message types for POC-2
enum class MessageType : uint8_t {
    FullSnapshot   = 1,
    DeltaSnapshot  = 2,
    SnapshotAck    = 3,
    ResyncRequest  = 4,
};

// Flags
constexpr uint8_t FLAG_COMPRESSED = 0x01;

// ENet channel assignments
constexpr int CHANNEL_ACK = 0;           // Reliable ordered
constexpr int CHANNEL_FULL_SNAPSHOT = 1;  // Reliable unordered
constexpr int CHANNEL_DELTA = 2;          // Unreliable
constexpr int NUM_CHANNELS = 3;

// 16-byte message header
// Layout: magic[4] + version[1] + type[1] + flags[1] + padding[1] + payload_length[4] + sequence[4]
struct MessageHeader {
    static constexpr uint8_t MAGIC[4] = {'Z', 'C', 'N', 'T'};
    static constexpr uint8_t VERSION = 1;
    static constexpr size_t HEADER_SIZE = 16;

    uint8_t version = VERSION;
    MessageType type = MessageType::FullSnapshot;
    uint8_t flags = 0;
    uint32_t payload_length = 0;
    uint32_t sequence = 0;

    void serialize(NetworkBuffer& buf) const {
        buf.write_bytes(MAGIC, 4);
        buf.write_u8(version);
        buf.write_u8(static_cast<uint8_t>(type));
        buf.write_u8(flags);
        buf.write_u8(0); // padding
        buf.write_u32(payload_length);
        buf.write_u32(sequence);
    }

    bool deserialize(NetworkBuffer& buf) {
        if (buf.remaining() < HEADER_SIZE) return false;

        uint8_t magic[4];
        buf.read_bytes(magic, 4);
        if (magic[0] != MAGIC[0] || magic[1] != MAGIC[1] ||
            magic[2] != MAGIC[2] || magic[3] != MAGIC[3]) {
            return false;
        }

        version = buf.read_u8();
        type = static_cast<MessageType>(buf.read_u8());
        flags = buf.read_u8();
        buf.read_u8(); // padding
        payload_length = buf.read_u32();
        sequence = buf.read_u32();
        return true;
    }

    bool is_compressed() const { return (flags & FLAG_COMPRESSED) != 0; }
    void set_compressed(bool v) { if (v) flags |= FLAG_COMPRESSED; else flags &= ~FLAG_COMPRESSED; }
};
