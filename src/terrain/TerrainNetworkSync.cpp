/**
 * @file TerrainNetworkSync.cpp
 * @brief Implementation of terrain network synchronization.
 */

#include "sims3000/terrain/TerrainNetworkSync.h"
#include "sims3000/terrain/ElevationGenerator.h"
#include "sims3000/terrain/BiomeGenerator.h"
#include "sims3000/terrain/WaterBodyGenerator.h"
#include "sims3000/terrain/WaterDistanceField.h"
#include "sims3000/terrain/TerrainTypes.h"

#include <cstring>
#include <algorithm>

namespace sims3000 {
namespace terrain {

// =============================================================================
// CRC32 Implementation
// =============================================================================

// Standard CRC32 polynomial (IEEE 802.3)
const std::uint32_t TerrainNetworkSync::CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7a07, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd706b3,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

std::uint32_t TerrainNetworkSync::crc32(const void* data, std::size_t size, std::uint32_t crc) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    crc = ~crc;
    for (std::size_t i = 0; i < size; ++i) {
        crc = CRC32_TABLE[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

std::uint32_t TerrainNetworkSync::computeChecksum(const TerrainGrid& grid) {
    if (grid.tiles.empty()) {
        return 0;
    }

    // Checksum dimensions and sea level first
    std::uint32_t crc = 0;
    crc = crc32(&grid.width, sizeof(grid.width), crc);
    crc = crc32(&grid.height, sizeof(grid.height), crc);
    crc = crc32(&grid.sea_level, sizeof(grid.sea_level), crc);

    // Checksum all tile data
    crc = crc32(grid.tiles.data(), grid.tiles.size() * sizeof(TerrainComponent), crc);

    return crc;
}

std::uint32_t TerrainNetworkSync::computeFullChecksum(
    const TerrainGrid& grid,
    const WaterData& waterData)
{
    std::uint32_t crc = computeChecksum(grid);

    // Add water body IDs
    if (!waterData.water_body_ids.body_ids.empty()) {
        crc = crc32(waterData.water_body_ids.body_ids.data(),
                    waterData.water_body_ids.body_ids.size() * sizeof(WaterBodyID), crc);
    }

    // Add flow directions
    if (!waterData.flow_directions.directions.empty()) {
        crc = crc32(waterData.flow_directions.directions.data(),
                    waterData.flow_directions.directions.size() * sizeof(FlowDirection), crc);
    }

    return crc;
}

// =============================================================================
// TerrainSyncRequestMessage
// =============================================================================

MessageType TerrainSyncRequestMessage::getType() const {
    return MessageType::TerrainSyncRequest;
}

void TerrainSyncRequestMessage::serializePayload(NetworkBuffer& buffer) const {
    // Write header
    buffer.write_u32(static_cast<std::uint32_t>(data.map_seed & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(data.map_seed >> 32));
    buffer.write_u16(data.width);
    buffer.write_u16(data.height);
    buffer.write_u8(data.sea_level);
    buffer.write_u8(0); // padding
    buffer.write_u8(0);
    buffer.write_u8(0);
    buffer.write_u32(data.authoritative_checksum);
    buffer.write_u32(data.modification_count);
    buffer.write_u32(data.latest_sequence);
    buffer.write_u32(data.reserved);

    // Write modifications
    for (const auto& mod : modifications) {
        buffer.write_u32(mod.sequence_num);
        buffer.write_u32(mod.timestamp_tick);
        buffer.write_u16(static_cast<std::uint16_t>(mod.x));
        buffer.write_u16(static_cast<std::uint16_t>(mod.y));
        buffer.write_u16(mod.width);
        buffer.write_u16(mod.height);
        buffer.write_u8(static_cast<std::uint8_t>(mod.modification_type));
        buffer.write_u8(mod.new_elevation);
        buffer.write_u8(mod.new_terrain_type);
        buffer.write_u8(mod.player_id);
        buffer.write_u32(0); // padding
    }
}

bool TerrainSyncRequestMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        // Read header
        std::uint32_t seedLow = buffer.read_u32();
        std::uint32_t seedHigh = buffer.read_u32();
        data.map_seed = (static_cast<std::uint64_t>(seedHigh) << 32) | seedLow;
        data.width = buffer.read_u16();
        data.height = buffer.read_u16();
        data.sea_level = buffer.read_u8();
        buffer.read_u8(); // padding
        buffer.read_u8();
        buffer.read_u8();
        data.authoritative_checksum = buffer.read_u32();
        data.modification_count = buffer.read_u32();
        data.latest_sequence = buffer.read_u32();
        data.reserved = buffer.read_u32();

        // Read modifications
        modifications.clear();
        modifications.reserve(data.modification_count);
        for (std::uint32_t i = 0; i < data.modification_count; ++i) {
            TerrainModification mod;
            mod.sequence_num = buffer.read_u32();
            mod.timestamp_tick = buffer.read_u32();
            mod.x = static_cast<std::int16_t>(buffer.read_u16());
            mod.y = static_cast<std::int16_t>(buffer.read_u16());
            mod.width = buffer.read_u16();
            mod.height = buffer.read_u16();
            mod.modification_type = static_cast<ModificationType>(buffer.read_u8());
            mod.new_elevation = buffer.read_u8();
            mod.new_terrain_type = buffer.read_u8();
            mod.player_id = buffer.read_u8();
            buffer.read_u32(); // padding
            modifications.push_back(mod);
        }
        return true;
    } catch (const BufferOverflowError&) {
        return false;
    }
}

std::size_t TerrainSyncRequestMessage::getPayloadSize() const {
    return 32 + (modifications.size() * 24);
}

// =============================================================================
// TerrainSyncVerifyMessage
// =============================================================================

MessageType TerrainSyncVerifyMessage::getType() const {
    return MessageType::TerrainSyncVerify;
}

void TerrainSyncVerifyMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(data.computed_checksum);
    buffer.write_u32(data.last_applied_sequence);
    buffer.write_u8(data.success);
    buffer.write_u8(0); // padding
    buffer.write_u8(0);
    buffer.write_u8(0);
}

bool TerrainSyncVerifyMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        data.computed_checksum = buffer.read_u32();
        data.last_applied_sequence = buffer.read_u32();
        data.success = buffer.read_u8();
        buffer.read_u8(); // padding
        buffer.read_u8();
        buffer.read_u8();
        return true;
    } catch (const BufferOverflowError&) {
        return false;
    }
}

