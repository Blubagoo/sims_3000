/**
 * @file test_terrain_types.cpp
 * @brief Unit tests for TerrainTypes.h (Ticket 3-001)
 *
 * Tests cover:
 * - TerrainType enum values (all 10 types)
 * - TerrainComponent size verification (4 bytes)
 * - Flag manipulation helpers (set, clear, test)
 * - Elevation range enforcement (0-31)
 * - Moisture full range (0-255)
 * - Convenience flag accessors
 */

#include <sims3000/terrain/TerrainTypes.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::terrain;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// TerrainType Enum Tests
// =============================================================================

TEST(terrain_type_values) {
    // Verify all 10 canonical terrain types have correct values
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::Substrate), 0);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::Ridge), 1);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::DeepVoid), 2);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::FlowChannel), 3);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::StillBasin), 4);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::BiolumeGrove), 5);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::PrismaFields), 6);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::SporeFlats), 7);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::BlightMires), 8);
    ASSERT_EQ(static_cast<std::uint8_t>(TerrainType::EmberCrust), 9);
}

TEST(terrain_type_count) {
    ASSERT_EQ(TERRAIN_TYPE_COUNT, 10);
}

TEST(terrain_type_size) {
    ASSERT_EQ(sizeof(TerrainType), 1);
}

TEST(terrain_type_validation) {
    // Valid types (0-9)
    for (std::uint8_t i = 0; i < 10; ++i) {
        ASSERT(isValidTerrainType(i));
    }
    // Invalid types (10+)
    ASSERT(!isValidTerrainType(10));
    ASSERT(!isValidTerrainType(255));
}

// =============================================================================
// TerrainComponent Size Tests
// =============================================================================

TEST(terrain_component_size) {
    // Critical: must be exactly 4 bytes for cache performance
    ASSERT_EQ(sizeof(TerrainComponent), 4);
}

TEST(terrain_component_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<TerrainComponent>::value);
}

// =============================================================================
// Flag Bit Definition Tests
// =============================================================================

TEST(flag_bit_definitions) {
    // Verify bit positions match documentation
    ASSERT_EQ(TerrainFlags::IS_CLEARED, 0x01);    // Bit 0
    ASSERT_EQ(TerrainFlags::IS_UNDERWATER, 0x02); // Bit 1
    ASSERT_EQ(TerrainFlags::IS_COASTAL, 0x04);    // Bit 2
    ASSERT_EQ(TerrainFlags::IS_SLOPE, 0x08);      // Bit 3
    ASSERT_EQ(TerrainFlags::RESERVED_MASK, 0xF0); // Bits 4-7
}

TEST(flag_bits_non_overlapping) {
    // Ensure no two flags share bits
    std::uint8_t all_flags = TerrainFlags::IS_CLEARED |
                             TerrainFlags::IS_UNDERWATER |
                             TerrainFlags::IS_COASTAL |
                             TerrainFlags::IS_SLOPE;
    ASSERT_EQ(all_flags, 0x0F);  // Should be exactly bits 0-3
}

// =============================================================================
// Flag Manipulation Helper Tests
// =============================================================================

TEST(flag_set_single) {
    TerrainComponent tc = {};
    tc.flags = 0;

    tc.setFlag(TerrainFlags::IS_CLEARED);
    ASSERT_EQ(tc.flags, 0x01);

    tc.setFlag(TerrainFlags::IS_UNDERWATER);
    ASSERT_EQ(tc.flags, 0x03);

    tc.setFlag(TerrainFlags::IS_COASTAL);
    ASSERT_EQ(tc.flags, 0x07);

    tc.setFlag(TerrainFlags::IS_SLOPE);
    ASSERT_EQ(tc.flags, 0x0F);
}

TEST(flag_clear_single) {
    TerrainComponent tc = {};
    tc.flags = 0x0F;  // All flags set

    tc.clearFlag(TerrainFlags::IS_CLEARED);
    ASSERT_EQ(tc.flags, 0x0E);

    tc.clearFlag(TerrainFlags::IS_UNDERWATER);
    ASSERT_EQ(tc.flags, 0x0C);

    tc.clearFlag(TerrainFlags::IS_COASTAL);
    ASSERT_EQ(tc.flags, 0x08);

    tc.clearFlag(TerrainFlags::IS_SLOPE);
    ASSERT_EQ(tc.flags, 0x00);
}

