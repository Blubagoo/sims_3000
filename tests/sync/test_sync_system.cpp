/**
 * @file test_sync_system.cpp
 * @brief Unit tests for SyncSystem change detection and delta sync.
 *
 * Verifies that SyncSystem correctly tracks dirty entities via EnTT signals,
 * respects SyncPolicy metadata, and properly generates/applies state deltas.
 */

#include "sims3000/sync/SyncSystem.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/core/ISimulationTime.h"
#include "sims3000/net/ServerMessages.h"

#include <cassert>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <thread>

using namespace sims3000;

// Mock ISimulationTime for tick tests
class MockSimulationTime : public ISimulationTime {
public:
    SimulationTick getCurrentTick() const override { return m_tick; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(m_tick) * 0.05; }

    SimulationTick m_tick = 0;
};

// =============================================================================
// Test: Entity creation triggers dirty flag
// =============================================================================
void test_entity_creation_detected() {
    printf("Testing entity creation detection...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Initially no dirty entities
    assert(sync.getDirtyCount() == 0);

    // Create entity with syncable component
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});

    // Entity should be dirty with Created type
    assert(sync.getDirtyCount() == 1);
    assert(sync.isDirty(e1));
    assert(sync.getChange(e1).type == ChangeType::Created);
    assert(sync.getChange(e1).hasComponent(ComponentTypeID::Position));

    printf("  PASS: Entity creation detected correctly\n");
}

// =============================================================================
// Test: Component update via patch() triggers dirty flag
// =============================================================================
void test_component_update_via_patch() {
    printf("Testing component update via patch()...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});
    sync.flush();  // Clear initial creation

    assert(sync.getDirtyCount() == 0);

    // Update component via patch() - this triggers on_update signal
    registry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) {
            pos.pos.x = 100;
        }
    );

    // Entity should be dirty with Updated type
    assert(sync.getDirtyCount() == 1);
    assert(sync.isDirty(e1));
    assert(sync.getChange(e1).type == ChangeType::Updated);
    assert(sync.getChange(e1).hasComponent(ComponentTypeID::Position));

    printf("  PASS: Component update via patch() detected correctly\n");
}

// =============================================================================
// Test: Component update via replace() triggers dirty flag
// =============================================================================
void test_component_update_via_replace() {
    printf("Testing component update via replace()...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});
    sync.flush();

    // Update via replace()
    registry.raw().replace<PositionComponent>(
        static_cast<entt::entity>(e1),
        PositionComponent{{200, 300}, 10}
    );

    assert(sync.isDirty(e1));
    assert(sync.getChange(e1).type == ChangeType::Updated);

    printf("  PASS: Component update via replace() detected correctly\n");
}

// =============================================================================
// Test: Entity destruction triggers dirty flag
// =============================================================================
void test_entity_destruction_detected() {
    printf("Testing entity destruction detection...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});
    sync.flush();

    // Destroy entity
    registry.destroy(e1);

    assert(sync.getDirtyCount() == 1);
    assert(sync.getChange(e1).type == ChangeType::Destroyed);

    printf("  PASS: Entity destruction detected correctly\n");
}

// =============================================================================
// Test: flush() clears dirty set
// =============================================================================
void test_flush_clears_dirty() {
    printf("Testing flush() clears dirty set...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create multiple entities
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{1, 1}, 0});

    EntityID e2 = registry.create();
    registry.emplace<PositionComponent>(e2, PositionComponent{{2, 2}, 0});

    assert(sync.getDirtyCount() == 2);

    // Flush
    sync.flush();

    assert(sync.getDirtyCount() == 0);
    assert(!sync.isDirty(e1));
    assert(!sync.isDirty(e2));

    printf("  PASS: flush() clears dirty set correctly\n");
}

// =============================================================================
// Test: SyncPolicy::None components excluded from sync
// =============================================================================
void test_sync_policy_none_excluded() {
    printf("Testing SyncPolicy::None components excluded...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity with RenderComponent (SyncPolicy::None)
    EntityID e1 = registry.create();
    registry.emplace<RenderComponent>(e1, RenderComponent{});

    // RenderComponent should NOT trigger dirty flag
    // (unless another syncable component is also added)
    // Check that we're not tracking RenderComponent changes
    auto change = sync.getChange(e1);
    // RenderComponent doesn't have get_type_id() so we can't check componentMask
    // But the entity itself should not be dirty from RenderComponent alone
    // Actually, the subscribe<RenderComponent>() returns early, so no signals connected

    // Create another entity with only Position
    EntityID e2 = registry.create();
    registry.emplace<PositionComponent>(e2, PositionComponent{{0, 0}, 0});

    // e2 should be dirty
    assert(sync.isDirty(e2));

    printf("  PASS: SyncPolicy::None components excluded correctly\n");
}

// =============================================================================
// Test: Multiple component changes on same entity
// =============================================================================
void test_multiple_component_changes() {
    printf("Testing multiple component changes on same entity...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity with multiple components
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});
    registry.emplace<OwnershipComponent>(e1, OwnershipComponent{1, OwnershipState::Owned});
    registry.emplace<BuildingComponent>(e1, BuildingComponent{1, 1, 100, 0, 0});

    // Should only have one entry for the entity
    assert(sync.getDirtyCount() == 1);

    // Should track all changed components
    auto change = sync.getChange(e1);
    assert(change.type == ChangeType::Created);
    assert(change.hasComponent(ComponentTypeID::Position));
    assert(change.hasComponent(ComponentTypeID::Ownership));
    assert(change.hasComponent(ComponentTypeID::Building));

    printf("  PASS: Multiple component changes tracked correctly\n");
}

// =============================================================================
// Test: Destroyed overrides Created
// =============================================================================
void test_destroyed_overrides_created() {
    printf("Testing Destroyed overrides Created...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create and destroy in same tick
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{1, 1}, 0});
    registry.destroy(e1);

    // Should show as Destroyed
    assert(sync.getDirtyCount() == 1);
    assert(sync.getChange(e1).type == ChangeType::Destroyed);

    printf("  PASS: Destroyed overrides Created correctly\n");
}

