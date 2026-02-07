/**
 * @file TerrainNetworkSync.h
 * @brief Optimized network sync for terrain using seed + modifications.
 *
 * Implements bandwidth-efficient terrain synchronization:
 * - On player join: server sends map_seed + ordered list of modifications
 * - Client generates terrain locally from seed (deterministic)
 * - Client applies modification records in order after generation
 * - Verification: client computes checksum, server compares with authoritative
 * - Fallback: full TerrainGrid snapshot if checksum mismatch
 * - During gameplay: TerrainModifiedEvent broadcast on each modification
 *
 * Network bandwidth comparison:
 * - Full 256x256 snapshot: ~448KB
 * - Seed + typical modifications: < 1KB
 *
 * Message flow:
 * 1. Client connects
 * 2. Server sends TerrainSyncRequest with map_seed + modifications
 * 3. Client generates terrain from seed
 * 4. Client applies modifications in sequence order
 * 5. Client computes checksum, sends TerrainSyncVerify
 * 6. Server compares checksums:
 *    - Match: TerrainSyncComplete (success)
 *    - Mismatch: Falls back to full snapshot (ticket 3-036)
 *
 * @see TerrainGridSerializer.h for full snapshot serialization
 * @see TerrainNetworkMessages.h for real-time modification broadcasts
 * @see ProceduralNoise.h for deterministic seeded generation
 */

#ifndef SIMS3000_TERRAIN_TERRAINNETWORKSYNC_H
#define SIMS3000_TERRAIN_TERRAINNETWORKSYNC_H

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/terrain/TerrainGrid.h"
#include "sims3000/terrain/TerrainEvents.h"
#include "sims3000/terrain/WaterData.h"
#include "sims3000/core/types.h"
#include <cstdint>
#include <vector>
#include <type_traits>

namespace sims3000 {
namespace terrain {

// Forward declarations
struct TerrainGrid;
struct WaterData;

// =============================================================================
// TerrainModification Record
// =============================================================================

/**
 * @struct TerrainModification
 * @brief Record of a single terrain modification for network replay.
 *
 * Stores all information needed to replay a terrain modification on the client.
 * Modifications are applied in sequence order to ensure consistency.
 *
 * Wire format (24 bytes):
 *   [0-3]   sequence_num (u32) - ordering for replay
 *   [4-7]   timestamp_tick (u32) - simulation tick when applied
 *   [8-9]   x (i16) - X coordinate
 *   [10-11] y (i16) - Y coordinate
 *   [12-13] width (u16) - width of affected area
 *   [14-15] height (u16) - height of affected area
 *   [16]    modification_type (ModificationType)
 *   [17]    new_elevation (u8) - for Leveled type
 *   [18]    new_terrain_type (u8) - for Terraformed type
 *   [19]    player_id (u8) - who made the modification
 *   [20-23] padding (4 bytes for alignment)
 */
struct TerrainModification {
    std::uint32_t sequence_num = 0;      ///< Sequence number for ordering
    std::uint32_t timestamp_tick = 0;    ///< Simulation tick when applied
    std::int16_t x = 0;                  ///< X coordinate of affected area
    std::int16_t y = 0;                  ///< Y coordinate of affected area
    std::uint16_t width = 1;             ///< Width of affected area
    std::uint16_t height = 1;            ///< Height of affected area
    ModificationType modification_type = ModificationType::Cleared;
    std::uint8_t new_elevation = 0;      ///< New elevation (for Leveled)
    std::uint8_t new_terrain_type = 0;   ///< New terrain type (for Terraformed)
    PlayerID player_id = 0;              ///< Player who made the modification
    std::uint8_t padding[4] = {0, 0, 0, 0}; ///< Alignment padding

    /**
     * @brief Get the affected area as a GridRect.
     */
    GridRect getAffectedArea() const {
        GridRect rect;
        rect.x = x;
        rect.y = y;
        rect.width = width;
        rect.height = height;
        return rect;
    }