// =============================================================================
// TerrainSyncCompleteMessage
// =============================================================================

MessageType TerrainSyncCompleteMessage::getType() const {
    return MessageType::TerrainSyncComplete;
}

void TerrainSyncCompleteMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u8(static_cast<std::uint8_t>(data.result));
    buffer.write_u8(0); // padding
    buffer.write_u8(0);
    buffer.write_u8(0);
    buffer.write_u32(data.final_sequence);
}

bool TerrainSyncCompleteMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        data.result = static_cast<TerrainSyncResult>(buffer.read_u8());
        buffer.read_u8(); // padding
        buffer.read_u8();
        buffer.read_u8();
        data.final_sequence = buffer.read_u32();
        return true;
    } catch (const BufferOverflowError&) {
        return false;
    }
}

// =============================================================================
// TerrainNetworkSync - Server-side API
// =============================================================================

void TerrainNetworkSync::setTerrainData(
    const TerrainGrid& grid,
    const WaterData& waterData,
    std::uint64_t mapSeed)
{
    m_mapSeed = mapSeed;
    m_width = grid.width;
    m_height = grid.height;
    m_seaLevel = grid.sea_level;
    m_authoritativeChecksum = computeFullChecksum(grid, waterData);
    m_modifications.clear();
    m_nextSequence = 1;
}

std::uint32_t TerrainNetworkSync::recordModification(
    const TerrainModifiedEvent& event,
    std::uint32_t tick,
    PlayerID playerId,
    std::uint8_t newElevation,
    std::uint8_t newTerrainType)
{
    std::uint32_t seqNum = m_nextSequence++;

    TerrainModification mod = TerrainModification::fromEvent(
        event, seqNum, tick, playerId, newElevation, newTerrainType);

    m_modifications.push_back(mod);

    // Note: The authoritative checksum should be updated when the actual
    // terrain data changes. This is typically done by recomputing after
    // the modification is applied to the grid.

    return seqNum;
}

