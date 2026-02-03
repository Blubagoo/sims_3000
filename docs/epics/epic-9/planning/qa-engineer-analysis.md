# QA Engineer Analysis: Epic 9 - City Services

**Author:** QA Engineer Agent
**Date:** 2026-01-29
**Canon Version:** 0.10.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 9 implements the ServicesSystem that provides coverage queries consumed by Epic 10's simulation systems. This epic replaces the `IServiceQueryable` stub from Epic 10, enabling real service effectiveness calculations for disorder reduction and population health.

**Key Testing Challenges:**
1. **Integration with Epic 10 stubs** - Must verify ServicesSystem correctly replaces stubbed behavior
2. **Two service models** - Radius-based (enforcer, hazard) vs global (medical, education) require different test strategies
3. **Funding dependency** - Service effectiveness depends on EconomySystem (Epic 11 stub)
4. **Coverage calculations** - Overlapping coverage areas, map edge cases, demolished buildings

---

## 1. Test Categories

### 1.1 Unit Tests

| Category | Description | Estimated Count |
|----------|-------------|-----------------|
| Component Tests | ServiceProviderComponent, ServiceCoverageComponent | 8-10 |
| Coverage Calculation | Radius-based coverage math, falloff curves | 15-20 |
| Global Service Effects | Medical/education effectiveness aggregation | 10-12 |
| Building State | Service building lifecycle, funding effects | 10-15 |
| Interface Compliance | IServiceQueryable contract tests | 8-10 |

**Total Unit Tests: ~50-65**

### 1.2 Integration Tests

| Category | Description | Estimated Count |
|----------|-------------|-----------------|
| DisorderSystem Integration | Enforcer coverage reducing disorder | 8-10 |
| PopulationSystem Integration | Medical coverage affecting longevity | 6-8 |
| BuildingSystem Integration | Service building placement/demolition | 6-8 |
| EconomySystem Stub | Funding affecting service effectiveness | 4-6 |

**Total Integration Tests: ~25-35**

### 1.3 Performance Tests

| Category | Description | Target |
|----------|-------------|--------|
| Coverage Query Latency | get_coverage_at() performance | < 1ms per 1000 queries |
| Grid Update | Full coverage recalculation | < 5ms at 256x256 |
| Building Iteration | Service building enumeration | < 2ms for 500 buildings |
| Memory Footprint | Coverage grid memory | < 512KB at 512x512 |

---

## 2. Critical Edge Cases

### 2.1 Service Building Lifecycle Edge Cases

| Edge Case | Test Scenario | Expected Behavior |
|-----------|---------------|-------------------|
| **Building placed but no funding** | Place enforcer post, set funding to 0% | Coverage radius exists but effectiveness = 0 |
| **Building demolished mid-tick** | Demolish service building during simulation | Coverage removed cleanly, no dangling references |
| **Building unpowered** | Service building loses power | Service effectiveness reduced (NOT zero - emergency power?) |
| **Building without water** | Service building loses fluid | Service effectiveness reduced but not disabled |
| **Building abandoned** | Owner disconnects, ghost town process | Service degrades over 50 cycles |

### 2.2 Coverage Calculation Edge Cases

| Edge Case | Test Scenario | Expected Behavior |
|-----------|---------------|-------------------|
| **Multiple overlapping areas** | 3 enforcer posts with overlapping radii | Coverage stacks (additive? max? diminishing returns?) |
| **Coverage at map edges** | Enforcer post at (0,0) corner | Radius clipped to map bounds, no wraparound |
| **Coverage across ownership** | Player A's enforcer near Player B's zone | Coverage applies to all tiles in radius |
| **Zero-radius building** | Service building with radius = 0 | No coverage area (point coverage only?) |
| **Maximum radius** | Building with very large radius (32+ tiles) | Performance acceptable, coverage correct |

### 2.3 Global Service Edge Cases

| Edge Case | Test Scenario | Expected Behavior |
|-----------|---------------|-------------------|
| **No medical buildings** | Player has no medical coverage | get_coverage(Medical, player) returns 0 |
| **Partially funded** | 50% funding on medical nexus | Effectiveness = 0.5 (linear) or curved? |
| **Multiple medical buildings** | 3 medical nexuses at different funding | Aggregate effectiveness calculation |
| **Education stacking** | School + library + archive | Effects stack within cap |

### 2.4 Stub Replacement Edge Cases

| Edge Case | Test Scenario | Expected Behavior |
|-----------|---------------|-------------------|
| **Stub returns 0.5** | Before Epic 9, stub returns neutral | DisorderSystem sees 50% suppression baseline |
| **Real returns 0.0** | After Epic 9, no enforcers | DisorderSystem sees 0% suppression |
| **Seamless transition** | Epic 10 code unchanged | Interface contract maintained |

