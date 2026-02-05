/**
 * @file test_interpolatable.cpp
 * @brief Unit tests for Interpolatable<T> template.
 */

#include "sims3000/core/Interpolatable.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Helper for float comparison with tolerance
bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::fabs(a - b) < epsilon;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

void test_default_construction() {
    printf("Testing default construction...\n");

    Interpolatable<float> f;
    assert(f.current() == 0.0f);
    assert(f.previous() == 0.0f);

    Interpolatable<glm::vec3> v;
    assert(v.current() == glm::vec3(0.0f));
    assert(v.previous() == glm::vec3(0.0f));

    printf("  PASS: Default construction works\n");
}

void test_initial_value_construction() {
    printf("Testing initial value construction...\n");

    Interpolatable<float> f(5.0f);
    assert(f.current() == 5.0f);
    assert(f.previous() == 5.0f);

    glm::vec3 pos(1.0f, 2.0f, 3.0f);
    Interpolatable<glm::vec3> v(pos);
    assert(v.current() == pos);
    assert(v.previous() == pos);

    printf("  PASS: Initial value construction works\n");
}

void test_two_value_construction() {
    printf("Testing two-value construction...\n");

    Interpolatable<float> f(0.0f, 10.0f);
    assert(f.previous() == 0.0f);
    assert(f.current() == 10.0f);

    printf("  PASS: Two-value construction works\n");
}

void test_rotate_tick() {
    printf("Testing rotateTick...\n");

    Interpolatable<float> f(5.0f);
    f.set(10.0f);

    assert(f.previous() == 5.0f);
    assert(f.current() == 10.0f);

    f.rotateTick();

    assert(f.previous() == 10.0f);
    assert(f.current() == 10.0f);

    f.set(15.0f);
    assert(f.previous() == 10.0f);
    assert(f.current() == 15.0f);

    printf("  PASS: rotateTick works correctly\n");
}

void test_set_both() {
    printf("Testing setBoth...\n");

    Interpolatable<float> f(0.0f, 10.0f);
    assert(f.previous() != f.current());

    f.setBoth(25.0f);
    assert(f.previous() == 25.0f);
    assert(f.current() == 25.0f);

    printf("  PASS: setBoth works correctly\n");
}

void test_lerp_float() {
    printf("Testing lerp with float...\n");

    Interpolatable<float> f(0.0f, 10.0f);

    assert(approxEqual(f.lerp(0.0f), 0.0f));
    assert(approxEqual(f.lerp(0.5f), 5.0f));
    assert(approxEqual(f.lerp(1.0f), 10.0f));
    assert(approxEqual(f.lerp(0.25f), 2.5f));
    assert(approxEqual(f.lerp(0.75f), 7.5f));

    printf("  PASS: Float lerp works correctly\n");
}

void test_lerp_vec3() {
    printf("Testing lerp with vec3...\n");

    glm::vec3 start(0.0f, 0.0f, 0.0f);
    glm::vec3 end(10.0f, 20.0f, 30.0f);

    Interpolatable<glm::vec3> v(start, end);

    assert(approxEqual(v.lerp(0.0f), start));
    assert(approxEqual(v.lerp(1.0f), end));
    assert(approxEqual(v.lerp(0.5f), glm::vec3(5.0f, 10.0f, 15.0f)));

    printf("  PASS: Vec3 lerp works correctly\n");
}

void test_lerp_integer_no_interpolation() {
    printf("Testing lerp with integer (should snap to current)...\n");

    Interpolatable<int> i(0, 10);

    // Integers should not interpolate - return current value
    assert(i.lerp(0.0f) == 10);
    assert(i.lerp(0.5f) == 10);
    assert(i.lerp(1.0f) == 10);

    printf("  PASS: Integer lerp returns current value\n");
}

void test_assignment_operator() {
    printf("Testing assignment operator...\n");

    Interpolatable<float> f(5.0f);
    f = 10.0f;

    assert(f.previous() == 5.0f);
    assert(f.current() == 10.0f);

    printf("  PASS: Assignment operator works correctly\n");
}