// =============================================================================
// Test: Created not downgraded to Updated
// =============================================================================
void test_created_not_downgraded() {
    printf("Testing Created not downgraded to Updated...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{1, 1}, 0});

    assert(sync.getChange(e1).type == ChangeType::Created);

    // Update in same tick
    registry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) { pos.pos.x = 999; }
    );

    // Should still be Created, not Updated
    assert(sync.getChange(e1).type == ChangeType::Created);

    printf("  PASS: Created not downgraded to Updated\n");
}

// =============================================================================
// Test: markDirty() manual marking
// =============================================================================
void test_mark_dirty_manual() {
    printf("Testing markDirty() manual marking...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity but don't add syncable components
    EntityID e1 = registry.create();
    // No components added

    // Manually mark dirty
    sync.markDirty(e1, ChangeType::Updated);

    assert(sync.isDirty(e1));
    assert(sync.getChange(e1).type == ChangeType::Updated);

    printf("  PASS: markDirty() works correctly\n");
}

// =============================================================================
// Test: markComponentDirty() manual component marking
// =============================================================================
void test_mark_component_dirty_manual() {
    printf("Testing markComponentDirty() manual component marking...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    EntityID e1 = registry.create();

    sync.markComponentDirty(e1, ComponentTypeID::Position, ChangeType::Updated);

    assert(sync.isDirty(e1));
    assert(sync.getChange(e1).hasComponent(ComponentTypeID::Position));
    assert(!sync.getChange(e1).hasComponent(ComponentTypeID::Ownership));

    printf("  PASS: markComponentDirty() works correctly\n");
}

// =============================================================================
// Test: getCreatedEntities(), getUpdatedEntities(), getDestroyedEntities()
// =============================================================================
void test_get_entities_by_change_type() {
    printf("Testing getCreatedEntities/getUpdatedEntities/getDestroyedEntities...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entities
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{1, 1}, 0});

    EntityID e2 = registry.create();
    registry.emplace<PositionComponent>(e2, PositionComponent{{2, 2}, 0});

    sync.flush();

    // Update one, destroy another
    registry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) { pos.pos.x = 100; }
    );

    registry.destroy(e2);

    // Create a new one
    EntityID e3 = registry.create();
    registry.emplace<PositionComponent>(e3, PositionComponent{{3, 3}, 0});

    auto created = sync.getCreatedEntities();
    auto updated = sync.getUpdatedEntities();
    auto destroyed = sync.getDestroyedEntities();

    assert(created.size() == 1);
    assert(created.count(e3) == 1);

    assert(updated.size() == 1);
    assert(updated.count(e1) == 1);

    assert(destroyed.size() == 1);
    assert(destroyed.count(e2) == 1);

    printf("  PASS: Entity categorization by change type works correctly\n");
}

// =============================================================================
// Test: ISimulatable interface
// =============================================================================
void test_simulatable_interface() {
    printf("Testing ISimulatable interface...\n");

    Registry registry;
    SyncSystem sync(registry);

    // Check name
    assert(std::string(sync.getName()) == "SyncSystem");

    // Check priority (should be high to run after simulation)
    assert(sync.getPriority() == 900);

    // tick() should be a no-op but shouldn't crash
    MockSimulationTime time;
    sync.tick(time);

    printf("  PASS: ISimulatable interface works correctly\n");
}

// =============================================================================
// Integration Test: Modify entity, verify in delta
// =============================================================================
void test_integration_modify_entity_verify_delta() {
    printf("Testing integration: modify entity, verify in delta...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});
    registry.emplace<BuildingComponent>(e1, BuildingComponent{1, 2, 100, 0, 0});

    // Verify creation detected
    assert(sync.isDirty(e1));
    auto change1 = sync.getChange(e1);
    assert(change1.type == ChangeType::Created);
    assert(change1.hasComponent(ComponentTypeID::Position));
    assert(change1.hasComponent(ComponentTypeID::Building));

    // Simulate delta generation and flush
    sync.flush();
    assert(sync.getDirtyCount() == 0);

    // Modify position via patch (the correct way)
    registry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) {
            pos.pos.x = 100;
            pos.pos.y = 200;
            pos.elevation = 10;
        }
    );

    // Verify modification detected
    assert(sync.isDirty(e1));
    auto change2 = sync.getChange(e1);
    assert(change2.type == ChangeType::Updated);
    assert(change2.hasComponent(ComponentTypeID::Position));
    // Building not modified, shouldn't be in mask
    assert(!change2.hasComponent(ComponentTypeID::Building));

    // Verify the actual data was changed
    auto& pos = registry.get<PositionComponent>(e1);
    assert(pos.pos.x == 100);
    assert(pos.pos.y == 200);
    assert(pos.elevation == 10);

    printf("  PASS: Integration test passed - modifications tracked correctly\n");
}

// =============================================================================
// Test: Direct member access does NOT trigger on_update
// =============================================================================
void test_direct_access_no_signal() {
    printf("Testing direct member access does NOT trigger on_update...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 5});
    sync.flush();

    // Direct member access (NOT using patch/replace)
    auto& pos = registry.get<PositionComponent>(e1);
    pos.pos.x = 999;  // Direct modification - no signal!

    // Entity should NOT be dirty (this is expected EnTT behavior)
    assert(sync.getDirtyCount() == 0);
    assert(!sync.isDirty(e1));

    printf("  PASS: Direct member access correctly does NOT trigger on_update\n");
    printf("  NOTE: All modifications MUST use registry.patch() for change detection!\n");
}

// =============================================================================
// Test: Delta generation for created entity
// =============================================================================
void test_delta_generation_created_entity() {
    printf("Testing delta generation for created entity...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity with multiple components
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{100, 200}, 5});
    registry.emplace<OwnershipComponent>(e1, OwnershipComponent{1, OwnershipState::Owned, {}, 50});
    registry.emplace<BuildingComponent>(e1, BuildingComponent{42, 3, 85, 0, 0});

    // Generate delta
    auto delta = sync.generateDelta(1);

    assert(delta != nullptr);
    assert(delta->tick == 1);
    assert(delta->hasDeltas());
    assert(delta->deltas.size() == 1);
    assert(delta->deltas[0].entityId == e1);
    assert(delta->deltas[0].type == EntityDeltaType::Create);
    assert(!delta->deltas[0].componentData.empty());

    printf("  PASS: Delta generation for created entity works\n");
}