    /**
     * @brief Create from a TerrainModifiedEvent.
     */
    static TerrainModification fromEvent(
        const TerrainModifiedEvent& event,
        std::uint32_t seqNum,
        std::uint32_t tick,
        PlayerID player,
        std::uint8_t elevation = 0,
        std::uint8_t terrainType = 0
    ) {
        TerrainModification mod;
        mod.sequence_num = seqNum;
        mod.timestamp_tick = tick;
        mod.x = event.affected_area.x;
        mod.y = event.affected_area.y;
        mod.width = event.affected_area.width;
        mod.height = event.affected_area.height;
        mod.modification_type = event.modification_type;
        mod.new_elevation = elevation;
        mod.new_terrain_type = terrainType;
        mod.player_id = player;
        return mod;
    }
};

// Verify TerrainModification is trivially copyable for network serialization
static_assert(std::is_trivially_copyable<TerrainModification>::value,
    "TerrainModification must be trivially copyable");
static_assert(sizeof(TerrainModification) == 24,
    "TerrainModification must be 24 bytes");

// =============================================================================
// TerrainSyncRequest Message (Server -> Client)
// =============================================================================

/**
 * @struct TerrainSyncRequestData
 * @brief Data for terrain sync request containing seed and modifications.
 *
 * Contains all information needed for client to regenerate terrain:
 * - Map seed for deterministic generation
 * - Map dimensions
 * - Sea level
 * - List of modifications since generation
 * - Authoritative checksum for verification
 *
 * Wire format (32 bytes):
 *   [0-7]   map_seed (u64)
 *   [8-9]   width (u16)
 *   [10-11] height (u16)
 *   [12]    sea_level (u8)
 *   [13-15] padding (3 bytes)
 *   [16-19] authoritative_checksum (u32)
 *   [20-23] modification_count (u32)
 *   [24-27] latest_sequence (u32)
 *   [28-31] reserved (4 bytes)
 */
struct TerrainSyncRequestData {
    std::uint64_t map_seed = 0;           ///< Seed for terrain generation
    std::uint16_t width = 0;              ///< Grid width (128, 256, or 512)
    std::uint16_t height = 0;             ///< Grid height (128, 256, or 512)
    std::uint8_t sea_level = 8;           ///< Sea level elevation
    std::uint8_t padding[3] = {0, 0, 0};  ///< Alignment padding
    std::uint32_t authoritative_checksum = 0; ///< Checksum for verification
    std::uint32_t modification_count = 0; ///< Number of modifications following
    std::uint32_t latest_sequence = 0;    ///< Latest modification sequence number
    std::uint32_t reserved = 0;           ///< Reserved for future use
    // Modifications follow in the payload (variable length)
};

static_assert(sizeof(TerrainSyncRequestData) == 32,
    "TerrainSyncRequestData must be 32 bytes");

/**
 * @class TerrainSyncRequestMessage
 * @brief Network message for terrain sync with seed + modifications.
 *
 * Sent from server to client on connection to synchronize terrain state.
 * Client regenerates terrain from seed, applies modifications, and verifies.
 *
 * Wire format:
 *   [28 bytes] TerrainSyncRequestData header
 *   [N * 24 bytes] TerrainModification records (N = modification_count)
 */
class TerrainSyncRequestMessage : public NetworkMessage {
public:
    TerrainSyncRequestData data;
    std::vector<TerrainModification> modifications;