TEST(flag_test_individual) {
    TerrainComponent tc = {};

    // Test with no flags
    tc.flags = 0;
    ASSERT(!tc.testFlag(TerrainFlags::IS_CLEARED));
    ASSERT(!tc.testFlag(TerrainFlags::IS_UNDERWATER));
    ASSERT(!tc.testFlag(TerrainFlags::IS_COASTAL));
    ASSERT(!tc.testFlag(TerrainFlags::IS_SLOPE));

    // Test with IS_CLEARED only
    tc.flags = TerrainFlags::IS_CLEARED;
    ASSERT(tc.testFlag(TerrainFlags::IS_CLEARED));
    ASSERT(!tc.testFlag(TerrainFlags::IS_UNDERWATER));
    ASSERT(!tc.testFlag(TerrainFlags::IS_COASTAL));
    ASSERT(!tc.testFlag(TerrainFlags::IS_SLOPE));

    // Test with multiple flags
    tc.flags = TerrainFlags::IS_UNDERWATER | TerrainFlags::IS_SLOPE;
    ASSERT(!tc.testFlag(TerrainFlags::IS_CLEARED));
    ASSERT(tc.testFlag(TerrainFlags::IS_UNDERWATER));
    ASSERT(!tc.testFlag(TerrainFlags::IS_COASTAL));
    ASSERT(tc.testFlag(TerrainFlags::IS_SLOPE));
}

TEST(flag_set_idempotent) {
    TerrainComponent tc = {};
    tc.flags = TerrainFlags::IS_CLEARED;

    // Setting same flag again should be idempotent
    tc.setFlag(TerrainFlags::IS_CLEARED);
    ASSERT_EQ(tc.flags, TerrainFlags::IS_CLEARED);
}

TEST(flag_clear_idempotent) {
    TerrainComponent tc = {};
    tc.flags = 0;

    // Clearing non-set flag should be idempotent
    tc.clearFlag(TerrainFlags::IS_CLEARED);
    ASSERT_EQ(tc.flags, 0);
}

// =============================================================================
// Convenience Flag Accessor Tests
// =============================================================================

TEST(convenience_isCleared) {
    TerrainComponent tc = {};
    tc.flags = 0;
    ASSERT(!tc.isCleared());

    tc.flags = TerrainFlags::IS_CLEARED;
    ASSERT(tc.isCleared());
}

TEST(convenience_isUnderwater) {
    TerrainComponent tc = {};
    tc.flags = 0;
    ASSERT(!tc.isUnderwater());

    tc.flags = TerrainFlags::IS_UNDERWATER;
    ASSERT(tc.isUnderwater());
}

TEST(convenience_isCoastal) {
    TerrainComponent tc = {};
    tc.flags = 0;
    ASSERT(!tc.isCoastal());

    tc.flags = TerrainFlags::IS_COASTAL;
    ASSERT(tc.isCoastal());
}

TEST(convenience_isSlope) {
    TerrainComponent tc = {};
    tc.flags = 0;
    ASSERT(!tc.isSlope());

    tc.flags = TerrainFlags::IS_SLOPE;
    ASSERT(tc.isSlope());
}

TEST(convenience_setters) {
    TerrainComponent tc = {};
    tc.flags = 0;

    tc.setCleared(true);
    ASSERT(tc.isCleared());
    tc.setCleared(false);
    ASSERT(!tc.isCleared());

    tc.setUnderwater(true);
    ASSERT(tc.isUnderwater());
    tc.setUnderwater(false);
    ASSERT(!tc.isUnderwater());

    tc.setCoastal(true);
    ASSERT(tc.isCoastal());
    tc.setCoastal(false);
    ASSERT(!tc.isCoastal());

    tc.setSlope(true);
    ASSERT(tc.isSlope());
    tc.setSlope(false);
    ASSERT(!tc.isSlope());
}

// =============================================================================
// Elevation Range Tests
// =============================================================================

TEST(elevation_valid_range) {
    TerrainComponent tc = {};

    // Test valid range 0-31
    for (std::uint8_t e = 0; e <= 31; ++e) {
        tc.setElevation(e);
        ASSERT_EQ(tc.getElevation(), e);
    }
}

TEST(elevation_clamp_to_max) {
    TerrainComponent tc = {};

    // Values above 31 should be clamped
    tc.setElevation(32);
    ASSERT_EQ(tc.getElevation(), 31);

    tc.setElevation(100);
    ASSERT_EQ(tc.getElevation(), 31);

    tc.setElevation(255);
    ASSERT_EQ(tc.getElevation(), 31);
}

TEST(elevation_max_constant) {
    ASSERT_EQ(TerrainComponent::MAX_ELEVATION, 31);
}

TEST(elevation_stored_in_byte) {
    // Even though range is 0-31, stored in full byte
    TerrainComponent tc = {};
    tc.elevation = 31;
    ASSERT_EQ(tc.elevation, 31);
}

// =============================================================================
// Moisture Range Tests
// =============================================================================

TEST(moisture_full_range) {
    TerrainComponent tc = {};

    // Moisture uses full byte range 0-255
    tc.moisture = 0;
    ASSERT_EQ(tc.moisture, 0);

    tc.moisture = 127;
    ASSERT_EQ(tc.moisture, 127);

    tc.moisture = 255;
    ASSERT_EQ(tc.moisture, 255);
}