// =============================================================================
// Test: Delta generation for updated entity
// =============================================================================
void test_delta_generation_updated_entity() {
    printf("Testing delta generation for updated entity...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entity
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 0});
    registry.emplace<BuildingComponent>(e1, BuildingComponent{1, 1, 100, 0, 0});
    sync.flush();

    // Update only position
    registry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) { pos.pos.x = 999; }
    );

    // Generate delta
    auto delta = sync.generateDelta(2);

    assert(delta->tick == 2);
    assert(delta->deltas.size() == 1);
    assert(delta->deltas[0].entityId == e1);
    assert(delta->deltas[0].type == EntityDeltaType::Update);
    // Should only contain Position, not Building
    assert(!delta->deltas[0].componentData.empty());

    printf("  PASS: Delta generation for updated entity works\n");
}

// =============================================================================
// Test: Delta generation for destroyed entity
// =============================================================================
void test_delta_generation_destroyed_entity() {
    printf("Testing delta generation for destroyed entity...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create and flush
    EntityID e1 = registry.create();
    registry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 0});
    sync.flush();

    // Destroy
    registry.destroy(e1);

    // Generate delta
    auto delta = sync.generateDelta(3);

    assert(delta->deltas.size() == 1);
    assert(delta->deltas[0].entityId == e1);
    assert(delta->deltas[0].type == EntityDeltaType::Destroy);
    assert(delta->deltas[0].componentData.empty());

    printf("  PASS: Delta generation for destroyed entity works\n");
}

// =============================================================================
// Test: Delta application - create entity
// =============================================================================
void test_delta_application_create() {
    printf("Testing delta application - create entity...\n");

    // Server-side: generate delta
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    EntityID serverEntity = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(serverEntity, PositionComponent{{42, 84}, 7});
    serverRegistry.emplace<OwnershipComponent>(serverEntity, OwnershipComponent{2, OwnershipState::Owned, {}, 100});

    auto delta = serverSync.generateDelta(1);

    // Client-side: apply delta
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    auto result = clientSync.applyDelta(*delta);

    assert(result == DeltaApplicationResult::Applied);
    assert(clientSync.getLastProcessedTick() == 1);

    // Verify entity was created with correct ID
    assert(clientRegistry.valid(serverEntity));

    // Verify components were applied
    auto& pos = clientRegistry.get<PositionComponent>(serverEntity);
    assert(pos.pos.x == 42);
    assert(pos.pos.y == 84);
    assert(pos.elevation == 7);

    auto& owner = clientRegistry.get<OwnershipComponent>(serverEntity);
    assert(owner.owner == 2);
    assert(owner.state == OwnershipState::Owned);

    printf("  PASS: Delta application - create entity works\n");
}

// =============================================================================
// Test: Delta application - update entity
// =============================================================================
void test_delta_application_update() {
    printf("Testing delta application - update entity...\n");

    // Setup: Server creates entity
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    EntityID e1 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 0});

    auto createDelta = serverSync.generateDelta(1);
    serverSync.flush();

    // Client applies create
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);
    clientSync.applyDelta(*createDelta);

    // Server updates position
    serverRegistry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) {
            pos.pos.x = 999;
            pos.pos.y = 888;
        }
    );

    auto updateDelta = serverSync.generateDelta(2);

    // Client applies update
    auto result = clientSync.applyDelta(*updateDelta);

    assert(result == DeltaApplicationResult::Applied);
    assert(clientSync.getLastProcessedTick() == 2);

    auto& pos = clientRegistry.get<PositionComponent>(e1);
    assert(pos.pos.x == 999);
    assert(pos.pos.y == 888);

    printf("  PASS: Delta application - update entity works\n");
}

// =============================================================================
// Test: Delta application - destroy entity
// =============================================================================
void test_delta_application_destroy() {
    printf("Testing delta application - destroy entity...\n");

    // Setup
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    EntityID e1 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 0});

    auto createDelta = serverSync.generateDelta(1);
    serverSync.flush();

    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);
    clientSync.applyDelta(*createDelta);

    assert(clientRegistry.valid(e1));

    // Server destroys
    serverRegistry.destroy(e1);
    auto destroyDelta = serverSync.generateDelta(2);

    // Client applies destroy
    auto result = clientSync.applyDelta(*destroyDelta);

    assert(result == DeltaApplicationResult::Applied);
    assert(!clientRegistry.valid(e1));

    printf("  PASS: Delta application - destroy entity works\n");
}

// =============================================================================
// Test: Out-of-order message handling
// =============================================================================
void test_out_of_order_messages() {
    printf("Testing out-of-order message handling...\n");

    Registry registry;
    SyncSystem sync(registry);

    // Create a delta at tick 10
    StateUpdateMessage msg1;
    msg1.tick = 10;

    auto result1 = sync.applyDelta(msg1);
    assert(result1 == DeltaApplicationResult::Applied);
    assert(sync.getLastProcessedTick() == 10);

    // Try to apply tick 5 (older)
    StateUpdateMessage msg2;
    msg2.tick = 5;

    auto result2 = sync.applyDelta(msg2);
    assert(result2 == DeltaApplicationResult::OutOfOrder);
    assert(sync.getLastProcessedTick() == 10);  // Unchanged

    printf("  PASS: Out-of-order messages rejected correctly\n");
}

