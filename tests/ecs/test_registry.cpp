/**
 * @file test_registry.cpp
 * @brief Unit tests for ECS Registry.
 */

#include "sims3000/ecs/Registry.h"
#include <cassert>
#include <cstdio>

using namespace sims3000;

// Test components
struct TestPosition {
    float x, y, z;
};

struct TestVelocity {
    float vx, vy, vz;
};

struct TestHealth {
    int current;
    int max;
};

void test_create_destroy() {
    printf("Testing entity create/destroy...\n");

    Registry reg;

    // Create entities
    EntityID e1 = reg.create();
    EntityID e2 = reg.create();
    EntityID e3 = reg.create();

    assert(reg.valid(e1));
    assert(reg.valid(e2));
    assert(reg.valid(e3));
    assert(reg.size() == 3);

    // Destroy one
    reg.destroy(e2);
    assert(reg.valid(e1));
    assert(!reg.valid(e2));
    assert(reg.valid(e3));

    // Create another (may reuse ID)
    EntityID e4 = reg.create();
    assert(reg.valid(e4));

    printf("  PASS: Create/destroy works correctly\n");
}

void test_components() {
    printf("Testing component operations...\n");

    Registry reg;
    EntityID entity = reg.create();

    // Add component
    auto& pos = reg.emplace<TestPosition>(entity, TestPosition{1.0f, 2.0f, 3.0f});
    assert(pos.x == 1.0f);
    assert(pos.y == 2.0f);
    assert(pos.z == 3.0f);

    // Check has
    assert(reg.has<TestPosition>(entity));
    assert(!reg.has<TestVelocity>(entity));

    // Get component
    auto& retrieved = reg.get<TestPosition>(entity);
    assert(retrieved.x == 1.0f);

    // Modify component
    retrieved.x = 10.0f;
    assert(reg.get<TestPosition>(entity).x == 10.0f);

    // Try get
    auto* ptr = reg.tryGet<TestPosition>(entity);
    assert(ptr != nullptr);
    assert(ptr->x == 10.0f);

    auto* nullPtr = reg.tryGet<TestVelocity>(entity);
    assert(nullPtr == nullptr);

    // Remove component
    reg.remove<TestPosition>(entity);
    assert(!reg.has<TestPosition>(entity));

    printf("  PASS: Component operations work correctly\n");
}

void test_multiple_components() {
    printf("Testing multiple components...\n");

    Registry reg;
    EntityID entity = reg.create();

    reg.emplace<TestPosition>(entity, TestPosition{1, 2, 3});
    reg.emplace<TestVelocity>(entity, TestVelocity{4, 5, 6});
    reg.emplace<TestHealth>(entity, TestHealth{100, 100});

    assert(reg.has<TestPosition>(entity));
    assert(reg.has<TestVelocity>(entity));
    assert(reg.has<TestHealth>(entity));

    assert(reg.get<TestPosition>(entity).x == 1.0f);
    assert(reg.get<TestVelocity>(entity).vx == 4.0f);
    assert(reg.get<TestHealth>(entity).current == 100);

    printf("  PASS: Multiple components work correctly\n");
}

void test_view() {
    printf("Testing view iteration...\n");

    Registry reg;

    // Create entities with different component combinations
    EntityID e1 = reg.create();
    reg.emplace<TestPosition>(e1, TestPosition{1, 0, 0});
    reg.emplace<TestVelocity>(e1, TestVelocity{1, 0, 0});

    EntityID e2 = reg.create();
    reg.emplace<TestPosition>(e2, TestPosition{2, 0, 0});
    // No velocity

    EntityID e3 = reg.create();
    reg.emplace<TestPosition>(e3, TestPosition{3, 0, 0});
    reg.emplace<TestVelocity>(e3, TestVelocity{3, 0, 0});

    // View with single component
    int posCount = 0;
    for (auto entity : reg.view<TestPosition>()) {
        (void)entity;
        posCount++;
    }
    assert(posCount == 3);

    // View with multiple components
    int bothCount = 0;
    auto view2 = reg.view<TestPosition, TestVelocity>();
    for (auto entity : view2) {
        auto& pos = view2.get<TestPosition>(entity);
        auto& vel = view2.get<TestVelocity>(entity);
        assert(pos.x == vel.vx);  // Our test data matches
        bothCount++;
    }
    assert(bothCount == 2);

    printf("  PASS: View iteration works correctly\n");
}

void test_clear() {
    printf("Testing clear...\n");

    Registry reg;

    for (int i = 0; i < 100; ++i) {
        EntityID e = reg.create();
        reg.emplace<TestPosition>(e, TestPosition{static_cast<float>(i), 0, 0});
    }
    assert(reg.size() == 100);

    reg.clear();
    assert(reg.size() == 0);

    // Can still create new entities
    EntityID e = reg.create();
    assert(reg.valid(e));

    printf("  PASS: Clear works correctly\n");
}

void test_raw_access() {
    printf("Testing raw registry access...\n");

    Registry reg;
    EntityID e = reg.create();
    reg.emplace<TestPosition>(e, TestPosition{1, 2, 3});

    // Access underlying EnTT registry
    entt::registry& raw = reg.raw();

    // Use EnTT directly
    auto view = raw.view<TestPosition>();
    int count = 0;
    for (auto entity : view) {
        (void)entity;
        count++;
    }
    assert(count == 1);

    printf("  PASS: Raw access works correctly\n");
}

int main() {
    printf("=== ECS Registry Unit Tests ===\n\n");

    test_create_destroy();
    test_components();
    test_multiple_components();
    test_view();
    test_clear();
    test_raw_access();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
