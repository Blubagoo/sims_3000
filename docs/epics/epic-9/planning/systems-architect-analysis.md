# Systems Architect Analysis: Epic 9 - City Services

**Author:** Systems Architect Agent
**Date:** 2026-01-29
**Canon Version:** 0.10.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 9 implements the **City Services** system - service buildings that provide coverage areas affecting simulation outcomes. This epic is the **integration bridge** between building placement (Epic 4) and simulation systems (Epic 10).

The critical architectural task is implementing the **real IServiceQueryable interface** to replace Epic 10's stub that currently returns neutral values (0.5 coverage everywhere). Once Epic 9 is complete, disorder suppression, medical effects on life expectancy, and education bonuses will become functional.

Key architectural characteristics:
1. **Single-purpose system:** ServicesSystem owns coverage calculation, nothing else
2. **Clean boundaries:** Does NOT simulate disorder/contamination (Epic 10 owns that)
3. **Interface-driven integration:** Replaces IServiceQueryable stub with real implementation
4. **Per-player service pools:** Each player's service buildings serve only their own tiles

---

## Key Work Items

### Phase 1: Core Infrastructure (5 items)

- [ ] **E9-001: ServiceType enum and ServiceConfig data** (S)
  Define ServiceType enum (enforcer, hazard_response, medical, education) and per-type configuration (base_radius, base_effectiveness, cost, maintenance).

- [ ] **E9-002: ServiceProviderComponent definition** (S)
  Component for service buildings: service_type, base_radius, current_radius, base_effectiveness, current_effectiveness, funding_level.

- [ ] **E9-003: ServiceCoverageGrid implementation** (M)
  Dense 1-byte grid per service type per player. Stores coverage strength (0-255) at each tile. Uses the dense_grid_exception pattern.

- [ ] **E9-004: ServicesSystem class skeleton** (M)
  Implement ISimulatable at tick_priority 55 (after PopulationSystem 50, before EconomySystem 60). Owns ServiceCoverageGrid instances.

- [ ] **E9-005: IServiceQueryable real implementation** (M)
  Replace Epic 10's stub with real implementation. get_coverage(), get_coverage_at(), get_effectiveness() return actual calculated values.

### Phase 2: Coverage Calculation (4 items)

- [ ] **E9-006: Service building registration** (M)
  Listen for BuildingConstructedEvent/BuildingDemolishedEvent to track service buildings. Maintain per-player service building lists.

- [ ] **E9-007: Coverage radius calculation** (M)
  Calculate effective coverage radius: base_radius * funding_modifier. Funding comes from IEconomyQueryable (stub returns 100% until Epic 11).

- [ ] **E9-008: Coverage strength calculation with falloff** (L)
  For each service building, calculate coverage strength at each tile in radius. Linear falloff: strength = effectiveness * (1 - distance/radius). Aggregate overlapping coverage (capped at 1.0).

- [ ] **E9-009: Coverage grid update cycle** (M)
  Clear and recalculate coverage grids each tick. Consider dirty-region optimization for performance.

### Phase 3: Service Building Types (4 items)

- [ ] **E9-010: Enforcer post service building** (M)
  Reduces disorder in radius. Standard config: radius 8 tiles, effectiveness 80%, provides coverage to DisorderSystem.

- [ ] **E9-011: Hazard response post service building** (M)
  Provides fire protection in radius. Standard config: radius 10 tiles, effectiveness 75%. Feeds into future DisasterSystem (Epic 13).

- [ ] **E9-012: Medical nexus service building** (M)
  Improves life expectancy. Standard config: radius 12 tiles, effectiveness 70%. PopulationSystem queries for life expectancy calculation.

- [ ] **E9-013: Learning center / Archive service buildings** (M)
  Improves education index. Standard config: radius 15 tiles, effectiveness 65%. Affects demand factors and land value.

### Phase 4: Integration (4 items)

- [ ] **E9-014: DisorderSystem enforcer integration** (M)
  Update Epic 10's DisorderSystem to use real IServiceQueryable.get_coverage_at(ServiceType::Enforcer, position) for suppression calculation.

- [ ] **E9-015: PopulationSystem medical integration** (S)
  Update Epic 10's PopulationSystem to use real IServiceQueryable for medical coverage in life expectancy and health index calculations.

- [ ] **E9-016: LandValueSystem service coverage bonus** (S)
  Add service coverage bonus to land value calculation. Education and medical coverage increase sector value.

- [ ] **E9-017: Service coverage overlay for UI** (S)
  Implement IGridOverlay for each service type. UISystem can display coverage maps.