    MessageType getType() const override;
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;
};

// =============================================================================
// TerrainSyncVerify Message (Client -> Server)
// =============================================================================

/**
 * @struct TerrainSyncVerifyData
 * @brief Data for client terrain verification response.
 *
 * Sent by client after regenerating terrain and applying modifications.
 * Server compares checksum to determine if sync was successful.
 */
struct TerrainSyncVerifyData {
    std::uint32_t computed_checksum = 0;  ///< Client's computed checksum
    std::uint32_t last_applied_sequence = 0; ///< Last modification sequence applied
    std::uint8_t success = 1;             ///< 1 if generation succeeded, 0 if failed
    std::uint8_t padding[3] = {0, 0, 0};  ///< Alignment padding
};

static_assert(sizeof(TerrainSyncVerifyData) == 12,
    "TerrainSyncVerifyData must be 12 bytes");

/**
 * @class TerrainSyncVerifyMessage
 * @brief Network message for client verification of terrain sync.
 *
 * Sent from client to server after regenerating terrain.
 * Server uses checksum to verify deterministic generation succeeded.
 */
class TerrainSyncVerifyMessage : public NetworkMessage {
public:
    TerrainSyncVerifyData data;

    MessageType getType() const override;
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return sizeof(TerrainSyncVerifyData); }
};

// =============================================================================
// TerrainSyncComplete Message (Server -> Client)
// =============================================================================

/**
 * @enum TerrainSyncResult
 * @brief Result codes for terrain synchronization.
 */
enum class TerrainSyncResult : std::uint8_t {
    Success = 0,           ///< Sync completed successfully
    ChecksumMismatch = 1,  ///< Checksum didn't match, fallback to snapshot
    GenerationFailed = 2,  ///< Client failed to generate terrain
    SnapshotFallback = 3   ///< Using full snapshot instead
};

/**
 * @struct TerrainSyncCompleteData
 * @brief Data for terrain sync completion notification.
 */
struct TerrainSyncCompleteData {
    TerrainSyncResult result = TerrainSyncResult::Success;
    std::uint8_t padding[3] = {0, 0, 0};
    std::uint32_t final_sequence = 0;  ///< Final modification sequence number
};

static_assert(sizeof(TerrainSyncCompleteData) == 8,
    "TerrainSyncCompleteData must be 8 bytes");

/**
 * @class TerrainSyncCompleteMessage
 * @brief Network message for terrain sync completion.
 *
 * Sent from server to client to indicate sync result.
 * If result is ChecksumMismatch, full snapshot transfer follows.
 */
class TerrainSyncCompleteMessage : public NetworkMessage {
public:
    TerrainSyncCompleteData data;

    MessageType getType() const override;
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return sizeof(TerrainSyncCompleteData); }
};

// =============================================================================
// TerrainNetworkSync Manager
// =============================================================================

/**
 * @enum TerrainSyncState
 * @brief State machine for terrain synchronization process.
 */
enum class TerrainSyncState : std::uint8_t {
    Idle = 0,              ///< No sync in progress
    AwaitingRequest = 1,   ///< Client waiting for sync request
    Generating = 2,        ///< Client generating terrain from seed
    ApplyingMods = 3,      ///< Client applying modification records
    Verifying = 4,         ///< Sent verify, waiting for complete
    Complete = 5,          ///< Sync completed successfully
    FallbackSnapshot = 6   ///< Falling back to full snapshot
};

/**
 * @class TerrainNetworkSync
 * @brief Manages terrain synchronization between server and clients.
 *
 * Server-side usage:
 * @code
 *   TerrainNetworkSync sync;
 *   sync.setTerrainData(grid, waterData, mapSeed);
 *
 *   // Record modifications as they happen
 *   sync.recordModification(event, tick, playerId);
 *
 *   // When client connects, create sync request
 *   auto request = sync.createSyncRequest();
 *   server.send(clientPeer, request);
 *
 *   // On verify response, check result
 *   if (sync.verifySyncResult(verifyMsg)) {
 *       // Send complete message
 *   } else {
 *       // Fall back to full snapshot
 *   }
 * @endcode
 *
 * Client-side usage:
 * @code
 *   TerrainNetworkSync sync;
 *
 *   // On receiving sync request
 *   sync.handleSyncRequest(requestMsg, grid, waterData);
 *
 *   // Apply modifications
 *   while (sync.hasModificationsToApply()) {
 *       sync.applyNextModification(grid);
 *   }
 *
 *   // Send verification
 *   auto verify = sync.createVerifyMessage(grid);
 *   client.send(verify);
 * @endcode
 */
