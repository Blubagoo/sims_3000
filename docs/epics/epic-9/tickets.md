# Epic 9: City Services - Tickets

**Epic:** 9 - City Services
**Date:** 2026-01-29
**Canon Version:** 0.13.0
**Discussion:** [epic-9-planning.discussion.yaml](../../discussions/epic-9-planning.discussion.yaml)

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.10.0 | Initial ticket creation |
| 2026-01-29 | canon-verification (v0.10.0 → v0.13.0) | No changes required |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. IServiceQueryable interface (ticket E9-004) matches interfaces.yaml. ServiceCoverageGrid in dense_grid_exception pattern. Tick priority 55 confirmed in systems.yaml.

---

## Epic Overview

Epic 9 implements the ServicesSystem that provides service coverage to colonies. This is an **integration epic** that bridges building placement (Epic 4) with simulation systems (Epic 10) by replacing the IServiceQueryable stub with real coverage calculations.

**Key Characteristics:**
- Two service models: radius-based (enforcer, hazard) and global (medical, education)
- Coverage grid storage for efficient per-tile queries
- Per-player service boundaries (no cross-ownership coverage)
- Integrates with DisorderSystem, PopulationSystem, LandValueSystem

**Scope Target:** ~30 tickets (focused, not granular)

---

## Ticket Summary

| ID | Title | Size | Priority | Phase |
|----|-------|------|----------|-------|
| **Phase 1: Core Infrastructure** |
| E9-001 | ServiceType enum and configuration data | S | P0 | 1 |
| E9-002 | ServiceProviderComponent definition | S | P0 | 1 |
| E9-003 | ServicesSystem class skeleton | M | P0 | 1 |
| E9-004 | IServiceQueryable real implementation | M | P0 | 1 |
| **Phase 2: Coverage Grid** |
| E9-010 | ServiceCoverageGrid class | M | P0 | 2 |
| E9-011 | Dirty flag and recalculation triggers | S | P0 | 2 |
| E9-012 | Building event handlers | M | P0 | 2 |
| **Phase 3: Coverage Algorithms** |
| E9-020 | Radius-based coverage calculation | L | P0 | 3 |
| E9-021 | Linear distance falloff | S | P0 | 3 |
| E9-022 | Coverage overlap handling (max-value) | S | P0 | 3 |
| E9-023 | Global service aggregation | M | P0 | 3 |
| E9-024 | Funding effectiveness modifier | S | P0 | 3 |
| **Phase 4: Service Building Types** |
| E9-030 | Enforcer post/station/nexus config | S | P0 | 4 |
| E9-031 | Hazard response post/station/nexus config | S | P0 | 4 |
| E9-032 | Medical post/center/nexus config | S | P0 | 4 |
| E9-033 | Learning center/archive/nexus config | S | P0 | 4 |
| **Phase 5: Epic 10 Integration** |
| E9-040 | DisorderSystem enforcer integration | M | P0 | 5 |
| E9-041 | PopulationSystem medical integration | M | P0 | 5 |
| E9-042 | LandValueSystem education bonus | S | P1 | 5 |
| E9-043 | Service coverage overlays (IGridOverlay) | M | P1 | 5 |
| **Phase 6: Testing & Polish** |
| E9-050 | Unit tests: coverage calculation | L | P1 | 6 |
| E9-051 | Unit tests: global services | M | P1 | 6 |
| E9-052 | Integration tests: Epic 10 systems | L | P1 | 6 |
| E9-053 | Service statistics (IStatQueryable) | S | P1 | 6 |

**Summary:** 24 tickets (12 S, 8 M, 4 L) — 17 P0, 7 P1

---

## Detailed Tickets

---

### Phase 1: Core Infrastructure

---

#### E9-001: ServiceType enum and configuration data

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define ServiceType enum and per-type configuration data for all service types.

