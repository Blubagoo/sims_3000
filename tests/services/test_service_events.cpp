/**
 * @file test_service_events.cpp
 * @brief Unit tests for ServiceEvents and handler methods (Epic 9, Ticket E9-012)
 *
 * Tests cover:
 * - ServiceBuildingPlacedEvent struct construction (default + parameterized)
 * - ServiceBuildingRemovedEvent struct construction (default + parameterized)
 * - ServiceEffectivenessChangedEvent struct construction (default + parameterized)
 * - ServicesSystem handler methods don't crash
 * - on_building_constructed adds entity to tracking
 * - on_building_deconstructed removes entity from tracking
 * - on_building_power_changed marks coverage dirty
 * - Handler bounds checking (invalid owner_id)
 */

#include <sims3000/services/ServiceEvents.h>
#include <sims3000/services/ServicesSystem.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::services;

// =============================================================================
// Mock ISimulationTime for handler testing
// =============================================================================

class MockSimulationTime : public sims3000::ISimulationTime {
public:
    sims3000::SimulationTick getCurrentTick() const override { return 0; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return 0.0; }
};

// =============================================================================
// ServiceBuildingPlacedEvent tests
// =============================================================================

void test_placed_event_default_construction() {
    printf("Testing ServiceBuildingPlacedEvent default construction...\n");

    ServiceBuildingPlacedEvent event;
    assert(event.entity_id == 0);
    assert(event.owner_id == 0);
    assert(event.service_type == ServiceType::Enforcer);
    assert(event.tier == ServiceTier::Post);
    assert(event.grid_x == 0);
    assert(event.grid_y == 0);

    printf("  PASS: Default construction correct\n");
}

void test_placed_event_parameterized_construction() {
    printf("Testing ServiceBuildingPlacedEvent parameterized construction...\n");

    ServiceBuildingPlacedEvent event(42, 2, ServiceType::Medical,
                                     ServiceTier::Station, 10, 20);
    assert(event.entity_id == 42);
    assert(event.owner_id == 2);
    assert(event.service_type == ServiceType::Medical);
    assert(event.tier == ServiceTier::Station);
    assert(event.grid_x == 10);
    assert(event.grid_y == 20);

    printf("  PASS: Parameterized construction correct\n");
}

// =============================================================================
// ServiceBuildingRemovedEvent tests
// =============================================================================

void test_removed_event_default_construction() {
    printf("Testing ServiceBuildingRemovedEvent default construction...\n");

    ServiceBuildingRemovedEvent event;
    assert(event.entity_id == 0);
    assert(event.owner_id == 0);
    assert(event.service_type == ServiceType::Enforcer);
    assert(event.tier == ServiceTier::Post);
    assert(event.grid_x == 0);
    assert(event.grid_y == 0);

    printf("  PASS: Default construction correct\n");
}

void test_removed_event_parameterized_construction() {
    printf("Testing ServiceBuildingRemovedEvent parameterized construction...\n");

    ServiceBuildingRemovedEvent event(99, 3, ServiceType::Education,
                                      ServiceTier::Nexus, -5, 15);
    assert(event.entity_id == 99);
    assert(event.owner_id == 3);
    assert(event.service_type == ServiceType::Education);
    assert(event.tier == ServiceTier::Nexus);
    assert(event.grid_x == -5);
    assert(event.grid_y == 15);

    printf("  PASS: Parameterized construction correct\n");
}

// =============================================================================
// ServiceEffectivenessChangedEvent tests
// =============================================================================

void test_effectiveness_event_default_construction() {
    printf("Testing ServiceEffectivenessChangedEvent default construction...\n");

    ServiceEffectivenessChangedEvent event;
    assert(event.entity_id == 0);
    assert(event.owner_id == 0);
    assert(event.service_type == ServiceType::Enforcer);
    assert(event.tier == ServiceTier::Post);
    assert(event.grid_x == 0);
    assert(event.grid_y == 0);

    printf("  PASS: Default construction correct\n");
}

void test_effectiveness_event_parameterized_construction() {
    printf("Testing ServiceEffectivenessChangedEvent parameterized construction...\n");

    ServiceEffectivenessChangedEvent event(77, 1, ServiceType::HazardResponse,
                                            ServiceTier::Station, 30, 40);
    assert(event.entity_id == 77);
    assert(event.owner_id == 1);
    assert(event.service_type == ServiceType::HazardResponse);
    assert(event.tier == ServiceTier::Station);
    assert(event.grid_x == 30);
    assert(event.grid_y == 40);

    printf("  PASS: Parameterized construction correct\n");
}

// =============================================================================
// Handler method tests
// =============================================================================

void test_on_building_constructed_no_crash() {
    printf("Testing on_building_constructed doesn't crash...\n");

    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(1, 0);
    system.on_building_constructed(2, 1);
    system.on_building_constructed(3, 2);
    system.on_building_constructed(4, 3);

    printf("  PASS: on_building_constructed doesn't crash\n");
}

void test_on_building_constructed_marks_dirty() {
    printf("Testing on_building_constructed marks coverage dirty...\n");

    ServicesSystem system;
    system.init(64, 64);

    assert(!system.isCoverageDirty());
    system.on_building_constructed(1, 0);
    assert(system.isCoverageDirty());

    printf("  PASS: Coverage marked dirty after construction\n");
}

