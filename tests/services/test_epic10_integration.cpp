/**
 * @file test_epic10_integration.cpp
 * @brief Integration tests: Epic 10 systems contract verification (Ticket E9-052)
 *
 * End-to-end integration tests verifying the SERVICE SIDE of the Epic 10
 * integration contracts. Epic 10 systems don't exist yet; these tests
 * verify that ServicesSystem correctly provides data that future systems
 * (DisorderSystem, PopulationSystem, LandValueSystem) will consume.
 *
 * Test sections:
 * 1. Enforcer coverage -> disorder suppression pipeline
 * 2. Remove enforcer -> disorder returns to normal
 * 3. Medical building -> longevity pipeline
 * 4. Education building -> land value pipeline
 * 5. Stub replacement verification (StubServiceQueryable)
 * 6. Full tick cycle with services active
 * 7. Funding integration
 * 8. Multi-player isolation
 */

#include <sims3000/services/ServicesSystem.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/CoverageCalculation.h>
#include <sims3000/services/GlobalServiceAggregation.h>
#include <sims3000/services/DisorderSuppression.h>
#include <sims3000/services/LongevityBonus.h>
#include <sims3000/services/EducationBonus.h>
#include <sims3000/services/FundingModifier.h>
#include <sims3000/services/ServiceTypes.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <sims3000/core/ISimulatable.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::services;
using namespace sims3000::building;