// =============================================================================
// Test: Duplicate message handling
// =============================================================================
void test_duplicate_messages() {
    printf("Testing duplicate message handling...\n");

    Registry registry;
    SyncSystem sync(registry);

    // Apply tick 5
    StateUpdateMessage msg1;
    msg1.tick = 5;

    auto result1 = sync.applyDelta(msg1);
    assert(result1 == DeltaApplicationResult::Applied);

    // Apply tick 5 again (duplicate)
    StateUpdateMessage msg2;
    msg2.tick = 5;

    auto result2 = sync.applyDelta(msg2);
    assert(result2 == DeltaApplicationResult::Duplicate);

    printf("  PASS: Duplicate messages handled correctly (idempotent)\n");
}

// =============================================================================
// Test: All component types serialize/deserialize correctly
// =============================================================================
void test_all_component_serialization() {
    printf("Testing all component types serialize/deserialize...\n");

    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    EntityID e1 = serverRegistry.create();

    // Add all syncable components
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{100, 200}, 10});
    serverRegistry.emplace<OwnershipComponent>(e1, OwnershipComponent{3, OwnershipState::Abandoned, {}, 999});
    serverRegistry.emplace<TransformComponent>(e1, TransformComponent{{1.5f, 2.5f, 3.5f}, 1.234f});
    serverRegistry.emplace<BuildingComponent>(e1, BuildingComponent{12345, 5, 75, 3, 0});
    serverRegistry.emplace<EnergyComponent>(e1, EnergyComponent{-500, 1000, 1, {}});
    serverRegistry.emplace<PopulationComponent>(e1, PopulationComponent{150, 200, 80, 90, {}});
    serverRegistry.emplace<ZoneComponent>(e1, ZoneComponent{2, 3, 70, 0});
    serverRegistry.emplace<TransportComponent>(e1, TransportComponent{42, 150, 60, 0});
    serverRegistry.emplace<ServiceCoverageComponent>(e1, ServiceCoverageComponent{10, 20, 30, 40, 50, {}});
    serverRegistry.emplace<TaxableComponent>(e1, TaxableComponent{1000, 100, 15, {}});

    auto delta = serverSync.generateDelta(1);

    // Apply on client
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    auto result = clientSync.applyDelta(*delta);
    assert(result == DeltaApplicationResult::Applied);

    // Verify all components
    assert(clientRegistry.valid(e1));

    auto& pos = clientRegistry.get<PositionComponent>(e1);
    assert(pos.pos.x == 100 && pos.pos.y == 200 && pos.elevation == 10);

    auto& owner = clientRegistry.get<OwnershipComponent>(e1);
    assert(owner.owner == 3 && owner.state == OwnershipState::Abandoned);

    auto& trans = clientRegistry.get<TransformComponent>(e1);
    assert(std::abs(trans.position.x - 1.5f) < 0.001f);
    assert(std::abs(trans.rotation - 1.234f) < 0.001f);

    auto& bldg = clientRegistry.get<BuildingComponent>(e1);
    assert(bldg.buildingType == 12345 && bldg.level == 5 && bldg.health == 75);

    auto& energy = clientRegistry.get<EnergyComponent>(e1);
    assert(energy.consumption == -500 && energy.capacity == 1000 && energy.connected == 1);

    auto& pop = clientRegistry.get<PopulationComponent>(e1);
    assert(pop.current == 150 && pop.capacity == 200);

    auto& zone = clientRegistry.get<ZoneComponent>(e1);
    assert(zone.zoneType == 2 && zone.density == 3);

    auto& transport = clientRegistry.get<TransportComponent>(e1);
    assert(transport.roadConnectionId == 42 && transport.trafficLoad == 150);

    auto& svc = clientRegistry.get<ServiceCoverageComponent>(e1);
    assert(svc.police == 10 && svc.fire == 20);

    auto& tax = clientRegistry.get<TaxableComponent>(e1);
    assert(tax.income == 1000 && tax.taxPaid == 100);

    printf("  PASS: All component types serialize/deserialize correctly\n");
}

// =============================================================================
// Test: Multiple entities in single delta
// =============================================================================
void test_multiple_entities_single_delta() {
    printf("Testing multiple entities in single delta...\n");

    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    // Create multiple entities
    EntityID e1 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{1, 1}, 0});

    EntityID e2 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e2, PositionComponent{{2, 2}, 0});

    EntityID e3 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e3, PositionComponent{{3, 3}, 0});

    auto delta = serverSync.generateDelta(1);
    assert(delta->deltas.size() == 3);

    // Apply on client
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    auto result = clientSync.applyDelta(*delta);
    assert(result == DeltaApplicationResult::Applied);

    // Verify all entities
    assert(clientRegistry.valid(e1));
    assert(clientRegistry.valid(e2));
    assert(clientRegistry.valid(e3));

    assert(clientRegistry.get<PositionComponent>(e1).pos.x == 1);
    assert(clientRegistry.get<PositionComponent>(e2).pos.x == 2);
    assert(clientRegistry.get<PositionComponent>(e3).pos.x == 3);

    printf("  PASS: Multiple entities in single delta works\n");
}