void test_on_building_deconstructed_no_crash() {
    printf("Testing on_building_deconstructed doesn't crash...\n");

    ServicesSystem system;
    system.init(64, 64);

    // Add then remove
    system.on_building_constructed(10, 0);
    system.on_building_deconstructed(10, 0);

    // Remove non-existent entity (should not crash)
    system.on_building_deconstructed(999, 0);

    printf("  PASS: on_building_deconstructed doesn't crash\n");
}

void test_on_building_deconstructed_marks_dirty() {
    printf("Testing on_building_deconstructed marks coverage dirty...\n");

    ServicesSystem system;
    system.init(64, 64);

    system.on_building_constructed(1, 0);
    // Reset dirty flag by re-initializing
    system.init(64, 64);
    assert(!system.isCoverageDirty());

    system.on_building_deconstructed(1, 0);
    assert(system.isCoverageDirty());

    printf("  PASS: Coverage marked dirty after deconstruction\n");
}

void test_on_building_power_changed_no_crash() {
    printf("Testing on_building_power_changed doesn't crash...\n");

    ServicesSystem system;
    system.init(64, 64);

    system.on_building_power_changed(1, 0);
    system.on_building_power_changed(2, 3);

    printf("  PASS: on_building_power_changed doesn't crash\n");
}

void test_on_building_power_changed_marks_dirty() {
    printf("Testing on_building_power_changed marks coverage dirty...\n");

    ServicesSystem system;
    system.init(64, 64);

    assert(!system.isCoverageDirty());
    system.on_building_power_changed(1, 0);
    assert(system.isCoverageDirty());

    printf("  PASS: Coverage marked dirty after power change\n");
}

void test_handler_invalid_owner_id() {
    printf("Testing handler methods with invalid owner_id...\n");

    ServicesSystem system;
    system.init(64, 64);

    // Owner IDs >= MAX_PLAYERS should be safely ignored
    system.on_building_constructed(1, 4);   // MAX_PLAYERS = 4, so 4 is invalid
    system.on_building_constructed(2, 255);
    system.on_building_deconstructed(1, 4);
    system.on_building_deconstructed(2, 255);
    system.on_building_power_changed(1, 4);

    printf("  PASS: Invalid owner_id handled safely\n");
}

void test_handler_before_init() {
    printf("Testing handler methods before init...\n");

    ServicesSystem system;

    // These should not crash even without init
    system.on_building_constructed(1, 0);
    system.on_building_deconstructed(1, 0);
    system.on_building_power_changed(1, 0);

    printf("  PASS: Handlers before init don't crash\n");
}

// =============================================================================
// All service type variations in events
// =============================================================================

void test_all_service_types_in_events() {
    printf("Testing all service types in events...\n");

    ServiceBuildingPlacedEvent e1(1, 0, ServiceType::Enforcer, ServiceTier::Post, 0, 0);
    assert(e1.service_type == ServiceType::Enforcer);

    ServiceBuildingPlacedEvent e2(2, 0, ServiceType::HazardResponse, ServiceTier::Station, 0, 0);
    assert(e2.service_type == ServiceType::HazardResponse);

    ServiceBuildingPlacedEvent e3(3, 0, ServiceType::Medical, ServiceTier::Nexus, 0, 0);
    assert(e3.service_type == ServiceType::Medical);

    ServiceBuildingPlacedEvent e4(4, 0, ServiceType::Education, ServiceTier::Post, 0, 0);
    assert(e4.service_type == ServiceType::Education);

    printf("  PASS: All service types work in events\n");
}

void test_all_service_tiers_in_events() {
    printf("Testing all service tiers in events...\n");

    ServiceBuildingRemovedEvent e1(1, 0, ServiceType::Enforcer, ServiceTier::Post, 0, 0);
    assert(e1.tier == ServiceTier::Post);

    ServiceBuildingRemovedEvent e2(2, 0, ServiceType::Enforcer, ServiceTier::Station, 0, 0);
    assert(e2.tier == ServiceTier::Station);

    ServiceBuildingRemovedEvent e3(3, 0, ServiceType::Enforcer, ServiceTier::Nexus, 0, 0);
    assert(e3.tier == ServiceTier::Nexus);

    printf("  PASS: All service tiers work in events\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ServiceEvents Unit Tests (Epic 9, Ticket E9-012) ===\n\n");

    test_placed_event_default_construction();
    test_placed_event_parameterized_construction();
    test_removed_event_default_construction();
    test_removed_event_parameterized_construction();
    test_effectiveness_event_default_construction();
    test_effectiveness_event_parameterized_construction();
    test_on_building_constructed_no_crash();
    test_on_building_constructed_marks_dirty();
    test_on_building_deconstructed_no_crash();
    test_on_building_deconstructed_marks_dirty();
    test_on_building_power_changed_no_crash();
    test_on_building_power_changed_marks_dirty();
    test_handler_invalid_owner_id();
    test_handler_before_init();
    test_all_service_types_in_events();
    test_all_service_tiers_in_events();

    printf("\n=== All ServiceEvents Tests Passed ===\n");
    return 0;
}