// =============================================================================
// TerrainType Accessor Tests
// =============================================================================

TEST(terrain_type_get_set) {
    TerrainComponent tc = {};

    tc.setTerrainType(TerrainType::Substrate);
    ASSERT_EQ(tc.getTerrainType(), TerrainType::Substrate);

    tc.setTerrainType(TerrainType::Ridge);
    ASSERT_EQ(tc.getTerrainType(), TerrainType::Ridge);

    tc.setTerrainType(TerrainType::DeepVoid);
    ASSERT_EQ(tc.getTerrainType(), TerrainType::DeepVoid);

    tc.setTerrainType(TerrainType::EmberCrust);
    ASSERT_EQ(tc.getTerrainType(), TerrainType::EmberCrust);
}

// =============================================================================
// Combined Usage Tests
// =============================================================================

TEST(component_initialization) {
    TerrainComponent tc = {};
    tc.terrain_type = static_cast<std::uint8_t>(TerrainType::BiolumeGrove);
    tc.elevation = 15;
    tc.moisture = 200;
    tc.flags = TerrainFlags::IS_COASTAL;

    ASSERT_EQ(tc.getTerrainType(), TerrainType::BiolumeGrove);
    ASSERT_EQ(tc.getElevation(), 15);
    ASSERT_EQ(tc.moisture, 200);
    ASSERT(tc.isCoastal());
    ASSERT(!tc.isCleared());
}

TEST(component_typical_usage) {
    // Simulate a coastal forest tile that gets cleared
    TerrainComponent tc = {};
    tc.setTerrainType(TerrainType::BiolumeGrove);
    tc.setElevation(5);
    tc.moisture = 180;
    tc.setCoastal(true);

    ASSERT_EQ(tc.getTerrainType(), TerrainType::BiolumeGrove);
    ASSERT_EQ(tc.getElevation(), 5);
    ASSERT(tc.isCoastal());
    ASSERT(!tc.isCleared());

    // Player clears the tile for building
    tc.setCleared(true);
    ASSERT(tc.isCleared());
    ASSERT(tc.isCoastal());  // Coastal status preserved
}

TEST(component_water_tile) {
    // Simulate an underwater tile
    TerrainComponent tc = {};
    tc.setTerrainType(TerrainType::DeepVoid);
    tc.setElevation(0);
    tc.moisture = 255;
    tc.setUnderwater(true);

    ASSERT_EQ(tc.getTerrainType(), TerrainType::DeepVoid);
    ASSERT_EQ(tc.getElevation(), 0);
    ASSERT(tc.isUnderwater());
    ASSERT(!tc.isCleared());  // Can't clear water
}

TEST(component_slope_tile) {
    // Simulate a ridge tile with slope
    TerrainComponent tc = {};
    tc.setTerrainType(TerrainType::Ridge);
    tc.setElevation(20);
    tc.moisture = 50;
    tc.setSlope(true);

    ASSERT_EQ(tc.getTerrainType(), TerrainType::Ridge);
    ASSERT_EQ(tc.getElevation(), 20);
    ASSERT(tc.isSlope());
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== TerrainTypes Unit Tests ===\n\n");

    // TerrainType enum tests
    RUN_TEST(terrain_type_values);
    RUN_TEST(terrain_type_count);
    RUN_TEST(terrain_type_size);
    RUN_TEST(terrain_type_validation);

    // TerrainComponent size tests
    RUN_TEST(terrain_component_size);
    RUN_TEST(terrain_component_trivially_copyable);

    // Flag bit definition tests
    RUN_TEST(flag_bit_definitions);
    RUN_TEST(flag_bits_non_overlapping);

    // Flag manipulation helper tests
    RUN_TEST(flag_set_single);
    RUN_TEST(flag_clear_single);
    RUN_TEST(flag_test_individual);
    RUN_TEST(flag_set_idempotent);
    RUN_TEST(flag_clear_idempotent);

    // Convenience flag accessor tests
    RUN_TEST(convenience_isCleared);
    RUN_TEST(convenience_isUnderwater);
    RUN_TEST(convenience_isCoastal);
    RUN_TEST(convenience_isSlope);
    RUN_TEST(convenience_setters);

    // Elevation range tests
    RUN_TEST(elevation_valid_range);
    RUN_TEST(elevation_clamp_to_max);
    RUN_TEST(elevation_max_constant);
    RUN_TEST(elevation_stored_in_byte);

    // Moisture range tests
    RUN_TEST(moisture_full_range);

    // TerrainType accessor tests
    RUN_TEST(terrain_type_get_set);

    // Combined usage tests
    RUN_TEST(component_initialization);
    RUN_TEST(component_typical_usage);
    RUN_TEST(component_water_tile);
    RUN_TEST(component_slope_tile);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