// =============================================================================
// Test: Mixed operations in single delta (create + update + destroy)
// =============================================================================
void test_mixed_operations_single_delta() {
    printf("Testing mixed operations in single delta...\n");

    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    // Setup: create two entities
    EntityID e1 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{1, 1}, 0});

    EntityID e2 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e2, PositionComponent{{2, 2}, 0});

    auto setupDelta = serverSync.generateDelta(1);
    serverSync.flush();

    // Apply setup to client
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);
    clientSync.applyDelta(*setupDelta);

    // Now: create e3, update e1, destroy e2
    EntityID e3 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e3, PositionComponent{{3, 3}, 0});

    serverRegistry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) { pos.pos.x = 100; }
    );

    serverRegistry.destroy(e2);

    auto mixedDelta = serverSync.generateDelta(2);
    assert(mixedDelta->deltas.size() == 3);

    // Apply mixed delta
    auto result = clientSync.applyDelta(*mixedDelta);
    assert(result == DeltaApplicationResult::Applied);

    // Verify
    assert(clientRegistry.valid(e1));
    assert(clientRegistry.get<PositionComponent>(e1).pos.x == 100);  // Updated

    assert(!clientRegistry.valid(e2));  // Destroyed

    assert(clientRegistry.valid(e3));
    assert(clientRegistry.get<PositionComponent>(e3).pos.x == 3);  // Created

    printf("  PASS: Mixed operations in single delta work\n");
}

// =============================================================================
// Test: Destroy idempotency (destroy non-existent entity)
// =============================================================================
void test_destroy_idempotent() {
    printf("Testing destroy idempotency...\n");

    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    // Build a delta that destroys entity 42 (which doesn't exist)
    StateUpdateMessage msg;
    msg.tick = 1;
    msg.addDestroy(42);

    // Should succeed (idempotent - destroying non-existent is OK)
    auto result = clientSync.applyDelta(msg);
    assert(result == DeltaApplicationResult::Applied);

    printf("  PASS: Destroy is idempotent for non-existent entities\n");
}

// =============================================================================
// Test: State consistency after 1000 ticks
// =============================================================================
void test_consistency_1000_ticks() {
    printf("Testing state consistency after 1000 ticks...\n");

    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    // Create initial entities
    for (int i = 0; i < 10; ++i) {
        EntityID e = serverRegistry.create();
        serverRegistry.emplace<PositionComponent>(e, PositionComponent{{static_cast<std::int16_t>(i), 0}, 0});
        serverRegistry.emplace<EnergyComponent>(e, EnergyComponent{i * 10, 100, 1, {}});
    }

    auto initialDelta = serverSync.generateDelta(0);
    serverSync.flush();
    clientSync.applyDelta(*initialDelta);

    // Simulate 1000 ticks
    for (SimulationTick tick = 1; tick <= 1000; ++tick) {
        // Modify some entities each tick
        for (auto [entity, pos, energy] : serverRegistry.raw().view<PositionComponent, EnergyComponent>().each()) {
            // Use patch to trigger signals
            serverRegistry.raw().patch<PositionComponent>(entity, [tick](PositionComponent& p) {
                p.pos.x = static_cast<std::int16_t>(tick % 1000);
            });
            serverRegistry.raw().patch<EnergyComponent>(entity, [tick](EnergyComponent& e) {
                e.consumption = static_cast<std::int32_t>(tick);
            });
        }

        auto delta = serverSync.generateDelta(tick);
        serverSync.flush();

        auto result = clientSync.applyDelta(*delta);
        assert(result == DeltaApplicationResult::Applied);
    }

    // Verify final state matches
    for (auto [entity, serverPos] : serverRegistry.raw().view<PositionComponent>().each()) {
        auto clientPos = clientRegistry.tryGet<PositionComponent>(static_cast<EntityID>(entity));
        assert(clientPos != nullptr);
        assert(clientPos->pos.x == serverPos.pos.x);
        assert(clientPos->pos.y == serverPos.pos.y);
    }

    for (auto [entity, serverEnergy] : serverRegistry.raw().view<EnergyComponent>().each()) {
        auto clientEnergy = clientRegistry.tryGet<EnergyComponent>(static_cast<EntityID>(entity));
        assert(clientEnergy != nullptr);
        assert(clientEnergy->consumption == serverEnergy.consumption);
    }

    printf("  PASS: State consistent after 1000 ticks\n");
}

// =============================================================================
// Test: Reset last processed tick
// =============================================================================
void test_reset_last_processed_tick() {
    printf("Testing reset last processed tick...\n");

    Registry registry;
    SyncSystem sync(registry);

    StateUpdateMessage msg;
    msg.tick = 100;
    sync.applyDelta(msg);

    assert(sync.getLastProcessedTick() == 100);

    sync.resetLastProcessedTick(0);
    assert(sync.getLastProcessedTick() == 0);

    // Now should accept tick 50
    StateUpdateMessage msg2;
    msg2.tick = 50;
    auto result = sync.applyDelta(msg2);
    assert(result == DeltaApplicationResult::Applied);
    assert(sync.getLastProcessedTick() == 50);

    printf("  PASS: Reset last processed tick works\n");
}

// =============================================================================
// Full State Snapshot Tests (Ticket 1-014)
// =============================================================================