---

## 3. Epic 10 Integration Tests

### 3.1 DisorderSystem Integration

**Test: Enforcer Coverage Reduces Disorder**
```
GIVEN: A tile with 100 disorder and an enforcer post within radius
WHEN: DisorderSystem.tick() is called
THEN: Disorder is reduced by enforcer suppression amount
```

**Test: No Enforcers = No Suppression**
```
GIVEN: A tile with 100 disorder and no enforcer coverage
WHEN: DisorderSystem.tick() is called
THEN: Disorder spreads without suppression
```

**Test: Partial Funding Reduces Effectiveness**
```
GIVEN: Enforcer post at 50% funding, tile with 100 disorder
WHEN: DisorderSystem.tick() is called
THEN: Suppression is 50% of full suppression power
```

**Test: Coverage Falloff**
```
GIVEN: Enforcer post at position (10,10) with radius 8
WHEN: Querying coverage at distances 0, 4, 8, 12
THEN: Coverage is 100%, ~50%, ~0%, 0% (linear falloff)
```

### 3.2 PopulationSystem Integration

**Test: Medical Coverage Improves Longevity**
```
GIVEN: Colony with 1000 beings, 80% medical coverage
WHEN: calculate_life_expectancy() is called
THEN: Life expectancy > baseline (75 cycles)
```

**Test: No Medical = Reduced Longevity**
```
GIVEN: Colony with 1000 beings, 0% medical coverage
WHEN: calculate_life_expectancy() is called
THEN: Life expectancy < baseline (75 cycles)
```

**Test: Health Index Reflects Medical Coverage**
```
GIVEN: Colony with varying medical coverage (0%, 50%, 100%)
WHEN: update_health_index() is called
THEN: health_index scales with coverage
```

### 3.3 Interface Contract Tests

**Test: IServiceQueryable.get_coverage Returns Valid Range**
```
FOR EACH: ServiceType in [Enforcer, HazardResponse, Medical, Education]
WHEN: get_coverage(type, player) is called
THEN: Result is in range [0.0, 1.0]
```

**Test: IServiceQueryable.get_coverage_at Handles Invalid Positions**
```
GIVEN: Invalid position (-1, -1) or (map_width+1, map_height+1)
WHEN: get_coverage_at(type, position) is called
THEN: Returns 0.0 (or throws - needs clarification)
```

**Test: IServiceQueryable.get_effectiveness Reflects Funding**
```
GIVEN: Service at 75% funding
WHEN: get_effectiveness(type, player) is called
THEN: Returns value reflecting funding level
```

---

## 4. Quality Gates

### 4.1 Unit Test Quality Gates

| Gate | Criteria | Blocking? |
|------|----------|-----------|
| Coverage Threshold | >= 80% line coverage on ServicesSystem | YES |
| All Tests Pass | 0 failures | YES |
| No Flaky Tests | 100% deterministic across 10 runs | YES |
| Edge Case Coverage | All Section 2 cases tested | YES |
| Test Isolation | No shared state between tests | YES |

### 4.2 Integration Test Quality Gates

| Gate | Criteria | Blocking? |
|------|----------|-----------|
| Epic 10 Stub Replacement | DisorderSystem works with real ServicesSystem | YES |
| Population Integration | Health/longevity calculations work | YES |
| Interface Compliance | All IServiceQueryable methods tested | YES |
| No Regression | Existing Epic 10 tests still pass | YES |

### 4.3 Performance Quality Gates

| Gate | Criteria | Blocking? |
|------|----------|-----------|
| Coverage Query Performance | < 1ms per 1000 queries | YES |
| Grid Update Performance | < 5ms at 256x256 | YES |
| Memory Budget | < 512KB for coverage data | NO (warning) |
| Tick Budget | ServicesSystem.tick() < 3ms | YES |

### 4.4 Build Quality Gates

| Gate | Criteria | Blocking? |
|------|----------|-----------|
| Compilation | No errors, no new warnings | YES |
| Static Analysis | No critical issues | YES |
| Canon Compliance | Interfaces match interfaces.yaml | YES |

---

## 5. Questions for Other Agents

### @systems-architect

1. **Coverage overlap behavior:** When multiple service buildings have overlapping coverage radii, should the coverage stack additively, take the maximum, or use diminishing returns? This affects test assertions significantly.

2. **Cross-ownership coverage:** Should Player A's enforcer post reduce disorder in Player B's tiles? The canon mentions coverage queries but doesn't specify ownership restrictions. Tests need to know expected behavior.

3. **Service building power dependency:** If a service building loses power, what happens to its coverage? Options:
   - (A) Coverage drops to 0% immediately
   - (B) Coverage reduced to 50% (emergency power)
   - (C) Coverage unaffected (services have backup power)
   This affects disaster scenario tests.