class TerrainNetworkSync {
public:
    TerrainNetworkSync() = default;
    ~TerrainNetworkSync() = default;

    // Non-copyable
    TerrainNetworkSync(const TerrainNetworkSync&) = delete;
    TerrainNetworkSync& operator=(const TerrainNetworkSync&) = delete;

    // =========================================================================
    // Server-side API
    // =========================================================================

    /**
     * @brief Set the terrain data for synchronization (server-side).
     *
     * @param grid The authoritative terrain grid.
     * @param waterData The authoritative water data.
     * @param mapSeed The seed used to generate the terrain.
     */
    void setTerrainData(const TerrainGrid& grid, const WaterData& waterData,
                        std::uint64_t mapSeed);

    /**
     * @brief Record a terrain modification (server-side).
     *
     * Adds the modification to the history for replay on clients.
     * Also updates the authoritative checksum.
     *
     * @param event The terrain modification event.
     * @param tick The simulation tick when the modification occurred.
     * @param playerId The player who made the modification.
     * @param newElevation New elevation value (for Leveled type).
     * @param newTerrainType New terrain type (for Terraformed type).
     * @return The sequence number assigned to this modification.
     */
    std::uint32_t recordModification(
        const TerrainModifiedEvent& event,
        std::uint32_t tick,
        PlayerID playerId,
        std::uint8_t newElevation = 0,
        std::uint8_t newTerrainType = 0);

    /**
     * @brief Create a sync request message for a connecting client.
     *
     * @return TerrainSyncRequestMessage containing seed and modifications.
     */
    TerrainSyncRequestMessage createSyncRequest() const;

    /**
     * @brief Verify client's sync result.
     *
     * @param verifyMsg The verification message from client.
     * @return true if checksum matches, false if mismatch (need snapshot).
     */
    bool verifySyncResult(const TerrainSyncVerifyMessage& verifyMsg) const;

    /**
     * @brief Create a sync complete message.
     *
     * @param result The sync result to send.
     * @return TerrainSyncCompleteMessage to send to client.
     */
    TerrainSyncCompleteMessage createCompleteMessage(TerrainSyncResult result) const;

    /**
     * @brief Get the authoritative checksum.
     */
    std::uint32_t getAuthoritativeChecksum() const { return m_authoritativeChecksum; }

    /**
     * @brief Get the current modification count.
     */
    std::size_t getModificationCount() const { return m_modifications.size(); }

    /**
     * @brief Get the latest modification sequence number.
     */
    std::uint32_t getLatestSequence() const { return m_nextSequence - 1; }

    /**
     * @brief Clear modification history (e.g., after full snapshot).
     *
     * Call this after sending a full snapshot to reset modification tracking.
     */
    void clearModificationHistory();

    /**
     * @brief Prune old modifications to limit memory usage.
     *
     * Keeps only modifications after the specified sequence.
     *
     * @param keepAfterSequence Keep modifications with sequence > this value.
     */
    void pruneModifications(std::uint32_t keepAfterSequence);

    // =========================================================================
    // Client-side API
    // =========================================================================

    /**
     * @brief Handle a sync request from the server (client-side).
     *
     * Generates terrain from seed using the same generation pipeline.
     *
     * @param request The sync request message.
     * @param grid Output: The terrain grid to fill.
     * @param waterData Output: The water data to fill.
     * @return true if generation succeeded, false on error.
     */
    bool handleSyncRequest(
        const TerrainSyncRequestMessage& request,
        TerrainGrid& grid,
        WaterData& waterData);

    /**
     * @brief Check if there are modifications pending to apply.
     */
    bool hasModificationsToApply() const;

    /**
     * @brief Apply the next pending modification to the grid.
     *
     * @param grid The terrain grid to modify.
     * @return true if a modification was applied, false if none pending.
     */
    bool applyNextModification(TerrainGrid& grid);

