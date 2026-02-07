/**
 * @file TerrainNetworkMessages.cpp
 * @brief Implementation of terrain network message serialization.
 */

#include "sims3000/terrain/TerrainNetworkMessages.h"

namespace sims3000 {
namespace terrain {

// =============================================================================
// Message Factory Registration
// =============================================================================

namespace {
    // Register message types with the factory on module load
    static bool s_requestRegistered = MessageFactory::registerType<TerrainModifyRequestMessage>(
        MessageType::TerrainModifyRequest);
    static bool s_responseRegistered = MessageFactory::registerType<TerrainModifyResponseMessage>(
        MessageType::TerrainModifyResponse);
    static bool s_eventRegistered = MessageFactory::registerType<TerrainModifiedEventMessage>(
        MessageType::TerrainModifiedEvent);
}

// =============================================================================
// Helper functions for missing NetworkBuffer methods
// =============================================================================

namespace {
    // Write a signed 16-bit integer (little-endian)
    inline void write_i16(NetworkBuffer& buffer, std::int16_t value) {
        buffer.write_u16(static_cast<std::uint16_t>(value));
    }

    // Read a signed 16-bit integer (little-endian)
    inline std::int16_t read_i16(NetworkBuffer& buffer) {
        return static_cast<std::int16_t>(buffer.read_u16());
    }

    // Write a signed 64-bit integer (little-endian)
    inline void write_i64(NetworkBuffer& buffer, std::int64_t value) {
        // Write as two 32-bit values (low then high)
        buffer.write_u32(static_cast<std::uint32_t>(value & 0xFFFFFFFF));
        buffer.write_u32(static_cast<std::uint32_t>((static_cast<std::uint64_t>(value) >> 32) & 0xFFFFFFFF));
    }