**Acceptance Criteria:**
- [ ] ServiceType enum: enforcer, hazard_response, medical, education
- [ ] ServiceConfig struct: base_radius, base_effectiveness, capacity, footprint_size
- [ ] Default configs for each service type (3 tiers each)
- [ ] Configs loadable from data file (future)

**Technical Notes:**
```cpp
enum class ServiceType : uint8_t {
    Enforcer,
    HazardResponse,
    Medical,
    Education
};

struct ServiceConfig {
    uint8_t base_radius;        // 0 for global services
    uint8_t base_effectiveness; // 0-100
    uint16_t capacity;          // For global services
    uint8_t footprint_width;    // 1, 2, or 3
    uint8_t footprint_height;
};
```

---

#### E9-002: ServiceProviderComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** E9-001

**Description:**
Define the component attached to service buildings.

**Acceptance Criteria:**
- [ ] ServiceProviderComponent with: service_type, tier, current_effectiveness, is_active
- [ ] Implements IServiceProvider interface
- [ ] Pure data, no methods (ECS pattern)
- [ ] Size: ~8 bytes

**Technical Notes:**
```cpp
struct ServiceProviderComponent {
    ServiceType service_type;
    uint8_t tier;                   // 1=post, 2=station, 3=nexus
    uint8_t current_effectiveness;  // 0-255, calculated from funding
    bool is_active;                 // False if unpowered
};
```

---

#### E9-003: ServicesSystem class skeleton

**Size:** M | **Priority:** P0 | **Dependencies:** E9-001, E9-002

**Description:**
Create the ServicesSystem class implementing ISimulatable with tick_priority 55.

**Acceptance Criteria:**
- [ ] ServicesSystem implements ISimulatable
- [ ] tick_priority: 55 (after PopulationSystem 50, before EconomySystem 60)
- [ ] tick() method stub
- [ ] Holds per-player ServiceCoverageGrid instances
- [ ] Registers with SimulationCore

**Technical Notes:**
- System is server-authoritative
- One coverage grid per service type per player (4 types * 4 players max)

---

#### E9-004: IServiceQueryable real implementation

**Size:** M | **Priority:** P0 | **Dependencies:** E9-003

**Description:**
Implement the real IServiceQueryable interface, replacing Epic 10's stub.

**Acceptance Criteria:**
- [ ] get_coverage(ServiceType, PlayerID) -> float returns actual coverage
- [ ] get_coverage_at(ServiceType, GridPosition) -> float returns tile coverage
- [ ] get_effectiveness(ServiceType, PlayerID) -> float returns funding-adjusted effectiveness
- [ ] Interface contract matches Epic 10 stub exactly
- [ ] Stub fallback when no service buildings exist (return 0.0, not 0.5)

**Canon Reference:** interfaces.yaml - IServiceQueryable

---

### Phase 2: Coverage Grid

---

#### E9-010: ServiceCoverageGrid class

**Size:** M | **Priority:** P0 | **Dependencies:** E9-003

**Description:**
Implement dense grid storage for radius-based service coverage.

**Acceptance Criteria:**
- [ ] ServiceCoverageGrid class with 1 byte per tile
- [ ] get_coverage_at(x, y) -> uint8_t (0-255)
- [ ] get_coverage_at_normalized(x, y) -> float (0.0-1.0)
- [ ] clear() and set_coverage_at(x, y, value)
- [ ] Follows dense_grid_exception pattern

**Technical Notes:**
- Memory: 64KB per 256x256 grid
- Only create grids for service types player has buildings for (lazy allocation)

---

#### E9-011: Dirty flag and recalculation triggers

**Size:** S | **Priority:** P0 | **Dependencies:** E9-010

**Description:**
Implement dirty tracking to avoid recalculating coverage every tick.

**Acceptance Criteria:**
- [ ] dirty_ flag per coverage grid
- [ ] mark_dirty() called when buildings added/removed/funding changed
- [ ] mark_clean() called after recalculation
- [ ] tick() only recalculates dirty grids

---

#### E9-012: Building event handlers

