/**
 * @file test_contamination_aggregator.cpp
 * @brief Unit tests for IContaminationSource and ContaminationAggregator
 *        (Ticket E10-082).
 *
 * Tests cover:
 * - ContaminationAggregator with no sources: no changes to grid
 * - Register a source, apply: grid gets contamination
 * - Multiple sources aggregate correctly
 * - Unregister source: no longer contributes
 * - Output clamped to 255
 */

#include <sims3000/contamination/IContaminationSource.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::contamination;

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
// Mock contamination source
// =============================================================================

class MockContaminationSource : public IContaminationSource {
public:
    std::vector<ContaminationSourceEntry> entries;

    void get_contamination_sources(std::vector<ContaminationSourceEntry>& out) const override {
        for (const auto& entry : entries) {
            out.push_back(entry);
        }
    }
};

// =============================================================================
// No sources
// =============================================================================

TEST(no_sources_no_changes) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    aggregator.apply_all_sources(grid);

    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(32, 32), 0);
    ASSERT_EQ(grid.get_level(63, 63), 0);
}

TEST(no_sources_count_zero) {
    ContaminationAggregator aggregator;
    ASSERT_EQ(aggregator.get_source_count(), static_cast<size_t>(0));
}

// =============================================================================
// Single source
// =============================================================================

TEST(register_and_apply_single_source) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source;
    source.entries.push_back({ 10, 20, 50, ContaminationType::Industrial });

    aggregator.register_source(&source);
    aggregator.apply_all_sources(grid);

    ASSERT_EQ(grid.get_level(10, 20), 50);
}

TEST(source_count_after_register) {
    ContaminationAggregator aggregator;
    MockContaminationSource source;

    aggregator.register_source(&source);
    ASSERT_EQ(aggregator.get_source_count(), static_cast<size_t>(1));
}

TEST(single_source_sets_type) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source;
    source.entries.push_back({ 10, 20, 50, ContaminationType::Energy });

    aggregator.register_source(&source);
    aggregator.apply_all_sources(grid);

    ASSERT_EQ(grid.get_dominant_type(10, 20),
              static_cast<uint8_t>(ContaminationType::Energy));
}

// =============================================================================
// Multiple sources
// =============================================================================

TEST(multiple_sources_aggregate) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source1;
    source1.entries.push_back({ 10, 20, 30, ContaminationType::Industrial });

    MockContaminationSource source2;
    source2.entries.push_back({ 10, 20, 40, ContaminationType::Traffic });

    aggregator.register_source(&source1);
    aggregator.register_source(&source2);
    aggregator.apply_all_sources(grid);

    // Should be 30 + 40 = 70
    ASSERT_EQ(grid.get_level(10, 20), 70);
}

TEST(multiple_sources_different_locations) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source1;
    source1.entries.push_back({ 5, 5, 25, ContaminationType::Industrial });

    MockContaminationSource source2;
    source2.entries.push_back({ 30, 30, 75, ContaminationType::Energy });

    aggregator.register_source(&source1);
    aggregator.register_source(&source2);
    aggregator.apply_all_sources(grid);

    ASSERT_EQ(grid.get_level(5, 5), 25);
    ASSERT_EQ(grid.get_level(30, 30), 75);
}

TEST(multiple_entries_from_single_source) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source;
    source.entries.push_back({ 5, 5, 20, ContaminationType::Industrial });
    source.entries.push_back({ 10, 10, 30, ContaminationType::Traffic });
    source.entries.push_back({ 5, 5, 10, ContaminationType::Energy });

    aggregator.register_source(&source);
    aggregator.apply_all_sources(grid);

    // (5,5) = 20 + 10 = 30
    ASSERT_EQ(grid.get_level(5, 5), 30);
    ASSERT_EQ(grid.get_level(10, 10), 30);
}

// =============================================================================
// Unregister
// =============================================================================

TEST(unregister_source_no_longer_contributes) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source1;
    source1.entries.push_back({ 10, 10, 50, ContaminationType::Industrial });

    MockContaminationSource source2;
    source2.entries.push_back({ 10, 10, 30, ContaminationType::Traffic });

    aggregator.register_source(&source1);
    aggregator.register_source(&source2);

    // Unregister source1
    aggregator.unregister_source(&source1);

    aggregator.apply_all_sources(grid);

    // Only source2 contributes now
    ASSERT_EQ(grid.get_level(10, 10), 30);
}

TEST(unregister_reduces_count) {
    ContaminationAggregator aggregator;
    MockContaminationSource source1;
    MockContaminationSource source2;

    aggregator.register_source(&source1);
    aggregator.register_source(&source2);
    ASSERT_EQ(aggregator.get_source_count(), static_cast<size_t>(2));

    aggregator.unregister_source(&source1);
    ASSERT_EQ(aggregator.get_source_count(), static_cast<size_t>(1));
}

TEST(unregister_nonexistent_is_noop) {
    ContaminationAggregator aggregator;
    MockContaminationSource source1;
    MockContaminationSource source2;

    aggregator.register_source(&source1);
    aggregator.unregister_source(&source2); // Not registered

    ASSERT_EQ(aggregator.get_source_count(), static_cast<size_t>(1));
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(output_clamped_to_255) {
    ContaminationGrid grid(64, 64);
    ContaminationAggregator aggregator;

    MockContaminationSource source;
    source.entries.push_back({ 5, 5, 1000, ContaminationType::Industrial });

    aggregator.register_source(&source);
    aggregator.apply_all_sources(grid);

    // Output 1000 should be clamped to 255
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(register_null_is_noop) {
    ContaminationAggregator aggregator;
    aggregator.register_source(nullptr);
    ASSERT_EQ(aggregator.get_source_count(), static_cast<size_t>(0));
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Contamination Aggregator Tests (E10-082) ===\n\n");

    RUN_TEST(no_sources_no_changes);
    RUN_TEST(no_sources_count_zero);
    RUN_TEST(register_and_apply_single_source);
    RUN_TEST(source_count_after_register);
    RUN_TEST(single_source_sets_type);
    RUN_TEST(multiple_sources_aggregate);
    RUN_TEST(multiple_sources_different_locations);
    RUN_TEST(multiple_entries_from_single_source);
    RUN_TEST(unregister_source_no_longer_contributes);
    RUN_TEST(unregister_reduces_count);
    RUN_TEST(unregister_nonexistent_is_noop);
    RUN_TEST(output_clamped_to_255);
    RUN_TEST(register_null_is_noop);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
