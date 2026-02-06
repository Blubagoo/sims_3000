/**
 * @file test_entity_id_generator.cpp
 * @brief Unit tests for EntityIdGenerator.
 *
 * Tests for ticket 1-015: Entity ID Synchronization
 * - Monotonic counter starting at 1
 * - ID 0 reserved for null/invalid
 * - No ID reuse during session
 * - Persistence save/restore
 * - Client-side entity creation with server IDs
 */

#include "sims3000/sync/EntityIdGenerator.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/Components.h"

#include <cassert>
#include <cstdio>
#include <vector>
#include <unordered_set>

using namespace sims3000;

// =============================================================================
// Test: Counter starts at 1
// =============================================================================
void test_counter_starts_at_one() {
    printf("Testing counter starts at 1...\n");

    EntityIdGenerator gen;

    // First ID should be 1
    EntityID first = gen.next();
    assert(first == 1);

    printf("  PASS: Counter starts at 1\n");
}

// =============================================================================
// Test: ID 0 is reserved for null/invalid
// =============================================================================
void test_id_zero_reserved() {
    printf("Testing ID 0 is reserved for null/invalid...\n");

    // NULL_ENTITY_ID should be 0
    assert(NULL_ENTITY_ID == 0);

    // isValid should return false for 0
    assert(!EntityIdGenerator::isValid(NULL_ENTITY_ID));
    assert(!EntityIdGenerator::isValid(0));

    // isValid should return true for any non-zero ID
    assert(EntityIdGenerator::isValid(1));
    assert(EntityIdGenerator::isValid(100));
    assert(EntityIdGenerator::isValid(0xFFFFFFFF));

    printf("  PASS: ID 0 is reserved for null/invalid\n");
}

// =============================================================================
// Test: Monotonic counter increments correctly
// =============================================================================
void test_monotonic_increment() {
    printf("Testing monotonic counter increments...\n");

    EntityIdGenerator gen;

    EntityID prev = gen.next();
    assert(prev == 1);

    for (int i = 0; i < 1000; ++i) {
        EntityID current = gen.next();
        assert(current == prev + 1);
        prev = current;
    }

    assert(prev == 1001);

    printf("  PASS: Monotonic counter increments correctly\n");
}

// =============================================================================
// Test: IDs never reused during session (no recycling)
// =============================================================================
void test_no_id_reuse() {
    printf("Testing IDs never reused during session...\n");

    EntityIdGenerator gen;
    std::unordered_set<EntityID> usedIds;

    // Generate 10000 IDs and verify none are duplicates
    for (int i = 0; i < 10000; ++i) {
        EntityID id = gen.next();
        assert(usedIds.find(id) == usedIds.end());
        usedIds.insert(id);
    }

    assert(usedIds.size() == 10000);

    printf("  PASS: No ID reuse during session\n");
}

// =============================================================================
// Test: getNextId returns next ID without consuming it
// =============================================================================
void test_get_next_id() {
    printf("Testing getNextId returns next ID without consuming...\n");

    EntityIdGenerator gen;

    // Initial state
    assert(gen.getNextId() == 1);

    // Generate some IDs
    gen.next();  // 1
    gen.next();  // 2
    gen.next();  // 3

    // getNextId should return 4
    assert(gen.getNextId() == 4);

    // Calling getNextId again should still return 4 (not consumed)
    assert(gen.getNextId() == 4);

    // Now consume it
    EntityID next = gen.next();
    assert(next == 4);
    assert(gen.getNextId() == 5);

    printf("  PASS: getNextId works correctly\n");
}

// =============================================================================
// Test: getGeneratedCount tracks generated IDs
// =============================================================================
void test_get_generated_count() {
    printf("Testing getGeneratedCount...\n");

    EntityIdGenerator gen;

    assert(gen.getGeneratedCount() == 0);

    gen.next();
    assert(gen.getGeneratedCount() == 1);

    gen.next();
    assert(gen.getGeneratedCount() == 2);

    for (int i = 0; i < 100; ++i) {
        gen.next();
    }

    assert(gen.getGeneratedCount() == 102);

    printf("  PASS: getGeneratedCount works correctly\n");
}

// =============================================================================
// Test: restore() for persistence
// =============================================================================
void test_restore_persistence() {
    printf("Testing restore() for persistence...\n");

    EntityIdGenerator gen1;

    // Generate some IDs
    for (int i = 0; i < 50; ++i) {
        gen1.next();
    }

    // Save state
    std::uint64_t savedNextId = gen1.getNextId();
    assert(savedNextId == 51);

    // Simulate server restart - create new generator
    EntityIdGenerator gen2;

    // Restore from saved state
    gen2.restore(savedNextId);

    // Next ID should continue from 51
    EntityID nextAfterRestore = gen2.next();
    assert(nextAfterRestore == 51);

    // Verify count is correct
    assert(gen2.getGeneratedCount() == 51);

    printf("  PASS: restore() for persistence works\n");
}

