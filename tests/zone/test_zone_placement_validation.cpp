/**
 * @file test_zone_placement_validation.cpp
 * @brief Tests for zone placement validation pipeline (Ticket 4-011)
 *
 * Tests:
 * - Bounds rejection
 * - Valid placement
 * - Terrain rejection (with mock ITerrainQueryable)
 * - Zone overlap rejection
 * - Area validation with partial success
 * - Player ID (overseer) validation
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/terrain/ITerrainQueryable.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Mock ITerrainQueryable for testing
// ============================================================================

class MockTerrainQueryable : public terrain::ITerrainQueryable {
public:
    MockTerrainQueryable()
        : m_all_buildable(true)
        , m_map_width(128)
        , m_map_height(128)
    {}

    void set_all_buildable(bool buildable) { m_all_buildable = buildable; }
    void set_unbuildable(std::int32_t x, std::int32_t y) {
        m_unbuildable.push_back({x, y});
    }

    // -- Core queries --
    terrain::TerrainType get_terrain_type(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return terrain::TerrainType::Substrate;
    }

    std::uint8_t get_elevation(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return 10;
    }

    bool is_buildable(std::int32_t x, std::int32_t y) const override {
        if (!m_all_buildable) return false;
        for (const auto& pos : m_unbuildable) {
            if (pos.first == x && pos.second == y) return false;
        }
        return true;
    }

    std::uint8_t get_slope(std::int32_t x1, std::int32_t y1,
                            std::int32_t x2, std::int32_t y2) const override {
        (void)x1; (void)y1; (void)x2; (void)y2;
        return 0;
    }

    float get_average_elevation(std::int32_t x, std::int32_t y,
                                 std::uint32_t radius) const override {
        (void)x; (void)y; (void)radius;
        return 10.0f;
    }

    std::uint32_t get_water_distance(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return 255;
    }

    float get_value_bonus(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return 0.0f;
    }

    float get_harmony_bonus(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return 0.0f;
    }

    std::int32_t get_build_cost_modifier(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return 100;
    }

    std::uint32_t get_contamination_output(std::int32_t x, std::int32_t y) const override {
        (void)x; (void)y;
        return 0;
    }

    std::uint32_t get_map_width() const override { return m_map_width; }
    std::uint32_t get_map_height() const override { return m_map_height; }
    std::uint8_t get_sea_level() const override { return 8; }

    void get_tiles_in_rect(const terrain::GridRect& rect,
                            std::vector<terrain::TerrainComponent>& out_tiles) const override {
        (void)rect;
        out_tiles.clear();
    }

    std::uint32_t get_buildable_tiles_in_rect(const terrain::GridRect& rect) const override {
        (void)rect;
        return 0;
    }

    std::uint32_t count_terrain_type_in_rect(const terrain::GridRect& rect,
                                              terrain::TerrainType type) const override {
        (void)rect; (void)type;
        return 0;
    }

private:
    bool m_all_buildable;
    std::uint32_t m_map_width;
    std::uint32_t m_map_height;
    std::vector<std::pair<std::int32_t, std::int32_t>> m_unbuildable;
};

// ============================================================================
// Test Fixture
// ============================================================================

class ZonePlacementValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default: no terrain, no transport (nullptr)
        system_no_terrain = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    std::unique_ptr<ZoneSystem> system_no_terrain;
};

// ============================================================================
// Single-cell validation tests
// ============================================================================

TEST_F(ZonePlacementValidationTest, BoundsRejection_NegativeCoords) {
    auto result = system_no_terrain->validate_zone_placement(-1, 0, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::OutOfBounds);
}

TEST_F(ZonePlacementValidationTest, BoundsRejection_TooLargeCoords) {
    auto result = system_no_terrain->validate_zone_placement(128, 0, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::OutOfBounds);
}

TEST_F(ZonePlacementValidationTest, BoundsRejection_NegativeY) {
    auto result = system_no_terrain->validate_zone_placement(0, -5, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::OutOfBounds);
}

TEST_F(ZonePlacementValidationTest, ValidPlacement_EmptyTile) {
    auto result = system_no_terrain->validate_zone_placement(10, 20, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::Ok);
}

TEST_F(ZonePlacementValidationTest, ValidPlacement_MaxValidCoord) {
    auto result = system_no_terrain->validate_zone_placement(127, 127, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::Ok);
}

TEST_F(ZonePlacementValidationTest, PlayerIdValidation_InvalidOverseer) {
    auto result = system_no_terrain->validate_zone_placement(10, 10, MAX_OVERSEERS);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::NotOwned);
}

TEST_F(ZonePlacementValidationTest, PlayerIdValidation_MaxValidOverseer) {
    auto result = system_no_terrain->validate_zone_placement(10, 10, MAX_OVERSEERS - 1);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::Ok);
}

TEST_F(ZonePlacementValidationTest, PlayerIdValidation_HighInvalidId) {
    auto result = system_no_terrain->validate_zone_placement(10, 10, 255);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::NotOwned);
}

TEST_F(ZonePlacementValidationTest, TerrainRejection_UnbuildableTerrain) {
    MockTerrainQueryable mock_terrain;
    mock_terrain.set_unbuildable(5, 5);

    ZoneSystem system(&mock_terrain, nullptr, 128);
    auto result = system.validate_zone_placement(5, 5, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::TerrainNotBuildable);
}

TEST_F(ZonePlacementValidationTest, TerrainRejection_AllUnbuildable) {
    MockTerrainQueryable mock_terrain;
    mock_terrain.set_all_buildable(false);

    ZoneSystem system(&mock_terrain, nullptr, 128);
    auto result = system.validate_zone_placement(10, 10, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::TerrainNotBuildable);
}

TEST_F(ZonePlacementValidationTest, TerrainCheck_SkippedWhenNull) {
    // m_terrain is nullptr, so terrain check is skipped
    auto result = system_no_terrain->validate_zone_placement(5, 5, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::Ok);
}

TEST_F(ZonePlacementValidationTest, TerrainBuildable_Passes) {
    MockTerrainQueryable mock_terrain;
    // Default: all buildable

    ZoneSystem system(&mock_terrain, nullptr, 128);
    auto result = system.validate_zone_placement(5, 5, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::Ok);
}

TEST_F(ZonePlacementValidationTest, ZoneOverlapRejection) {
    // Place a zone first
    system_no_terrain->place_zone(10, 10, ZoneType::Habitation,
                                   ZoneDensity::LowDensity, 0, 100);

    // Try to validate placement at same position
    auto result = system_no_terrain->validate_zone_placement(10, 10, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::ZoneAlreadyPresent);
}

TEST_F(ZonePlacementValidationTest, ZoneOverlap_AdjacentTileOk) {
    system_no_terrain->place_zone(10, 10, ZoneType::Habitation,
                                   ZoneDensity::LowDensity, 0, 100);

    // Adjacent tile should be fine
    auto result = system_no_terrain->validate_zone_placement(11, 10, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::Ok);
}

// ============================================================================
// Area validation tests
// ============================================================================

TEST_F(ZonePlacementValidationTest, AreaValidation_AllValid) {
    ZonePlacementRequest req;
    req.x = 10;
    req.y = 10;
    req.width = 3;
    req.height = 3;
    req.zone_type = ZoneType::Habitation;
    req.density = ZoneDensity::LowDensity;
    req.player_id = 0;

    auto result = system_no_terrain->validate_zone_area(req);
    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 9u);
    EXPECT_EQ(result.skipped_count, 0u);
}

TEST_F(ZonePlacementValidationTest, AreaValidation_PartialSuccess) {
    // Place a zone in the middle of a 3x3 area
    system_no_terrain->place_zone(11, 11, ZoneType::Exchange,
                                   ZoneDensity::HighDensity, 0, 200);

    ZonePlacementRequest req;
    req.x = 10;
    req.y = 10;
    req.width = 3;
    req.height = 3;
    req.zone_type = ZoneType::Habitation;
    req.density = ZoneDensity::LowDensity;
    req.player_id = 0;

    auto result = system_no_terrain->validate_zone_area(req);
    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 8u);
    EXPECT_EQ(result.skipped_count, 1u);
}

TEST_F(ZonePlacementValidationTest, AreaValidation_AllOutOfBounds) {
    ZonePlacementRequest req;
    req.x = 128;
    req.y = 128;
    req.width = 2;
    req.height = 2;
    req.zone_type = ZoneType::Fabrication;
    req.density = ZoneDensity::HighDensity;
    req.player_id = 0;

    auto result = system_no_terrain->validate_zone_area(req);
    EXPECT_FALSE(result.any_placed);
    EXPECT_EQ(result.placed_count, 0u);
    EXPECT_EQ(result.skipped_count, 4u);
}

TEST_F(ZonePlacementValidationTest, AreaValidation_EdgeOverlap) {
    // Area that straddles the grid boundary
    ZonePlacementRequest req;
    req.x = 126;
    req.y = 126;
    req.width = 4;
    req.height = 4;
    req.zone_type = ZoneType::Habitation;
    req.density = ZoneDensity::LowDensity;
    req.player_id = 0;

    auto result = system_no_terrain->validate_zone_area(req);
    // 126,126 127,126 128(oob),126 129(oob),126
    // 126,127 127,127 128(oob),127 129(oob),127
    // 126,128(oob) etc.
    // Valid: 4 tiles (126..127 x 126..127)
    // Out of bounds: 12 tiles
    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 4u);
    EXPECT_EQ(result.skipped_count, 12u);
}

TEST_F(ZonePlacementValidationTest, AreaValidation_WithTerrainRejection) {
    MockTerrainQueryable mock_terrain;
    mock_terrain.set_unbuildable(11, 10);
    mock_terrain.set_unbuildable(10, 11);

    ZoneSystem system(&mock_terrain, nullptr, 128);

    ZonePlacementRequest req;
    req.x = 10;
    req.y = 10;
    req.width = 2;
    req.height = 2;
    req.zone_type = ZoneType::Exchange;
    req.density = ZoneDensity::LowDensity;
    req.player_id = 0;

    auto result = system.validate_zone_area(req);
    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 2u);  // (10,10) and (11,11) are buildable
    EXPECT_EQ(result.skipped_count, 2u); // (11,10) and (10,11) are unbuildable
}

TEST_F(ZonePlacementValidationTest, AreaValidation_SingleTile) {
    ZonePlacementRequest req;
    req.x = 50;
    req.y = 50;
    req.width = 1;
    req.height = 1;
    req.zone_type = ZoneType::Habitation;
    req.density = ZoneDensity::LowDensity;
    req.player_id = 0;

    auto result = system_no_terrain->validate_zone_area(req);
    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 1u);
    EXPECT_EQ(result.skipped_count, 0u);
}

// ============================================================================
// Check ordering: bounds is checked before ownership
// ============================================================================

TEST_F(ZonePlacementValidationTest, CheckOrder_BoundsBeforeOwnership) {
    // Out of bounds AND invalid player_id; should return OutOfBounds
    auto result = system_no_terrain->validate_zone_placement(-1, -1, 255);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::OutOfBounds);
}

TEST_F(ZonePlacementValidationTest, CheckOrder_OwnershipBeforeTerrain) {
    MockTerrainQueryable mock_terrain;
    mock_terrain.set_all_buildable(false);

    ZoneSystem system(&mock_terrain, nullptr, 128);
    // Valid bounds, invalid player, unbuildable terrain => should return NotOwned
    auto result = system.validate_zone_placement(10, 10, MAX_OVERSEERS);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::NotOwned);
}

TEST_F(ZonePlacementValidationTest, CheckOrder_TerrainBeforeZoneOverlap) {
    MockTerrainQueryable mock_terrain;
    mock_terrain.set_unbuildable(10, 10);

    ZoneSystem system(&mock_terrain, nullptr, 128);
    system.place_zone(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 100);

    // Both terrain and zone overlap fail. Terrain should be checked first.
    auto result = system.validate_zone_placement(10, 10, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, ZoneSystem::ValidationResult::Reason::TerrainNotBuildable);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