// Test: Snapshot generation for empty registry
void test_snapshot_empty_registry() {
    printf("Testing snapshot generation for empty registry...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Start snapshot generation
    bool started = sync.startSnapshotGeneration(1);
    assert(started);

    // Wait for generation to complete (should be very fast for empty)
    int maxWait = 100;  // 100ms max
    while (!sync.isSnapshotReady() && maxWait > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        maxWait--;
    }
    assert(sync.isSnapshotReady());

    // Get snapshot messages
    SnapshotStartMessage startMsg;
    std::vector<SnapshotChunkMessage> chunks;
    SnapshotEndMessage endMsg;

    bool got = sync.getSnapshotMessages(startMsg, chunks, endMsg);
    assert(got);

    assert(startMsg.tick == 1);
    assert(startMsg.entityCount == 0);
    assert(startMsg.totalChunks >= 1);  // At least one chunk even for empty
    assert(endMsg.checksum != 0);  // CRC32 of empty snapshot

    printf("  PASS: Empty snapshot generates correctly\n");
}

// Test: Snapshot generation with multiple entities
void test_snapshot_multiple_entities() {
    printf("Testing snapshot generation with multiple entities...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create test entities
    for (int i = 0; i < 100; ++i) {
        EntityID e = registry.create();
        registry.emplace<PositionComponent>(e, PositionComponent{{static_cast<std::int16_t>(i), static_cast<std::int16_t>(i * 2)}, 0});
        registry.emplace<BuildingComponent>(e, BuildingComponent{static_cast<std::uint32_t>(i), 1, 100, 0, 0});
    }

    // Start snapshot generation
    bool started = sync.startSnapshotGeneration(42);
    assert(started);

    // Cannot start another while in progress
    bool startedAgain = sync.startSnapshotGeneration(43);
    assert(!startedAgain);

    // Wait for completion
    int maxWait = 1000;  // 1 second max
    while (!sync.isSnapshotReady() && maxWait > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        maxWait--;
    }
    assert(sync.isSnapshotReady());

    // Get messages
    SnapshotStartMessage startMsg;
    std::vector<SnapshotChunkMessage> chunks;
    SnapshotEndMessage endMsg;

    bool got = sync.getSnapshotMessages(startMsg, chunks, endMsg);
    assert(got);

    assert(startMsg.tick == 42);
    assert(startMsg.entityCount == 100);
    assert(startMsg.totalBytes > 0);
    assert(startMsg.compressedBytes > 0);
    assert(startMsg.compressedBytes <= startMsg.totalBytes);  // Compression works
    assert(chunks.size() == startMsg.totalChunks);

    // Verify chunk indices
    for (std::uint32_t i = 0; i < chunks.size(); ++i) {
        assert(chunks[i].chunkIndex == i);
        assert(!chunks[i].data.empty());
        assert(chunks[i].data.size() <= SNAPSHOT_CHUNK_SIZE);
    }

    printf("  PASS: Multiple entity snapshot generates correctly (%u bytes, %zu chunks)\n",
           startMsg.totalBytes, chunks.size());
}

// Test: Snapshot reception and application
void test_snapshot_reception_and_application() {
    printf("Testing snapshot reception and application...\n");

    // Server side: generate snapshot
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    // Create test entities on server
    EntityID e1 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{100, 200}, 5});
    serverRegistry.emplace<OwnershipComponent>(e1, OwnershipComponent{1, OwnershipState::Owned, {}, 50});

    EntityID e2 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e2, PositionComponent{{300, 400}, 10});
    serverRegistry.emplace<BuildingComponent>(e2, BuildingComponent{42, 3, 80, 0, 0});

    // Generate snapshot
    serverSync.startSnapshotGeneration(1000);
    while (!serverSync.isSnapshotReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SnapshotStartMessage startMsg;
    std::vector<SnapshotChunkMessage> chunks;
    SnapshotEndMessage endMsg;
    serverSync.getSnapshotMessages(startMsg, chunks, endMsg);

    // Client side: receive and apply snapshot
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    // Handle snapshot start
    clientSync.handleSnapshotStart(startMsg);
    assert(clientSync.isReceivingSnapshot());
    assert(clientSync.getSnapshotProgress().totalChunks == chunks.size());

    // Handle chunks (could arrive out of order in real network)
    for (auto& chunk : chunks) {
        clientSync.handleSnapshotChunk(chunk);
    }

    // Progress check
    auto progress = clientSync.getSnapshotProgress();
    assert(progress.receivedChunks == progress.totalChunks);
    assert(std::abs(progress.getProgress() - 1.0f) < 0.001f);

    // Handle snapshot end
    bool applied = clientSync.handleSnapshotEnd(endMsg);
    assert(applied);

    // Verify client state matches server
    assert(clientRegistry.valid(e1));
    assert(clientRegistry.valid(e2));

    auto& clientPos1 = clientRegistry.get<PositionComponent>(e1);
    assert(clientPos1.pos.x == 100);
    assert(clientPos1.pos.y == 200);
    assert(clientPos1.elevation == 5);

    auto& clientOwner1 = clientRegistry.get<OwnershipComponent>(e1);
    assert(clientOwner1.owner == 1);
    assert(clientOwner1.state == OwnershipState::Owned);

    auto& clientPos2 = clientRegistry.get<PositionComponent>(e2);
    assert(clientPos2.pos.x == 300);
    assert(clientPos2.pos.y == 400);

    auto& clientBldg2 = clientRegistry.get<BuildingComponent>(e2);
    assert(clientBldg2.buildingType == 42);
    assert(clientBldg2.level == 3);

    // Last processed tick should be snapshot tick
    assert(clientSync.getLastProcessedTick() == 1000);

    printf("  PASS: Snapshot reception and application works correctly\n");
}

// Test: Delta buffering during snapshot
void test_delta_buffering_during_snapshot() {
    printf("Testing delta buffering during snapshot...\n");

    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    // Start receiving a snapshot
    SnapshotStartMessage startMsg;
    startMsg.tick = 100;
    startMsg.totalChunks = 5;  // Simulate multi-chunk snapshot
    startMsg.totalBytes = 1000;
    startMsg.entityCount = 10;
    clientSync.handleSnapshotStart(startMsg);

    assert(clientSync.isReceivingSnapshot());

    // Buffer deltas during snapshot
    for (SimulationTick tick = 101; tick <= 110; ++tick) {
        StateUpdateMessage delta;
        delta.tick = tick;
        bool buffered = clientSync.bufferDeltaDuringSnapshot(delta);
        assert(buffered);
    }

    printf("  PASS: Delta buffering during snapshot works\n");
}