void test_implicit_conversion() {
    printf("Testing implicit conversion...\n");

    Interpolatable<float> f(10.0f);

    // Should implicitly convert to float (current value)
    float value = f;
    assert(value == 10.0f);

    // Should work in expressions
    float result = f + 5.0f;
    assert(result == 15.0f);

    printf("  PASS: Implicit conversion works correctly\n");
}

void test_mutable_current() {
    printf("Testing mutable current access...\n");

    Interpolatable<glm::vec3> v(glm::vec3(0.0f));

    v.current().x = 5.0f;
    v.current().y = 10.0f;

    assert(v.current().x == 5.0f);
    assert(v.current().y == 10.0f);

    printf("  PASS: Mutable current access works correctly\n");
}

void test_type_aliases() {
    printf("Testing type aliases...\n");

    InterpolatableFloat f(1.0f);
    InterpolatableVec2 v2(glm::vec2(1.0f, 2.0f));
    InterpolatableVec3 v3(glm::vec3(1.0f, 2.0f, 3.0f));
    InterpolatableVec4 v4(glm::vec4(1.0f, 2.0f, 3.0f, 4.0f));

    assert(f.current() == 1.0f);
    assert(v2.current() == glm::vec2(1.0f, 2.0f));
    assert(v3.current() == glm::vec3(1.0f, 2.0f, 3.0f));
    assert(v4.current() == glm::vec4(1.0f, 2.0f, 3.0f, 4.0f));

    printf("  PASS: Type aliases work correctly\n");
}

void test_helper_functions() {
    printf("Testing helper functions...\n");

    // lerpValue
    float result = lerpValue(0.0f, 10.0f, 0.5f);
    assert(approxEqual(result, 5.0f));

    glm::vec3 vResult = lerpValue(glm::vec3(0.0f), glm::vec3(10.0f), 0.25f);
    assert(approxEqual(vResult, glm::vec3(2.5f)));

    // clampAlpha
    assert(clampAlpha(-0.5f) == 0.0f);
    assert(clampAlpha(0.5f) == 0.5f);
    assert(clampAlpha(1.5f) == 1.0f);

    printf("  PASS: Helper functions work correctly\n");
}

void test_simulation_workflow() {
    printf("Testing simulation workflow (rotateTick -> update -> lerp)...\n");

    // Simulate a typical usage pattern
    Interpolatable<glm::vec3> position(glm::vec3(0.0f, 0.0f, 0.0f));

    // Tick 1: move to (10, 0, 0)
    position.rotateTick();
    position.set(glm::vec3(10.0f, 0.0f, 0.0f));

    // Render at alpha = 0.5 (halfway through tick)
    glm::vec3 renderPos = position.lerp(0.5f);
    assert(approxEqual(renderPos, glm::vec3(5.0f, 0.0f, 0.0f)));

    // Tick 2: move to (10, 10, 0)
    position.rotateTick();
    position.set(glm::vec3(10.0f, 10.0f, 0.0f));

    // Render at alpha = 0.25
    renderPos = position.lerp(0.25f);
    // prev = (10, 0, 0), curr = (10, 10, 0), result = (10, 2.5, 0)
    assert(approxEqual(renderPos, glm::vec3(10.0f, 2.5f, 0.0f)));

    printf("  PASS: Simulation workflow works correctly\n");
}

int main() {
    printf("=== Interpolatable<T> Unit Tests ===\n\n");

    test_default_construction();
    test_initial_value_construction();
    test_two_value_construction();
    test_rotate_tick();
    test_set_both();
    test_lerp_float();
    test_lerp_vec3();
    test_lerp_integer_no_interpolation();
    test_assignment_operator();
    test_implicit_conversion();
    test_mutable_current();
    test_type_aliases();
    test_helper_functions();
    test_simulation_workflow();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