4. **Tick priority:** What is ServicesSystem's tick_priority? It's not in systems.yaml. Needs to run before DisorderSystem (70) to provide fresh coverage data. Recommend priority 65?

### @services-engineer

5. **Coverage radius values:** What are the default coverage radii for each service type?
   - Enforcer post: ?
   - Hazard response post: ?
   - Medical nexus: Global? Radius?
   - Learning center: Global? Radius?

   These are needed for test fixture setup.

6. **Effectiveness formula:** Is service effectiveness linear with funding, or is there a curve?
   - Linear: effectiveness = funding_percent / 100
   - Curved: effectiveness = sqrt(funding_percent / 100) or similar

   Tests need to verify the correct formula.

7. **Suppression power values:** What are the base suppression values for enforcers?
   - Standard enforcer: suppression_power = ?
   - Suppression radius = ?
   - Falloff type = linear? exponential?

### @population-engineer

8. **Medical coverage formula:** How does medical coverage translate to health_index and life_expectancy?
   - Is it additive? Multiplicative?
   - What's the max bonus from 100% coverage?
   - What's the penalty from 0% coverage?

### @economy-engineer

9. **Funding stub behavior:** The IEconomyQueryable stub returns 7% tribute rate. For service funding stubs, what default value should be used?
   - 100% funding (fully operational)?
   - 50% funding (neutral)?
   - 0% funding (disabled)?

   This affects test baselines when running without Epic 11.

---

## 6. Recommended Test Infrastructure

### 6.1 Mock Systems

```cpp
// MockServiceProvider.h
class MockServiceProvider : public IServiceQueryable {
public:
    void set_coverage(ServiceType type, PlayerID player, float value);
    void set_coverage_at(ServiceType type, GridPosition pos, float value);
    void set_effectiveness(ServiceType type, PlayerID player, float value);

    // IServiceQueryable implementation
    float get_coverage(ServiceType type, PlayerID player) const override;
    float get_coverage_at(ServiceType type, GridPosition pos) const override;
    float get_effectiveness(ServiceType type, PlayerID player) const override;

private:
    std::map<std::pair<ServiceType, PlayerID>, float> coverage_map_;
    std::map<std::pair<ServiceType, GridPosition>, float> position_coverage_map_;
    std::map<std::pair<ServiceType, PlayerID>, float> effectiveness_map_;
};
```

### 6.2 Test Fixtures

```cpp
// ServiceTestFixtures.h

// Creates a minimal service building setup
ServiceTestFixture create_single_enforcer_fixture(
    GridPosition position,
    PlayerID owner,
    float funding_percent = 100.0f
);

// Creates overlapping service coverage scenario
ServiceTestFixture create_overlapping_enforcers_fixture(
    std::vector<GridPosition> positions,
    PlayerID owner
);

// Creates global service (medical/education) scenario
ServiceTestFixture create_global_services_fixture(
    PlayerID owner,
    uint32_t medical_count,
    uint32_t education_count,
    float funding_percent = 100.0f
);

// Creates edge case scenarios
ServiceTestFixture create_map_edge_fixture(GridPosition corner);
ServiceTestFixture create_zero_funding_fixture();
ServiceTestFixture create_unpowered_service_fixture();
```

### 6.3 Test Utilities

```cpp
// ServiceTestUtils.h

// Coverage assertions
void assert_coverage_in_radius(
    const IServiceQueryable& services,
    ServiceType type,
    GridPosition center,
    uint32_t radius,
    float expected_coverage,
    float tolerance = 0.05f
);

void assert_coverage_falloff(
    const IServiceQueryable& services,
    ServiceType type,
    GridPosition center,
    uint32_t radius,
    FalloffType expected_falloff  // Linear, Exponential, Step
);

void assert_no_coverage_outside_radius(
    const IServiceQueryable& services,
    ServiceType type,
    GridPosition center,
    uint32_t radius
);

// Integration helpers
void simulate_ticks_with_services(
    SimulationCore& sim,
    ServicesSystem& services,
    DisorderSystem& disorder,
    uint32_t tick_count
);
```

### 6.4 Performance Test Harness

```cpp
// ServicePerformanceTests.cpp

// Query performance
PERFORMANCE_TEST(coverage_query_batch) {
    ServicesSystem services;
    // Setup: 100 service buildings

    BENCHMARK_START();
    for (int i = 0; i < 1000; ++i) {
        services.get_coverage_at(ServiceType::Enforcer, {rand() % 256, rand() % 256});
    }
    BENCHMARK_END();

    ASSERT_LESS_THAN(elapsed_ms, 1.0);  // < 1ms for 1000 queries
}

// Grid update performance
PERFORMANCE_TEST(coverage_grid_update) {
    ServicesSystem services;
    // Setup: 256x256 map, 50 service buildings

    BENCHMARK_START();
    services.recalculate_coverage();
    BENCHMARK_END();

    ASSERT_LESS_THAN(elapsed_ms, 5.0);  // < 5ms for full recalc
}
```

