/**
 * @file test_path_cache.cpp
 * @brief Tests for PathCache (Epic 7, Ticket E7-041)
 *
 * Tests:
 * - Cache hit for repeated queries
 * - Cache miss for unknown routes
 * - Expiration after max_age_ticks
 * - Invalidation on network change
 * - Size tracking
 */

#include <sims3000/transport/PathCache.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

// =============================================================================
// Helper
// =============================================================================

static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) \
    do { \
        tests_total++; \
        printf("  TEST: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

// =============================================================================
// Tests
// =============================================================================

static void test_construction_default() {
    TEST("Default construction with max_age=100");
    PathCache cache;
    assert(cache.size() == 0);
    PASS();
}

static void test_construction_custom() {
    TEST("Custom max_age construction");
    PathCache cache(50);
    assert(cache.size() == 0);
    PASS();
}

static void test_put_and_get() {
    TEST("Put and get returns cached result");
    PathCache cache;

    GridPosition start{0, 0};
    GridPosition end{10, 10};

    PathResult result;
    result.found = true;
    result.total_cost = 100;
    result.path.push_back(start);
    result.path.push_back(end);

    cache.put(start, end, result, 50);
    assert(cache.size() == 1);

    const PathResult* cached = cache.get(start, end, 50);
    assert(cached != nullptr);
    assert(cached->found == true);
    assert(cached->total_cost == 100);
    assert(cached->path.size() == 2);
    PASS();
}

static void test_cache_miss() {
    TEST("Get returns nullptr for missing entry");
    PathCache cache;

    GridPosition start{0, 0};
    GridPosition end{10, 10};

    const PathResult* cached = cache.get(start, end, 0);
    assert(cached == nullptr);
    PASS();
}

static void test_cache_hit_different_query() {
    TEST("Different start/end is a cache miss");
    PathCache cache;

    GridPosition start1{0, 0};
    GridPosition end1{10, 10};
    GridPosition start2{1, 1};
    GridPosition end2{11, 11};

    PathResult result;
    result.found = true;
    cache.put(start1, end1, result, 0);

    const PathResult* cached = cache.get(start2, end2, 0);
    assert(cached == nullptr);
    PASS();
}

static void test_expiration() {
    TEST("Entry expires after max_age_ticks");
    PathCache cache(100);

    GridPosition start{0, 0};
    GridPosition end{10, 10};

    PathResult result;
    result.found = true;
    cache.put(start, end, result, 50);

    // At tick 149, should still be valid (age = 99)
    const PathResult* cached1 = cache.get(start, end, 149);
    assert(cached1 != nullptr);

    // At tick 150, should be expired (age = 100 >= max_age=100)
    const PathResult* cached2 = cache.get(start, end, 150);
    assert(cached2 == nullptr);
    PASS();
}

static void test_expiration_exact_boundary() {
    TEST("Expiration at exact max_age boundary");
    PathCache cache(100);

    GridPosition start{0, 0};
    GridPosition end{10, 10};

    PathResult result;
    result.found = true;
    cache.put(start, end, result, 0);

    // At tick 99, valid (age = 99 < 100)
    assert(cache.get(start, end, 99) != nullptr);

    // At tick 100, expired (age = 100 >= 100)
    assert(cache.get(start, end, 100) == nullptr);
    PASS();
}

static void test_invalidation() {
    TEST("Invalidation clears all entries");
    PathCache cache;

    PathResult result;
    result.found = true;

    for (int i = 0; i < 5; ++i) {
        GridPosition start{0, 0};
        GridPosition end{i * 10, i * 10};
        cache.put(start, end, result, 0);
    }
    assert(cache.size() == 5);

    cache.invalidate();
    assert(cache.size() == 0);

    // All entries should be gone
    GridPosition start{0, 0};
    GridPosition end{0, 0};
    assert(cache.get(start, end, 0) == nullptr);
    PASS();
}

static void test_overwrite() {
    TEST("Put overwrites existing entry");
    PathCache cache;

    GridPosition start{0, 0};
    GridPosition end{10, 10};

    PathResult result1;
    result1.found = true;
    result1.total_cost = 50;
    cache.put(start, end, result1, 0);

    PathResult result2;
    result2.found = true;
    result2.total_cost = 100;
    cache.put(start, end, result2, 10);

    assert(cache.size() == 1);

    const PathResult* cached = cache.get(start, end, 10);
    assert(cached != nullptr);
    assert(cached->total_cost == 100);
    PASS();
}

static void test_size() {
    TEST("Size tracks entry count");
    PathCache cache;

    assert(cache.size() == 0);

    PathResult result;
    result.found = false;

    GridPosition s1{0,0}, e1{1,1};
    GridPosition s2{2,2}, e2{3,3};
    GridPosition s3{4,4}, e3{5,5};

    cache.put(s1, e1, result, 0);
    assert(cache.size() == 1);

    cache.put(s2, e2, result, 0);
    assert(cache.size() == 2);

    cache.put(s3, e3, result, 0);
    assert(cache.size() == 3);

    cache.invalidate();
    assert(cache.size() == 0);
    PASS();
}

static void test_not_found_result_cached() {
    TEST("Not-found results are cached too");
    PathCache cache;

    GridPosition start{0, 0};
    GridPosition end{99, 99};

    PathResult result;
    result.found = false;
    result.total_cost = 0;

    cache.put(start, end, result, 0);

    const PathResult* cached = cache.get(start, end, 0);
    assert(cached != nullptr);
    assert(cached->found == false);
    PASS();
}

static void test_max_age_1() {
    TEST("Max age of 1 tick expires immediately");
    PathCache cache(1);

    GridPosition start{0, 0};
    GridPosition end{10, 10};

    PathResult result;
    result.found = true;
    cache.put(start, end, result, 0);

    // Same tick is valid (age = 0 < 1)
    assert(cache.get(start, end, 0) != nullptr);

    // Next tick is expired (age = 1 >= 1)
    assert(cache.get(start, end, 1) == nullptr);
    PASS();
}

static void test_symmetric_keys() {
    TEST("Reversed start/end is a different cache key");
    PathCache cache;

    GridPosition a{0, 0};
    GridPosition b{10, 10};

    PathResult result_ab;
    result_ab.found = true;
    result_ab.total_cost = 50;

    PathResult result_ba;
    result_ba.found = true;
    result_ba.total_cost = 75;

    cache.put(a, b, result_ab, 0);
    cache.put(b, a, result_ba, 0);

    assert(cache.size() == 2);

    const PathResult* cached_ab = cache.get(a, b, 0);
    const PathResult* cached_ba = cache.get(b, a, 0);

    assert(cached_ab != nullptr && cached_ab->total_cost == 50);
    assert(cached_ba != nullptr && cached_ba->total_cost == 75);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== PathCache Tests (E7-041) ===\n");

    test_construction_default();
    test_construction_custom();
    test_put_and_get();
    test_cache_miss();
    test_cache_hit_different_query();
    test_expiration();
    test_expiration_exact_boundary();
    test_invalidation();
    test_overwrite();
    test_size();
    test_not_found_result_cached();
    test_max_age_1();
    test_symmetric_keys();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
