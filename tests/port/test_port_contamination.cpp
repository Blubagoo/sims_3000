/**
 * @file test_port_contamination.cpp
 * @brief Unit tests for port noise/contamination effects (Epic 8, Ticket E8-033)
 *
 * Tests cover:
 * - Default contamination radius per port type
 * - Single source contamination calculation (linear falloff)
 * - Multiple source stacking (capped at 255)
 * - Non-operational ports produce no contamination
 * - In-contamination-zone check
 * - Edge cases: zero intensity, zero radius, exact boundary
 * - Contamination type names
 */

#include <sims3000/port/PortContamination.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// Default Radius Tests
// =============================================================================

void test_aero_default_radius() {
    printf("Testing aero port default contamination radius...\n");
    assert(get_default_contamination_radius(PortType::Aero) == 10);
    printf("  PASS: Aero radius = 10\n");
}

void test_aqua_default_radius() {
    printf("Testing aqua port default contamination radius...\n");
    assert(get_default_contamination_radius(PortType::Aqua) == 8);
    printf("  PASS: Aqua radius = 8\n");
}

// =============================================================================
// Single Source Contamination Tests
// =============================================================================

void test_contamination_at_source() {
    printf("Testing contamination at source position (distance=0)...\n");
    PortContaminationSource source = {50, 50, PortType::Aero, 10, 200, true};
    uint8_t value = calculate_single_source_contamination(50, 50, source);
    assert(value == 200);
    printf("  PASS: At source -> intensity = %d (expected 200)\n", value);
}

void test_contamination_at_half_radius() {
    printf("Testing contamination at half radius...\n");
    // radius=10, intensity=200, distance=5 -> 200 * (1 - 5/10) = 100
    PortContaminationSource source = {50, 50, PortType::Aero, 10, 200, true};
    uint8_t value = calculate_single_source_contamination(55, 50, source);
    assert(value == 100);
    printf("  PASS: At half radius -> intensity = %d (expected 100)\n", value);
}

void test_contamination_at_boundary() {
    printf("Testing contamination at exact boundary...\n");
    // radius=10, intensity=200, distance=10 -> 200 * (1 - 10/10) = 0
    PortContaminationSource source = {50, 50, PortType::Aero, 10, 200, true};
    uint8_t value = calculate_single_source_contamination(60, 50, source);
    assert(value == 0);
    printf("  PASS: At boundary -> intensity = %d (expected 0)\n", value);
}

void test_contamination_beyond_radius() {
    printf("Testing contamination beyond radius...\n");
    PortContaminationSource source = {50, 50, PortType::Aero, 10, 200, true};
    uint8_t value = calculate_single_source_contamination(65, 50, source);
    assert(value == 0);
    printf("  PASS: Beyond radius -> intensity = %d (expected 0)\n", value);
}

void test_contamination_linear_falloff() {
    printf("Testing linear falloff at multiple distances...\n");
    // radius=10, intensity=100
    PortContaminationSource source = {0, 0, PortType::Aqua, 10, 100, true};

    // distance=0 -> 100
    assert(calculate_single_source_contamination(0, 0, source) == 100);
    // distance=2 -> 100 * 0.8 = 80
    assert(calculate_single_source_contamination(2, 0, source) == 80);
    // distance=5 -> 100 * 0.5 = 50
    assert(calculate_single_source_contamination(5, 0, source) == 50);
    // distance=8 -> 100 * 0.2 = 20
    assert(calculate_single_source_contamination(8, 0, source) == 20);
    // distance=9 -> 100 * 0.1 = 10
    assert(calculate_single_source_contamination(9, 0, source) == 10);

    printf("  PASS: Linear falloff verified at distances 0,2,5,8,9\n");
}

void test_contamination_manhattan_distance() {
    printf("Testing Manhattan distance calculation...\n");
    // Port at (10,10), radius=10, intensity=100
    PortContaminationSource source = {10, 10, PortType::Aero, 10, 100, true};

    // (13, 12) -> manhattan = |3| + |2| = 5 -> 100 * (1 - 5/10) = 50
    uint8_t value = calculate_single_source_contamination(13, 12, source);
    assert(value == 50);

    // (10, 17) -> manhattan = 7 -> 100 * (1 - 7/10) = 30
    value = calculate_single_source_contamination(10, 17, source);
    assert(value == 30);

    printf("  PASS: Manhattan distance verified\n");
}

// =============================================================================
// Non-operational Source Tests
// =============================================================================

void test_non_operational_source() {
    printf("Testing non-operational source produces no contamination...\n");
    PortContaminationSource source = {50, 50, PortType::Aero, 10, 200, false};
    uint8_t value = calculate_single_source_contamination(50, 50, source);
    assert(value == 0);
    printf("  PASS: Non-operational at source -> intensity = %d (expected 0)\n", value);
}

// =============================================================================
// Zero Intensity/Radius Edge Cases
// =============================================================================

void test_zero_intensity() {
    printf("Testing zero intensity source...\n");
    PortContaminationSource source = {50, 50, PortType::Aero, 10, 0, true};
    uint8_t value = calculate_single_source_contamination(50, 50, source);
    assert(value == 0);
    printf("  PASS: Zero intensity -> %d\n", value);
}

void test_zero_radius() {
    printf("Testing zero radius source...\n");
    PortContaminationSource source = {50, 50, PortType::Aero, 0, 200, true};
    uint8_t value = calculate_single_source_contamination(50, 50, source);
    assert(value == 0);
    printf("  PASS: Zero radius -> %d\n", value);
}

// =============================================================================
// Multiple Source (Stacking) Tests
// =============================================================================

