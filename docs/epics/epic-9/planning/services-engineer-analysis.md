# Services Engineer Analysis: Epic 9 - City Services

**Author:** Services Engineer Agent
**Date:** 2026-01-29
**Canon Version:** 0.10.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 9 implements city services that affect colony quality and simulation metrics. The ServicesSystem provides coverage calculations for two distinct service models:

1. **RADIUS-BASED services** (enforcer, hazard_response): Affect tiles within a coverage radius from service buildings
2. **GLOBAL services** (medical, education): Affect the entire colony based on service building presence and funding

The key technical challenge is efficient coverage calculation for radius-based services, as these directly reduce disorder (enforcers) and provide fire protection (hazard response). The system must integrate with Epic 10's DisorderSystem via the IServiceQueryable interface, replacing the stub implementation.

This epic is **dependency-light** compared to Epic 10:
- ServicesSystem calculates coverage; DisorderSystem uses it
- Coverage algorithms run once on building placement, not every tick
- Global services use simple aggregation, not spatial algorithms

Estimated scope: **15-25 work items** as requested.

---

## Table of Contents

1. [Technical Work Items](#1-technical-work-items)
2. [Coverage Algorithm Design](#2-coverage-algorithm-design)
3. [Data Structures](#3-data-structures)
4. [Performance Considerations](#4-performance-considerations)
5. [Questions for Other Agents](#5-questions-for-other-agents)
6. [Testing Strategy](#6-testing-strategy)

---

## 1. Technical Work Items

### Phase 1: Components and Interfaces

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| SVC-001 | ServiceType enum | Define enforcer, hazard_response, medical, education | S |
| SVC-002 | ServiceProviderComponent | Per-building service data (type, radius, effectiveness, funding_level) | S |
| SVC-003 | ServiceCoverageComponent | Per-tile cached coverage for radius-based services | S |
| SVC-004 | IServiceProvider interface implementation | Building-level interface from canon | M |
| SVC-005 | IServiceQueryable implementation | Replace Epic 10 stub with real implementation | M |

### Phase 2: Coverage Grid Infrastructure

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| SVC-006 | ServiceCoverageGrid class | Dense grid for enforcer/hazard coverage (2 bytes/tile) | M |
| SVC-007 | Multi-service overlay storage | Track coverage from multiple overlapping providers | M |
| SVC-008 | Dirty flag tracking | Per-player dirty flags for coverage recalculation | S |
| SVC-009 | Coverage recalculation trigger | Listen to building placed/removed events | S |

### Phase 3: Coverage Algorithms

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| SVC-010 | Radius-based coverage calculation | BFS/flood-fill for enforcer and hazard_response | L |
| SVC-011 | Distance falloff function | Configurable falloff (linear, quadratic, stepped) | M |
| SVC-012 | Coverage overlap handling | Max-value strategy for overlapping coverage zones | M |
| SVC-013 | Funding effectiveness modifier | Scale coverage by funding level (0.0-1.0) | S |

### Phase 4: Global Services

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| SVC-014 | Global medical coverage aggregation | Sum capacity, calculate colony-wide coverage | M |
| SVC-015 | Global education coverage aggregation | Sum capacity from learning centers, archives | M |
| SVC-016 | Capacity-per-population ratio | Calculate effectiveness based on population served | S |

### Phase 5: ServicesSystem Integration

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| SVC-017 | ServicesSystem class skeleton | ISimulatable implementation, tick ordering | M |
| SVC-018 | Building event handlers | Handle BuildingConstructedEvent, BuildingDemolishedEvent | M |
| SVC-019 | Funding level integration | Query EconomySystem for department funding (stub until Epic 11) | S |
| SVC-020 | Network sync for coverage data | Sync coverage state to clients | M |

### Phase 6: Service Building Types

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| SVC-021 | Enforcer post configuration | Radius, suppression power, cost, maintenance | S |
| SVC-022 | Hazard response post configuration | Radius, protection level, cost, maintenance | S |
| SVC-023 | Medical nexus configuration | Capacity, coverage, longevity bonus | S |
| SVC-024 | Learning center/archive configuration | Capacity, education quotient bonus | S |

### Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 10 | SVC-001, SVC-002, SVC-003, SVC-008, SVC-009, SVC-013, SVC-016, SVC-019, SVC-021, SVC-022, SVC-023, SVC-024 |
| M | 11 | SVC-004, SVC-005, SVC-006, SVC-007, SVC-011, SVC-012, SVC-014, SVC-015, SVC-017, SVC-018, SVC-020 |
| L | 1 | SVC-010 |
| **Total** | **22** | |

---

## 2. Coverage Algorithm Design

### 2.1 Two Coverage Models

Per canon, services use two distinct coverage models:

#### Model A: Radius-Based Coverage (enforcer, hazard_response)

These services affect tiles within a radius from the service building. Coverage strength decreases with distance.

```
+--------+--------+--------+--------+--------+
|   25   |   50   |   75   |   50   |   25   |
+--------+--------+--------+--------+--------+
|   50   |   75   |  100   |   75   |   50   |
+--------+--------+--------+--------+--------+
|   75   |  100   |[POST]  |  100   |   75   |
+--------+--------+--------+--------+--------+
|   50   |   75   |  100   |   75   |   50   |
+--------+--------+--------+--------+--------+
|   25   |   50   |   75   |   50   |   25   |
+--------+--------+--------+--------+--------+

Enforcer post with radius 2, 100% effectiveness
Values show coverage strength (0-255 scale)
```

#### Model B: Global Coverage (medical, education)

These services affect the entire colony with a single effectiveness value based on total capacity vs. population.

```
medical_effectiveness = medical_capacity / (population * BEINGS_PER_MEDICAL_UNIT)
education_effectiveness = education_capacity / (population * BEINGS_PER_EDUCATION_UNIT)

Clamped to 0.0-1.0 range
```

### 2.2 Radius-Based Coverage Algorithm

**Recommendation: Pre-computed Coverage Grid with Dirty Tracking**

Coverage is expensive to compute but changes infrequently (only when buildings placed/removed or funding changes significantly). Use the same pattern as EnergySystem (Epic 5).

```cpp
class ServiceCoverageGrid {
    // Per-service-type storage (enforcer has separate grid from hazard_response)
    struct ServiceGrid {
        uint8_t* coverage;        // Coverage strength per tile (0-255)
        uint8_t* provider_count;  // Number of overlapping providers (for debugging)
    };

    ServiceGrid enforcer_grid_;
    ServiceGrid hazard_grid_;

    bool dirty_[MAX_PLAYERS];

public:
    void mark_dirty(PlayerID owner) { dirty_[owner] = true; }

    void recalculate_if_dirty(PlayerID owner) {
        if (dirty_[owner]) {
            recalculate_coverage(owner, ServiceType::Enforcer, enforcer_grid_);
            recalculate_coverage(owner, ServiceType::HazardResponse, hazard_grid_);
            dirty_[owner] = false;
        }
    }

    uint8_t get_enforcer_coverage(int32_t x, int32_t y, PlayerID owner) const;
    uint8_t get_hazard_coverage(int32_t x, int32_t y, PlayerID owner) const;
};
```

### 2.3 Distance Falloff Options

Three falloff strategies to consider:

#### Option A: Linear Falloff (RECOMMENDED)

```
coverage = max_coverage * (1.0 - distance / radius)
```

**Pros:** Simple, intuitive, easy to balance
**Cons:** Sharp drop-off at edge

#### Option B: Quadratic Falloff

```
coverage = max_coverage * (1.0 - (distance / radius)^2)
```

**Pros:** More gradual center-to-edge transition
**Cons:** More complex, harder to tune

#### Option C: Stepped Falloff (SimCity 2000 style)

```
if distance == 0: coverage = 100%
elif distance <= radius/3: coverage = 80%
elif distance <= 2*radius/3: coverage = 50%
else: coverage = 20%
```

**Pros:** Classic feel, clear zones
**Cons:** Discrete jumps may feel artificial

**Recommendation:** Linear falloff for MVP. Data-driven so can be changed later.

### 2.4 Coverage Overlap Handling

When multiple service buildings have overlapping coverage zones:

#### Option A: Max Value (RECOMMENDED)

```
final_coverage = max(coverage_from_post_1, coverage_from_post_2, ...)
```

**Pros:** Simple, no over-powered stacking, predictable
**Cons:** Building close posts has diminishing returns

#### Option B: Additive (Capped)

```
final_coverage = min(255, sum(all_coverages))
```

**Pros:** Encourages building more posts
**Cons:** Can lead to overpowered stacking, harder to balance

#### Option C: Diminishing Returns

```
final_coverage = max_coverage * (1 - product(1 - coverage_i / max_coverage))
```

**Pros:** Realistic diminishing returns
**Cons:** Complex to understand, harder to communicate to players

**Recommendation:** Max-value for MVP. Simple and prevents coverage stacking exploits.

### 2.5 Coverage Algorithm Implementation

```cpp
void recalculate_coverage(PlayerID owner, ServiceType type, ServiceGrid& grid) {
    // Clear grid for this owner
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (get_tile_owner(x, y) == owner) {
                grid.coverage[y * width_ + x] = 0;
                grid.provider_count[y * width_ + x] = 0;
            }
        }
    }

    // Query all service providers of this type for this owner
    for (auto& [entity, service, pos] :
         registry_.view<ServiceProviderComponent, PositionComponent>().each()) {

        if (service.service_type != type) continue;
        if (get_owner(entity) != owner) continue;
        if (!is_service_active(entity)) continue;

        // Calculate coverage radius with funding modifier
        float effective_radius = service.base_radius * service.funding_effectiveness;
        uint8_t max_coverage = static_cast<uint8_t>(255 * service.funding_effectiveness);

        // Mark coverage in radius using chosen falloff
        mark_coverage_in_radius(
            grid,
            pos.grid_x, pos.grid_y,
            effective_radius,
            max_coverage,
            owner
        );
    }
}

void mark_coverage_in_radius(
    ServiceGrid& grid,
    int32_t cx, int32_t cy,
    float radius,
    uint8_t max_coverage,
    PlayerID owner
) {
    int32_t int_radius = static_cast<int32_t>(std::ceil(radius));

    for (int32_t dy = -int_radius; dy <= int_radius; ++dy) {
        for (int32_t dx = -int_radius; dx <= int_radius; ++dx) {
            int32_t x = cx + dx;
            int32_t y = cy + dy;

            if (!in_bounds(x, y)) continue;
            if (get_tile_owner(x, y) != owner) continue;  // Don't cover other players' tiles

            float distance = std::sqrt(dx*dx + dy*dy);
            if (distance > radius) continue;

            // Linear falloff
            float falloff = 1.0f - (distance / radius);
            uint8_t coverage = static_cast<uint8_t>(max_coverage * falloff);

            // Max-value overlap handling
            size_t idx = y * width_ + x;
            if (coverage > grid.coverage[idx]) {
                grid.coverage[idx] = coverage;
            }
            grid.provider_count[idx]++;
        }
    }
}
```

---

## 3. Data Structures

### 3.1 ServiceType Enum

```cpp
enum class ServiceType : uint8_t {
    Enforcer = 0,        // Reduces disorder in radius
    HazardResponse = 1,  // Provides fire protection in radius
    Medical = 2,         // Improves longevity globally
    Education = 3        // Improves knowledge quotient globally
};

constexpr bool is_radius_based(ServiceType type) {
    return type == ServiceType::Enforcer || type == ServiceType::HazardResponse;
}

constexpr bool is_global(ServiceType type) {
    return type == ServiceType::Medical || type == ServiceType::Education;
}
```

### 3.2 ServiceProviderComponent

```cpp
struct ServiceProviderComponent {
    ServiceType service_type;           // 1 byte: What type of service
    uint8_t base_radius;                // 1 byte: Base coverage radius (0 for global services)
    uint8_t base_effectiveness;         // 1 byte: Base effectiveness (0-255)
    uint8_t funding_level;              // 1 byte: Current funding (0-100%)
    uint16_t capacity;                  // 2 bytes: Beings served (for global services)
    uint16_t suppression_power;         // 2 bytes: For enforcers - disorder reduction strength
    float funding_effectiveness;        // 4 bytes: Computed: funding_level / 100.0
    // Total: 12 bytes
};
```

**Field Details:**

| Field | Purpose |
|-------|---------|
| service_type | Which service this building provides |
| base_radius | Radius-based services: tiles of coverage |
| base_effectiveness | Base coverage/effectiveness at 100% funding |
| funding_level | Department funding from EconomySystem (0-100) |
| capacity | Global services: how many beings can be served |
| suppression_power | Enforcers: how much disorder is reduced per tick |
| funding_effectiveness | Pre-computed multiplier for coverage calculations |

### 3.3 ServiceCoverageComponent (Optional)

For per-tile cached coverage when needed by other systems:

```cpp
struct ServiceCoverageComponent {
    uint8_t enforcer_coverage;          // 1 byte: Cached enforcer coverage at this tile
    uint8_t hazard_coverage;            // 1 byte: Cached hazard response coverage
    // Total: 2 bytes
};
```

This component may be attached to entities at specific positions for quick lookup, or we can rely entirely on the grid.

### 3.4 ServiceCoverageGrid

```cpp
class ServiceCoverageGrid {
public:
    static constexpr size_t BYTES_PER_TILE = 2;  // enforcer + hazard

private:
    uint32_t width_;
    uint32_t height_;

    // Per-service-type grids (could merge if memory is concern)
    std::vector<uint8_t> enforcer_coverage_;     // 1 byte/tile
    std::vector<uint8_t> hazard_coverage_;       // 1 byte/tile

    // Dirty tracking per player
    bool dirty_[MAX_PLAYERS];

public:
    // O(1) queries
    uint8_t get_enforcer_coverage(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return 0;
        return enforcer_coverage_[y * width_ + x];
    }

    uint8_t get_hazard_coverage(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return 0;
        return hazard_coverage_[y * width_ + x];
    }

    // Normalized query for IServiceQueryable (0.0-1.0)
    float get_coverage_at(ServiceType type, int32_t x, int32_t y) const {
        uint8_t raw;
        switch (type) {
            case ServiceType::Enforcer:
                raw = get_enforcer_coverage(x, y);
                break;
            case ServiceType::HazardResponse:
                raw = get_hazard_coverage(x, y);
                break;
            default:
                return 0.0f;  // Global services don't have per-tile coverage
        }
        return raw / 255.0f;
    }
};
```

**Memory Budget:**

| Map Size | Enforcer Grid | Hazard Grid | Total |
|----------|---------------|-------------|-------|
| 128x128 | 16 KB | 16 KB | 32 KB |
| 256x256 | 64 KB | 64 KB | 128 KB |
| 512x512 | 256 KB | 256 KB | 512 KB |

This aligns with the dense_grid_exception pattern from canon and is consistent with other grids (EnergyCoverageGrid, DisorderGrid).

### 3.5 Global Service State

```cpp
struct GlobalServiceState {
    // Medical
    uint32_t total_medical_capacity;
    float medical_effectiveness;         // 0.0-1.0
    float longevity_modifier;            // Multiplier for life expectancy

    // Education
    uint32_t total_education_capacity;
    float education_effectiveness;       // 0.0-1.0
    float knowledge_quotient;            // Modifier for education stat
};

// Per-player tracking
std::array<GlobalServiceState, MAX_PLAYERS> global_services_;
```

### 3.6 Service Building Configurations

```cpp
struct ServiceBuildingConfig {
    ServiceType type;
    uint8_t base_radius;          // 0 for global services
    uint8_t base_effectiveness;   // 0-255
    uint16_t capacity;            // Beings served (global) or 0
    uint16_t suppression_power;   // Disorder reduction (enforcer) or 0
    uint32_t build_cost;
    uint32_t maintenance_cost;
    const char* canonical_name;
};

const ServiceBuildingConfig SERVICE_CONFIGS[] = {
    // Radius-based services
    { Enforcer,       8, 200,    0, 80, 5000,  100, "enforcer_post" },
    { HazardResponse, 8, 200,    0,  0, 6000,  120, "hazard_response_post" },

    // Global services
    { Medical,        0, 255, 500,  0, 20000, 300, "medical_nexus" },
    { Education,      0, 255, 300,  0, 15000, 200, "learning_center" },
    { Education,      0, 180, 150,  0,  8000, 100, "archive" },
};
```

---

## 4. Performance Considerations

### 4.1 Coverage Recalculation Frequency

**Key Insight:** Coverage only changes when:
1. Service building placed or removed
2. Service building changes state (active/inactive)
3. Funding level changes significantly (crosses threshold)

**Optimization:** Use dirty flag tracking. Recalculate coverage at most once per tick, only for dirty players.

```cpp
void ServicesSystem::tick(float delta_time) {
    // Phase 1: Recalculate coverage for dirty players
    for (PlayerID i = 1; i <= num_players_; ++i) {
        if (coverage_dirty_[i]) {
            recalculate_radius_coverage(i);
            recalculate_global_services(i);
            coverage_dirty_[i] = false;
        }
    }

    // Phase 2: No other per-tick work needed
    // Coverage grid is queried by DisorderSystem during its tick
}
```

### 4.2 Query Performance

All coverage queries are O(1) via grid lookup:

```cpp
// Called by DisorderSystem every tick for every tile
float get_enforcer_coverage_at(int32_t x, int32_t y) {
    return coverage_grid_.get_coverage_at(ServiceType::Enforcer, x, y);
}
```

### 4.3 Service Building Iteration

Worst case: Full coverage recalculation iterates all service buildings.

| Scenario | Service Buildings | Tiles in Radius (r=8) | Total Iterations |
|----------|-------------------|----------------------|------------------|
| Small colony | ~10 | ~200 | ~2,000 |
| Medium colony | ~50 | ~200 | ~10,000 |
| Large colony | ~200 | ~200 | ~40,000 |

At 40,000 iterations with simple math per iteration, this is <1ms. Acceptable for occasional recalculation.

### 4.4 Memory Considerations

| Data Structure | 256x256 Map | 512x512 Map |
|----------------|-------------|-------------|
| ServiceCoverageGrid | 128 KB | 512 KB |
| Global service state (4 players) | ~256 bytes | ~256 bytes |
| ServiceProviderComponent (200 buildings) | 2.4 KB | 2.4 KB |
| **Total Epic 9** | ~130 KB | ~515 KB |

Combined with existing grids from other epics, total grid memory remains well under 10 MB.

### 4.5 Tick Priority

ServicesSystem should run **before** DisorderSystem so coverage is available when disorder calculates suppression.

**Proposed tick_priority: 65**

```
Priority 60: EconomySystem       -- funding levels available
Priority 65: ServicesSystem      <-- Epic 9 (NEW)
Priority 70: DisorderSystem      -- uses service coverage
Priority 80: ContaminationSystem
Priority 85: LandValueSystem
```

**Rationale:**
- Needs EconomySystem (60) for funding levels
- Must complete before DisorderSystem (70) reads coverage
- Doesn't depend on Disorder/Contamination/LandValue

---

## 5. Questions for Other Agents

### @systems-architect

1. **Coverage grid ownership:** Should the ServiceCoverageGrid be a member of ServicesSystem, or should it be a shared resource that DisorderSystem also owns a reference to? I propose ServicesSystem owns it and DisorderSystem queries via IServiceQueryable.

2. **Dense grid exception approval:** My analysis proposes a 2-byte/tile ServiceCoverageGrid (1 byte enforcer + 1 byte hazard). Should this be added to canon patterns.yaml under dense_grid_exception.applies_to?

3. **Tick priority 65:** I've proposed tick_priority 65 for ServicesSystem. This slots between EconomySystem (60) and DisorderSystem (70). Does this align with your system ordering analysis?

4. **Service coverage across ownership:** Per canon, energy conduits don't cross ownership boundaries. Should enforcer coverage also stop at ownership boundaries, or can an enforcer post reduce disorder in adjacent players' tiles? (I assume ownership boundary stops coverage.)

### @game-designer

5. **Falloff curve:** Should coverage use linear, quadratic, or stepped falloff? I've recommended linear for simplicity, but stepped (SimCity 2000 style) may feel more authentic. What's your preference?

6. **Overlap handling:** When multiple enforcer posts overlap, should coverage stack (additive up to cap) or take max value? I've recommended max-value to prevent stacking exploits. Does this match your game feel?

7. **Service building stats:** The configurations in Section 3.6 are placeholders. Can you provide canonical values for:
   - Enforcer post: radius, suppression_power, cost, maintenance
   - Hazard response post: radius, protection_level, cost, maintenance
   - Medical nexus: capacity (beings per building), longevity bonus
   - Learning center: capacity, education bonus
   - Archive: capacity, education bonus

8. **Funding effectiveness curve:** At 50% funding, should services operate at 50% effectiveness (linear), or should there be a non-linear curve (e.g., 70% funding for 50% effectiveness)? SimCity had significant dropoff at low funding.

9. **Recreation services (parks, zoos, stadiums):** The canon mentions recreation as part of Epic 9. Should these provide:
   - Land value bonus (like a passive aura)?
   - Harmony bonus (global modifier)?
   - Both?

   I've excluded them from current work items pending clarification.

### @population-engineer

10. **Medical coverage integration:** How should medical coverage affect PopulationSystem? I assume:
    - Query get_effectiveness(ServiceType::Medical, owner) -> float
    - Apply as multiplier to life expectancy and birth rate
    - Is this correct, or do you need per-tile medical coverage?

11. **Education coverage integration:** How should education coverage affect PopulationSystem? I assume:
    - Query get_effectiveness(ServiceType::Education, owner) -> float
    - Apply as modifier to knowledge_quotient
    - Affects labor participation rate and building desirability
    - Is this correct?

### @economy-engineer

12. **Department funding source:** How does ServicesSystem query funding levels? I assume:
    - EconomySystem tracks per-department funding sliders (0-100%)
    - ServicesSystem queries via IEconomyQueryable (stub returns 100% until Epic 11)
    - Is there a specific interface method for service department funding?

---

## 6. Testing Strategy

### 6.1 Unit Tests

#### Coverage Calculation Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_single_post_coverage | Single enforcer post marks correct tiles | P0 |
| test_coverage_falloff | Coverage decreases with distance (linear) | P0 |
| test_coverage_at_edge | Coverage at exact radius boundary | P0 |
| test_coverage_outside_radius | No coverage outside radius | P0 |
| test_ownership_boundary | Coverage stops at ownership boundary | P0 |
| test_multiple_posts_max | Overlapping posts use max, not sum | P0 |
| test_funding_modifier | 50% funding = 50% coverage | P0 |
| test_inactive_post | Unpowered post provides no coverage | P1 |

#### Global Service Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_medical_single_building | Single medical nexus provides correct coverage | P0 |
| test_medical_multiple_buildings | Multiple medical buildings aggregate capacity | P0 |
| test_medical_effectiveness_ratio | Effectiveness scales with population | P0 |
| test_education_aggregation | Learning centers + archives aggregate | P0 |
| test_funding_effect_global | Funding affects global service effectiveness | P0 |

#### Integration Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_building_placed_updates_coverage | BuildingConstructedEvent triggers recalculation | P0 |
| test_building_removed_updates_coverage | BuildingDemolishedEvent triggers recalculation | P0 |
| test_disorder_suppression_integration | DisorderSystem correctly queries enforcer coverage | P0 |
| test_population_health_integration | PopulationSystem correctly queries medical coverage | P1 |

### 6.2 Edge Cases to Test

#### Spatial Edge Cases

- **Corner posts:** Enforcer post at map corner (only covers 1/4 of radius)
- **Adjacent to water:** Coverage radius extends over water? (Propose: yes, water doesn't block coverage)
- **Adjacent to enemy territory:** Coverage stops at boundary
- **Overlapping 3+ posts:** Verify max-value, not cumulative

#### Funding Edge Cases

- **Zero funding:** Services provide zero coverage/effectiveness
- **100% funding:** Services at full effectiveness
- **Funding changes mid-tick:** Coverage updated on next tick, not immediately

#### Building State Edge Cases

- **Unpowered service building:** No coverage until powered
- **Abandoned service building:** No coverage (building inactive)
- **Under-construction building:** No coverage until complete

### 6.3 Performance Tests

| Test | Target | Priority |
|------|--------|----------|
| test_coverage_recalc_small_map | <1ms for 128x128, 20 posts | P1 |
| test_coverage_recalc_large_map | <5ms for 512x512, 100 posts | P1 |
| test_query_performance | O(1) grid lookup | P1 |
| test_no_recalc_when_clean | Skip recalc if not dirty | P1 |

### 6.4 Disorder Suppression Integration Test

```cpp
TEST(ServiceDisorderIntegration, EnforcerReducesDisorder) {
    // Setup: High disorder area
    disorder_grid_.set_level(5, 5, 200);

    // Place enforcer post at (5, 5) with radius 3
    EntityID post = create_enforcer_post(5, 5, player_1);
    services_system_.tick(0.05f);  // Recalculate coverage

    // Verify coverage exists
    EXPECT_GT(services_.get_coverage_at(ServiceType::Enforcer, 5, 5), 0.9f);
    EXPECT_GT(services_.get_coverage_at(ServiceType::Enforcer, 7, 5), 0.3f);  // Edge of radius
    EXPECT_EQ(services_.get_coverage_at(ServiceType::Enforcer, 10, 5), 0.0f); // Outside radius

    // Run disorder system tick - should apply suppression
    disorder_system_.tick(0.05f);

    // Verify disorder reduced at covered tiles
    EXPECT_LT(disorder_grid_.get_level(5, 5), 200);
}
```

---

## Appendix A: IServiceProvider Interface (from Canon)

```cpp
// Per interfaces.yaml
class IServiceProvider {
public:
    virtual ~IServiceProvider() = default;

    // What type of service (enforcer, hazard_response, medical, education)
    virtual ServiceType get_service_type() const = 0;

    // Radius of effect in tiles (0 for global services)
    virtual uint32_t get_coverage_radius() const = 0;

    // Current effectiveness (0.0-1.0) based on funding
    virtual float get_effectiveness() const = 0;

    // Service coverage strength at position (0.0-1.0)
    virtual float get_coverage_at(GridPosition position) const = 0;
};
```

**Note:** Buildings implement IServiceProvider. ServicesSystem aggregates all providers.

---

## Appendix B: IServiceQueryable Interface (Replaces Epic 10 Stub)

```cpp
// Per interfaces.yaml (stubs section)
class IServiceQueryable {
public:
    virtual ~IServiceQueryable() = default;

    // Overall coverage for service type (0.0-1.0)
    // For radius-based: average coverage across owned tiles
    // For global: effectiveness based on capacity/population
    virtual float get_coverage(ServiceType service_type, PlayerID owner) const = 0;

    // Service coverage at specific position (0.0-1.0)
    // For radius-based: coverage from grid
    // For global: same as get_coverage (position doesn't matter)
    virtual float get_coverage_at(ServiceType service_type, GridPosition position) const = 0;

    // Service effectiveness based on funding (0.0-1.0)
    virtual float get_effectiveness(ServiceType service_type, PlayerID owner) const = 0;
};
```

**Epic 10 Stub Behavior (to be replaced):**
- get_coverage() returns 0.5 (neutral baseline)
- get_coverage_at() returns 0.5 (neutral baseline)
- get_effectiveness() returns 1.0 (full effectiveness)

**Epic 9 Real Implementation:**
- get_coverage() returns actual aggregate coverage
- get_coverage_at() returns grid value for radius-based, aggregate for global
- get_effectiveness() returns funding-based effectiveness

---

## Appendix C: Canonical Terminology Reference

| Human Term | Canonical (Alien) Term |
|------------|------------------------|
| Police | Enforcers |
| Police station | Enforcer post |
| Fire department | Hazard response |
| Fire station | Hazard response post |
| Hospital | Medical nexus |
| Clinic | Medical post |
| Health | Vitality |
| Life expectancy | Longevity |
| School | Learning center |
| College | Advanced learning center |
| Library | Archive |
| Museum | Heritage vault |
| Education | Knowledge quotient |
| Park | Green zone |
| Stadium | Arena |
| Zoo | Creature preserve |

---

## Appendix D: Service Building Summary

| Building | Type | Coverage Model | Key Metric |
|----------|------|----------------|------------|
| Enforcer Post | Enforcer | Radius (8 tiles) | Suppression power |
| Hazard Response Post | HazardResponse | Radius (8 tiles) | Protection level |
| Medical Nexus | Medical | Global | Capacity (beings) |
| Learning Center | Education | Global | Capacity (beings) |
| Archive | Education | Global | Capacity (beings) |
| Heritage Vault | Education | Global | Capacity (beings) |
| Green Zone (Small) | Recreation | TBD | Land value bonus |
| Green Zone (Large) | Recreation | TBD | Land value bonus |
| Creature Preserve | Recreation | TBD | Land value + harmony |
| Arena | Recreation | TBD | Harmony bonus |

---

**End of Services Engineer Analysis: Epic 9 - City Services**