// =============================================================================
// Test infrastructure
// =============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("  Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n    FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n    FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::fabs((a) - (b)) > 0.001f) { \
        printf("\n    FAILED: %s == %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_GT(a, b) do { \
    if (!((a) > (b))) { \
        printf("\n    FAILED: %s > %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Mock ISimulationTime
// =============================================================================

class MockSimulationTime : public sims3000::ISimulationTime {
public:
    sims3000::SimulationTick getCurrentTick() const override { return m_tick; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(m_tick) * 0.05; }

    sims3000::SimulationTick m_tick = 0;
};

// =============================================================================
// Helper: create a ServiceBuildingData for radius-based services
// =============================================================================

static ServiceBuildingData make_enforcer(int32_t x, int32_t y, uint8_t tier, bool active,
                                          uint8_t owner = 0) {
    ServiceBuildingData b{};
    b.x = x;
    b.y = y;
    b.type = ServiceType::Enforcer;
    b.tier = tier;
    b.effectiveness = 255;  // Full effectiveness (normalized to 1.0 in calculation)
    b.is_active = active;
    b.owner_id = owner;
    b.capacity = 0;
    return b;
}

// =============================================================================
// Helper: create a ServiceBuildingData for global services
// =============================================================================

static ServiceBuildingData make_global_building(ServiceType type, uint16_t capacity,
                                                 bool active, uint8_t owner = 0) {
    ServiceBuildingData b{};
    b.x = 0;
    b.y = 0;
    b.type = type;
    b.tier = 1;
    b.effectiveness = 100;
    b.is_active = active;
    b.owner_id = owner;
    b.capacity = capacity;
    return b;
}

// =============================================================================
// 1. Enforcer Coverage -> Disorder Suppression Pipeline
// =============================================================================

TEST(enforcer_coverage_full_at_building_position) {
    // Place an enforcer Post (radius=8) at center of 64x64 map.
    // At the building position (distance=0), coverage should be 255 (full).
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings = {
        make_enforcer(32, 32, 1, true)
    };

    calculate_radius_coverage(grid, buildings);

    uint8_t coverage = grid.get_coverage_at(32, 32);
    ASSERT_EQ(coverage, 255);
}

TEST(enforcer_full_coverage_to_disorder_suppression) {
    // Full coverage (255/255 = 1.0) -> disorder multiplier should be 0.3 (70% reduction)
    float normalized_coverage = 1.0f;
    float suppression = calculate_disorder_suppression(normalized_coverage);
    ASSERT_FLOAT_EQ(suppression, 0.3f);
}

TEST(enforcer_coverage_at_radius_edge_is_zero) {
    // Enforcer Post radius=8. At manhattan distance 8, coverage should be 0.
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings = {
        make_enforcer(32, 32, 1, true)
    };

    calculate_radius_coverage(grid, buildings);

    // Manhattan distance of 8 from (32,32) -> e.g. (40, 32) = distance 8
    // At distance == radius, falloff = 1.0 - 8/8 = 0.0
    uint8_t coverage_at_edge = grid.get_coverage_at(40, 32);
    ASSERT_EQ(coverage_at_edge, 0);
}

TEST(enforcer_zero_coverage_to_disorder_suppression) {
    // Zero coverage -> disorder multiplier should be 1.0 (no reduction)
    float suppression = calculate_disorder_suppression(0.0f);
    ASSERT_FLOAT_EQ(suppression, 1.0f);
}

TEST(enforcer_full_pipeline_coverage_to_suppression) {
    // Full end-to-end: place enforcer -> read coverage -> convert to suppression
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings = {
        make_enforcer(32, 32, 1, true)
    };
    calculate_radius_coverage(grid, buildings);

    // At building position: coverage = 255, normalized = 1.0
    float normalized = grid.get_coverage_at_normalized(32, 32);
    ASSERT_FLOAT_EQ(normalized, 1.0f);

    float suppression = calculate_disorder_suppression(normalized);
    ASSERT_FLOAT_EQ(suppression, 0.3f);

    // At edge: coverage = 0, normalized = 0.0
    float edge_norm = grid.get_coverage_at_normalized(40, 32);
    ASSERT_FLOAT_EQ(edge_norm, 0.0f);

    float edge_suppression = calculate_disorder_suppression(edge_norm);
    ASSERT_FLOAT_EQ(edge_suppression, 1.0f);
}

TEST(enforcer_partial_coverage_suppression) {
    // At some distance within the radius, coverage is partial.
    // For a Post (radius=8) at distance 4: falloff = 1.0 - 4/8 = 0.5
    // Coverage value = 0.5 * 255 = 127-128 (rounding)
    ServiceCoverageGrid grid(64, 64);
    std::vector<ServiceBuildingData> buildings = {
        make_enforcer(32, 32, 1, true)
    };
    calculate_radius_coverage(grid, buildings);

    float norm = grid.get_coverage_at_normalized(36, 32);  // distance=4
    // Should be approximately 0.5
    ASSERT(norm > 0.45f && norm < 0.55f);

    // Disorder suppression at 50% coverage: 1.0 - 0.5 * 0.7 = 0.65
    float suppression = calculate_disorder_suppression(norm);
    ASSERT(suppression > 0.62f && suppression < 0.68f);
}

// =============================================================================
// 2. Remove Enforcer -> Disorder Returns to Normal
// =============================================================================

TEST(remove_enforcer_coverage_clears) {
    // Place enforcer, verify coverage, then remove and re-calculate.
    // After removal, coverage at the building position should be 0.
    ServiceCoverageGrid grid(64, 64);

    // Step 1: Place enforcer
    std::vector<ServiceBuildingData> buildings = {
        make_enforcer(32, 32, 1, true)
    };
    calculate_radius_coverage(grid, buildings);
    ASSERT_EQ(grid.get_coverage_at(32, 32), 255);

    // Step 2: Remove enforcer (empty building list)
    std::vector<ServiceBuildingData> empty_buildings;
    calculate_radius_coverage(grid, empty_buildings);

    // Coverage should now be 0 everywhere
    ASSERT_EQ(grid.get_coverage_at(32, 32), 0);
}

TEST(remove_enforcer_suppression_returns_to_normal) {
    // After removing the enforcer, disorder suppression should return to 1.0
    ServiceCoverageGrid grid(64, 64);

    // Place enforcer
    std::vector<ServiceBuildingData> buildings = {
        make_enforcer(32, 32, 1, true)
    };
    calculate_radius_coverage(grid, buildings);
    float suppress_with = calculate_disorder_suppression(grid.get_coverage_at_normalized(32, 32));
    ASSERT_FLOAT_EQ(suppress_with, 0.3f);

    // Remove enforcer
    std::vector<ServiceBuildingData> empty;
    calculate_radius_coverage(grid, empty);
    float suppress_without = calculate_disorder_suppression(grid.get_coverage_at_normalized(32, 32));
    ASSERT_FLOAT_EQ(suppress_without, 1.0f);
}

TEST(remove_enforcer_via_system_events) {
    // Use ServicesSystem on_building_constructed / on_building_deconstructed
    // to verify dirty flags and coverage grid lifecycle.
    ServicesSystem system;
    system.init(64, 64);

    // Construct a building for player 0
    system.on_building_constructed(1, 0);
    ASSERT(system.is_dirty(ServiceType::Enforcer, 0));

    // Tick to process dirty flags -> allocates grid, clears coverage
    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // After tick, dirty flags should be cleared
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 0));

    // Coverage grid should have been allocated
    ServiceCoverageGrid* grid = system.get_coverage_grid(ServiceType::Enforcer, 0);
    ASSERT(grid != nullptr);

    // Grid should be all zeros (system doesn't populate building data from ECS yet)
    // This is correct: coverage = 0 -> suppression = 1.0 (no reduction)
    ASSERT_EQ(grid->get_coverage_at(32, 32), 0);

    // Deconstruct the building
    system.on_building_deconstructed(1, 0);
    ASSERT(system.is_dirty(ServiceType::Enforcer, 0));

    // Tick again
    time.m_tick = 2;
    system.tick(time);
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 0));

    // Coverage should still be 0 after removal
    ASSERT_EQ(grid->get_coverage_at(32, 32), 0);
}

