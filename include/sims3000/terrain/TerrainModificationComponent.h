/**
 * @file TerrainModificationComponent.h
 * @brief ECS component for tracking in-progress terrain modification operations
 *
 * TerrainModificationComponent tracks multi-tick terrain operations such as
 * grade terrain (leveling). The component is attached to a temporary entity
 * that persists until the operation completes or is cancelled.
 *
 * Multi-tick operation flow:
 * 1. Player requests grade_terrain(x, y, target_elevation, player_id)
 * 2. System validates request and creates temporary entity with this component
 * 3. Each simulation tick: elevation changes by 1 level toward target
 * 4. TerrainModifiedEvent fires each tick as elevation changes
 * 5. When target reached or cancelled, temporary entity is destroyed
 *
 * @see ITerrainModifier for the interface that initiates operations
 * @see TerrainEvents.h for TerrainModifiedEvent
 */

#ifndef SIMS3000_TERRAIN_TERRAINMODIFICATIONCOMPONENT_H
#define SIMS3000_TERRAIN_TERRAINMODIFICATIONCOMPONENT_H

#include <sims3000/terrain/ITerrainModifier.h>
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace terrain {

/**
 * @enum TerrainOperationType
 * @brief Types of terrain modification operations.
 */
enum class TerrainOperationType : std::uint8_t {
    None = 0,             ///< No operation (invalid state)
    GradeTerrain = 1,     ///< Raise or lower elevation over multiple ticks
    TerraformTerrain = 2, ///< Convert terrain type to another (e.g., BlightMires -> Substrate)
};

/**
 * @struct GradingOperation
 * @brief Data specific to grade terrain (leveling) operations.
 *
 * Stores the start/target elevation and progress tracking for
 * multi-tick terrain leveling.
 */
struct GradingOperation {
    std::uint8_t start_elevation = 0;   ///< Elevation when operation began
    std::uint8_t target_elevation = 0;  ///< Desired final elevation
    std::uint8_t ticks_remaining = 0;   ///< Ticks left until completion
    std::uint8_t padding = 0;           ///< Alignment padding
};

static_assert(sizeof(GradingOperation) == 4, "GradingOperation must be 4 bytes");
static_assert(std::is_trivially_copyable<GradingOperation>::value,
    "GradingOperation must be trivially copyable");

/**
 * @struct TerraformingOperation
 * @brief Data specific to terraform terrain type conversion operations.
 *
 * Stores the source/target terrain types and progress tracking for
 * multi-tick terrain type conversions.
 */
struct TerraformingOperation {
    std::uint8_t source_type = 0;     ///< TerrainType when operation began
    std::uint8_t target_type = 0;     ///< Desired TerrainType (typically Substrate)
    std::uint16_t ticks_remaining = 0; ///< Ticks left until completion
    std::uint16_t total_ticks = 0;    ///< Total ticks for this operation (for refund calc)
    std::uint16_t padding = 0;        ///< Alignment padding
};

static_assert(sizeof(TerraformingOperation) == 8, "TerraformingOperation must be 8 bytes");
static_assert(std::is_trivially_copyable<TerraformingOperation>::value,
    "TerraformingOperation must be trivially copyable");

/**
 * @struct TerrainModificationComponent
 * @brief Component tracking an in-progress terrain modification.
 *
 * Attached to temporary entities that represent ongoing terrain operations.
 * The entity is destroyed when the operation completes or is cancelled.
 *
 * Multi-tick behavior:
 * - GradeTerrain: Each tick changes elevation by 1 toward target
 * - TerraformTerrain: Counts down ticks, changes type on completion
 * - Operation completes when ticks_remaining reaches 0
 * - Cancel support: set cancelled flag, operation stops with partial result
 *
 * Memory layout (32 bytes):
 * - tile_x, tile_y: 4 bytes (target tile)
 * - player_id: 1 byte (requesting player)
 * - operation_type: 1 byte (type of operation)
 * - cancelled: 1 byte (cancellation flag)
 * - padding: 1 byte
 * - operation data: 8 bytes (union of GradingOperation + padding or TerraformingOperation)
 * - total_cost: 8 bytes (pre-computed cost)
 * - cost_paid: 4 bytes (cost paid so far for partial refund on cancel)
 * - padding2: 4 bytes
 */
struct TerrainModificationComponent {
    // =========================================================================
    // Target Tile Information
    // =========================================================================

    std::int16_t tile_x;            ///< X coordinate of target tile
    std::int16_t tile_y;            ///< Y coordinate of target tile

    // =========================================================================
    // Operation Metadata
    // =========================================================================

    PlayerID player_id;                            ///< Player who initiated the operation
    TerrainOperationType operation_type;           ///< Type of operation
    std::uint8_t cancelled;                        ///< Non-zero if operation was cancelled
    std::uint8_t padding1;                         ///< Alignment padding

    // =========================================================================
    // Operation-Specific Data (union for multiple op types)
    // =========================================================================

    union {
        struct {
            GradingOperation grading;      ///< Grading operation data (4 bytes)
            std::uint32_t grading_pad;     ///< Padding to match union size (4 bytes)
        };
        TerraformingOperation terraforming; ///< Terraforming operation data (8 bytes)
    };

    // =========================================================================
    // Cost Tracking
    // =========================================================================

    std::int64_t total_cost;        ///< Total cost of the operation
    std::int32_t cost_paid;         ///< Cost paid so far (for partial refund on cancel)
    std::int32_t padding2;          ///< Alignment padding

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - zero-initializes all fields.
     */
    TerrainModificationComponent()
        : tile_x(0)
        , tile_y(0)
        , player_id(0)
        , operation_type(TerrainOperationType::None)
        , cancelled(0)
        , padding1(0)
        , grading{}
        , grading_pad(0)
        , total_cost(0)
        , cost_paid(0)
        , padding2(0)
    {}

    // =========================================================================
    // Helper Methods
    // =========================================================================

    /**
     * @brief Check if this is a grading operation.
     * @return true if operation_type is GradeTerrain.
     */
    bool isGrading() const {
        return operation_type == TerrainOperationType::GradeTerrain;
    }

    /**
     * @brief Check if this is a terraforming operation.
     * @return true if operation_type is TerraformTerrain.
     */
    bool isTerraforming() const {
        return operation_type == TerrainOperationType::TerraformTerrain;
    }

    /**
     * @brief Check if the operation is complete.
     * @return true if no more ticks remaining or cancelled.
     */
    bool isComplete() const {
        if (cancelled != 0) return true;
        if (operation_type == TerrainOperationType::GradeTerrain) {
            return grading.ticks_remaining == 0;
        }
        if (operation_type == TerrainOperationType::TerraformTerrain) {
            return terraforming.ticks_remaining == 0;
        }
        return true; // Unknown operation types complete immediately
    }

    /**
     * @brief Cancel this operation.
     *
     * The operation will stop on the next tick, leaving terrain at
     * its current (partially modified) state. Partial refund may be
     * calculated based on cost_paid.
     */
    void cancel() {
        cancelled = 1;
    }

    /**
     * @brief Get the direction of elevation change.
     * @return +1 if raising, -1 if lowering, 0 if at target.
     */
    std::int8_t getElevationDirection() const {
        if (grading.target_elevation > grading.start_elevation) {
            // Raising: but we need to check current progress
            // ticks_remaining tells us how many more changes needed
            return (grading.ticks_remaining > 0) ? 1 : 0;
        } else if (grading.target_elevation < grading.start_elevation) {
            return (grading.ticks_remaining > 0) ? -1 : 0;
        }
        return 0;
    }

    /**
     * @brief Calculate current elevation based on progress.
     * @return Current elevation after progress so far.
     */
    std::uint8_t getCurrentElevation() const {
        std::uint8_t total_change = (grading.start_elevation > grading.target_elevation)
            ? (grading.start_elevation - grading.target_elevation)
            : (grading.target_elevation - grading.start_elevation);

        std::uint8_t changes_made = total_change - grading.ticks_remaining;

        if (grading.target_elevation > grading.start_elevation) {
            return grading.start_elevation + changes_made;
        } else {
            return grading.start_elevation - changes_made;
        }
    }

    /**
     * @brief Get terraform progress as a percentage (0-100).
     * @return Completion percentage for terraform operations.
     */
    std::uint8_t getTerraformProgress() const {
        if (terraforming.total_ticks == 0) return 100;
        std::uint16_t ticks_done = terraforming.total_ticks - terraforming.ticks_remaining;
        return static_cast<std::uint8_t>((ticks_done * 100) / terraforming.total_ticks);
    }
};

static_assert(sizeof(TerrainModificationComponent) == 32,
    "TerrainModificationComponent must be 32 bytes");
static_assert(std::is_trivially_copyable<TerrainModificationComponent>::value,
    "TerrainModificationComponent must be trivially copyable");

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINMODIFICATIONCOMPONENT_H
