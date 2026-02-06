/**
 * @file ServerMessages.cpp
 * @brief Implementation of server-to-client network messages.
 */

#include "sims3000/net/ServerMessages.h"
#include "sims3000/core/Logger.h"
#include <lz4.h>
#include <cstring>
#include <algorithm>

namespace sims3000 {

// =============================================================================
// Factory Registration
// =============================================================================

static bool stateUpdateRegistered =
    MessageFactory::registerType<StateUpdateMessage>(MessageType::StateUpdate);

static bool snapshotStartRegistered =
    MessageFactory::registerType<SnapshotStartMessage>(MessageType::SnapshotStart);

static bool snapshotChunkRegistered =
    MessageFactory::registerType<SnapshotChunkMessage>(MessageType::SnapshotChunk);

static bool snapshotEndRegistered =
    MessageFactory::registerType<SnapshotEndMessage>(MessageType::SnapshotEnd);

static bool playerListRegistered =
    MessageFactory::registerType<PlayerListMessage>(MessageType::PlayerList);

static bool rejectionRegistered =
    MessageFactory::registerType<RejectionMessage>(MessageType::Rejection);

static bool eventRegistered =
    MessageFactory::registerType<EventMessage>(MessageType::Event);

static bool heartbeatResponseRegistered =
    MessageFactory::registerType<HeartbeatResponseMessage>(MessageType::HeartbeatResponse);

static bool serverStatusRegistered =
    MessageFactory::registerType<ServerStatusMessage>(MessageType::ServerStatus);

static bool joinAcceptRegistered =
    MessageFactory::registerType<JoinAcceptMessage>(MessageType::JoinAccept);

static bool joinRejectRegistered =
    MessageFactory::registerType<JoinRejectMessage>(MessageType::JoinReject);

static bool kickRegistered =
    MessageFactory::registerType<KickMessage>(MessageType::Kick);

// Silence unused variable warnings
namespace {
    [[maybe_unused]] auto _reg1 = stateUpdateRegistered;
    [[maybe_unused]] auto _reg2 = snapshotStartRegistered;
    [[maybe_unused]] auto _reg3 = snapshotChunkRegistered;
    [[maybe_unused]] auto _reg4 = snapshotEndRegistered;
    [[maybe_unused]] auto _reg5 = playerListRegistered;
    [[maybe_unused]] auto _reg6 = rejectionRegistered;
    [[maybe_unused]] auto _reg7 = eventRegistered;
    [[maybe_unused]] auto _reg8 = heartbeatResponseRegistered;
    [[maybe_unused]] auto _reg9 = serverStatusRegistered;
    [[maybe_unused]] auto _reg10 = joinAcceptRegistered;
    [[maybe_unused]] auto _reg11 = joinRejectRegistered;
    [[maybe_unused]] auto _reg12 = kickRegistered;
}

// =============================================================================
// EntityDelta Implementation
// =============================================================================

void EntityDelta::serialize(NetworkBuffer& buffer) const {
    buffer.write_u32(entityId);
    buffer.write_u8(static_cast<std::uint8_t>(type));
    buffer.write_u32(static_cast<std::uint32_t>(componentData.size()));
    if (!componentData.empty()) {
        buffer.write_bytes(componentData.data(), componentData.size());
    }
}

bool EntityDelta::deserialize(NetworkBuffer& buffer) {
    try {
        entityId = buffer.read_u32();
        type = static_cast<EntityDeltaType>(buffer.read_u8());
        std::uint32_t dataSize = buffer.read_u32();

        if (dataSize > MAX_PAYLOAD_SIZE) {
            LOG_ERROR("EntityDelta data size too large: %u", dataSize);
            return false;
        }

        componentData.resize(dataSize);
        if (dataSize > 0) {
            buffer.read_bytes(componentData.data(), dataSize);
        }
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("EntityDelta deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// PlayerInfo Implementation
// =============================================================================

void PlayerInfo::serialize(NetworkBuffer& buffer) const {
    buffer.write_u8(playerId);
    buffer.write_string(name);
    buffer.write_u8(static_cast<std::uint8_t>(status));
    buffer.write_u32(latencyMs);
}

bool PlayerInfo::deserialize(NetworkBuffer& buffer) {
    try {
        playerId = buffer.read_u8();
        name = buffer.read_string();
        status = static_cast<PlayerStatus>(buffer.read_u8());
        latencyMs = buffer.read_u32();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("PlayerInfo deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// StateUpdateMessage Implementation
// =============================================================================

void StateUpdateMessage::serializePayload(NetworkBuffer& buffer) const {
    // First, serialize the delta data to a temporary buffer
    NetworkBuffer deltaBuffer;
    deltaBuffer.write_u32(static_cast<std::uint32_t>(tick & 0xFFFFFFFF));
    deltaBuffer.write_u32(static_cast<std::uint32_t>(tick >> 32));
    deltaBuffer.write_u32(static_cast<std::uint32_t>(deltas.size()));

    for (const auto& delta : deltas) {
        delta.serialize(deltaBuffer);
    }

    // Determine if compression should be applied (>1KB threshold per canon)
    const bool shouldCompress = deltaBuffer.size() > COMPRESSION_THRESHOLD;

    if (shouldCompress) {
        // Compress the delta data
        std::vector<std::uint8_t> uncompressed(deltaBuffer.data(),
                                                deltaBuffer.data() + deltaBuffer.size());
        std::vector<std::uint8_t> compressedData;

        if (compressLZ4(uncompressed, compressedData)) {
            // Only use compression if it actually reduces size
            if (compressedData.size() < uncompressed.size()) {
                buffer.write_u8(1);  // compressed = true
                buffer.write_bytes(compressedData.data(), compressedData.size());
                return;
            }
        }
        // Compression failed or didn't help - fall through to uncompressed
    }

    // Write uncompressed
    buffer.write_u8(0);  // compressed = false
    buffer.write_bytes(deltaBuffer.data(), deltaBuffer.size());
}

bool StateUpdateMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        compressed = buffer.read_u8() != 0;

        if (compressed) {
            // Read remaining data as compressed payload
            std::size_t compressedSize = buffer.remaining();
            std::vector<std::uint8_t> compressedData(compressedSize);
            buffer.read_bytes(compressedData.data(), compressedSize);

            // Decompress
            std::vector<std::uint8_t> decompressed;
            if (!decompressLZ4(compressedData, decompressed)) {
                LOG_ERROR("Failed to decompress StateUpdateMessage");
                return false;
            }

            // Parse decompressed data
            NetworkBuffer decompBuffer(decompressed.data(), decompressed.size());
            return parseUncompressedDeltas(decompBuffer);
        } else {
            // Parse uncompressed data directly
            return parseUncompressedDeltas(buffer);
        }
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("StateUpdateMessage deserialize failed: %s", e.what());
        return false;
    }
}

bool StateUpdateMessage::parseUncompressedDeltas(NetworkBuffer& buffer) {
    std::uint32_t tickLow = buffer.read_u32();
    std::uint32_t tickHigh = buffer.read_u32();
    tick = (static_cast<SimulationTick>(tickHigh) << 32) | tickLow;

    std::uint32_t deltaCount = buffer.read_u32();
    if (deltaCount > MAX_ENTITY_DELTAS_PER_MESSAGE) {
        LOG_ERROR("Too many deltas in StateUpdate: %u", deltaCount);
        return false;
    }

    deltas.clear();
    deltas.reserve(deltaCount);

    for (std::uint32_t i = 0; i < deltaCount; ++i) {
        EntityDelta delta;
        if (!delta.deserialize(buffer)) {
            return false;
        }
        deltas.push_back(std::move(delta));
    }

    return true;
}

std::size_t StateUpdateMessage::getPayloadSize() const {
    // 1 (compressed) + 8 (tick) + 4 (count) + sum of delta sizes
    std::size_t size = 1 + 8 + 4;
    for (const auto& delta : deltas) {
        // 4 (entityId) + 1 (type) + 4 (dataSize) + data
        size += 4 + 1 + 4 + delta.componentData.size();
    }
    return size;
}

void StateUpdateMessage::addCreate(EntityID id, const std::vector<std::uint8_t>& components) {
    EntityDelta delta;
    delta.entityId = id;
    delta.type = EntityDeltaType::Create;
    delta.componentData = components;
    deltas.push_back(std::move(delta));
}

void StateUpdateMessage::addUpdate(EntityID id, const std::vector<std::uint8_t>& components) {
    EntityDelta delta;
    delta.entityId = id;
    delta.type = EntityDeltaType::Update;
    delta.componentData = components;
    deltas.push_back(std::move(delta));
}

void StateUpdateMessage::addDestroy(EntityID id) {
    EntityDelta delta;
    delta.entityId = id;
    delta.type = EntityDeltaType::Destroy;
    deltas.push_back(std::move(delta));
}

void StateUpdateMessage::clear() {
    tick = 0;
    deltas.clear();
    compressed = false;
}

// =============================================================================
// SnapshotStartMessage Implementation
// =============================================================================

void SnapshotStartMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(static_cast<std::uint32_t>(tick & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(tick >> 32));
    buffer.write_u32(totalChunks);
    buffer.write_u32(totalBytes);
    buffer.write_u32(compressedBytes);
    buffer.write_u32(entityCount);
}

bool SnapshotStartMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        std::uint32_t tickLow = buffer.read_u32();
        std::uint32_t tickHigh = buffer.read_u32();
        tick = (static_cast<SimulationTick>(tickHigh) << 32) | tickLow;
        totalChunks = buffer.read_u32();
        totalBytes = buffer.read_u32();
        compressedBytes = buffer.read_u32();
        entityCount = buffer.read_u32();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("SnapshotStartMessage deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// SnapshotChunkMessage Implementation
// =============================================================================

void SnapshotChunkMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(chunkIndex);
    buffer.write_u32(static_cast<std::uint32_t>(data.size()));
    if (!data.empty()) {
        buffer.write_bytes(data.data(), data.size());
    }
}

bool SnapshotChunkMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        chunkIndex = buffer.read_u32();
        std::uint32_t dataSize = buffer.read_u32();

        if (dataSize > SNAPSHOT_CHUNK_SIZE) {
            LOG_ERROR("SnapshotChunk data size too large: %u", dataSize);
            return false;
        }

        data.resize(dataSize);
        if (dataSize > 0) {
            buffer.read_bytes(data.data(), dataSize);
        }
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("SnapshotChunkMessage deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// SnapshotEndMessage Implementation
// =============================================================================

void SnapshotEndMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(checksum);
}

bool SnapshotEndMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        checksum = buffer.read_u32();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("SnapshotEndMessage deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// PlayerListMessage Implementation
// =============================================================================

void PlayerListMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u8(static_cast<std::uint8_t>(players.size()));
    for (const auto& player : players) {
        player.serialize(buffer);
    }
}

bool PlayerListMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        std::uint8_t playerCount = buffer.read_u8();
        players.clear();
        players.reserve(playerCount);

        for (std::uint8_t i = 0; i < playerCount; ++i) {
            PlayerInfo info;
            if (!info.deserialize(buffer)) {
                return false;
            }
            players.push_back(std::move(info));
        }
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("PlayerListMessage deserialize failed: %s", e.what());
        return false;
    }
}

std::size_t PlayerListMessage::getPayloadSize() const {
    std::size_t size = 1;  // player count
    for (const auto& player : players) {
        // 1 (id) + 4 (name length) + name + 1 (status) + 4 (latency)
        size += 1 + 4 + player.name.size() + 1 + 4;
    }
    return size;
}

void PlayerListMessage::addPlayer(PlayerID id, const std::string& name,
                                  PlayerStatus status, std::uint32_t latencyMs) {
    PlayerInfo info;
    info.playerId = id;
    info.name = name;
    info.status = status;
    info.latencyMs = latencyMs;
    players.push_back(std::move(info));
}

const PlayerInfo* PlayerListMessage::findPlayer(PlayerID id) const {
    for (const auto& player : players) {
        if (player.playerId == id) {
            return &player;
        }
    }
    return nullptr;
}

// =============================================================================
// RejectionMessage Implementation
// =============================================================================

void RejectionMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(inputSequenceNum);
    buffer.write_u8(static_cast<std::uint8_t>(reason));
    buffer.write_string(message);
}

bool RejectionMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        inputSequenceNum = buffer.read_u32();
        reason = static_cast<RejectionReason>(buffer.read_u8());
        message = buffer.read_string();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("RejectionMessage deserialize failed: %s", e.what());
        return false;
    }
}