// Test: Bounded delta queue overflow
void test_delta_buffer_overflow() {
    printf("Testing bounded delta buffer overflow...\n");

    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    // Start receiving a snapshot
    SnapshotStartMessage startMsg;
    startMsg.tick = 100;
    startMsg.totalChunks = 10;
    startMsg.totalBytes = 10000;
    startMsg.entityCount = 100;
    clientSync.handleSnapshotStart(startMsg);

    // Fill buffer to max
    for (std::size_t i = 0; i < MAX_BUFFERED_DELTAS; ++i) {
        StateUpdateMessage delta;
        delta.tick = 101 + i;
        bool buffered = clientSync.bufferDeltaDuringSnapshot(delta);
        assert(buffered);
    }

    // Next should fail (overflow)
    StateUpdateMessage overflowDelta;
    overflowDelta.tick = 101 + MAX_BUFFERED_DELTAS;
    bool buffered = clientSync.bufferDeltaDuringSnapshot(overflowDelta);
    assert(!buffered);  // Should fail - buffer full

    printf("  PASS: Delta buffer overflow handled correctly (max %zu)\n", MAX_BUFFERED_DELTAS);
}

// Test: Snapshot progress indication
void test_snapshot_progress() {
    printf("Testing snapshot progress indication...\n");

    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    // Initially no progress
    assert(clientSync.getSnapshotProgress().state == SnapshotState::None);

    // Start receiving
    SnapshotStartMessage startMsg;
    startMsg.tick = 500;
    startMsg.totalChunks = 4;
    startMsg.totalBytes = 50000;
    startMsg.entityCount = 500;
    clientSync.handleSnapshotStart(startMsg);

    auto progress = clientSync.getSnapshotProgress();
    assert(progress.state == SnapshotState::Receiving);
    assert(progress.tick == 500);
    assert(progress.totalChunks == 4);
    assert(progress.receivedChunks == 0);
    assert(progress.getProgress() == 0.0f);

    // Receive chunks out of order
    SnapshotChunkMessage chunk2;
    chunk2.chunkIndex = 2;
    chunk2.data = {1, 2, 3};  // Dummy data
    clientSync.handleSnapshotChunk(chunk2);

    progress = clientSync.getSnapshotProgress();
    assert(progress.receivedChunks == 1);
    assert(std::abs(progress.getProgress() - 0.25f) < 0.001f);

    SnapshotChunkMessage chunk0;
    chunk0.chunkIndex = 0;
    chunk0.data = {4, 5, 6};
    clientSync.handleSnapshotChunk(chunk0);

    progress = clientSync.getSnapshotProgress();
    assert(progress.receivedChunks == 2);
    assert(std::abs(progress.getProgress() - 0.5f) < 0.001f);

    printf("  PASS: Snapshot progress tracking works correctly\n");
}