TerrainSyncRequestMessage TerrainNetworkSync::createSyncRequest() const {
    TerrainSyncRequestMessage msg;
    msg.data.map_seed = m_mapSeed;
    msg.data.width = m_width;
    msg.data.height = m_height;
    msg.data.sea_level = m_seaLevel;
    msg.data.authoritative_checksum = m_authoritativeChecksum;
    msg.data.modification_count = static_cast<std::uint32_t>(m_modifications.size());
    msg.data.latest_sequence = m_nextSequence > 0 ? m_nextSequence - 1 : 0;
    msg.modifications = m_modifications;
    return msg;
}

bool TerrainNetworkSync::verifySyncResult(const TerrainSyncVerifyMessage& verifyMsg) const {
    if (!verifyMsg.data.success) {
        return false;
    }
    return verifyMsg.data.computed_checksum == m_authoritativeChecksum;
}

TerrainSyncCompleteMessage TerrainNetworkSync::createCompleteMessage(
    TerrainSyncResult result) const
{
    TerrainSyncCompleteMessage msg;
    msg.data.result = result;
    msg.data.final_sequence = m_nextSequence > 0 ? m_nextSequence - 1 : 0;
    return msg;
}

void TerrainNetworkSync::clearModificationHistory() {
    m_modifications.clear();
    // Don't reset sequence number - keep it monotonic
}

void TerrainNetworkSync::pruneModifications(std::uint32_t keepAfterSequence) {
    m_modifications.erase(
        std::remove_if(m_modifications.begin(), m_modifications.end(),
            [keepAfterSequence](const TerrainModification& mod) {
                return mod.sequence_num <= keepAfterSequence;
            }),
        m_modifications.end());
}

// =============================================================================
// TerrainNetworkSync - Client-side API
// =============================================================================

bool TerrainNetworkSync::handleSyncRequest(
    const TerrainSyncRequestMessage& request,
    TerrainGrid& grid,
    WaterData& waterData)
{
    m_state = TerrainSyncState::Generating;

    // Generate terrain from seed
    if (!generateFromSeed(grid, waterData,
                          request.data.map_seed,
                          request.data.width,
                          request.data.height,
                          request.data.sea_level))
    {
        m_state = TerrainSyncState::FallbackSnapshot;
        return false;
    }

    // Store pending modifications for replay
    m_pendingModifications = request.modifications;
    m_modificationIndex = 0;
    m_lastAppliedSequence = 0;

    // Store expected checksum for verification
    m_authoritativeChecksum = request.data.authoritative_checksum;

    // Apply all modifications immediately
    if (m_pendingModifications.empty()) {
        // No modifications, go directly to verifying
        m_state = TerrainSyncState::Verifying;
    } else {
        m_state = TerrainSyncState::ApplyingMods;
        applyAllModifications(grid);
        // applyAllModifications will set state to Verifying when done
    }

    return true;
}

bool TerrainNetworkSync::hasModificationsToApply() const {
    return m_modificationIndex < m_pendingModifications.size();
}

bool TerrainNetworkSync::applyNextModification(TerrainGrid& grid) {
    if (!hasModificationsToApply()) {
        return false;
    }

    const auto& mod = m_pendingModifications[m_modificationIndex];
    applyModification(grid, mod);
    m_lastAppliedSequence = mod.sequence_num;
    ++m_modificationIndex;

    if (!hasModificationsToApply()) {
        m_state = TerrainSyncState::Verifying;
    }

    return true;
}

std::size_t TerrainNetworkSync::applyAllModifications(TerrainGrid& grid) {
    std::size_t applied = 0;
    while (applyNextModification(grid)) {
        ++applied;
    }
    return applied;
}