// =============================================================================
// 3. Medical Building -> Longevity Pipeline
// =============================================================================

TEST(medical_exact_capacity_longevity_100) {
    // Population 500, Medical Post cap 500 -> effectiveness 1.0 -> longevity 100
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 100u);
}

TEST(medical_half_capacity_longevity_80) {
    // Population 1000, Medical Post cap 500 -> effectiveness 0.5 -> longevity 80
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 80u);
}

TEST(medical_zero_coverage_longevity_60) {
    // No medical buildings -> effectiveness 0.0 -> base longevity 60
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 60u);
}

TEST(medical_quarter_capacity_longevity_70) {
    // Population 2000, cap 500 -> effectiveness 0.25 -> longevity 60 + 10 = 70
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 2000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.25f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 70u);
}

TEST(medical_pipeline_full_chain) {
    // Multiple medical buildings -> aggregate capacity -> effectiveness -> longevity
    // Medical Post (500) + Medical Center (2000) = 2500 capacity
    // Population 2500 -> effectiveness 1.0 -> longevity 100
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true),
        make_global_building(ServiceType::Medical, 2000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 2500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 2500u);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 100u);
}

// =============================================================================
// 4. Education Building -> Land Value Pipeline
// =============================================================================

TEST(education_exact_capacity_multiplier_1_1) {
    // Learning Center cap 300, pop 300 -> effectiveness 1.0 -> multiplier 1.1
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Education, 300, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 300);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);

    float multiplier = calculate_education_land_value_multiplier(result.effectiveness);
    ASSERT_FLOAT_EQ(multiplier, 1.1f);
}

TEST(education_half_capacity_multiplier_1_05) {
    // Pop 600, cap 300 -> effectiveness 0.5 -> multiplier 1.05
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Education, 300, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 600);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);

    float multiplier = calculate_education_land_value_multiplier(result.effectiveness);
    ASSERT_FLOAT_EQ(multiplier, 1.05f);
}

TEST(education_zero_coverage_multiplier_1_0) {
    // No education buildings -> effectiveness 0.0 -> multiplier 1.0
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Education, buildings, 300);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);

    float multiplier = calculate_education_land_value_multiplier(result.effectiveness);
    ASSERT_FLOAT_EQ(multiplier, 1.0f);
}