const char* RejectionMessage::getDefaultMessage(RejectionReason reason) {
    switch (reason) {
        case RejectionReason::None:
            return "No error";
        case RejectionReason::InsufficientFunds:
            return "Insufficient funds to complete this action";
        case RejectionReason::InvalidLocation:
            return "Invalid location for this action";
        case RejectionReason::AreaOccupied:
            return "This area is already occupied";
        case RejectionReason::NotOwner:
            return "You do not own this entity";
        case RejectionReason::BuildingLimitReached:
            return "Building limit has been reached";
        case RejectionReason::InvalidBuildingType:
            return "Invalid building type";
        case RejectionReason::ZoneConflict:
            return "Zone conflict prevents this action";
        case RejectionReason::InfrastructureRequired:
            return "Required infrastructure is missing";
        case RejectionReason::ActionNotAllowed:
            return "This action is not allowed";
        case RejectionReason::ServerBusy:
            return "Server is too busy to process this action";
        case RejectionReason::RateLimited:
            return "Action rate limit exceeded";
        case RejectionReason::InvalidInput:
            return "Invalid input data";
        case RejectionReason::Unknown:
        default:
            return "Unknown error";
    }
}

// =============================================================================
// EventMessage Implementation
// =============================================================================

void EventMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(static_cast<std::uint32_t>(tick & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(tick >> 32));
    buffer.write_u8(static_cast<std::uint8_t>(eventType));
    buffer.write_u32(relatedEntity);
    buffer.write_i32(static_cast<std::int32_t>(location.x));
    buffer.write_i32(static_cast<std::int32_t>(location.y));
    buffer.write_u32(param1);
    buffer.write_u32(param2);
    buffer.write_string(description);
}

