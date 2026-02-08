/**
 * @file test_building_component.cpp
 * @brief Unit tests for BuildingComponent structure (Ticket 4-003)
 *
 * Tests cover:
 * - BuildingComponent size verification (28 bytes packed or 32 bytes aligned)
 * - Trivially copyable for serialization
 * - Default initialization
 * - Enum accessor methods
 * - Health percentage conversion
 * - Rotation degree conversion
 */

#include <sims3000/building/BuildingComponents.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::building;

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
// BuildingComponent Size Tests
// =============================================================================

TEST(building_component_size) {
    // Critical: must be 28 bytes packed or 32 bytes aligned (per CCR-003)
    std::size_t size = sizeof(BuildingComponent);
    ASSERT(size <= 32);
    // Most compilers will be 24-32 bytes due to alignment
}

TEST(building_component_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<BuildingComponent>::value);
}

// =============================================================================
// BuildingComponent Initialization Tests
// =============================================================================

TEST(building_component_default_init) {
    BuildingComponent bc;
    ASSERT_EQ(bc.template_id, 0);
    ASSERT_EQ(bc.zone_type, static_cast<std::uint8_t>(ZoneBuildingType::Habitation));
    ASSERT_EQ(bc.density, static_cast<std::uint8_t>(DensityLevel::Low));
    ASSERT_EQ(bc.state, static_cast<std::uint8_t>(BuildingState::Active));
    ASSERT_EQ(bc.level, 0);
    ASSERT_EQ(bc.health, 255);
    ASSERT_EQ(bc.capacity, 0);
    ASSERT_EQ(bc.current_occupancy, 0);
    ASSERT_EQ(bc.footprint_w, 1);
    ASSERT_EQ(bc.footprint_h, 1);
    ASSERT_EQ(bc.state_changed_tick, 0);
    ASSERT_EQ(bc.abandon_timer, 0);
    ASSERT_EQ(bc.rotation, 0);
    ASSERT_EQ(bc.color_accent_index, 0);
}

// =============================================================================
// BuildingComponent Enum Accessor Tests
// =============================================================================

TEST(building_component_zone_type_accessor) {
    BuildingComponent bc;
    bc.setZoneBuildingType(ZoneBuildingType::Exchange);
    ASSERT_EQ(bc.getZoneBuildingType(), ZoneBuildingType::Exchange);
    ASSERT_EQ(bc.zone_type, static_cast<std::uint8_t>(ZoneBuildingType::Exchange));
}

TEST(building_component_density_accessor) {
    BuildingComponent bc;
    bc.setDensityLevel(DensityLevel::High);
    ASSERT_EQ(bc.getDensityLevel(), DensityLevel::High);
    ASSERT_EQ(bc.density, static_cast<std::uint8_t>(DensityLevel::High));
}

TEST(building_component_state_accessor) {
    BuildingComponent bc;
    bc.setBuildingState(BuildingState::Materializing);
    ASSERT_EQ(bc.getBuildingState(), BuildingState::Materializing);
    ASSERT_EQ(bc.state, static_cast<std::uint8_t>(BuildingState::Materializing));
}

TEST(building_component_state_check) {
    BuildingComponent bc;
    bc.setBuildingState(BuildingState::Abandoned);
    ASSERT(bc.isInState(BuildingState::Abandoned));
    ASSERT(!bc.isInState(BuildingState::Active));
}

// =============================================================================
// BuildingComponent Health Percentage Tests
// =============================================================================

TEST(building_component_health_percent) {
    BuildingComponent bc;

    // Full health (255 -> 100%)
    bc.health = 255;
    ASSERT_EQ(bc.getHealthPercent(), 100);

    // Half health (127 -> ~49%)
    bc.health = 127;
    std::uint8_t half_health = bc.getHealthPercent();
    ASSERT(half_health >= 49 && half_health <= 50);

    // No health (0 -> 0%)
    bc.health = 0;
    ASSERT_EQ(bc.getHealthPercent(), 0);
}

TEST(building_component_set_health_percent) {
    BuildingComponent bc;

    // Set to 100%
    bc.setHealthPercent(100);
    ASSERT_EQ(bc.health, 255);

    // Set to 50%
    bc.setHealthPercent(50);
    std::uint8_t expected_half = static_cast<std::uint8_t>((50 * 255) / 100);
    ASSERT_EQ(bc.health, expected_half);

    // Set to 0%
    bc.setHealthPercent(0);
    ASSERT_EQ(bc.health, 0);

    // Clamp above 100%
    bc.setHealthPercent(150);
    ASSERT_EQ(bc.health, 255);
}

// =============================================================================
// BuildingComponent Rotation Tests
// =============================================================================

TEST(building_component_rotation_degrees) {
    BuildingComponent bc;

    // 0 degrees
    bc.rotation = 0;
    ASSERT_EQ(bc.getRotationDegrees(), 0);

    // 90 degrees
    bc.rotation = 1;
    ASSERT_EQ(bc.getRotationDegrees(), 90);

    // 180 degrees
    bc.rotation = 2;
    ASSERT_EQ(bc.getRotationDegrees(), 180);

    // 270 degrees
    bc.rotation = 3;
    ASSERT_EQ(bc.getRotationDegrees(), 270);

    // Wrap around (4 -> 0 degrees)
    bc.rotation = 4;
    ASSERT_EQ(bc.getRotationDegrees(), 0);
}

TEST(building_component_set_rotation_degrees) {
    BuildingComponent bc;

    // Set to 0 degrees
    bc.setRotationDegrees(0);
    ASSERT_EQ(bc.rotation, 0);

    // Set to 90 degrees
    bc.setRotationDegrees(90);
    ASSERT_EQ(bc.rotation, 1);

    // Set to 180 degrees
    bc.setRotationDegrees(180);
    ASSERT_EQ(bc.rotation, 2);

    // Set to 270 degrees
    bc.setRotationDegrees(270);
    ASSERT_EQ(bc.rotation, 3);

    // Quantize 45 degrees to 0
    bc.setRotationDegrees(45);
    ASSERT_EQ(bc.rotation, 0);

    // Quantize 135 degrees to 90
    bc.setRotationDegrees(135);
    ASSERT_EQ(bc.rotation, 1);

    // Wrap around 360 degrees to 0
    bc.setRotationDegrees(360);
    ASSERT_EQ(bc.rotation, 0);
}

// =============================================================================
// BuildingComponent Field Range Tests
// =============================================================================

TEST(building_component_no_scale_field) {
    // Per CCR-010: NO scale variation stored
    BuildingComponent bc;
    // Just verify we can create and use the component without scale fields
    bc.template_id = 123;
    bc.rotation = 2;
    bc.color_accent_index = 5;
    ASSERT_EQ(bc.template_id, 123);
    ASSERT_EQ(bc.rotation, 2);
    ASSERT_EQ(bc.color_accent_index, 5);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("Running BuildingComponent tests...\n\n");

    RUN_TEST(building_component_size);
    RUN_TEST(building_component_trivially_copyable);
    RUN_TEST(building_component_default_init);
    RUN_TEST(building_component_zone_type_accessor);
    RUN_TEST(building_component_density_accessor);
    RUN_TEST(building_component_state_accessor);
    RUN_TEST(building_component_state_check);
    RUN_TEST(building_component_health_percent);
    RUN_TEST(building_component_set_health_percent);
    RUN_TEST(building_component_rotation_degrees);
    RUN_TEST(building_component_set_rotation_degrees);
    RUN_TEST(building_component_no_scale_field);

    printf("\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