void test_multiple_sources_stack() {
    printf("Testing multiple sources stack...\n");
    std::vector<PortContaminationSource> sources;
    // Two sources at same location, both at full intensity
    sources.push_back({50, 50, PortType::Aero, 10, 100, true});
    sources.push_back({50, 50, PortType::Aqua, 8, 80, true});

    // Query at source -> 100 + 80 = 180
    uint8_t value = calculate_port_contamination(50, 50, sources);
    assert(value == 180);
    printf("  PASS: Two sources at origin -> %d (expected 180)\n", value);
}

void test_multiple_sources_capped_at_255() {
    printf("Testing multiple sources capped at 255...\n");
    std::vector<PortContaminationSource> sources;
    // Three sources with high intensity
    sources.push_back({50, 50, PortType::Aero, 10, 200, true});
    sources.push_back({50, 50, PortType::Aqua, 8, 200, true});
    sources.push_back({50, 50, PortType::Aero, 10, 200, true});

    // Query at source -> 200 + 200 + 200 = 600, capped at 255
    uint8_t value = calculate_port_contamination(50, 50, sources);
    assert(value == 255);
    printf("  PASS: Three high-intensity sources -> %d (expected 255, capped)\n", value);
}

void test_multiple_sources_different_positions() {
    printf("Testing multiple sources at different positions...\n");
    std::vector<PortContaminationSource> sources;
    sources.push_back({0, 0, PortType::Aero, 10, 100, true});
    sources.push_back({15, 0, PortType::Aero, 10, 100, true});

    // Query at (5, 0): dist from source1 = 5, dist from source2 = 10
    // source1: 100 * (1 - 5/10) = 50
    // source2: 100 * (1 - 10/10) = 0
    uint8_t value = calculate_port_contamination(5, 0, sources);
    assert(value == 50);

    // Query at (10, 0): dist from source1 = 10, dist from source2 = 5
    // source1: 100 * (1 - 10/10) = 0
    // source2: 100 * (1 - 5/10) = 50
    value = calculate_port_contamination(10, 0, sources);
    assert(value == 50);

    // Query at (7, 0): dist from source1 = 7, dist from source2 = 8
    // source1: 100 * (1 - 7/10) = 30
    // source2: 100 * (1 - 8/10) = 20
    value = calculate_port_contamination(7, 0, sources);
    assert(value == 50);

    printf("  PASS: Multiple sources at different positions verified\n");
}

void test_empty_sources() {
    printf("Testing empty sources vector...\n");
    std::vector<PortContaminationSource> empty;
    uint8_t value = calculate_port_contamination(50, 50, empty);
    assert(value == 0);
    printf("  PASS: Empty sources -> %d\n", value);
}

void test_non_operational_excluded_from_total() {
    printf("Testing non-operational sources excluded from total...\n");
    std::vector<PortContaminationSource> sources;
    sources.push_back({50, 50, PortType::Aero, 10, 100, false});  // Not operational
    sources.push_back({50, 50, PortType::Aqua, 8, 80, true});     // Operational

    uint8_t value = calculate_port_contamination(50, 50, sources);
    assert(value == 80);
    printf("  PASS: 1 non-op + 1 op -> %d (expected 80)\n", value);
}

// =============================================================================
// In-Contamination-Zone Tests
// =============================================================================

void test_in_contamination_zone() {
    printf("Testing is_in_contamination_zone...\n");
    std::vector<PortContaminationSource> sources;
    sources.push_back({50, 50, PortType::Aero, 10, 100, true});

    assert(is_in_contamination_zone(50, 50, sources) == true);   // At source
    assert(is_in_contamination_zone(55, 50, sources) == true);   // dist=5
    assert(is_in_contamination_zone(60, 50, sources) == true);   // dist=10 (boundary)
    assert(is_in_contamination_zone(61, 50, sources) == false);  // dist=11 (outside)
    printf("  PASS: Zone boundary checks correct\n");
}

void test_not_in_zone_when_non_operational() {
    printf("Testing not in zone when source is non-operational...\n");
    std::vector<PortContaminationSource> sources;
    sources.push_back({50, 50, PortType::Aero, 10, 100, false});

    assert(is_in_contamination_zone(50, 50, sources) == false);
    printf("  PASS: Non-operational source -> not in zone\n");
}

// =============================================================================
// Contamination Type Name Tests
// =============================================================================

void test_contamination_type_names() {
    printf("Testing contamination type names...\n");
    assert(std::string(contamination_type_name(PortType::Aero)) == "Noise");
    assert(std::string(contamination_type_name(PortType::Aqua)) == "Industrial");
    printf("  PASS: Aero=Noise, Aqua=Industrial\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Noise/Contamination Tests (Epic 8, Ticket E8-033) ===\n\n");

    // Default radius
    test_aero_default_radius();
    test_aqua_default_radius();

    // Single source
    test_contamination_at_source();
    test_contamination_at_half_radius();
    test_contamination_at_boundary();
    test_contamination_beyond_radius();
    test_contamination_linear_falloff();
    test_contamination_manhattan_distance();

    // Non-operational
    test_non_operational_source();

    // Edge cases
    test_zero_intensity();
    test_zero_radius();

    // Multiple sources
    test_multiple_sources_stack();
    test_multiple_sources_capped_at_255();
    test_multiple_sources_different_positions();
    test_empty_sources();
    test_non_operational_excluded_from_total();

    // In-zone checks
    test_in_contamination_zone();
    test_not_in_zone_when_non_operational();

    // Type names
    test_contamination_type_names();

    printf("\n=== All Port Noise/Contamination Tests Passed ===\n");
    return 0;
}