TerrainSyncVerifyMessage TerrainNetworkSync::createVerifyMessage(
    const TerrainGrid& grid) const
{
    TerrainSyncVerifyMessage msg;
    msg.data.computed_checksum = computeChecksum(grid);
    msg.data.last_applied_sequence = m_lastAppliedSequence;
    msg.data.success = (m_state != TerrainSyncState::FallbackSnapshot) ? 1 : 0;
    return msg;
}

bool TerrainNetworkSync::handleSyncComplete(const TerrainSyncCompleteMessage& complete) {
    if (complete.data.result == TerrainSyncResult::Success) {
        m_state = TerrainSyncState::Complete;
        return true;
    }

    // Need fallback to full snapshot
    m_state = TerrainSyncState::FallbackSnapshot;
    return false;
}

// =============================================================================
// TerrainNetworkSync - Private Methods
// =============================================================================

void TerrainNetworkSync::applyModification(
    TerrainGrid& grid,
    const TerrainModification& mod)
{
    GridRect area = mod.getAffectedArea();

    // Clamp to grid bounds
    std::int16_t startX = std::max(static_cast<std::int16_t>(0), area.x);
    std::int16_t startY = std::max(static_cast<std::int16_t>(0), area.y);
    std::int16_t endX = std::min(static_cast<std::int16_t>(grid.width), area.right());
    std::int16_t endY = std::min(static_cast<std::int16_t>(grid.height), area.bottom());

    for (std::int16_t y = startY; y < endY; ++y) {
        for (std::int16_t x = startX; x < endX; ++x) {
            auto& tile = grid.at(x, y);

            switch (mod.modification_type) {
                case ModificationType::Cleared:
                    // Set cleared flag (terrain cleared for building)
                    tile.setCleared(true);
                    break;

                case ModificationType::Leveled:
                    // Set elevation to new value
                    tile.setElevation(mod.new_elevation);
                    break;

                case ModificationType::Terraformed:
                    // Change terrain type
                    tile.setTerrainType(static_cast<TerrainType>(mod.new_terrain_type));
                    break;

                case ModificationType::Generated:
                case ModificationType::SeaLevelChanged:
                    // These are global operations, not typically replayed per-tile
                    break;
            }
        }
    }
}

bool TerrainNetworkSync::generateFromSeed(
    TerrainGrid& grid,
    WaterData& waterData,
    std::uint64_t seed,
    std::uint16_t width,
    std::uint16_t height,
    std::uint8_t seaLevel)
{
    // Validate dimensions
    if (!isValidMapSize(width) || !isValidMapSize(height)) {
        return false;
    }

    // Initialize grid
    MapSize mapSize;
    if (width == 128) {
        mapSize = MapSize::Small;
    } else if (width == 256) {
        mapSize = MapSize::Medium;
    } else {
        mapSize = MapSize::Large;
    }

    grid.initialize(mapSize, seaLevel);

    // Initialize water data
    waterData.initialize(mapSize);

    // Run the generation pipeline with the seed
    // 1. Generate elevation
    ElevationGenerator::generate(grid, seed, ElevationConfig::defaultConfig());

    // 2. Generate water bodies and compute distance field
    WaterDistanceField distanceField(mapSize);
    WaterBodyGenerator::generate(grid, waterData, distanceField, seed, WaterBodyConfig::defaultConfig());

    // 3. Generate biomes
    BiomeGenerator::generate(grid, distanceField, seed);

    return true;
}

// =============================================================================
// Message Registration
// =============================================================================

bool initTerrainSyncMessages() {
    static bool registered = false;
    if (registered) {
        return true;
    }

    MessageFactory::registerType<TerrainSyncRequestMessage>(MessageType::TerrainSyncRequest);
    MessageFactory::registerType<TerrainSyncVerifyMessage>(MessageType::TerrainSyncVerify);
    MessageFactory::registerType<TerrainSyncCompleteMessage>(MessageType::TerrainSyncComplete);

    registered = true;
    return true;
}

// Force registration at static initialization
static bool s_terrainSyncMessagesRegistered = initTerrainSyncMessages();

} // namespace terrain
} // namespace sims3000
