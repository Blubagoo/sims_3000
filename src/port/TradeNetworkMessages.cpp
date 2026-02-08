/**
 * @file TradeNetworkMessages.cpp
 * @brief Implementation of trade network message serialization (Ticket E8-026)
 *
 * Uses little-endian encoding for all multi-byte fields, matching the
 * pattern established in PathwayNetworkMessages.cpp.
 */

#include <sims3000/port/TradeNetworkMessages.h>
#include <cstring>
#include <stdexcept>

namespace sims3000 {
namespace port {

// ============================================================================
// Little-endian helper functions
// ============================================================================

namespace {

void write_u8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

void write_u32_le(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

uint8_t read_u8(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

uint32_t read_u32_le(const uint8_t* data, size_t& offset) {
    uint32_t value = static_cast<uint32_t>(data[offset])
                   | (static_cast<uint32_t>(data[offset + 1]) << 8)
                   | (static_cast<uint32_t>(data[offset + 2]) << 16)
                   | (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return value;
}

} // anonymous namespace

// ============================================================================
// TradeOfferRequestMsg
// ============================================================================

void TradeOfferRequestMsg::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u8(buffer, target_player);                    // 1 byte
    write_u8(buffer, proposed_type);                    // 1 byte
    // Total: 2 bytes
}

TradeOfferRequestMsg TradeOfferRequestMsg::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TradeOfferRequestMsg::deserialize: buffer too small");
    }

    TradeOfferRequestMsg msg;
    size_t offset = 0;
    msg.target_player = read_u8(data, offset);
    msg.proposed_type = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TradeOfferResponseMsg
// ============================================================================

void TradeOfferResponseMsg::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, offer_id);                     // 4 bytes
    write_u8(buffer, accepted ? 1 : 0);                 // 1 byte
    // Total: 5 bytes
}

TradeOfferResponseMsg TradeOfferResponseMsg::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TradeOfferResponseMsg::deserialize: buffer too small");
    }

    TradeOfferResponseMsg msg;
    size_t offset = 0;
    msg.offer_id = read_u32_le(data, offset);
    msg.accepted = (read_u8(data, offset) != 0);
    return msg;
}

// ============================================================================
// TradeCancelRequestMsg
// ============================================================================

void TradeCancelRequestMsg::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, route_id);                     // 4 bytes
    // Total: 4 bytes
}

TradeCancelRequestMsg TradeCancelRequestMsg::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TradeCancelRequestMsg::deserialize: buffer too small");
    }

    TradeCancelRequestMsg msg;
    size_t offset = 0;
    msg.route_id = read_u32_le(data, offset);
    return msg;
}

// ============================================================================
// TradeOfferNotificationMsg
// ============================================================================

void TradeOfferNotificationMsg::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, offer_id);                     // 4 bytes
    write_u8(buffer, from_player);                      // 1 byte
    write_u8(buffer, proposed_type);                    // 1 byte
    // Total: 6 bytes
}

TradeOfferNotificationMsg TradeOfferNotificationMsg::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TradeOfferNotificationMsg::deserialize: buffer too small");
    }

    TradeOfferNotificationMsg msg;
    size_t offset = 0;
    msg.offer_id = read_u32_le(data, offset);
    msg.from_player = read_u8(data, offset);
    msg.proposed_type = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TradeRouteEstablishedMsg
// ============================================================================

void TradeRouteEstablishedMsg::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, route_id);                     // 4 bytes
    write_u8(buffer, party_a);                          // 1 byte
    write_u8(buffer, party_b);                          // 1 byte
    write_u8(buffer, agreement_type);                   // 1 byte
    // Total: 7 bytes
}

TradeRouteEstablishedMsg TradeRouteEstablishedMsg::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TradeRouteEstablishedMsg::deserialize: buffer too small");
    }

    TradeRouteEstablishedMsg msg;
    size_t offset = 0;
    msg.route_id = read_u32_le(data, offset);
    msg.party_a = read_u8(data, offset);
    msg.party_b = read_u8(data, offset);
    msg.agreement_type = read_u8(data, offset);
    return msg;
}

// ============================================================================
// TradeRouteCancelledMsg
// ============================================================================

void TradeRouteCancelledMsg::serialize(std::vector<uint8_t>& buffer) const {
    buffer.reserve(buffer.size() + SERIALIZED_SIZE);
    write_u32_le(buffer, route_id);                     // 4 bytes
    write_u8(buffer, cancelled_by);                     // 1 byte
    // Total: 5 bytes
}

TradeRouteCancelledMsg TradeRouteCancelledMsg::deserialize(const uint8_t* data, size_t len) {
    if (len < SERIALIZED_SIZE) {
        throw std::runtime_error("TradeRouteCancelledMsg::deserialize: buffer too small");
    }

    TradeRouteCancelledMsg msg;
    size_t offset = 0;
    msg.route_id = read_u32_le(data, offset);
    msg.cancelled_by = read_u8(data, offset);
    return msg;
}

} // namespace port
} // namespace sims3000