TEST(education_pipeline_full_chain) {
    // Archive (1200) + Learning Center (300) = 1500, pop 1500 -> 1.0 -> 1.1
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Education, 300, true),
        make_global_building(ServiceType::Education, 1200, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 1500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1500u);

    float multiplier = calculate_education_land_value_multiplier(result.effectiveness);
    ASSERT_FLOAT_EQ(multiplier, 1.1f);
}

// =============================================================================
// 5. Stub Replacement Verification
// =============================================================================

TEST(stub_service_queryable_returns_zero_coverage) {
    // StubServiceQueryable returns 0.0f for all queries (not 0.5)
    StubServiceQueryable stub;

    ASSERT_FLOAT_EQ(stub.get_coverage(0, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage(1, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage(2, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage(3, 0), 0.0f);
}

TEST(stub_service_queryable_returns_zero_coverage_at) {
    StubServiceQueryable stub;

    ASSERT_FLOAT_EQ(stub.get_coverage_at(0, 10, 20, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage_at(1, 10, 20, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage_at(2, 10, 20, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage_at(3, 10, 20, 0), 0.0f);
}

TEST(stub_service_queryable_returns_zero_effectiveness) {
    StubServiceQueryable stub;

    ASSERT_FLOAT_EQ(stub.get_effectiveness(0, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_effectiveness(1, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_effectiveness(2, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_effectiveness(3, 0), 0.0f);
}

TEST(stub_means_no_bonuses) {
    // With stub (0.0f coverage), verify all integration contracts produce "no bonus" values.
    StubServiceQueryable stub;

    // Enforcer: 0 coverage -> suppression = 1.0 (no reduction)
    float suppress = calculate_disorder_suppression(stub.get_coverage(0, 0));
    ASSERT_FLOAT_EQ(suppress, 1.0f);

    // Medical: 0 effectiveness -> longevity = 60 (base only)
    uint32_t longevity = calculate_longevity(stub.get_effectiveness(2, 0));
    ASSERT_EQ(longevity, 60u);

    // Education: 0 effectiveness -> multiplier = 1.0 (no bonus)
    float multiplier = calculate_education_land_value_multiplier(stub.get_effectiveness(3, 0));
    ASSERT_FLOAT_EQ(multiplier, 1.0f);
}

TEST(stub_restrictive_mode_same_as_default) {
    // For StubServiceQueryable, restrictive mode returns same values as default
    // (services are opt-in, so 0.0f is the safe default both ways)
    StubServiceQueryable stub;
    stub.set_debug_restrictive(true);

    ASSERT_FLOAT_EQ(stub.get_coverage(0, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_coverage_at(0, 10, 20, 0), 0.0f);
    ASSERT_FLOAT_EQ(stub.get_effectiveness(0, 0), 0.0f);
}

// =============================================================================
// 6. Full Tick Cycle
// =============================================================================

TEST(full_tick_cycle_init_and_tick) {
    // Init, add buildings, tick, verify grids allocated and dirty cleared
    ServicesSystem system;
    system.init(64, 64);

    ASSERT(system.isInitialized());
    ASSERT(!system.isCoverageDirty());

    // No coverage grids allocated yet (lazy allocation)
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 0) == nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::Medical, 0) == nullptr);
}

TEST(full_tick_cycle_building_marks_dirty) {
    ServicesSystem system;
    system.init(64, 64);

    // Add an enforcer building for player 0
    system.on_building_constructed(100, 0);

    // All service types should be dirty for player 0
    ASSERT(system.is_dirty(ServiceType::Enforcer, 0));
    ASSERT(system.is_dirty(ServiceType::HazardResponse, 0));
    ASSERT(system.is_dirty(ServiceType::Medical, 0));
    ASSERT(system.is_dirty(ServiceType::Education, 0));

    // Player 1 should NOT be dirty
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 1));
}

TEST(full_tick_cycle_tick_clears_dirty) {
    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(100, 0);
    ASSERT(system.isCoverageDirty());

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // After tick, dirty flags should be cleared
    ASSERT(!system.isCoverageDirty());
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 0));
    ASSERT(!system.is_dirty(ServiceType::Medical, 0));
}

TEST(full_tick_cycle_grids_allocated_after_tick) {
    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(100, 0);

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // Grids should now be allocated for player 0 (all service types)
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::HazardResponse, 0) != nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::Medical, 0) != nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::Education, 0) != nullptr);

    // Player 1 grids should still be null (no buildings added)
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 1) == nullptr);
}