### Phase 5: Testing & Polish (3 items)

- [ ] **E9-018: Unit tests for coverage calculation** (L)
  Test radius calculation, falloff, aggregation, grid updates.

- [ ] **E9-019: Integration test with Epic 10 systems** (L)
  End-to-end test: place service buildings, verify disorder suppression works, verify life expectancy improves.

- [ ] **E9-020: Service statistics for IStatQueryable** (S)
  Expose coverage percentages, service counts, effectiveness stats.

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 6 | E9-001, E9-002, E9-015, E9-016, E9-017, E9-020 |
| M | 10 | E9-003, E9-004, E9-005, E9-006, E9-007, E9-009, E9-010, E9-011, E9-012, E9-013 |
| L | 4 | E9-008, E9-014, E9-018, E9-019 |
| **Total** | **20** | |

---

## Questions for Other Agents

### @game-designer

1. **Coverage radius vs building cost:** Should larger service buildings cost more but have bigger radius, or should radius be upgradeable via funding? (Current assumption: funding affects effectiveness, not radius)

2. **Overlapping coverage behavior:** When two enforcer posts overlap, should coverage be:
   - (A) Max of the two values (simpler)
   - (B) Additive with cap (more buildings = better)
   - (C) Diminishing returns formula

   Current assumption: (B) additive with cap at 1.0.

3. **Service building activation requirements:** Do service buildings need power AND water to function, or just power? (Current assumption: power only, like SimCity 2000)

4. **Education vs Medical global effects:** Canon says medical and education improve metrics "globally" - does this mean:
   - (A) Average coverage across all owned tiles
   - (B) Per-tile coverage affecting nearby habitation
   - (C) Threshold system (need X% coverage citywide)

   Current assumption: (B) per-tile coverage.

### @services-engineer

5. **Enforcer suppression formula:** What's the relationship between coverage strength and disorder reduction? Linear (coverage 0.5 = 50% reduction) or curved?

6. **Hazard response integration:** Since DisasterSystem is Epic 13, how should hazard_response coverage be used now? Store for future use, or does it affect anything in Epic 10?

7. **Service building maintenance costs:** Should maintenance scale with coverage area, or be flat per building?

### @economy-engineer

8. **Funding levels:** How does funding affect service effectiveness?
   - 0% funding: Building inactive (0% effectiveness)?
   - 50% funding: Reduced effectiveness?
   - 100% funding: Full effectiveness
   - 150% funding: Bonus effectiveness?

   Need to define the curve for IEconomyQueryable integration.

---

## Integration Points

### Epic 10 Integration (CRITICAL)

Epic 10 created an **IServiceQueryable stub** that returns neutral values:

```cpp
// Epic 10 stub behavior (to be replaced)
get_coverage(service_type, owner) -> 0.5f  // 50% neutral
get_coverage_at(service_type, position) -> 0.5f  // 50% neutral
get_effectiveness(service_type, owner) -> 1.0f  // Full effectiveness
```

**Epic 9 must:**
1. Implement the REAL IServiceQueryable with actual coverage calculations
2. Ensure DisorderSystem uses get_coverage_at() for enforcer suppression
3. Ensure PopulationSystem uses coverage for medical/education effects
4. Ensure LandValueSystem can query service coverage for land value bonuses

### IServiceProvider Interface (From Canon)

Service buildings implement IServiceProvider:

```cpp
class IServiceProvider {
public:
    virtual ServiceType get_service_type() const = 0;
    virtual uint32_t get_coverage_radius() const = 0;
    virtual float get_effectiveness() const = 0;
    virtual float get_coverage_at(GridPosition position) const = 0;
};
```

### Integration Flow

```
BuildingSystem (Epic 4)
    |
    | BuildingConstructedEvent (service building placed)
    v
ServicesSystem (Epic 9)
    |
    | Registers service building
    | Updates ServiceCoverageGrid per service type
    |
    v
+-- DisorderSystem (Epic 10)
|       |
|       v
|   Query: IServiceQueryable.get_coverage_at(enforcer, tile)
|   Apply: Disorder suppression based on coverage
|
+-- PopulationSystem (Epic 10)
|       |
|       v
|   Query: IServiceQueryable.get_coverage(medical, player)
|   Apply: Life expectancy modifier
|
+-- LandValueSystem (Epic 10)
        |
        v
    Query: IServiceQueryable.get_coverage_at(education, tile)
    Apply: Land value bonus from service coverage
```

### Tick Order