**Size:** M | **Priority:** P0 | **Dependencies:** E9-003

**Description:**
Listen for building events to track service buildings.

**Acceptance Criteria:**
- [ ] Subscribe to BuildingConstructedEvent
- [ ] Subscribe to BuildingDemolishedEvent
- [ ] Subscribe to BuildingPowerStateChangedEvent
- [ ] Maintain per-player list of service building entities
- [ ] Mark coverage grids dirty on any change

---

### Phase 3: Coverage Algorithms

---

#### E9-020: Radius-based coverage calculation

**Size:** L | **Priority:** P0 | **Dependencies:** E9-010, E9-012

**Description:**
Implement the core coverage calculation for radius-based services (enforcer, hazard_response).

**Acceptance Criteria:**
- [ ] For each service building, calculate coverage at all tiles within radius
- [ ] Skip tiles not owned by building owner
- [ ] Skip tiles outside map bounds (no wraparound)
- [ ] Apply coverage to ServiceCoverageGrid
- [ ] Clear grid before recalculating

**Algorithm:**
```
For each service building B of type T owned by player P:
    if B is not active: continue
    radius = get_config(T, B.tier).base_radius
    effectiveness = B.current_effectiveness / 255.0

    For each tile (x,y) within radius of B.position:
        if tile not owned by P: continue
        distance = manhattan_distance(B.position, (x,y))
        if distance > radius: continue

        strength = calculate_falloff(effectiveness, distance, radius)
        apply_coverage(T, P, x, y, strength)
```

---

#### E9-021: Linear distance falloff

**Size:** S | **Priority:** P0 | **Dependencies:** E9-020

**Description:**
Implement linear falloff for coverage strength based on distance.

**Acceptance Criteria:**
- [ ] falloff = 1.0 - (distance / radius)
- [ ] strength = max_effectiveness * falloff
- [ ] At building position: 100% strength
- [ ] At radius edge: 0% strength
- [ ] Beyond radius: 0% (handled by radius check)

---

#### E9-022: Coverage overlap handling (max-value)

**Size:** S | **Priority:** P0 | **Dependencies:** E9-020

**Description:**
When multiple service buildings cover the same tile, take the maximum value.

**Acceptance Criteria:**
- [ ] When applying coverage, check existing value
- [ ] new_value = max(existing_value, calculated_value)
- [ ] No stacking (prevents "pile enforcers" exploit)

---

#### E9-023: Global service aggregation

**Size:** M | **Priority:** P0 | **Dependencies:** E9-003, E9-012

**Description:**
Implement capacity-based effectiveness for global services (medical, education).

**Acceptance Criteria:**
- [ ] Sum capacity from all medical buildings for player
- [ ] Sum capacity from all education buildings for player
- [ ] Calculate effectiveness: capacity / (population * BEINGS_PER_UNIT)
- [ ] Clamp to 0.0-1.0
- [ ] No coverage grid needed (single value per player)

**Constants:**
- BEINGS_PER_MEDICAL_UNIT: 500
- BEINGS_PER_EDUCATION_UNIT: 300

---

#### E9-024: Funding effectiveness modifier

**Size:** S | **Priority:** P0 | **Dependencies:** E9-003

**Description:**
Scale service effectiveness based on funding level from EconomySystem.

**Acceptance Criteria:**
- [ ] Query IEconomyQueryable for funding level (stub returns 100%)
- [ ] Apply funding modifier: effectiveness = base * funding_modifier
- [ ] Funding curve: linear, capped at 115% for 150% funding
- [ ] 0% funding = 0% effectiveness (building disabled)
- [ ] Update effectiveness on funding change events

---

### Phase 4: Service Building Types

---

#### E9-030: Enforcer post/station/nexus config

**Size:** S | **Priority:** P0 | **Dependencies:** E9-001

**Description:**
Define configuration for enforcer service buildings.