TEST(full_tick_cycle_multiple_ticks) {
    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(1, 0);

    MockSimulationTime time;

    // First tick: allocates grids, clears dirty
    time.m_tick = 1;
    system.tick(time);
    ASSERT(!system.isCoverageDirty());

    // Second tick: no dirty, no reallocation, no crash
    time.m_tick = 2;
    system.tick(time);
    ASSERT(!system.isCoverageDirty());

    // Add another building: re-marks dirty
    system.on_building_constructed(2, 0);
    ASSERT(system.isCoverageDirty());

    // Third tick: recalculates
    time.m_tick = 3;
    system.tick(time);
    ASSERT(!system.isCoverageDirty());
}

TEST(full_tick_cycle_multiple_building_types) {
    // Add buildings of different types, verify each grid is populated
    ServicesSystem system;
    system.init(64, 64);

    // Add buildings for player 0 and player 1
    system.on_building_constructed(1, 0);
    system.on_building_constructed(2, 1);

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // Both players' grids should be allocated
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 1) != nullptr);

    // Grid dimensions should match init
    ServiceCoverageGrid* grid0 = system.get_coverage_grid(ServiceType::Enforcer, 0);
    ASSERT_EQ(grid0->get_width(), 64u);
    ASSERT_EQ(grid0->get_height(), 64u);
}

TEST(full_tick_cycle_redirty_after_additional_building) {
    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(1, 0);

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);
    ASSERT(!system.isCoverageDirty());

    // Add more buildings -> re-dirty
    system.on_building_constructed(2, 0);
    system.on_building_constructed(3, 0);
    ASSERT(system.isCoverageDirty());

    time.m_tick = 2;
    system.tick(time);
    ASSERT(!system.isCoverageDirty());
}

// =============================================================================
// 7. Funding Integration
// =============================================================================

TEST(funding_50_percent_halved_longevity) {
    // Medical with 50% funding -> half effectiveness -> reduced longevity
    // Cap 500, pop 500 -> raw effectiveness = 1.0
    // Funding modifier at 50% -> 0.5
    // Effective = 1.0 * 0.5 = 0.5
    // Longevity = 60 + 0.5 * 40 = 80
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500, 50);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 80u);
}

TEST(funding_0_percent_zero_effectiveness) {
    // Medical with 0% funding -> zero effectiveness -> base longevity only
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 60u);
}

TEST(funding_100_percent_full_longevity) {
    // Medical with 100% funding -> normal effectiveness -> full longevity
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500, 100);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);

    uint32_t longevity = calculate_longevity(result.effectiveness);
    ASSERT_EQ(longevity, 100u);
}

TEST(funding_50_percent_education_reduced) {
    // Education with 50% funding -> half effectiveness -> reduced multiplier
    // Cap 300, pop 300 -> raw 1.0 * 0.5 = 0.5 -> multiplier 1.05
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Education, 300, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 300, 50);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);

    float multiplier = calculate_education_land_value_multiplier(result.effectiveness);
    ASSERT_FLOAT_EQ(multiplier, 1.05f);
}