```
Priority 50: PopulationSystem      -- queries service coverage for health/education
Priority 52: DemandSystem          -- may query service coverage for demand factors
Priority 55: ServicesSystem        -- EPIC 9: calculates coverage grids  <-- NEW
Priority 60: EconomySystem         -- provides funding levels to services
Priority 65: LandValueSystem       -- queries service coverage for bonuses
Priority 70: DisorderSystem        -- queries enforcer coverage for suppression
Priority 80: ContaminationSystem   -- no direct service interaction
```

**Note:** ServicesSystem at priority 55 allows:
- PopulationSystem (50) to use PREVIOUS tick's coverage (acceptable lag)
- DisorderSystem (70) to use CURRENT tick's coverage (critical for responsiveness)

---

## Risks & Concerns

### RISK: Coverage Grid Memory (LOW)

Each service type per player requires a coverage grid.

| Players | Service Types | Grid Size (256x256) | Total |
|---------|---------------|---------------------|-------|
| 4 | 4 | 64 KB | 1 MB |

At largest map size (512x512): 4 MB total for service coverage. Acceptable.

**Mitigation:** Only allocate grids for service types the player has buildings for.

### RISK: Coverage Recalculation Performance (MEDIUM)

Recalculating coverage for all service buildings every tick could be expensive.

**Mitigation:**
- Dirty-region tracking: only recalculate when buildings added/removed
- Spatial partitioning: only affect tiles in building radius
- Consider calculating every N ticks instead of every tick (coverage changes slowly)

### RISK: Stub Replacement Timing (MEDIUM)

Epic 10 systems depend on IServiceQueryable stub. Replacing it mid-development could cause issues.

**Mitigation:**
- Epic 9 implementation must maintain same interface contract
- Integration tests verify behavior matches expectations
- Stub can remain as fallback if no service buildings exist

### RISK: Circular Dependency with Economy (LOW)

ServicesSystem depends on EconomySystem for funding levels. EconomySystem depends on ServicesSystem for maintenance costs.

**Mitigation:**
- Use previous tick's funding levels (acceptable lag)
- Or: ServicesSystem tick_priority 55 < EconomySystem tick_priority 60 means services use previous tick's funding
- Maintenance costs are constant per building, not dependent on current tick's coverage

### DESIGN AMBIGUITY: Global vs Local Service Effects

Canon states medical/education have "global" effects but also describes "coverage radius." Need clarity:

**Proposed Resolution:**
- Enforcer, Hazard Response: Per-tile coverage (affects specific tiles)
- Medical, Education: Per-tile coverage BUT aggregated for player-wide stats (life expectancy is player-wide, influenced by average medical coverage)

---

## Multiplayer Considerations

### Per-Player Service Boundaries

| Aspect | Behavior |
|--------|----------|
| Coverage scope | Service buildings only cover tiles owned by same player |
| Cross-boundary | Player A's enforcer does NOT reduce disorder on Player B's tiles |
| Visibility | All players can SEE other players' service buildings but don't benefit |

### Authority Model

| Data | Authority | Notes |
|------|-----------|-------|
| Service building placement | Server | Via BuildingSystem |
| Coverage calculation | Server | ServicesSystem runs server-side |
| Funding levels | Server | EconomySystem provides |
| Coverage queries | Server | IServiceQueryable is server-authoritative |

### Network Sync

Service coverage is derived data (calculated from building positions + funding). Does NOT need direct sync:
- Clients receive building placement updates
- Clients can calculate local coverage for UI display
- Server is authoritative for simulation effects

**Optional optimization:** Server could send pre-calculated coverage grids to clients for overlay rendering, avoiding client-side recalculation.

### Late Join

When a player joins mid-game:
- ServicesSystem detects new player's buildings
- Coverage grids calculated for new player
- Immediate effect on that player's disorder/population

### Ghost Town Services

When a player abandons (triggers ghost town process):
- Service buildings stop functioning (OwnershipState::Abandoned)
- Coverage drops to 0 in that player's former territory
- Disorder increases, life expectancy drops
- Other players are unaffected (their coverage remains)

---

## Component Definitions

### ServiceProviderComponent

```cpp
struct ServiceProviderComponent {
    ServiceType service_type;           // enforcer, hazard_response, medical, education
    uint8_t base_radius = 8;            // Base coverage radius in tiles
    uint8_t current_radius = 8;         // Effective radius (may be modified by upgrades)
    uint8_t base_effectiveness = 80;    // Base effectiveness (0-100)
    uint8_t current_effectiveness = 80; // Effective effectiveness (modified by funding)
    uint8_t funding_level = 100;        // Current funding percentage (from EconomySystem)
    bool is_active = true;              // False if unpowered/abandoned
};
// Size: 7 bytes
```

