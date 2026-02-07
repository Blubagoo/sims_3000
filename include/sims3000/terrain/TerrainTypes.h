/**
 * @file TerrainTypes.h
 * @brief Foundational terrain data types for the TerrainSystem
 *
 * Defines TerrainType enum with 10 canonical alien terrain types and
 * TerrainComponent struct as the atomic unit of terrain data (4 bytes per tile).
 *
 * Terrain types use alien terminology per /docs/canon/terminology.yaml:
 * - Substrate: Standard buildable terrain (flat_ground)
 * - Ridge: Elevated terrain (hills)
 * - DeepVoid: Map-edge water (ocean)
 * - FlowChannel: Flowing water (river)
 * - StillBasin: Inland water body (lake)
 * - BiolumeGrove: Alien vegetation cluster (forest)
 * - PrismaFields: Luminous crystal formations (crystal_fields)
 * - SporeFlats: Bioluminescent spore flora (spore_plains)
 * - BlightMires: Chemical runoff pools (toxic_marshes)
 * - EmberCrust: Hardened volcanic terrain (volcanic_rock)
 */

#ifndef SIMS3000_TERRAIN_TERRAINTYPES_H
#define SIMS3000_TERRAIN_TERRAINTYPES_H

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace terrain {

/**
 * @enum TerrainType
 * @brief Canonical alien terrain types.
 *
 * Each terrain type has unique gameplay effects and visual appearance.
 * Uses canonical alien terminology.
 */
enum class TerrainType : std::uint8_t {
    Substrate = 0,     ///< Standard buildable terrain (flat ground)
    Ridge = 1,         ///< Elevated terrain (hills)
    DeepVoid = 2,      ///< Map-edge deep water (ocean)
    FlowChannel = 3,   ///< Flowing water (river)
    StillBasin = 4,    ///< Inland water body (lake)
    BiolumeGrove = 5,  ///< Alien vegetation cluster (forest)
    PrismaFields = 6,  ///< Luminous crystal formations
    SporeFlats = 7,    ///< Bioluminescent spore flora
    BlightMires = 8,   ///< Chemical runoff pools (toxic)
    EmberCrust = 9     ///< Hardened volcanic terrain
};

/// Total number of terrain types
constexpr std::uint8_t TERRAIN_TYPE_COUNT = 10;

/**
 * @brief Check if a terrain type value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid TerrainType (0-9).
 */
constexpr bool isValidTerrainType(std::uint8_t value) {
    return value < TERRAIN_TYPE_COUNT;
}

/**
 * @struct TerrainFlags
 * @brief Bit field definitions for TerrainComponent flags.
 *
 * Flag bit layout (8 bits total):
 * - Bit 0: is_cleared - Vegetation/crystals have been purged for building
 * - Bit 1: is_underwater - Tile is below water level
 * - Bit 2: is_coastal - Tile is adjacent to water
 * - Bit 3: is_slope - Tile has elevation change (non-flat)
 * - Bits 4-7: Reserved for future use
 */
namespace TerrainFlags {
    constexpr std::uint8_t IS_CLEARED    = 0x01;  ///< Bit 0: terrain cleared for building
    constexpr std::uint8_t IS_UNDERWATER = 0x02;  ///< Bit 1: tile is underwater
    constexpr std::uint8_t IS_COASTAL    = 0x04;  ///< Bit 2: adjacent to water
    constexpr std::uint8_t IS_SLOPE      = 0x08;  ///< Bit 3: has elevation change
    constexpr std::uint8_t RESERVED_MASK = 0xF0;  ///< Bits 4-7: reserved
} // namespace TerrainFlags

/**
 * @struct TerrainComponent
 * @brief Atomic unit of terrain data at exactly 4 bytes per tile.
 *
 * This component stores all per-tile terrain information in a compact format
 * optimized for cache performance in dense grid storage.
 *
 * Layout:
 * - terrain_type: 1 byte (TerrainType enum, 0-9)
 * - elevation: 1 byte (0-31 effective range, 5 bits used)
 * - moisture: 1 byte (0-255 full range)
 * - flags: 1 byte (bit field, see TerrainFlags)
 *
 * Total: 4 bytes per tile, allowing 256x256 grid to fit in 256KB.
 */
struct TerrainComponent {
    std::uint8_t terrain_type;  ///< TerrainType value (0-9)
    std::uint8_t elevation;     ///< Height level (0-31, stored in full byte)
    std::uint8_t moisture;      ///< Moisture level (0-255)
    std::uint8_t flags;         ///< Bit flags (see TerrainFlags namespace)

    // =========================================================================
    // Flag manipulation helpers (inline for performance)
    // =========================================================================

    /**
     * @brief Set a specific flag bit.
     * @param flag The flag bit to set (e.g., TerrainFlags::IS_CLEARED).
     */
    void setFlag(std::uint8_t flag) {
        flags |= flag;
    }

    /**
     * @brief Clear a specific flag bit.
     * @param flag The flag bit to clear (e.g., TerrainFlags::IS_CLEARED).
     */
    void clearFlag(std::uint8_t flag) {
        flags &= ~flag;
    }

    /**
     * @brief Test if a specific flag bit is set.
     * @param flag The flag bit to test (e.g., TerrainFlags::IS_CLEARED).
     * @return true if the flag is set.
     */
    bool testFlag(std::uint8_t flag) const {
        return (flags & flag) != 0;
    }

    // =========================================================================
    // Convenience flag accessors
    // =========================================================================

    /// Check if terrain has been cleared for building
    bool isCleared() const { return testFlag(TerrainFlags::IS_CLEARED); }

    /// Check if tile is underwater
    bool isUnderwater() const { return testFlag(TerrainFlags::IS_UNDERWATER); }

    /// Check if tile is coastal (adjacent to water)
    bool isCoastal() const { return testFlag(TerrainFlags::IS_COASTAL); }

    /// Check if tile has a slope (elevation change)
    bool isSlope() const { return testFlag(TerrainFlags::IS_SLOPE); }

    /// Set the cleared flag
    void setCleared(bool value) {
        if (value) setFlag(TerrainFlags::IS_CLEARED);
        else clearFlag(TerrainFlags::IS_CLEARED);
    }

    /// Set the underwater flag
    void setUnderwater(bool value) {
        if (value) setFlag(TerrainFlags::IS_UNDERWATER);
        else clearFlag(TerrainFlags::IS_UNDERWATER);
    }

    /// Set the coastal flag
    void setCoastal(bool value) {
        if (value) setFlag(TerrainFlags::IS_COASTAL);
        else clearFlag(TerrainFlags::IS_COASTAL);
    }

    /// Set the slope flag
    void setSlope(bool value) {
        if (value) setFlag(TerrainFlags::IS_SLOPE);
        else clearFlag(TerrainFlags::IS_SLOPE);
    }

    // =========================================================================
    // TerrainType accessors
    // =========================================================================

    /**
     * @brief Get the terrain type as the enum value.
     * @return TerrainType enum value.
     */
    TerrainType getTerrainType() const {
        return static_cast<TerrainType>(terrain_type);
    }

    /**
     * @brief Set the terrain type.
     * @param type TerrainType to set.
     */
    void setTerrainType(TerrainType type) {
        terrain_type = static_cast<std::uint8_t>(type);
    }

    // =========================================================================
    // Elevation accessors with range enforcement
    // =========================================================================

    /// Maximum valid elevation value (5 bits = 0-31)
    static constexpr std::uint8_t MAX_ELEVATION = 31;

    /**
     * @brief Get elevation with range validation.
     * @return Elevation value clamped to 0-31.
     */
    std::uint8_t getElevation() const {
        return elevation <= MAX_ELEVATION ? elevation : MAX_ELEVATION;
    }

    /**
     * @brief Set elevation with range enforcement.
     * @param value Elevation to set (will be clamped to 0-31).
     */
    void setElevation(std::uint8_t value) {
        elevation = value <= MAX_ELEVATION ? value : MAX_ELEVATION;
    }
};

// Verify TerrainComponent is exactly 4 bytes as required for cache performance
static_assert(sizeof(TerrainComponent) == 4,
    "TerrainComponent must be exactly 4 bytes per tile");

// Verify TerrainComponent is trivially copyable for network serialization
static_assert(std::is_trivially_copyable<TerrainComponent>::value,
    "TerrainComponent must be trivially copyable");

// Verify TerrainType underlying size
static_assert(sizeof(TerrainType) == 1,
    "TerrainType must be 1 byte");

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINTYPES_H