bool EventMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        std::uint32_t tickLow = buffer.read_u32();
        std::uint32_t tickHigh = buffer.read_u32();
        tick = (static_cast<SimulationTick>(tickHigh) << 32) | tickLow;
        eventType = static_cast<GameEventType>(buffer.read_u8());
        relatedEntity = buffer.read_u32();
        location.x = static_cast<std::int16_t>(buffer.read_i32());
        location.y = static_cast<std::int16_t>(buffer.read_i32());
        param1 = buffer.read_u32();
        param2 = buffer.read_u32();
        description = buffer.read_string();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("EventMessage deserialize failed: %s", e.what());
        return false;
    }
}

std::size_t EventMessage::getPayloadSize() const {
    // 8 (tick) + 1 (type) + 4 (entity) + 8 (location) + 4 + 4 + 4 + string
    return 8 + 1 + 4 + 8 + 4 + 4 + 4 + description.size();
}

// =============================================================================
// HeartbeatResponseMessage Implementation
// =============================================================================

void HeartbeatResponseMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(static_cast<std::uint32_t>(clientTimestamp & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(clientTimestamp >> 32));
    buffer.write_u32(static_cast<std::uint32_t>(serverTimestamp & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(serverTimestamp >> 32));
    buffer.write_u32(static_cast<std::uint32_t>(serverTick & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(serverTick >> 32));
}

bool HeartbeatResponseMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        std::uint32_t clientLow = buffer.read_u32();
        std::uint32_t clientHigh = buffer.read_u32();
        clientTimestamp = (static_cast<std::uint64_t>(clientHigh) << 32) | clientLow;

        std::uint32_t serverLow = buffer.read_u32();
        std::uint32_t serverHigh = buffer.read_u32();
        serverTimestamp = (static_cast<std::uint64_t>(serverHigh) << 32) | serverLow;

        std::uint32_t tickLow = buffer.read_u32();
        std::uint32_t tickHigh = buffer.read_u32();
        serverTick = (static_cast<SimulationTick>(tickHigh) << 32) | tickLow;

        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("HeartbeatResponseMessage deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// ServerStatusMessage Implementation
// =============================================================================

void ServerStatusMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u8(static_cast<std::uint8_t>(state));
    buffer.write_u8(static_cast<std::uint8_t>(mapSizeTier));
    buffer.write_u16(mapWidth);
    buffer.write_u16(mapHeight);
    buffer.write_u8(maxPlayers);
    buffer.write_u8(currentPlayers);
    buffer.write_u32(static_cast<std::uint32_t>(currentTick & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(currentTick >> 32));
    buffer.write_string(serverName);
}

bool ServerStatusMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        state = static_cast<ServerState>(buffer.read_u8());
        mapSizeTier = static_cast<MapSizeTier>(buffer.read_u8());
        mapWidth = buffer.read_u16();
        mapHeight = buffer.read_u16();
        maxPlayers = buffer.read_u8();
        currentPlayers = buffer.read_u8();

        std::uint32_t tickLow = buffer.read_u32();
        std::uint32_t tickHigh = buffer.read_u32();
        currentTick = (static_cast<SimulationTick>(tickHigh) << 32) | tickLow;

        serverName = buffer.read_string();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("ServerStatusMessage deserialize failed: %s", e.what());
        return false;
    }
}

std::size_t ServerStatusMessage::getPayloadSize() const {
    // 1 (state) + 1 (tier) + 2 (width) + 2 (height) + 1 (max) + 1 (current)
    // + 8 (tick) + 4 (name length) + name
    return 1 + 1 + 2 + 2 + 1 + 1 + 8 + 4 + serverName.size();
}

void ServerStatusMessage::getDimensionsForTier(MapSizeTier tier,
                                               std::uint16_t& width,
                                               std::uint16_t& height) {
    // Delegate to the canonical function in types.h
    getMapDimensionsForTier(tier, width, height);
}

// =============================================================================
// Compression Utilities Implementation
// =============================================================================

bool compressLZ4(const std::vector<std::uint8_t>& input,
                 std::vector<std::uint8_t>& output) {
    if (input.empty()) {
        output.clear();
        return true;
    }

    // Calculate max compressed size
    int maxCompressedSize = LZ4_compressBound(static_cast<int>(input.size()));
    if (maxCompressedSize <= 0) {
        LOG_ERROR("LZ4_compressBound failed for input size %zu", input.size());
        return false;
    }

    // Allocate output buffer: 4 bytes for original size + compressed data
    output.resize(sizeof(std::uint32_t) + static_cast<std::size_t>(maxCompressedSize));

    // Store original size (little-endian)
    std::uint32_t originalSize = static_cast<std::uint32_t>(input.size());
    std::memcpy(output.data(), &originalSize, sizeof(std::uint32_t));

    // Compress
    int compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(input.data()),
        reinterpret_cast<char*>(output.data() + sizeof(std::uint32_t)),
        static_cast<int>(input.size()),
        maxCompressedSize
    );

    if (compressedSize <= 0) {
        LOG_ERROR("LZ4_compress_default failed");
        output.clear();
        return false;
    }

    // Resize to actual size
    output.resize(sizeof(std::uint32_t) + static_cast<std::size_t>(compressedSize));
    return true;
}

bool decompressLZ4(const std::vector<std::uint8_t>& input,
                   std::vector<std::uint8_t>& output) {
    if (input.size() < sizeof(std::uint32_t)) {
        LOG_ERROR("LZ4 decompress: input too small (%zu bytes)", input.size());
        return false;
    }

    // Read original size
    std::uint32_t originalSize;
    std::memcpy(&originalSize, input.data(), sizeof(std::uint32_t));

    // Sanity check on size (max 50MB for snapshots)
    constexpr std::uint32_t MAX_DECOMPRESSED_SIZE = 50 * 1024 * 1024;
    if (originalSize > MAX_DECOMPRESSED_SIZE) {
        LOG_ERROR("LZ4 decompress: original size too large (%u bytes)", originalSize);
        return false;
    }

    output.resize(originalSize);

    int decompressedSize = LZ4_decompress_safe(
        reinterpret_cast<const char*>(input.data() + sizeof(std::uint32_t)),
        reinterpret_cast<char*>(output.data()),
        static_cast<int>(input.size() - sizeof(std::uint32_t)),
        static_cast<int>(originalSize)
    );

    if (decompressedSize < 0) {
        LOG_ERROR("LZ4_decompress_safe failed");
        output.clear();
        return false;
    }

    if (static_cast<std::uint32_t>(decompressedSize) != originalSize) {
        LOG_ERROR("LZ4 decompress: size mismatch (got %d, expected %u)",
                  decompressedSize, originalSize);
        output.clear();
        return false;
    }

    return true;
}