### ServiceCoverageComponent (Optional - for per-building queries)

```cpp
struct ServiceCoverageComponent {
    uint32_t tiles_covered = 0;         // Count of tiles in coverage area
    uint32_t beings_covered = 0;        // Population within coverage area
    float coverage_efficiency = 0.0f;   // beings_covered / theoretical_max
};
// Size: 12 bytes
```

---

## Dense Grid Design

### ServiceCoverageGrid

Per service type per player:

```cpp
class ServiceCoverageGrid {
private:
    std::vector<uint8_t> coverage_;  // 0-255 coverage strength per tile
    uint32_t width_;
    uint32_t height_;
    PlayerID owner_;
    ServiceType service_type_;
    bool dirty_ = true;

public:
    uint8_t get_coverage_at(int32_t x, int32_t y) const;
    float get_coverage_at_normalized(int32_t x, int32_t y) const;  // 0.0-1.0
    void clear();
    void add_coverage(int32_t center_x, int32_t center_y, uint8_t radius, uint8_t strength);
    bool is_dirty() const { return dirty_; }
    void mark_dirty() { dirty_ = true; }
    void mark_clean() { dirty_ = false; }
};
```

Memory per grid: 1 byte/tile
- 256x256 map: 64 KB per service type per player
- 4 players x 4 service types: 1 MB total

---

## Coverage Calculation Algorithm

### Per-Building Coverage Application

```
For each service building B owned by player P:
    if B is not active (unpowered/abandoned):
        continue

    radius = B.base_radius * funding_modifier(B.funding_level)
    effectiveness = B.base_effectiveness * funding_modifier(B.funding_level) / 100.0

    For each tile (x, y) within radius of B.position:
        if tile is owned by P:  # Only cover own tiles
            distance = manhattan_distance(B.position, (x, y))
            falloff = 1.0 - (distance / radius)
            strength = effectiveness * falloff * 255

            # Additive with cap
            current = coverage_grid[service_type][P].get(x, y)
            new_value = min(255, current + strength)
            coverage_grid[service_type][P].set(x, y, new_value)
```

### Funding Modifier Curve (Proposed)

```
funding_modifier(level):
    if level == 0: return 0.0      # Building inactive
    if level < 50: return level / 100.0  # Linear below 50%
    if level <= 100: return 0.5 + (level - 50) / 100.0  # Linear 50-100%
    if level <= 150: return 1.0 + (level - 100) / 200.0  # Diminishing returns above 100%
    return 1.25  # Cap at 125% effectiveness
```

---

## Canon Updates Required

After Epic 9 implementation, update:

### interfaces.yaml

- Add ServicesSystem tick_priority: 55
- Confirm IServiceQueryable interface (move from stubs to services section)
- Add IServiceProvider interface if not already present

### systems.yaml

- Update ServicesSystem entry with tick_priority: 55
- Add note that ServicesSystem replaces Epic 10's service stub

### patterns.yaml

- Add ServiceCoverageGrid to dense_grid_exception.applies_to:
  - "ServiceCoverageGrid (Epic 9): 1 byte/tile per service type per player for coverage strength"

---

## Critical Path

```
E9-001, E9-002 (Components/Config)
    |
    +-- E9-003 (ServiceCoverageGrid)
    |       |
    |       +-- E9-004 (ServicesSystem skeleton)
    |               |
    |               +-- E9-005 (IServiceQueryable implementation)
    |               |
    |               +-- E9-006 (Building registration)
    |                       |
    |                       +-- E9-007, E9-008, E9-009 (Coverage calculation)
    |                               |
    |                               +-- E9-010, E9-011, E9-012, E9-013 (Service types)
    |                                       |
    |                                       +-- E9-014, E9-015, E9-016 (Integration)
    |                                               |
    |                                               +-- E9-018, E9-019 (Testing)
```

**Minimum Viable Epic 9:** E9-001 through E9-014 (enforcer integration)
This enables disorder suppression to work, proving the Epic 10 integration.

---

## Summary

Epic 9 is a **focused integration epic** that:

1. Implements the **real IServiceQueryable** to replace Epic 10's stub
2. Provides **coverage calculations** for four service types
3. Integrates with **DisorderSystem, PopulationSystem, and LandValueSystem**
4. Uses **dense grid storage** for efficient coverage queries
5. Respects **per-player boundaries** for multiplayer

The epic is intentionally scoped to 20 work items. Coverage calculation and integration are the core challenges; service building variety is straightforward once the infrastructure exists.

---

**End of Systems Architect Analysis: Epic 9 - City Services**