    /**
     * @brief Apply all pending modifications to the grid.
     *
     * @param grid The terrain grid to modify.
     * @return Number of modifications applied.
     */
    std::size_t applyAllModifications(TerrainGrid& grid);

    /**
     * @brief Create a verification message (client-side).
     *
     * Computes checksum of the local terrain and creates verify message.
     *
     * @param grid The generated terrain grid.
     * @return TerrainSyncVerifyMessage to send to server.
     */
    TerrainSyncVerifyMessage createVerifyMessage(const TerrainGrid& grid) const;

    /**
     * @brief Handle sync complete message from server.
     *
     * @param complete The sync complete message.
     * @return true if sync succeeded, false if fallback needed.
     */
    bool handleSyncComplete(const TerrainSyncCompleteMessage& complete);

    /**
     * @brief Get the current sync state.
     */
    TerrainSyncState getState() const { return m_state; }

    /**
     * @brief Check if sync needs fallback to full snapshot.
     */
    bool needsSnapshotFallback() const { return m_state == TerrainSyncState::FallbackSnapshot; }

    // =========================================================================
    // Checksum Utilities
    // =========================================================================

    /**
     * @brief Compute checksum of a terrain grid.
     *
     * Uses CRC32 over all tile data for efficient verification.
     *
     * @param grid The terrain grid to checksum.
     * @return CRC32 checksum value.
     */
    static std::uint32_t computeChecksum(const TerrainGrid& grid);

    /**
     * @brief Compute checksum of terrain + water data.
     *
     * @param grid The terrain grid.
     * @param waterData The water data.
     * @return Combined CRC32 checksum.
     */
    static std::uint32_t computeFullChecksum(
        const TerrainGrid& grid,
        const WaterData& waterData);

private:
    // =========================================================================
    // Terrain Data (Server-side)
    // =========================================================================
    std::uint64_t m_mapSeed = 0;
    std::uint16_t m_width = 0;
    std::uint16_t m_height = 0;
    std::uint8_t m_seaLevel = 8;
    std::uint32_t m_authoritativeChecksum = 0;

    // =========================================================================
    // Modification History (Server-side)
    // =========================================================================
    std::vector<TerrainModification> m_modifications;
    std::uint32_t m_nextSequence = 1;

    // =========================================================================
    // Client-side State
    // =========================================================================
    TerrainSyncState m_state = TerrainSyncState::Idle;
    std::vector<TerrainModification> m_pendingModifications;
    std::size_t m_modificationIndex = 0;
    std::uint32_t m_lastAppliedSequence = 0;

    /**
     * @brief Apply a single modification to the terrain grid.
     *
     * @param grid The terrain grid to modify.
     * @param mod The modification to apply.
     */
    void applyModification(TerrainGrid& grid, const TerrainModification& mod);

    /**
     * @brief Generate terrain from seed using the full pipeline.
     *
     * @param grid Output terrain grid.
     * @param waterData Output water data.
     * @param seed The generation seed.
     * @param width Grid width.
     * @param height Grid height.
     * @param seaLevel Sea level elevation.
     * @return true if generation succeeded.
     */
    bool generateFromSeed(
        TerrainGrid& grid,
        WaterData& waterData,
        std::uint64_t seed,
        std::uint16_t width,
        std::uint16_t height,
        std::uint8_t seaLevel);

    /**
     * @brief CRC32 lookup table for checksum computation.
     */
    static const std::uint32_t CRC32_TABLE[256];

    /**
     * @brief Compute CRC32 of raw data.
     */
    static std::uint32_t crc32(const void* data, std::size_t size, std::uint32_t crc = 0);
};

// =============================================================================
// Message Registration
// =============================================================================

/**
 * @brief Force registration of terrain sync network messages with MessageFactory.
 *
 * Call this function once during initialization to ensure the terrain
 * sync messages are registered with the factory.
 *
 * @return true if messages are registered (always returns true).
 */
bool initTerrainSyncMessages();

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINNETWORKSYNC_H