std::vector<std::vector<std::uint8_t>> splitIntoChunks(
    const std::vector<std::uint8_t>& data,
    std::size_t chunkSize) {

    std::vector<std::vector<std::uint8_t>> chunks;

    if (data.empty()) {
        return chunks;
    }

    std::size_t offset = 0;
    while (offset < data.size()) {
        std::size_t remaining = data.size() - offset;
        std::size_t thisChunkSize = std::min(remaining, chunkSize);

        chunks.emplace_back(data.begin() + static_cast<std::ptrdiff_t>(offset),
                           data.begin() + static_cast<std::ptrdiff_t>(offset + thisChunkSize));
        offset += thisChunkSize;
    }

    return chunks;
}

std::vector<std::uint8_t> reassembleChunks(
    const std::vector<std::vector<std::uint8_t>>& chunks) {

    std::size_t totalSize = 0;
    for (const auto& chunk : chunks) {
        totalSize += chunk.size();
    }

    std::vector<std::uint8_t> data;
    data.reserve(totalSize);

    for (const auto& chunk : chunks) {
        data.insert(data.end(), chunk.begin(), chunk.end());
    }

    return data;
}

// =============================================================================
// JoinAcceptMessage Implementation
// =============================================================================

void JoinAcceptMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u8(playerId);
    buffer.write_bytes(sessionToken.data(), sessionToken.size());
    buffer.write_u32(static_cast<std::uint32_t>(serverTick & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(serverTick >> 32));
}

