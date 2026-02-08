/**
 * @file test_zone_placement_execution.cpp
 * @brief Tests for zone placement execution (Ticket 4-012)
 *
 * Tests:
 * - Single zone placement via place_zones()
 * - Rectangular area placement
 * - Partial area (some invalid cells)
 * - Events emitted per zone
 * - ZonePlacementResult counts correct
 * - Entity IDs are unique and auto-incrementing
 * - ZoneCounts updated correctly
 * - Cost calculation based on density
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <set>

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

class ZonePlacementExecutionTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    ZonePlacementRequest make_request(std::int32_t x, std::int32_t y,
                                       std::int32_t w, std::int32_t h,
                                       ZoneType type = ZoneType::Habitation,
                                       ZoneDensity density = ZoneDensity::LowDensity,
                                       std::uint8_t player_id = 0) {
        ZonePlacementRequest req;
        req.x = x;
        req.y = y;
        req.width = w;
        req.height = h;
        req.zone_type = type;
        req.density = density;
        req.player_id = player_id;
        return req;
    }

    std::unique_ptr<ZoneSystem> system;
};

// ============================================================================
// Single zone placement tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, SingleZonePlacement) {
    auto req = make_request(10, 10, 1, 1);
    auto result = system->place_zones(req);

    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 1u);
    EXPECT_EQ(result.skipped_count, 0u);

    // Verify zone exists in grid
    EXPECT_TRUE(system->is_zoned(10, 10));

    // Verify zone type
    ZoneType type;
    EXPECT_TRUE(system->get_zone_type(10, 10, type));
    EXPECT_EQ(type, ZoneType::Habitation);
}

TEST_F(ZonePlacementExecutionTest, SingleZonePlacement_Exchange) {
    auto req = make_request(20, 20, 1, 1, ZoneType::Exchange, ZoneDensity::HighDensity);
    auto result = system->place_zones(req);

    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 1u);

    ZoneType type;
    EXPECT_TRUE(system->get_zone_type(20, 20, type));
    EXPECT_EQ(type, ZoneType::Exchange);

    ZoneDensity density;
    EXPECT_TRUE(system->get_zone_density(20, 20, density));
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

// ============================================================================
// Rectangular area placement tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, RectangularAreaPlacement) {
    auto req = make_request(10, 10, 3, 3);
    auto result = system->place_zones(req);

    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 9u);
    EXPECT_EQ(result.skipped_count, 0u);

    // Verify all 9 tiles are zoned
    for (std::int32_t dy = 0; dy < 3; ++dy) {
        for (std::int32_t dx = 0; dx < 3; ++dx) {
            EXPECT_TRUE(system->is_zoned(10 + dx, 10 + dy))
                << "Expected zone at (" << (10 + dx) << ", " << (10 + dy) << ")";
        }
    }
}

TEST_F(ZonePlacementExecutionTest, RectangularAreaPlacement_Tall) {
    auto req = make_request(5, 5, 1, 5);
    auto result = system->place_zones(req);

    EXPECT_EQ(result.placed_count, 5u);
    EXPECT_EQ(result.skipped_count, 0u);

    for (std::int32_t dy = 0; dy < 5; ++dy) {
        EXPECT_TRUE(system->is_zoned(5, 5 + dy));
    }
}

// ============================================================================
// Partial area (some invalid cells) tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, PartialArea_SomeOutOfBounds) {
    // Place near grid boundary so some cells are out of bounds
    auto req = make_request(126, 126, 4, 4);
    auto result = system->place_zones(req);

    // Only 2x2 = 4 cells are in bounds (126..127 x 126..127)
    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 4u);
    EXPECT_EQ(result.skipped_count, 12u);
}

TEST_F(ZonePlacementExecutionTest, PartialArea_SomeAlreadyZoned) {
    // Pre-place a zone
    system->place_zone(11, 11, ZoneType::Exchange, ZoneDensity::HighDensity, 0, 999);

    auto req = make_request(10, 10, 3, 3);
    auto result = system->place_zones(req);

    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 8u);
    EXPECT_EQ(result.skipped_count, 1u);
}

TEST_F(ZonePlacementExecutionTest, PartialArea_WithTerrainRejection) {
    MockTerrainQueryable mock_terrain;
    mock_terrain.set_unbuildable(11, 10);
    mock_terrain.set_unbuildable(10, 11);

    ZoneSystem sys(&mock_terrain, nullptr, 128);

    auto req = make_request(10, 10, 2, 2);
    auto result = sys.place_zones(req);

    EXPECT_TRUE(result.any_placed);
    EXPECT_EQ(result.placed_count, 2u);  // (10,10) and (11,11)
    EXPECT_EQ(result.skipped_count, 2u); // (11,10) and (10,11)
}

TEST_F(ZonePlacementExecutionTest, PartialArea_AllInvalid) {
    auto req = make_request(128, 128, 2, 2);
    auto result = system->place_zones(req);

    EXPECT_FALSE(result.any_placed);
    EXPECT_EQ(result.placed_count, 0u);
    EXPECT_EQ(result.skipped_count, 4u);
}

// ============================================================================
// Events emitted per zone tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, EventsEmittedPerZone) {
    auto req = make_request(10, 10, 2, 2, ZoneType::Fabrication, ZoneDensity::HighDensity, 1);
    system->place_zones(req);

    const auto& events = system->get_pending_designated_events();
    EXPECT_EQ(events.size(), 4u);

    for (const auto& evt : events) {
        EXPECT_EQ(evt.zone_type, ZoneType::Fabrication);
        EXPECT_EQ(evt.density, ZoneDensity::HighDensity);
        EXPECT_EQ(evt.owner_id, 1);
        EXPECT_NE(evt.entity_id, 0u);
    }
}

TEST_F(ZonePlacementExecutionTest, EventsCleared) {
    auto req = make_request(10, 10, 2, 1);
    system->place_zones(req);
    EXPECT_EQ(system->get_pending_designated_events().size(), 2u);

    system->clear_pending_designated_events();
    EXPECT_EQ(system->get_pending_designated_events().size(), 0u);
}

TEST_F(ZonePlacementExecutionTest, EventsAccumulate) {
    auto req1 = make_request(10, 10, 1, 1);
    system->place_zones(req1);
    EXPECT_EQ(system->get_pending_designated_events().size(), 1u);

    auto req2 = make_request(20, 20, 1, 1);
    system->place_zones(req2);
    EXPECT_EQ(system->get_pending_designated_events().size(), 2u);
}

TEST_F(ZonePlacementExecutionTest, EventCoordinatesCorrect) {
    auto req = make_request(15, 25, 1, 1, ZoneType::Exchange, ZoneDensity::LowDensity, 2);
    system->place_zones(req);

    const auto& events = system->get_pending_designated_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].grid_x, 15);
    EXPECT_EQ(events[0].grid_y, 25);
    EXPECT_EQ(events[0].zone_type, ZoneType::Exchange);
    EXPECT_EQ(events[0].density, ZoneDensity::LowDensity);
    EXPECT_EQ(events[0].owner_id, 2);
}

// ============================================================================
// Entity ID uniqueness and auto-increment tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, EntityIdsAreUnique) {
    auto req = make_request(10, 10, 3, 3);
    system->place_zones(req);

    const auto& events = system->get_pending_designated_events();
    std::set<std::uint32_t> ids;
    for (const auto& evt : events) {
        ids.insert(evt.entity_id);
    }
    EXPECT_EQ(ids.size(), 9u); // All 9 IDs are unique
}

TEST_F(ZonePlacementExecutionTest, EntityIdsAreAutoIncrementing) {
    auto req = make_request(10, 10, 3, 1);
    system->place_zones(req);

    const auto& events = system->get_pending_designated_events();
    ASSERT_EQ(events.size(), 3u);

    // IDs should be sequential starting from 1
    EXPECT_EQ(events[0].entity_id, 1u);
    EXPECT_EQ(events[1].entity_id, 2u);
    EXPECT_EQ(events[2].entity_id, 3u);
}

TEST_F(ZonePlacementExecutionTest, EntityIdsIncrementAcrossCalls) {
    auto req1 = make_request(10, 10, 2, 1);
    system->place_zones(req1);
    system->clear_pending_designated_events();

    auto req2 = make_request(20, 20, 1, 1);
    system->place_zones(req2);

    const auto& events = system->get_pending_designated_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, 3u); // Continues from where previous left off
}

TEST_F(ZonePlacementExecutionTest, EntityIdsNonZero) {
    auto req = make_request(10, 10, 1, 1);
    system->place_zones(req);

    const auto& events = system->get_pending_designated_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_GT(events[0].entity_id, 0u);
}

// ============================================================================
// ZoneCounts updated tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, ZoneCountsUpdated_Habitation) {
    auto req = make_request(10, 10, 3, 3, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->place_zones(req);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 9u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Exchange), 0u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Fabrication), 0u);

    const auto& counts = system->get_zone_counts(0);
    EXPECT_EQ(counts.total, 9u);
    EXPECT_EQ(counts.designated_total, 9u);
    EXPECT_EQ(counts.low_density_total, 9u);
    EXPECT_EQ(counts.high_density_total, 0u);
}

TEST_F(ZonePlacementExecutionTest, ZoneCountsUpdated_HighDensity) {
    auto req = make_request(10, 10, 2, 2, ZoneType::Exchange, ZoneDensity::HighDensity, 1);
    system->place_zones(req);

    const auto& counts = system->get_zone_counts(1);
    EXPECT_EQ(counts.total, 4u);
    EXPECT_EQ(counts.exchange_total, 4u);
    EXPECT_EQ(counts.high_density_total, 4u);
    EXPECT_EQ(counts.low_density_total, 0u);
}

TEST_F(ZonePlacementExecutionTest, ZoneCountsUpdated_MultipleOverseers) {
    auto req0 = make_request(10, 10, 2, 2, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->place_zones(req0);

    auto req1 = make_request(30, 30, 3, 1, ZoneType::Fabrication, ZoneDensity::HighDensity, 1);
    system->place_zones(req1);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 4u);
    EXPECT_EQ(system->get_zone_count(1, ZoneType::Fabrication), 3u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Fabrication), 0u);
    EXPECT_EQ(system->get_zone_count(1, ZoneType::Habitation), 0u);
}

// ============================================================================
// Cost calculation tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, CostCalculation_LowDensity) {
    auto req = make_request(10, 10, 3, 3, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    auto result = system->place_zones(req);

    // Default low density cost = 2, 9 zones = 18
    EXPECT_EQ(result.total_cost, 18u);
}

TEST_F(ZonePlacementExecutionTest, CostCalculation_HighDensity) {
    auto req = make_request(10, 10, 2, 2, ZoneType::Exchange, ZoneDensity::HighDensity, 0);
    auto result = system->place_zones(req);

    // Default high density cost = 5, 4 zones = 20
    EXPECT_EQ(result.total_cost, 20u);
}

TEST_F(ZonePlacementExecutionTest, CostCalculation_CustomConfig) {
    ZoneSystem::PlacementCostConfig config;
    config.low_density_cost = 10;
    config.high_density_cost = 25;
    system->set_placement_cost_config(config);

    auto req = make_request(10, 10, 2, 1, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    auto result = system->place_zones(req);

    EXPECT_EQ(result.total_cost, 20u); // 2 * 10
}

TEST_F(ZonePlacementExecutionTest, CostCalculation_PartialPlacement) {
    // Pre-place one zone
    system->place_zone(11, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 0, 999);

    auto req = make_request(10, 10, 3, 1, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    auto result = system->place_zones(req);

    // Only 2 zones placed (cell 11,10 was occupied)
    EXPECT_EQ(result.placed_count, 2u);
    EXPECT_EQ(result.total_cost, 4u); // 2 * 2 (low density default)
}

TEST_F(ZonePlacementExecutionTest, CostConfig_GetSet) {
    ZoneSystem::PlacementCostConfig config;
    config.low_density_cost = 7;
    config.high_density_cost = 15;
    system->set_placement_cost_config(config);

    const auto& retrieved = system->get_placement_cost_config();
    EXPECT_EQ(retrieved.low_density_cost, 7u);
    EXPECT_EQ(retrieved.high_density_cost, 15u);
}

TEST_F(ZonePlacementExecutionTest, CostConfig_DefaultValues) {
    const auto& config = system->get_placement_cost_config();
    EXPECT_EQ(config.low_density_cost, 2u);
    EXPECT_EQ(config.high_density_cost, 5u);
}

// ============================================================================
// Zone state after placement tests
// ============================================================================

TEST_F(ZonePlacementExecutionTest, ZoneStateIsDesignated) {
    auto req = make_request(10, 10, 1, 1);
    system->place_zones(req);

    ZoneState state;
    EXPECT_TRUE(system->get_zone_state(10, 10, state));
    EXPECT_EQ(state, ZoneState::Designated);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