// =============================================================================
// Test: restore() with value 0 sets to 1
// =============================================================================
void test_restore_zero_becomes_one() {
    printf("Testing restore(0) sets to 1...\n");

    EntityIdGenerator gen;

    // Generate some IDs first
    for (int i = 0; i < 10; ++i) {
        gen.next();
    }

    // Restore with 0 (should become 1 to protect reserved ID)
    gen.restore(0);

    assert(gen.getNextId() == 1);
    assert(gen.next() == 1);

    printf("  PASS: restore(0) correctly becomes 1\n");
}

// =============================================================================
// Test: reset() returns to initial state
// =============================================================================
void test_reset() {
    printf("Testing reset()...\n");

    EntityIdGenerator gen;

    // Generate many IDs
    for (int i = 0; i < 100; ++i) {
        gen.next();
    }

    assert(gen.getGeneratedCount() == 100);
    assert(gen.getNextId() == 101);

    // Reset
    gen.reset();

    assert(gen.getGeneratedCount() == 0);
    assert(gen.getNextId() == 1);
    assert(gen.next() == 1);

    printf("  PASS: reset() works correctly\n");
}

// =============================================================================
// Test: Large ID generation (verify no overflow issues)
// =============================================================================
void test_large_id_generation() {
    printf("Testing large ID generation...\n");

    EntityIdGenerator gen;

    // Restore to a large starting point
    std::uint64_t largeStart = 1000000000ULL;  // 1 billion
    gen.restore(largeStart);

    EntityID id1 = gen.next();
    EntityID id2 = gen.next();

    // EntityID is uint32_t, so we check truncation behavior
    assert(id1 == static_cast<EntityID>(largeStart));
    assert(id2 == static_cast<EntityID>(largeStart + 1));

    printf("  PASS: Large ID generation works\n");
}

// =============================================================================
// Registry Tests: createWithId for client-side entity creation
// =============================================================================

// Test: Registry createWithId basic functionality
void test_registry_create_with_id() {
    printf("Testing Registry::createWithId...\n");

    Registry registry;

    // Create entity with specific ID
    EntityID id1 = registry.createWithId(42);
    assert(id1 == 42);
    assert(registry.valid(42));

    // Create another entity with different ID
    EntityID id2 = registry.createWithId(100);
    assert(id2 == 100);
    assert(registry.valid(100));

    // Both should be valid
    assert(registry.valid(42));
    assert(registry.valid(100));

    printf("  PASS: Registry::createWithId basic functionality works\n");
}

// Test: Registry createWithId handles reconnection (existing entity)
void test_registry_create_with_id_reconnection() {
    printf("Testing Registry::createWithId reconnection scenario...\n");

    Registry registry;

    // Create entity with ID 42
    registry.createWithId(42);
    registry.emplace<PositionComponent>(42, PositionComponent{{10, 20}, 5});

    // Verify it exists with original data
    assert(registry.valid(42));
    auto& pos1 = registry.get<PositionComponent>(42);
    assert(pos1.pos.x == 10);

    // Simulate reconnection - create same ID again
    EntityID recreated = registry.createWithId(42);
    assert(recreated == 42);
    assert(registry.valid(42));

    // Old components should be gone (entity was destroyed and recreated)
    assert(!registry.has<PositionComponent>(42));

    // Add new components
    registry.emplace<PositionComponent>(42, PositionComponent{{100, 200}, 10});
    auto& pos2 = registry.get<PositionComponent>(42);
    assert(pos2.pos.x == 100);

    printf("  PASS: Registry::createWithId handles reconnection correctly\n");
}

// Test: Registry createWithId with sequential IDs
void test_registry_create_with_sequential_ids() {
    printf("Testing Registry::createWithId with sequential IDs...\n");

    Registry registry;

    // Create entities with sequential IDs (mimicking server behavior)
    for (EntityID id = 1; id <= 100; ++id) {
        EntityID created = registry.createWithId(id);
        assert(created == id);
    }

    // All should be valid
    for (EntityID id = 1; id <= 100; ++id) {
        assert(registry.valid(id));
    }

    assert(registry.size() == 100);

    printf("  PASS: Registry::createWithId with sequential IDs works\n");
}