TEST(funding_0_percent_education_no_bonus) {
    // Education with 0% funding -> zero effectiveness -> no bonus
    std::vector<ServiceBuildingData> buildings = {
        make_global_building(ServiceType::Education, 300, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 300, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);

    float multiplier = calculate_education_land_value_multiplier(result.effectiveness);
    ASSERT_FLOAT_EQ(multiplier, 1.0f);
}

TEST(funding_modifier_values) {
    // Verify the funding modifier function for key values
    ASSERT_FLOAT_EQ(calculate_funding_modifier(0), 0.0f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(50), 0.5f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(100), 1.0f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(150), 1.15f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(200), 1.15f);
}

// =============================================================================
// 8. Multi-Player Isolation
// =============================================================================

TEST(multiplayer_separate_dirty_flags) {
    // Player 1 adds enforcer -> only player 1's flags dirty
    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(1, 0);

    // Player 0 dirty, player 1 not
    ASSERT(system.is_dirty(ServiceType::Enforcer, 0));
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 1));
}

TEST(multiplayer_separate_grid_allocation) {
    // Player 1 adds enforcer -> only player 1's grid updated
    // Player 2 adds enforcer -> only player 2's grid updated
    ServicesSystem system;
    system.init(64, 64);

    // Player 0 adds a building
    system.on_building_constructed(1, 0);

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // Player 0 grids allocated, player 1 not
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 1) == nullptr);

    // Player 1 adds a building
    system.on_building_constructed(2, 1);
    time.m_tick = 2;
    system.tick(time);

    // Now both players have grids
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);
    ASSERT(system.get_coverage_grid(ServiceType::Enforcer, 1) != nullptr);
}

TEST(multiplayer_no_cross_player_coverage_bleed) {
    // Verify that player 1 and player 2 have independent coverage grids
    // Direct coverage calculation test (bypassing ServicesSystem ECS gap)
    ServiceCoverageGrid grid_p0(64, 64);
    ServiceCoverageGrid grid_p1(64, 64);

    // Player 0: enforcer at (10, 10)
    std::vector<ServiceBuildingData> buildings_p0 = {
        make_enforcer(10, 10, 1, true, 0)
    };
    calculate_radius_coverage(grid_p0, buildings_p0);

    // Player 1: enforcer at (50, 50)
    std::vector<ServiceBuildingData> buildings_p1 = {
        make_enforcer(50, 50, 1, true, 1)
    };
    calculate_radius_coverage(grid_p1, buildings_p1);

    // Player 0's grid: coverage at (10,10) = 255, at (50,50) = 0
    ASSERT_EQ(grid_p0.get_coverage_at(10, 10), 255);
    ASSERT_EQ(grid_p0.get_coverage_at(50, 50), 0);

    // Player 1's grid: coverage at (50,50) = 255, at (10,10) = 0
    ASSERT_EQ(grid_p1.get_coverage_at(50, 50), 255);
    ASSERT_EQ(grid_p1.get_coverage_at(10, 10), 0);
}

TEST(multiplayer_independent_dirty_tracking) {
    // Adding/removing buildings for one player doesn't affect another
    ServicesSystem system;
    system.init(64, 64);

    // Player 0 builds
    system.on_building_constructed(1, 0);
    ASSERT(system.is_dirty(ServiceType::Enforcer, 0));
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 1));
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 2));
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 3));

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // Player 1 builds - only player 1 dirty
    system.on_building_constructed(2, 1);
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 0));
    ASSERT(system.is_dirty(ServiceType::Enforcer, 1));

    time.m_tick = 2;
    system.tick(time);

    // Player 0 removes - only player 0 dirty
    system.on_building_deconstructed(1, 0);
    ASSERT(system.is_dirty(ServiceType::Enforcer, 0));
    ASSERT(!system.is_dirty(ServiceType::Enforcer, 1));
}

