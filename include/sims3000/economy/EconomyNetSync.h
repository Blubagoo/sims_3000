/**
 * @file EconomyNetSync.h
 * @brief Network synchronization for economy/treasury data (Ticket E11-022)
 *
 * Provides serializable snapshot of treasury data for multiplayer sync.
 * Uses simple POD structure with memcpy-based serialization and a magic
 * byte prefix for validation.
 *
 * Follows the PopulationNetSync pattern from Epic 10.
 */

#ifndef SIMS3000_ECONOMY_ECONOMYNETSYNC_H
#define SIMS3000_ECONOMY_ECONOMYNETSYNC_H

#include <cstdint>
#include <cstddef>

namespace sims3000 {
namespace economy {

// Forward declaration
struct TreasuryState;

// ============================================================================
// Magic byte for packet validation
// ============================================================================

constexpr uint8_t TREASURY_SNAPSHOT_MAGIC = 0xEC;  ///< Magic byte prefix for treasury snapshots
constexpr uint8_t TRIBUTE_RATE_MSG_MAGIC  = 0xED;  ///< Magic byte prefix for tribute rate messages
constexpr uint8_t FUNDING_LEVEL_MSG_MAGIC = 0xEE;  ///< Magic byte prefix for funding level messages

// ============================================================================
// Treasury Snapshot (sent each phase)
// ============================================================================

/**
 * @struct TreasurySnapshot
 * @brief Compact treasury snapshot for network sync.
 *
 * POD structure containing essential treasury data for network transmission.
 * Sent each budget phase from server to clients.
 */
#pragma pack(push, 1)
struct TreasurySnapshot {
    int64_t balance;                        ///< Current credit balance
    int64_t last_income;                    ///< Total income from last phase
    int64_t last_expense;                   ///< Total expense from last phase
    uint8_t tribute_rate_habitation;        ///< Habitation tribute rate (0-20%)
    uint8_t tribute_rate_exchange;          ///< Exchange tribute rate (0-20%)
    uint8_t tribute_rate_fabrication;       ///< Fabrication tribute rate (0-20%)
    uint8_t funding_enforcer;              ///< Enforcer service funding (0-150%)
    uint8_t funding_hazard_response;       ///< Hazard response funding (0-150%)
    uint8_t funding_medical;               ///< Medical service funding (0-150%)
    uint8_t funding_education;             ///< Education service funding (0-150%)
    uint8_t active_bond_count;             ///< Number of active bonds
    int64_t total_debt;                    ///< Sum of remaining principal across all bonds
    uint8_t player_id;                     ///< Player this snapshot belongs to
};
#pragma pack(pop)

static_assert(sizeof(TreasurySnapshot) == 41, "TreasurySnapshot should be 41 bytes packed");

/// Serialized size: magic byte + snapshot
constexpr size_t TREASURY_SNAPSHOT_SERIALIZED_SIZE = 1 + sizeof(TreasurySnapshot);

// ============================================================================
// On-change messages
// ============================================================================

/**
 * @struct TributeRateChangeMessage
 * @brief Message sent when a player changes a tribute rate.
 */
#pragma pack(push, 1)
struct TributeRateChangeMessage {
    uint8_t player_id;      ///< Player who changed the rate
    uint8_t zone_type;      ///< ZoneBuildingType (0=Habitation, 1=Exchange, 2=Fabrication)
    uint8_t new_rate;       ///< New tribute rate (0-20%)
};
#pragma pack(pop)

static_assert(sizeof(TributeRateChangeMessage) == 3, "TributeRateChangeMessage should be 3 bytes");

/// Serialized size: magic byte + message
constexpr size_t TRIBUTE_RATE_MSG_SERIALIZED_SIZE = 1 + sizeof(TributeRateChangeMessage);

/**
 * @struct FundingLevelChangeMessage
 * @brief Message sent when a player changes a service funding level.
 */
#pragma pack(push, 1)
struct FundingLevelChangeMessage {
    uint8_t player_id;      ///< Player who changed the funding
    uint8_t service_type;   ///< Service type (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education)
    uint8_t new_level;      ///< New funding level (0-150%)
};
#pragma pack(pop)

static_assert(sizeof(FundingLevelChangeMessage) == 3, "FundingLevelChangeMessage should be 3 bytes");

/// Serialized size: magic byte + message
constexpr size_t FUNDING_LEVEL_MSG_SERIALIZED_SIZE = 1 + sizeof(FundingLevelChangeMessage);

// ============================================================================
// Snapshot creation and application
// ============================================================================

/**
 * @brief Create a TreasurySnapshot from a TreasuryState.
 *
 * Copies relevant fields from the full treasury state into the compact
 * snapshot structure for network transmission.
 *
 * @param treasury The player's treasury state.
 * @param player_id The player ID to embed in the snapshot.
 * @return TreasurySnapshot ready for serialization.
 */
TreasurySnapshot create_treasury_snapshot(const TreasuryState& treasury, uint8_t player_id);

/**
 * @brief Apply a received snapshot to a local TreasuryState.
 *
 * Updates the local treasury state with values from the network snapshot.
 * Used on the client side to sync with the server-authoritative state.
 *
 * @param treasury The local treasury state to update.
 * @param snapshot The received snapshot.
 */
void apply_treasury_snapshot(TreasuryState& treasury, const TreasurySnapshot& snapshot);

// ============================================================================
// Serialization / Deserialization
// ============================================================================

/**
 * @brief Serialize a TreasurySnapshot to a byte buffer.
 *
 * Writes a magic byte prefix followed by the snapshot data via memcpy.
 *
 * @param snapshot The snapshot to serialize.
 * @param buffer Output buffer (must be at least TREASURY_SNAPSHOT_SERIALIZED_SIZE bytes).
 * @param buffer_size Size of the output buffer.
 * @return Number of bytes written, or 0 if buffer too small.
 */
size_t serialize_treasury_snapshot(const TreasurySnapshot& snapshot, uint8_t* buffer, size_t buffer_size);

/**
 * @brief Deserialize a TreasurySnapshot from a byte buffer.
 *
 * Validates the magic byte prefix and copies the snapshot data.
 *
 * @param buffer Input buffer containing serialized snapshot.
 * @param buffer_size Size of the input buffer.
 * @param out Output snapshot.
 * @return true if deserialization succeeded, false if buffer too small or invalid magic.
 */
bool deserialize_treasury_snapshot(const uint8_t* buffer, size_t buffer_size, TreasurySnapshot& out);

/**
 * @brief Serialize a TributeRateChangeMessage to a byte buffer.
 *
 * @param msg The message to serialize.
 * @param buffer Output buffer (must be at least TRIBUTE_RATE_MSG_SERIALIZED_SIZE bytes).
 * @param buffer_size Size of the output buffer.
 * @return Number of bytes written, or 0 if buffer too small.
 */
size_t serialize_tribute_rate_change(const TributeRateChangeMessage& msg, uint8_t* buffer, size_t buffer_size);

/**
 * @brief Deserialize a TributeRateChangeMessage from a byte buffer.
 *
 * @param buffer Input buffer containing serialized message.
 * @param buffer_size Size of the input buffer.
 * @param out Output message.
 * @return true if deserialization succeeded, false if buffer too small or invalid magic.
 */
bool deserialize_tribute_rate_change(const uint8_t* buffer, size_t buffer_size, TributeRateChangeMessage& out);

/**
 * @brief Serialize a FundingLevelChangeMessage to a byte buffer.
 *
 * @param msg The message to serialize.
 * @param buffer Output buffer (must be at least FUNDING_LEVEL_MSG_SERIALIZED_SIZE bytes).
 * @param buffer_size Size of the output buffer.
 * @return Number of bytes written, or 0 if buffer too small.
 */
size_t serialize_funding_level_change(const FundingLevelChangeMessage& msg, uint8_t* buffer, size_t buffer_size);

/**
 * @brief Deserialize a FundingLevelChangeMessage from a byte buffer.
 *
 * @param buffer Input buffer containing serialized message.
 * @param buffer_size Size of the input buffer.
 * @param out Output message.
 * @return true if deserialization succeeded, false if buffer too small or invalid magic.
 */
bool deserialize_funding_level_change(const uint8_t* buffer, size_t buffer_size, FundingLevelChangeMessage& out);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_ECONOMYNETSYNC_H