// Test: Registry createWithId with non-sequential IDs
void test_registry_create_with_non_sequential_ids() {
    printf("Testing Registry::createWithId with non-sequential IDs...\n");

    Registry registry;

    // Create entities with non-sequential IDs
    std::vector<EntityID> ids = {5, 100, 42, 1, 999, 50};

    for (EntityID id : ids) {
        EntityID created = registry.createWithId(id);
        assert(created == id);
    }

    // All should be valid
    for (EntityID id : ids) {
        assert(registry.valid(id));
    }

    // ID 0 should not be valid (never created)
    assert(!registry.valid(0));

    // Random IDs should not be valid
    assert(!registry.valid(2));
    assert(!registry.valid(3));

    printf("  PASS: Registry::createWithId with non-sequential IDs works\n");
}

// =============================================================================
// Integration Test: Full server-client ID sync flow
// =============================================================================
void test_full_server_client_id_sync() {
    printf("Testing full server-client ID sync flow...\n");

    // Server side
    EntityIdGenerator serverGen;
    Registry serverRegistry;

    // Server creates entities
    for (int i = 0; i < 10; ++i) {
        EntityID id = serverGen.next();
        serverRegistry.createWithId(id);
        serverRegistry.emplace<PositionComponent>(id, PositionComponent{{static_cast<std::int16_t>(i), 0}, 0});
    }

    assert(serverRegistry.size() == 10);
    assert(serverGen.getGeneratedCount() == 10);

    // Client side - receives IDs from server and creates entities
    Registry clientRegistry;

    // Simulate receiving entity IDs from server (1 through 10)
    for (EntityID id = 1; id <= 10; ++id) {
        clientRegistry.createWithId(id);
        // Get position data from "server" (simulated)
        auto& serverPos = serverRegistry.get<PositionComponent>(id);
        clientRegistry.emplace<PositionComponent>(id, serverPos);
    }

    // Verify client state matches server
    assert(clientRegistry.size() == 10);

    for (EntityID id = 1; id <= 10; ++id) {
        assert(clientRegistry.valid(id));
        auto& serverPos = serverRegistry.get<PositionComponent>(id);
        auto& clientPos = clientRegistry.get<PositionComponent>(id);
        assert(serverPos.pos.x == clientPos.pos.x);
    }

    // Verify entity 42 is the same on both (entity 42 = ID 42)
    EntityID entity42 = serverGen.next();  // Gets 11
    assert(entity42 == 11);

    printf("  PASS: Full server-client ID sync flow works\n");
}

// =============================================================================
// Test: Persistence roundtrip
// =============================================================================
void test_persistence_roundtrip() {
    printf("Testing persistence roundtrip...\n");

    // First session
    EntityIdGenerator session1;
    std::vector<EntityID> session1Ids;

    for (int i = 0; i < 100; ++i) {
        session1Ids.push_back(session1.next());
    }

    // Save state
    std::uint64_t savedState = session1.getNextId();

    // Verify saved state
    assert(savedState == 101);

    // Second session (after restart)
    EntityIdGenerator session2;
    session2.restore(savedState);

    // New IDs should not overlap with session 1
    std::vector<EntityID> session2Ids;
    for (int i = 0; i < 100; ++i) {
        session2Ids.push_back(session2.next());
    }

    // Verify no overlap
    std::unordered_set<EntityID> allIds(session1Ids.begin(), session1Ids.end());
    for (EntityID id : session2Ids) {
        assert(allIds.find(id) == allIds.end());
        allIds.insert(id);
    }

    assert(allIds.size() == 200);

    printf("  PASS: Persistence roundtrip works correctly\n");
}

// =============================================================================
// Include component header for PositionComponent
// =============================================================================
#include "sims3000/ecs/Components.h"

// =============================================================================
// Main
// =============================================================================
int main() {
    printf("=== EntityIdGenerator Unit Tests (Ticket 1-015) ===\n\n");

    // EntityIdGenerator tests
    test_counter_starts_at_one();
    test_id_zero_reserved();
    test_monotonic_increment();
    test_no_id_reuse();
    test_get_next_id();
    test_get_generated_count();
    test_restore_persistence();
    test_restore_zero_becomes_one();
    test_reset();
    test_large_id_generation();

    printf("\n=== Registry createWithId Tests ===\n\n");

    test_registry_create_with_id();
    test_registry_create_with_id_reconnection();
    test_registry_create_with_sequential_ids();
    test_registry_create_with_non_sequential_ids();

    printf("\n=== Integration Tests ===\n\n");

    test_full_server_client_id_sync();
    test_persistence_roundtrip();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