    // Read a signed 64-bit integer (little-endian)
    inline std::int64_t read_i64(NetworkBuffer& buffer) {
        std::uint32_t low = buffer.read_u32();
        std::uint32_t high = buffer.read_u32();
        return static_cast<std::int64_t>((static_cast<std::uint64_t>(high) << 32) | low);
    }
}

// =============================================================================
// TerrainModifyRequestMessage
// =============================================================================

MessageType TerrainModifyRequestMessage::getType() const {
    return MessageType::TerrainModifyRequest;
}

void TerrainModifyRequestMessage::serializePayload(NetworkBuffer& buffer) const {
    // Wire format (12 bytes):
    // [0-1]  x (i16)
    // [2-3]  y (i16)
    // [4]    operation
    // [5]    target_value
    // [6]    player_id
    // [7]    padding
    // [8-11] sequence_num (u32)
    write_i16(buffer, data.x);
    write_i16(buffer, data.y);
    buffer.write_u8(static_cast<std::uint8_t>(data.operation));
    buffer.write_u8(data.target_value);
    buffer.write_u8(data.player_id);
    buffer.write_u8(0);  // padding
    buffer.write_u32(data.sequence_num);
}

bool TerrainModifyRequestMessage::deserializePayload(NetworkBuffer& buffer) {
    if (buffer.remaining() < 12) {
        return false;
    }

    data.x = read_i16(buffer);
    data.y = read_i16(buffer);
    data.operation = static_cast<TerrainNetOpType>(buffer.read_u8());
    data.target_value = buffer.read_u8();
    data.player_id = buffer.read_u8();
    buffer.read_u8();  // padding
    data.sequence_num = buffer.read_u32();

    return true;
}

bool TerrainModifyRequestMessage::isValid() const {
    // Check operation type is known
    if (data.operation != TerrainNetOpType::Clear &&
        data.operation != TerrainNetOpType::Grade &&
        data.operation != TerrainNetOpType::Terraform) {
        return false;
    }

    // Check target value is in valid elevation range for grade
    if (data.operation == TerrainNetOpType::Grade && data.target_value > 31) {
        return false;
    }

    return true;
}

// =============================================================================
// TerrainModifyResponseMessage
// =============================================================================

MessageType TerrainModifyResponseMessage::getType() const {
    return MessageType::TerrainModifyResponse;
}

void TerrainModifyResponseMessage::serializePayload(NetworkBuffer& buffer) const {
    // Wire format (16 bytes):
    // [0-3]  sequence_num (u32)
    // [4]    result
    // [5-7]  padding
    // [8-15] cost_applied (i64)
    buffer.write_u32(data.sequence_num);
    buffer.write_u8(static_cast<std::uint8_t>(data.result));
    buffer.write_u8(0);  // padding
    buffer.write_u8(0);  // padding
    buffer.write_u8(0);  // padding
    write_i64(buffer, data.cost_applied);
}

bool TerrainModifyResponseMessage::deserializePayload(NetworkBuffer& buffer) {
    if (buffer.remaining() < 16) {
        return false;
    }

    data.sequence_num = buffer.read_u32();
    data.result = static_cast<TerrainModifyResult>(buffer.read_u8());
    buffer.read_u8();  // padding
    buffer.read_u8();  // padding
    buffer.read_u8();  // padding
    data.cost_applied = read_i64(buffer);

    return true;
}

// =============================================================================
// TerrainModifiedEventMessage
// =============================================================================

MessageType TerrainModifiedEventMessage::getType() const {
    return MessageType::TerrainModifiedEvent;
}

void TerrainModifiedEventMessage::serializePayload(NetworkBuffer& buffer) const {
    // Wire format (16 bytes):
    // [0-1]  affected_area.x (i16)
    // [2-3]  affected_area.y (i16)
    // [4-5]  affected_area.width (u16)
    // [6-7]  affected_area.height (u16)
    // [8]    modification_type
    // [9]    new_elevation
    // [10-11] padding
    // [12]   player_id
    // [13-15] padding
    write_i16(buffer, data.affected_area.x);
    write_i16(buffer, data.affected_area.y);
    buffer.write_u16(data.affected_area.width);
    buffer.write_u16(data.affected_area.height);
    buffer.write_u8(static_cast<std::uint8_t>(data.modification_type));
    buffer.write_u8(data.new_elevation);
    buffer.write_u8(0);  // padding
    buffer.write_u8(0);  // padding
    buffer.write_u8(data.player_id);
    buffer.write_u8(0);  // padding
    buffer.write_u8(0);  // padding
    buffer.write_u8(0);  // padding
}

bool TerrainModifiedEventMessage::deserializePayload(NetworkBuffer& buffer) {
    if (buffer.remaining() < 16) {
        return false;
    }

    data.affected_area.x = read_i16(buffer);
    data.affected_area.y = read_i16(buffer);
    data.affected_area.width = buffer.read_u16();
    data.affected_area.height = buffer.read_u16();
    data.modification_type = static_cast<ModificationType>(buffer.read_u8());
    data.new_elevation = buffer.read_u8();
    buffer.read_u8();  // padding
    buffer.read_u8();  // padding
    data.player_id = buffer.read_u8();
    buffer.read_u8();  // padding
    buffer.read_u8();  // padding
    buffer.read_u8();  // padding

    return true;
}

TerrainModifiedEventMessage TerrainModifiedEventMessage::fromEvent(
    const TerrainModifiedEvent& event,
    PlayerID player_id,
    std::uint8_t new_elevation) {

    TerrainModifiedEventMessage msg;
    msg.data.affected_area = event.affected_area;
    msg.data.modification_type = event.modification_type;
    msg.data.new_elevation = new_elevation;
    msg.data.player_id = player_id;
    return msg;
}

// =============================================================================
// Helper Functions
// =============================================================================

const char* getTerrainOpTypeName(TerrainNetOpType type) {
    switch (type) {
        case TerrainNetOpType::Clear:     return "Clear";
        case TerrainNetOpType::Grade:     return "Grade";
        case TerrainNetOpType::Terraform: return "Terraform";
        default:                          return "Unknown";
    }
}

const char* getTerrainModifyResultName(TerrainModifyResult result) {
    switch (result) {
        case TerrainModifyResult::Success:            return "Success";
        case TerrainModifyResult::InsufficientFunds:  return "InsufficientFunds";
        case TerrainModifyResult::NotOwner:           return "NotOwner";
        case TerrainModifyResult::InvalidLocation:    return "InvalidLocation";
        case TerrainModifyResult::NotClearable:       return "NotClearable";
        case TerrainModifyResult::NotGradeable:       return "NotGradeable";
        case TerrainModifyResult::AlreadyCleared:     return "AlreadyCleared";
        case TerrainModifyResult::AlreadyAtElevation: return "AlreadyAtElevation";
        case TerrainModifyResult::OperationInProgress:return "OperationInProgress";
        case TerrainModifyResult::InvalidOperation:   return "InvalidOperation";
        case TerrainModifyResult::ServerError:        return "ServerError";
        default:                                      return "Unknown";
    }
}

bool initTerrainNetworkMessages() {
    // The static registrations happen at file load time.
    // This function exists to force the linker to include this
    // translation unit and trigger the static initialization.
    // We can verify registration succeeded by checking the flags.
    return s_requestRegistered && s_responseRegistered && s_eventRegistered;
}

} // namespace terrain
} // namespace sims3000