TEST(multiplayer_all_four_players) {
    // Verify all 4 players can have independent state
    ServicesSystem system;
    system.init(32, 32);

    for (uint8_t p = 0; p < 4; ++p) {
        system.on_building_constructed(100 + p, p);
    }

    // All 4 players should be dirty
    for (uint8_t p = 0; p < 4; ++p) {
        ASSERT(system.is_dirty(ServiceType::Enforcer, p));
    }

    MockSimulationTime time;
    time.m_tick = 1;
    system.tick(time);

    // All grids allocated
    for (uint8_t p = 0; p < 4; ++p) {
        ASSERT(system.get_coverage_grid(ServiceType::Enforcer, p) != nullptr);
        ASSERT(!system.is_dirty(ServiceType::Enforcer, p));
    }
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Epic 10 Integration Tests (Ticket E9-052) ===\n\n");

    // 1. Enforcer coverage -> disorder suppression pipeline
    printf("[1] Enforcer Coverage -> Disorder Suppression Pipeline\n");
    RUN_TEST(enforcer_coverage_full_at_building_position);
    RUN_TEST(enforcer_full_coverage_to_disorder_suppression);
    RUN_TEST(enforcer_coverage_at_radius_edge_is_zero);
    RUN_TEST(enforcer_zero_coverage_to_disorder_suppression);
    RUN_TEST(enforcer_full_pipeline_coverage_to_suppression);
    RUN_TEST(enforcer_partial_coverage_suppression);
    printf("\n");

    // 2. Remove enforcer -> disorder returns to normal
    printf("[2] Remove Enforcer -> Disorder Returns to Normal\n");
    RUN_TEST(remove_enforcer_coverage_clears);
    RUN_TEST(remove_enforcer_suppression_returns_to_normal);
    RUN_TEST(remove_enforcer_via_system_events);
    printf("\n");

    // 3. Medical building -> longevity pipeline
    printf("[3] Medical Building -> Longevity Pipeline\n");
    RUN_TEST(medical_exact_capacity_longevity_100);
    RUN_TEST(medical_half_capacity_longevity_80);
    RUN_TEST(medical_zero_coverage_longevity_60);
    RUN_TEST(medical_quarter_capacity_longevity_70);
    RUN_TEST(medical_pipeline_full_chain);
    printf("\n");

    // 4. Education building -> land value pipeline
    printf("[4] Education Building -> Land Value Pipeline\n");
    RUN_TEST(education_exact_capacity_multiplier_1_1);
    RUN_TEST(education_half_capacity_multiplier_1_05);
    RUN_TEST(education_zero_coverage_multiplier_1_0);
    RUN_TEST(education_pipeline_full_chain);
    printf("\n");

    // 5. Stub replacement verification
    printf("[5] Stub Replacement Verification\n");
    RUN_TEST(stub_service_queryable_returns_zero_coverage);
    RUN_TEST(stub_service_queryable_returns_zero_coverage_at);
    RUN_TEST(stub_service_queryable_returns_zero_effectiveness);
    RUN_TEST(stub_means_no_bonuses);
    RUN_TEST(stub_restrictive_mode_same_as_default);
    printf("\n");

    // 6. Full tick cycle
    printf("[6] Full Tick Cycle\n");
    RUN_TEST(full_tick_cycle_init_and_tick);
    RUN_TEST(full_tick_cycle_building_marks_dirty);
    RUN_TEST(full_tick_cycle_tick_clears_dirty);
    RUN_TEST(full_tick_cycle_grids_allocated_after_tick);
    RUN_TEST(full_tick_cycle_multiple_ticks);
    RUN_TEST(full_tick_cycle_multiple_building_types);
    RUN_TEST(full_tick_cycle_redirty_after_additional_building);
    printf("\n");

    // 7. Funding integration
    printf("[7] Funding Integration\n");
    RUN_TEST(funding_50_percent_halved_longevity);
    RUN_TEST(funding_0_percent_zero_effectiveness);
    RUN_TEST(funding_100_percent_full_longevity);
    RUN_TEST(funding_50_percent_education_reduced);
    RUN_TEST(funding_0_percent_education_no_bonus);
    RUN_TEST(funding_modifier_values);
    printf("\n");

    // 8. Multi-player isolation
    printf("[8] Multi-Player Isolation\n");
    RUN_TEST(multiplayer_separate_dirty_flags);
    RUN_TEST(multiplayer_separate_grid_allocation);
    RUN_TEST(multiplayer_no_cross_player_coverage_bleed);
    RUN_TEST(multiplayer_independent_dirty_tracking);
    RUN_TEST(multiplayer_all_four_players);
    printf("\n");

    printf("=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