---

## 7. Test Implementation Priority

### Phase 1: Core Functionality (P0)

1. **IServiceQueryable interface tests** - Verify contract
2. **Coverage calculation unit tests** - Radius-based math
3. **Global service unit tests** - Aggregation logic
4. **Basic DisorderSystem integration** - Suppression works

### Phase 2: Edge Cases (P0)

5. **Map edge coverage tests**
6. **Overlapping coverage tests**
7. **Zero funding tests**
8. **Building lifecycle tests** (demolition, abandonment)

### Phase 3: Integration (P0)

9. **Full DisorderSystem integration**
10. **PopulationSystem health integration**
11. **Stub replacement verification**

### Phase 4: Performance (P1)

12. **Coverage query benchmarks**
13. **Grid update benchmarks**
14. **Memory usage tests**

### Phase 5: Polish (P1)

15. **Flakiness elimination pass**
16. **Test documentation**
17. **Coverage gap analysis**

---

## 8. Risk Assessment

### High Risk: Circular Dependency with Epic 10

**Risk:** ServicesSystem depends on building data, DisorderSystem depends on service coverage, and building state depends on disorder. Testing in isolation may miss integration bugs.

**Mitigation:**
- Write integration tests that run full tick cycles
- Use mock systems for isolated unit tests
- Add contract tests that verify interface behavior matches expected

### Medium Risk: Stub Replacement Behavior Change

**Risk:** Epic 10's stub returns 0.5 (neutral). Real ServicesSystem may return different values, causing simulation behavior changes.

**Mitigation:**
- Document expected behavior difference
- Write "behavior change" tests that verify intentional changes
- Ensure DisorderSystem handles both 0% and 100% coverage gracefully

### Medium Risk: Performance at Scale

**Risk:** Coverage calculations may be slow with many service buildings or large maps.

**Mitigation:**
- Performance tests with realistic building counts (50-100 service buildings)
- Profile coverage recalculation at 512x512
- Consider caching strategy if needed

### Low Risk: Coverage Grid Memory

**Risk:** Dense coverage grid may exceed memory budget on very large maps.

**Mitigation:**
- Memory tests at each map size
- Consider sparse representation if needed
- Document memory requirements

---

## 9. Test Data Requirements

### Test Maps

| Map | Description | Use Case |
|-----|-------------|----------|
| 32x32 Mini | Minimal test map | Fast unit tests |
| 128x128 Small | Standard test map | Integration tests |
| 256x256 Medium | Production-like | Performance tests |
| 512x512 Large | Stress testing | Memory/performance limits |

### Test Building Configurations

| Config | Buildings | Description |
|--------|-----------|-------------|
| Single Enforcer | 1 enforcer post | Basic coverage test |
| Overlapping | 3 enforcers, overlapping radii | Stacking test |
| Full Services | 2 enforcers, 1 hazard, 1 medical, 1 education | Complete service set |
| No Services | 0 service buildings | Stub behavior baseline |
| Map Edge | 1 enforcer at (0,0) | Edge case coverage |

---

## Appendix A: IServiceQueryable Contract Summary

From interfaces.yaml (stubs section):

```yaml
IServiceQueryable:
  methods:
    - get_coverage(ServiceType, PlayerID) -> float [0.0-1.0]
    - get_coverage_at(ServiceType, GridPosition) -> float [0.0-1.0]
    - get_effectiveness(ServiceType, PlayerID) -> float [0.0-1.0]

  stub_behavior:
    get_coverage: "Returns 0.5 (neutral baseline)"
    get_coverage_at: "Returns 0.5 (neutral baseline)"
    get_effectiveness: "Returns 1.0 (full effectiveness)"
```

**Real implementation requirements:**
- `get_coverage`: Returns overall coverage for service type (0.0-1.0)
- `get_coverage_at`: Returns coverage at specific tile (0.0-1.0)
- `get_effectiveness`: Returns funding-based effectiveness (0.0-1.0)

---

## Appendix B: Service Types from Canon

From interfaces.yaml (services section):

```yaml
service_types:
  enforcer: "Reduces disorder in radius"
  hazard_response: "Provides fire protection in radius"
  medical: "Improves longevity globally"
  education: "Improves knowledge quotient globally"
```

**Radius-based services:** enforcer, hazard_response
**Global services:** medical, education

---

*End of QA Engineer Analysis: Epic 9 - City Services*