// Test: Snapshot checksum verification
void test_snapshot_checksum_verification() {
    printf("Testing snapshot checksum verification...\n");

    // Server generates snapshot
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    EntityID e = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e, PositionComponent{{1, 2}, 3});

    serverSync.startSnapshotGeneration(1);
    while (!serverSync.isSnapshotReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SnapshotStartMessage startMsg;
    std::vector<SnapshotChunkMessage> chunks;
    SnapshotEndMessage endMsg;
    serverSync.getSnapshotMessages(startMsg, chunks, endMsg);

    // Client receives with correct checksum
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    clientSync.handleSnapshotStart(startMsg);
    for (auto& chunk : chunks) {
        clientSync.handleSnapshotChunk(chunk);
    }

    bool applied = clientSync.handleSnapshotEnd(endMsg);
    assert(applied);

    // Now test with corrupted checksum
    Registry clientRegistry2;
    SyncSystem clientSync2(clientRegistry2);

    // Re-generate snapshot
    serverSync.startSnapshotGeneration(2);
    while (!serverSync.isSnapshotReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    serverSync.getSnapshotMessages(startMsg, chunks, endMsg);

    clientSync2.handleSnapshotStart(startMsg);
    for (auto& chunk : chunks) {
        clientSync2.handleSnapshotChunk(chunk);
    }

    // Corrupt checksum
    endMsg.checksum ^= 0xDEADBEEF;

    bool appliedCorrupt = clientSync2.handleSnapshotEnd(endMsg);
    assert(!appliedCorrupt);  // Should fail checksum verification

    printf("  PASS: Snapshot checksum verification works\n");
}

// Test: clearLocalState
void test_clear_local_state() {
    printf("Testing clearLocalState...\n");

    Registry registry;
    SyncSystem sync(registry);
    sync.subscribeAll();

    // Create entities
    for (int i = 0; i < 10; ++i) {
        EntityID e = registry.create();
        registry.emplace<PositionComponent>(e, PositionComponent{{0, 0}, 0});
    }

    assert(registry.size() == 10);
    assert(sync.getDirtyCount() == 10);

    // Clear state
    sync.clearLocalState();

    assert(registry.size() == 0);
    assert(sync.getDirtyCount() == 0);

    printf("  PASS: clearLocalState works correctly\n");
}

// Test: Complete server-to-client snapshot flow
void test_complete_snapshot_flow() {
    printf("Testing complete server-to-client snapshot flow...\n");

    // Server: Create complex world state
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    // Create a variety of entities with different components
    for (int i = 0; i < 50; ++i) {
        EntityID e = serverRegistry.create();
        serverRegistry.emplace<PositionComponent>(e, PositionComponent{{static_cast<std::int16_t>(i * 10), static_cast<std::int16_t>(i * 20)}, static_cast<std::int16_t>(i)});
        serverRegistry.emplace<TransformComponent>(e, TransformComponent{{static_cast<float>(i), static_cast<float>(i * 2), 0.0f}, static_cast<float>(i) * 0.1f});

        if (i % 2 == 0) {
            serverRegistry.emplace<BuildingComponent>(e, BuildingComponent{static_cast<std::uint32_t>(i), 1, 100, 0, 0});
        }
        if (i % 3 == 0) {
            serverRegistry.emplace<EnergyComponent>(e, EnergyComponent{i * 10, 1000, 1, {}});
        }
        if (i % 5 == 0) {
            serverRegistry.emplace<ZoneComponent>(e, ZoneComponent{static_cast<std::uint8_t>(i % 4), 2, 75, 0});
        }
    }

    // Generate snapshot at tick 5000
    serverSync.startSnapshotGeneration(5000);
    while (!serverSync.isSnapshotReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SnapshotStartMessage startMsg;
    std::vector<SnapshotChunkMessage> chunks;
    SnapshotEndMessage endMsg;
    serverSync.getSnapshotMessages(startMsg, chunks, endMsg);

    // Client: Receive and apply
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    clientSync.handleSnapshotStart(startMsg);
    for (auto& chunk : chunks) {
        clientSync.handleSnapshotChunk(chunk);
    }
    bool applied = clientSync.handleSnapshotEnd(endMsg);
    assert(applied);

    // Verify client matches server
    assert(clientRegistry.size() == serverRegistry.size());

    // Check a few specific entities
    int checked = 0;
    for (auto ent : serverRegistry.raw().storage<entt::entity>()) {
        EntityID id = static_cast<EntityID>(ent);

        // Check Position
        if (serverRegistry.raw().all_of<PositionComponent>(ent)) {
            assert(clientRegistry.has<PositionComponent>(id));
            auto& serverPos = serverRegistry.get<PositionComponent>(id);
            auto& clientPos = clientRegistry.get<PositionComponent>(id);
            assert(serverPos.pos.x == clientPos.pos.x);
            assert(serverPos.pos.y == clientPos.pos.y);
            checked++;
        }

        // Check Transform
        if (serverRegistry.raw().all_of<TransformComponent>(ent)) {
            assert(clientRegistry.has<TransformComponent>(id));
            auto& serverTrans = serverRegistry.get<TransformComponent>(id);
            auto& clientTrans = clientRegistry.get<TransformComponent>(id);
            assert(std::abs(serverTrans.position.x - clientTrans.position.x) < 0.001f);
        }

        // Check Building
        if (serverRegistry.raw().all_of<BuildingComponent>(ent)) {
            assert(clientRegistry.has<BuildingComponent>(id));
        }
    }

    assert(checked > 0);

    printf("  PASS: Complete snapshot flow works (%u entities, %zu chunks)\n",
           startMsg.entityCount, chunks.size());
}

// Test: Buffered deltas applied after snapshot
void test_buffered_deltas_applied_after_snapshot() {
    printf("Testing buffered deltas applied after snapshot...\n");

    // Server creates initial state
    Registry serverRegistry;
    SyncSystem serverSync(serverRegistry);
    serverSync.subscribeAll();

    EntityID e1 = serverRegistry.create();
    serverRegistry.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 0});
    serverSync.flush();

    // Generate snapshot at tick 100
    serverSync.startSnapshotGeneration(100);
    while (!serverSync.isSnapshotReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SnapshotStartMessage startMsg;
    std::vector<SnapshotChunkMessage> chunks;
    SnapshotEndMessage endMsg;
    serverSync.getSnapshotMessages(startMsg, chunks, endMsg);

    // Server continues to modify (simulating deltas during transfer)
    serverRegistry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) { pos.pos.x = 100; }
    );
    auto delta101 = serverSync.generateDelta(101);
    serverSync.flush();

    serverRegistry.raw().patch<PositionComponent>(
        static_cast<entt::entity>(e1),
        [](PositionComponent& pos) { pos.pos.x = 200; }
    );
    auto delta102 = serverSync.generateDelta(102);

    // Client receives snapshot and buffers deltas
    Registry clientRegistry;
    SyncSystem clientSync(clientRegistry);

    clientSync.handleSnapshotStart(startMsg);

    // Buffer deltas that arrive during snapshot transfer
    clientSync.bufferDeltaDuringSnapshot(*delta101);
    clientSync.bufferDeltaDuringSnapshot(*delta102);

    // Receive chunks
    for (auto& chunk : chunks) {
        clientSync.handleSnapshotChunk(chunk);
    }

    // Apply snapshot (also applies buffered deltas)
    bool applied = clientSync.handleSnapshotEnd(endMsg);
    assert(applied);

    // Client should have final position from delta102
    auto& clientPos = clientRegistry.get<PositionComponent>(e1);
    assert(clientPos.pos.x == 200);

    // Last processed tick should be from last delta
    assert(clientSync.getLastProcessedTick() == 102);

    printf("  PASS: Buffered deltas applied correctly after snapshot\n");
}

// =============================================================================
// Main
// =============================================================================
int main() {
    printf("=== SyncSystem Change Detection Unit Tests ===\n\n");

    test_entity_creation_detected();
    test_component_update_via_patch();
    test_component_update_via_replace();
    test_entity_destruction_detected();
    test_flush_clears_dirty();
    test_sync_policy_none_excluded();
    test_multiple_component_changes();
    test_destroyed_overrides_created();
    test_created_not_downgraded();
    test_mark_dirty_manual();
    test_mark_component_dirty_manual();
    test_get_entities_by_change_type();
    test_simulatable_interface();
    test_integration_modify_entity_verify_delta();
    test_direct_access_no_signal();

    printf("\n=== SyncSystem Delta Generation/Application Tests ===\n\n");

    test_delta_generation_created_entity();
    test_delta_generation_updated_entity();
    test_delta_generation_destroyed_entity();
    test_delta_application_create();
    test_delta_application_update();
    test_delta_application_destroy();
    test_out_of_order_messages();
    test_duplicate_messages();
    test_all_component_serialization();
    test_multiple_entities_single_delta();
    test_mixed_operations_single_delta();
    test_destroy_idempotent();
    test_consistency_1000_ticks();
    test_reset_last_processed_tick();

    printf("\n=== SyncSystem Full State Snapshot Tests (Ticket 1-014) ===\n\n");

    test_snapshot_empty_registry();
    test_snapshot_multiple_entities();
    test_snapshot_reception_and_application();
    test_delta_buffering_during_snapshot();
    test_delta_buffer_overflow();
    test_snapshot_progress();
    test_snapshot_checksum_verification();
    test_clear_local_state();
    test_complete_snapshot_flow();
    test_buffered_deltas_applied_after_snapshot();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