**Acceptance Criteria:**
- [ ] Enforcer Post: radius=8, footprint=1x1
- [ ] Enforcer Station: radius=12, footprint=2x2
- [ ] Enforcer Nexus: radius=16, footprint=3x3
- [ ] Suppression multiplier: 0.7 (70% max disorder reduction)
- [ ] All require power to function

---

#### E9-031: Hazard response post/station/nexus config

**Size:** S | **Priority:** P0 | **Dependencies:** E9-001

**Description:**
Define configuration for hazard response service buildings.

**Acceptance Criteria:**
- [ ] Hazard Post: radius=10, footprint=1x1
- [ ] Hazard Station: radius=15, footprint=2x2
- [ ] Hazard Nexus: radius=20, footprint=3x3
- [ ] Fire suppression speed: 3x faster in coverage
- [ ] All require power to function

---

#### E9-032: Medical post/center/nexus config

**Size:** S | **Priority:** P0 | **Dependencies:** E9-001

**Description:**
Define configuration for medical service buildings (global effect).

**Acceptance Criteria:**
- [ ] Medical Post: capacity=500, footprint=1x1
- [ ] Medical Center: capacity=2000, footprint=2x2
- [ ] Medical Nexus: capacity=5000, footprint=3x3
- [ ] Longevity bonus: base 60 cycles + up to 40 cycles at 100% coverage
- [ ] All require power to function

---

#### E9-033: Learning center/archive/nexus config

**Size:** S | **Priority:** P0 | **Dependencies:** E9-001

**Description:**
Define configuration for education service buildings (global effect).

**Acceptance Criteria:**
- [ ] Learning Center: capacity=300, footprint=1x1
- [ ] Archive: capacity=1200, footprint=2x2
- [ ] Knowledge Nexus: capacity=3000, footprint=3x3
- [ ] Education bonus: knowledge quotient improvement
- [ ] All require power to function

---

### Phase 5: Epic 10 Integration

---

#### E9-040: DisorderSystem enforcer integration

**Size:** M | **Priority:** P0 | **Dependencies:** E9-004, E9-020

**Description:**
Update Epic 10's DisorderSystem to use real enforcer coverage for disorder suppression.

**Acceptance Criteria:**
- [ ] DisorderSystem queries IServiceQueryable.get_coverage_at(Enforcer, position)
- [ ] Apply suppression: disorder_generation *= (1 - coverage * 0.7)
- [ ] At 100% coverage: 70% reduction in disorder generation
- [ ] At 0% coverage: no reduction
- [ ] Verify behavior matches stub transition expectations

**Canon Reference:** interfaces.yaml - IServiceQueryable

---

#### E9-041: PopulationSystem medical integration

**Size:** M | **Priority:** P0 | **Dependencies:** E9-004, E9-023

**Description:**
Update Epic 10's PopulationSystem to use real medical coverage for longevity.

**Acceptance Criteria:**
- [ ] PopulationSystem queries IServiceQueryable.get_coverage(Medical, player)
- [ ] Apply longevity bonus: longevity = 60 + (coverage * 40)
- [ ] At 100% coverage: 100 cycles longevity
- [ ] At 0% coverage: 60 cycles longevity
- [ ] Verify behavior matches stub transition expectations

---

#### E9-042: LandValueSystem education bonus

**Size:** S | **Priority:** P1 | **Dependencies:** E9-004, E9-023

**Description:**
Add education coverage bonus to land value calculation.

**Acceptance Criteria:**
- [ ] LandValueSystem queries IServiceQueryable.get_coverage(Education, player)
- [ ] Apply land value bonus: +10% at 100% education coverage
- [ ] Bonus applies colony-wide (not per-tile for education)

---

#### E9-043: Service coverage overlays (IGridOverlay)

**Size:** M | **Priority:** P1 | **Dependencies:** E9-010

**Description:**
Implement IGridOverlay for service coverage visualization.

