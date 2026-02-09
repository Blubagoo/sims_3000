/**
 * @file test_population_net_sync.cpp
 * @brief Tests for population network synchronization (Ticket E10-032)
 *
 * Validates:
 * - Snapshot creation from PopulationData/EmploymentData
 * - Snapshot application to local state
 * - Serialization to byte buffer
 * - Deserialization from byte buffer
 * - Round-trip snapshot → serialize → deserialize → apply
 * - Buffer size validation
 * - Data integrity after sync
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "sims3000/population/PopulationNetSync.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Create snapshot from population data
// --------------------------------------------------------------------------
static void test_create_snapshot() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.max_capacity = 6000;
    pop.youth_percent = 30;
    pop.adult_percent = 50;
    pop.elder_percent = 20;
    pop.growth_rate = 0.025f;  // 2.5%
    pop.harmony_index = 75;
    pop.health_index = 80;
    pop.education_index = 60;

    EmploymentData emp{};
    emp.unemployment_rate = 8;
    emp.employed_laborers = 2300;
    emp.total_jobs = 2500;

    PopulationSnapshot snapshot = create_snapshot(pop, emp);

    assert(snapshot.total_beings == 5000 && "Total beings should match");
    assert(snapshot.max_capacity == 6000 && "Max capacity should match");
    assert(snapshot.youth_percent == 30 && "Youth percent should match");
    assert(snapshot.adult_percent == 50 && "Adult percent should match");
    assert(snapshot.elder_percent == 20 && "Elder percent should match");
    assert(snapshot.growth_rate == 25 && "Growth rate should be 25 per 1000");
    assert(snapshot.harmony_index == 75 && "Harmony should match");
    assert(snapshot.health_index == 80 && "Health should match");
    assert(snapshot.education_index == 60 && "Education should match");
    assert(snapshot.unemployment_rate == 8 && "Unemployment rate should match");
    assert(snapshot.employed_laborers == 2300 && "Employed laborers should match");
    assert(snapshot.total_jobs == 2500 && "Total jobs should match");

    std::printf("  PASS: Create snapshot from population data\n");
}

// --------------------------------------------------------------------------
// Test: Apply snapshot to local state
// --------------------------------------------------------------------------
static void test_apply_snapshot() {
    PopulationSnapshot snapshot{};
    snapshot.total_beings = 3000;
    snapshot.max_capacity = 4000;
    snapshot.youth_percent = 25;
    snapshot.adult_percent = 55;
    snapshot.elder_percent = 20;
    snapshot.growth_rate = 15;  // 1.5% as per-1000
    snapshot.harmony_index = 65;
    snapshot.health_index = 70;
    snapshot.education_index = 55;
    snapshot.unemployment_rate = 10;
    snapshot.employed_laborers = 1500;
    snapshot.total_jobs = 1667;

    PopulationData pop{};
    EmploymentData emp{};

    apply_snapshot(pop, emp, snapshot);

    assert(pop.total_beings == 3000 && "Total beings should be applied");
    assert(pop.max_capacity == 4000 && "Max capacity should be applied");
    assert(pop.youth_percent == 25 && "Youth percent should be applied");
    assert(pop.adult_percent == 55 && "Adult percent should be applied");
    assert(pop.elder_percent == 20 && "Elder percent should be applied");
    assert(approx(pop.growth_rate, 0.015f) && "Growth rate should be 0.015");
    assert(pop.harmony_index == 65 && "Harmony should be applied");
    assert(pop.health_index == 70 && "Health should be applied");
    assert(pop.education_index == 55 && "Education should be applied");
    assert(emp.unemployment_rate == 10 && "Unemployment rate should be applied");
    assert(emp.employed_laborers == 1500 && "Employed laborers should be applied");
    assert(emp.total_jobs == 1667 && "Total jobs should be applied");

    std::printf("  PASS: Apply snapshot to local state\n");
}

// --------------------------------------------------------------------------
// Test: Serialize snapshot
// --------------------------------------------------------------------------
static void test_serialize_snapshot() {
    PopulationSnapshot snapshot{};
    snapshot.total_beings = 1234;
    snapshot.max_capacity = 5678;
    snapshot.youth_percent = 33;
    snapshot.adult_percent = 34;
    snapshot.elder_percent = 33;
    snapshot.growth_rate = 20;
    snapshot.harmony_index = 50;
    snapshot.health_index = 60;
    snapshot.education_index = 70;
    snapshot.unemployment_rate = 5;
    snapshot.employed_laborers = 1000;
    snapshot.total_jobs = 1053;

    uint8_t buffer[256];
    size_t written = serialize_snapshot(snapshot, buffer, sizeof(buffer));

    assert(written == sizeof(PopulationSnapshot) && "Should write snapshot size bytes");
    assert(written > 0 && "Should write non-zero bytes");

    std::printf("  PASS: Serialize snapshot\n");
}

// --------------------------------------------------------------------------
// Test: Deserialize snapshot
// --------------------------------------------------------------------------
static void test_deserialize_snapshot() {
    PopulationSnapshot original{};
    original.total_beings = 9999;
    original.max_capacity = 11000;
    original.youth_percent = 40;
    original.adult_percent = 40;
    original.elder_percent = 20;
    original.growth_rate = -5;  // Negative growth
    original.harmony_index = 45;
    original.health_index = 55;
    original.education_index = 65;
    original.unemployment_rate = 15;
    original.employed_laborers = 3400;
    original.total_jobs = 4000;

    uint8_t buffer[256];
    size_t written = serialize_snapshot(original, buffer, sizeof(buffer));

    PopulationSnapshot deserialized{};
    bool success = deserialize_snapshot(buffer, written, deserialized);

    assert(success && "Deserialization should succeed");
    assert(deserialized.total_beings == 9999 && "Total beings should match");
    assert(deserialized.max_capacity == 11000 && "Max capacity should match");
    assert(deserialized.youth_percent == 40 && "Youth percent should match");
    assert(deserialized.adult_percent == 40 && "Adult percent should match");
    assert(deserialized.elder_percent == 20 && "Elder percent should match");
    assert(deserialized.growth_rate == -5 && "Growth rate should match (including negative)");
    assert(deserialized.harmony_index == 45 && "Harmony should match");
    assert(deserialized.health_index == 55 && "Health should match");
    assert(deserialized.education_index == 65 && "Education should match");
    assert(deserialized.unemployment_rate == 15 && "Unemployment rate should match");
    assert(deserialized.employed_laborers == 3400 && "Employed laborers should match");
    assert(deserialized.total_jobs == 4000 && "Total jobs should match");

    std::printf("  PASS: Deserialize snapshot\n");
}

// --------------------------------------------------------------------------
// Test: Round-trip sync
// --------------------------------------------------------------------------
static void test_round_trip_sync() {
    // Create source data
    PopulationData source_pop{};
    source_pop.total_beings = 7500;
    source_pop.max_capacity = 8000;
    source_pop.youth_percent = 28;
    source_pop.adult_percent = 52;
    source_pop.elder_percent = 20;
    source_pop.growth_rate = 0.032f;
    source_pop.harmony_index = 68;
    source_pop.health_index = 72;
    source_pop.education_index = 58;

    EmploymentData source_emp{};
    source_emp.unemployment_rate = 7;
    source_emp.employed_laborers = 3600;
    source_emp.total_jobs = 3870;

    // Create snapshot
    PopulationSnapshot snapshot = create_snapshot(source_pop, source_emp);

    // Serialize
    uint8_t buffer[256];
    size_t written = serialize_snapshot(snapshot, buffer, sizeof(buffer));
    assert(written > 0 && "Serialization should succeed");

    // Deserialize
    PopulationSnapshot received{};
    bool success = deserialize_snapshot(buffer, written, received);
    assert(success && "Deserialization should succeed");

    // Apply to destination
    PopulationData dest_pop{};
    EmploymentData dest_emp{};
    apply_snapshot(dest_pop, dest_emp, received);

    // Verify all data matches
    assert(dest_pop.total_beings == source_pop.total_beings && "Total beings should match");
    assert(dest_pop.max_capacity == source_pop.max_capacity && "Max capacity should match");
    assert(dest_pop.youth_percent == source_pop.youth_percent && "Youth percent should match");
    assert(dest_pop.adult_percent == source_pop.adult_percent && "Adult percent should match");
    assert(dest_pop.elder_percent == source_pop.elder_percent && "Elder percent should match");
    assert(approx(dest_pop.growth_rate, source_pop.growth_rate) && "Growth rate should match");
    assert(dest_pop.harmony_index == source_pop.harmony_index && "Harmony should match");
    assert(dest_pop.health_index == source_pop.health_index && "Health should match");
    assert(dest_pop.education_index == source_pop.education_index && "Education should match");
    assert(dest_emp.unemployment_rate == source_emp.unemployment_rate && "Unemployment rate should match");
    assert(dest_emp.employed_laborers == source_emp.employed_laborers && "Employed laborers should match");
    assert(dest_emp.total_jobs == source_emp.total_jobs && "Total jobs should match");

    std::printf("  PASS: Round-trip sync\n");
}

// --------------------------------------------------------------------------
// Test: Buffer too small for serialization
// --------------------------------------------------------------------------
static void test_serialize_buffer_too_small() {
    PopulationSnapshot snapshot{};
    snapshot.total_beings = 100;

    uint8_t small_buffer[4];  // Too small
    size_t written = serialize_snapshot(snapshot, small_buffer, sizeof(small_buffer));

    assert(written == 0 && "Should return 0 when buffer too small");

    std::printf("  PASS: Serialize buffer too small\n");
}

// --------------------------------------------------------------------------
// Test: Buffer too small for deserialization
// --------------------------------------------------------------------------
static void test_deserialize_buffer_too_small() {
    uint8_t small_buffer[4] = { 0, 0, 0, 0 };
    PopulationSnapshot snapshot{};

    bool success = deserialize_snapshot(small_buffer, sizeof(small_buffer), snapshot);

    assert(!success && "Should fail when buffer too small");

    std::printf("  PASS: Deserialize buffer too small\n");
}

// --------------------------------------------------------------------------
// Test: Snapshot size is reasonable
// --------------------------------------------------------------------------
static void test_snapshot_size() {
    size_t size = sizeof(PopulationSnapshot);

    // Should be compact for network transmission (under 64 bytes)
    assert(size <= 64 && "Snapshot should be compact for network");
    assert(size >= 32 && "Snapshot should contain meaningful data");

    std::printf("  PASS: Snapshot size is reasonable (%zu bytes)\n", size);
}

// --------------------------------------------------------------------------
// Test: Negative growth rate handling
// --------------------------------------------------------------------------
static void test_negative_growth_rate() {
    PopulationData pop{};
    pop.growth_rate = -0.018f;  // -1.8% (shrinking)

    EmploymentData emp{};

    PopulationSnapshot snapshot = create_snapshot(pop, emp);
    assert(snapshot.growth_rate == -18 && "Negative growth rate should be preserved");

    PopulationData pop2{};
    EmploymentData emp2{};
    apply_snapshot(pop2, emp2, snapshot);
    assert(approx(pop2.growth_rate, -0.018f) && "Negative growth rate should be restored");

    std::printf("  PASS: Negative growth rate handling\n");
}

// --------------------------------------------------------------------------
// Test: Zero values
// --------------------------------------------------------------------------
static void test_zero_values() {
    PopulationData pop{};
    EmploymentData emp{};

    // All zeros
    PopulationSnapshot snapshot = create_snapshot(pop, emp);
    assert(snapshot.total_beings == 0 && "Zero total beings should work");
    assert(snapshot.growth_rate == 0 && "Zero growth rate should work");

    PopulationData pop2{};
    EmploymentData emp2{};
    apply_snapshot(pop2, emp2, snapshot);
    assert(pop2.total_beings == 0 && "Zero values should round-trip");

    std::printf("  PASS: Zero values\n");
}

// --------------------------------------------------------------------------
// Test: Maximum values
// --------------------------------------------------------------------------
static void test_maximum_values() {
    PopulationData pop{};
    pop.total_beings = UINT32_MAX;
    pop.max_capacity = UINT32_MAX;
    pop.youth_percent = 100;
    pop.adult_percent = 0;
    pop.elder_percent = 0;
    pop.growth_rate = 10.0f;  // Very high growth
    pop.harmony_index = 100;
    pop.health_index = 100;
    pop.education_index = 100;

    EmploymentData emp{};
    emp.unemployment_rate = 100;
    emp.employed_laborers = UINT32_MAX;
    emp.total_jobs = UINT32_MAX;

    PopulationSnapshot snapshot = create_snapshot(pop, emp);

    uint8_t buffer[256];
    size_t written = serialize_snapshot(snapshot, buffer, sizeof(buffer));
    assert(written > 0 && "Should handle maximum values");

    PopulationSnapshot snapshot2{};
    bool success = deserialize_snapshot(buffer, written, snapshot2);
    assert(success && "Should deserialize maximum values");

    assert(snapshot2.total_beings == UINT32_MAX && "Max uint32 should round-trip");
    assert(snapshot2.harmony_index == 100 && "Max index should round-trip");

    std::printf("  PASS: Maximum values\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Population Network Sync Tests (E10-032) ===\n");

    test_create_snapshot();
    test_apply_snapshot();
    test_serialize_snapshot();
    test_deserialize_snapshot();
    test_round_trip_sync();
    test_serialize_buffer_too_small();
    test_deserialize_buffer_too_small();
    test_snapshot_size();
    test_negative_growth_rate();
    test_zero_values();
    test_maximum_values();

    std::printf("All population network sync tests passed.\n");
    return 0;
}