bool JoinAcceptMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        playerId = buffer.read_u8();
        buffer.read_bytes(sessionToken.data(), sessionToken.size());
        std::uint32_t tickLow = buffer.read_u32();
        std::uint32_t tickHigh = buffer.read_u32();
        serverTick = (static_cast<SimulationTick>(tickHigh) << 32) | tickLow;
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("JoinAcceptMessage deserialize failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// JoinRejectMessage Implementation
// =============================================================================

void JoinRejectMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u8(static_cast<std::uint8_t>(reason));
    buffer.write_string(message);
}

bool JoinRejectMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        reason = static_cast<JoinRejectReason>(buffer.read_u8());
        message = buffer.read_string();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("JoinRejectMessage deserialize failed: %s", e.what());
        return false;
    }
}

const char* JoinRejectMessage::getDefaultMessage(JoinRejectReason reason) {
    switch (reason) {
        case JoinRejectReason::ServerFull:
            return "Server is full";
        case JoinRejectReason::InvalidName:
            return "Invalid player name";
        case JoinRejectReason::Banned:
            return "You have been banned from this server";
        case JoinRejectReason::InvalidToken:
            return "Invalid session token";
        case JoinRejectReason::SessionExpired:
            return "Session has expired";
        case JoinRejectReason::Unknown:
        default:
            return "Unknown error";
    }
}

// =============================================================================
// KickMessage Implementation
// =============================================================================

void KickMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_string(reason);
}

bool KickMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        reason = buffer.read_string();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("KickMessage deserialize failed: %s", e.what());
        return false;
    }
}

} // namespace sims3000