**Acceptance Criteria:**
- [ ] EnforcerCoverageOverlay implements IGridOverlay
- [ ] HazardCoverageOverlay implements IGridOverlay
- [ ] Color scheme: enforcer=cyan/blue, hazard=amber/orange
- [ ] UISystem can request overlays for rendering

---

### Phase 6: Testing & Polish

---

#### E9-050: Unit tests: coverage calculation

**Size:** L | **Priority:** P1 | **Dependencies:** E9-020, E9-021, E9-022

**Description:**
Comprehensive unit tests for radius-based coverage.

**Acceptance Criteria:**
- [ ] Test single building coverage pattern
- [ ] Test linear falloff values at distances 0, half-radius, radius
- [ ] Test map edge clipping (no wraparound)
- [ ] Test ownership boundary (only own tiles covered)
- [ ] Test overlap handling (max-value)
- [ ] Test unpowered building (0 coverage)

---

#### E9-051: Unit tests: global services

**Size:** M | **Priority:** P1 | **Dependencies:** E9-023

**Description:**
Unit tests for global service aggregation.

**Acceptance Criteria:**
- [ ] Test single medical building effectiveness
- [ ] Test multiple buildings capacity stacking
- [ ] Test population scaling (effectiveness decreases with population)
- [ ] Test funding modifier application
- [ ] Test zero buildings returns 0.0

---

#### E9-052: Integration tests: Epic 10 systems

**Size:** L | **Priority:** P1 | **Dependencies:** E9-040, E9-041

**Description:**
End-to-end integration tests with Epic 10 simulation systems.

**Acceptance Criteria:**
- [ ] Test: Place enforcer, verify disorder reduction in coverage area
- [ ] Test: Remove enforcer, verify disorder returns to normal
- [ ] Test: Place medical, verify longevity increase
- [ ] Test: Verify stub replacement behavior (0.0 not 0.5 for no buildings)
- [ ] Test: Full tick cycle with services active

---

#### E9-053: Service statistics (IStatQueryable)

**Size:** S | **Priority:** P1 | **Dependencies:** E9-003

**Description:**
Expose service statistics for UI display.

**Acceptance Criteria:**
- [ ] get_service_building_count(ServiceType, PlayerID)
- [ ] get_average_coverage(ServiceType, PlayerID)
- [ ] get_total_capacity(ServiceType, PlayerID)
- [ ] get_service_effectiveness(ServiceType, PlayerID)

---

## Critical Path

```
E9-001 (ServiceType enum)
    |
    +-- E9-002 (Component)
    |       |
    |       +-- E9-003 (System skeleton)
    |               |
    |               +-- E9-004 (IServiceQueryable)
    |               |
    |               +-- E9-010 (CoverageGrid)
    |               |       |
    |               |       +-- E9-020 (Coverage calculation)
    |               |               |
    |               |               +-- E9-040 (DisorderSystem integration)
    |               |
    |               +-- E9-023 (Global aggregation)
    |                       |
    |                       +-- E9-041 (PopulationSystem integration)
```

**MVP:** E9-001 through E9-041 (enforcer + medical working)
**Full Epic:** All 24 tickets

---

## Dependencies

### Depends On (Epic 9 needs these)
- **Epic 4:** BuildingSystem for service building placement
- **Epic 5:** EnergySystem for power dependency
- **Epic 10:** DisorderSystem, PopulationSystem, LandValueSystem for integration

### Depended On By (These need Epic 9)
- **Epic 11:** EconomySystem will provide real funding levels
- **Epic 12:** UISystem will display coverage overlays
- **Epic 13:** DisasterSystem will use hazard_response coverage

---

## Canon Updates Required

After Epic 9 implementation:

### interfaces.yaml
- Add ServicesSystem tick_priority: 55 to ISimulatable
- Move IServiceQueryable from stubs to services section

### patterns.yaml
- Add ServiceCoverageGrid to dense_grid_exception.applies_to

### systems.yaml
- Add tick_priority: 55 to ServicesSystem
- Add note: replaces Epic 10 IServiceQueryable stub
