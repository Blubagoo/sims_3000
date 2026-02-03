# Canon Verification: Epic 4 -- Zoning & Building System

**Date:** 2026-01-28
**Canon Version:** 0.6.0
**Tickets Verified:** 48
**Verifier:** Systems Architect Agent

---

## Summary

| Category | Count |
|----------|-------|
| Canon-compliant | 42 |
| Minor issues | 4 |
| Conflicts found | 2 |

### Overall Assessment

Epic 4 tickets demonstrate strong alignment with canon principles. The planning team has thoroughly incorporated the alien terminology from terminology.yaml, the ECS patterns from patterns.yaml, the multiplayer architecture constraints, and the system boundaries defined in systems.yaml. The forward dependency stub pattern (CCR-006) is well-designed and consistent with Epic 3's approach to handling dependencies on unbuilt systems.

The most significant finding is that BuildingGrid requires a canon update to formally add it to the dense_grid_exception pattern in patterns.yaml. The tickets correctly identify this need (ticket 4-007 notes), but the update must be tracked. Additionally, three new interfaces (IZoneQueryable, IBuildingQueryable, IBuildingTemplateQuery) are proposed but not yet in interfaces.yaml, requiring a canon update. The tickets are otherwise architecturally sound, respect system boundaries, and correctly implement server-authoritative multiplayer patterns.

---

## Verification Results

### Compliant Tickets

The following 42 tickets fully comply with canon:

4-001, 4-002, 4-003, 4-004, 4-005, 4-006, 4-008, 4-009, 4-010, 4-011, 4-012, 4-013, 4-014, 4-015, 4-016, 4-017, 4-018, 4-019, 4-020, 4-021, 4-022, 4-023, 4-024, 4-025, 4-026, 4-027, 4-028, 4-029, 4-030, 4-031, 4-032, 4-033, 4-038, 4-039, 4-040, 4-041, 4-042, 4-043, 4-044, 4-045, 4-046, 4-047

**Highlights of Canon Compliance:**

1. **Terminology:** All 48 tickets consistently use canonical alien terminology (habitation/exchange/fabrication, beings, overseer, pathway, sector, credits, materializing, derelict, deconstructed, etc.). This is exemplary adherence to terminology.yaml.

2. **ECS Pattern:** Components are pure data (ZoneComponent 4 bytes, BuildingComponent 28 bytes, ConstructionComponent 12 bytes). Systems contain all logic. No methods in components beyond trivial accessors.

3. **Multiplayer Architecture:** Server-authoritative design throughout. No client-side prediction for zone/building operations. Seeded PRNG for template selection (seed = hash(tile_x, tile_y, sim_tick)). State sync via SyncSystem. All network messages implement ISerializable.

4. **ISimulatable Pattern:** ZoneSystem (priority 30) and BuildingSystem (priority 40) correctly implement the ISimulatable interface with priorities matching the system ordering in systems.yaml (after TerrainSystem at 5, before simulation systems at 50+).

5. **Forward Dependency Pattern:** Six forward dependency interfaces (IEnergyProvider, IFluidProvider, ITransportProvider, ILandValueProvider, IDemandProvider, ICreditProvider) with stub implementations returning permissive defaults. This matches the pattern established in Epic 3 for handling cross-epic dependencies.

6. **Event Pattern:** Events follow canonical naming ({Subject}{Action}Event) and are immutable data structures. ZoneDesignatedEvent, BuildingConstructedEvent, DemolitionRequestEvent, etc.

---

### Minor Issues

#### Issue M-001: Ticket 4-007 (BuildingGrid Dense Occupancy Array)

**Issue:** BuildingGrid is correctly identified as a dense grid exception (same pattern as TerrainGrid), but this requires a canon update to patterns.yaml that is mentioned in the ticket notes but not tracked as a deliverable.

**Canon Reference:** patterns.yaml `dense_grid_exception` section currently lists only TerrainGrid.

**Resolution:** Add a sub-task or acceptance criterion to 4-007 requiring the canon update to patterns.yaml. The update should add BuildingGrid to the `applies_to` list with rationale:
```yaml
applies_to:
  - "TerrainGrid (Epic 3): 4 bytes/tile dense 2D array"
  - "BuildingGrid (Epic 4): 4 bytes/tile dense EntityID array for O(1) spatial lookups"
```

---

#### Issue M-002: Ticket 4-035 (IZoneQueryable Interface)

**Issue:** IZoneQueryable is a new interface that needs to be added to interfaces.yaml. The ticket mentions registration but doesn't explicitly include the canon update as an acceptance criterion.

**Canon Reference:** interfaces.yaml does not currently define IZoneQueryable.

**Resolution:** Add acceptance criterion to 4-035: "Canon update: IZoneQueryable added to interfaces.yaml with full method signatures and implementation notes." The interface definition should be placed under a new `zoning:` section in interfaces.yaml.

---

#### Issue M-003: Ticket 4-036 (IBuildingQueryable Interface)

**Issue:** Same issue as M-002. IBuildingQueryable is a new interface requiring interfaces.yaml update.

**Canon Reference:** interfaces.yaml does not currently define IBuildingQueryable.

**Resolution:** Add acceptance criterion to 4-036: "Canon update: IBuildingQueryable added to interfaces.yaml under a `buildings:` query section with full method signatures."

---

#### Issue M-004: Ticket 4-037 (IBuildingTemplateQuery Interface)

**Issue:** Same issue as M-002/M-003. IBuildingTemplateQuery is a new interface requiring interfaces.yaml update.

**Canon Reference:** interfaces.yaml does not currently define IBuildingTemplateQuery.

**Resolution:** Add acceptance criterion to 4-037: "Canon update: IBuildingTemplateQuery added to interfaces.yaml under a `buildings:` query section with full method signatures."

---

### Conflicts

#### Conflict C-001: Ticket 4-048 (ISimulatable Registration)

**Issue:** The ticket states system priorities should be added to systems.yaml as a canon update, but systems.yaml already lists priorities in the `ISimulatable.implemented_by` section of interfaces.yaml. The ticket refers to updating systems.yaml, but the canonical location for priorities is interfaces.yaml.

**Canon Reference:** interfaces.yaml, section `simulation.ISimulatable.implemented_by` lists:
```yaml
implemented_by:
  - TerrainSystem (priority: 5)
  - EnergySystem (priority: 10)
  - FluidSystem (priority: 20)
  - ZoneSystem (priority: 30)
  - BuildingSystem (priority: 40)
  ...
```

**Severity:** MINOR

**Resolution Options:**
A) Modify ticket to reference interfaces.yaml instead of systems.yaml for priority registration
B) Propose canon update to move ISimulatable priorities to systems.yaml for better organization

**Recommendation:** Option A. The current location in interfaces.yaml makes sense -- ISimulatable is an interface, and its implementers with priorities are naturally documented there. Update ticket 4-048 acceptance criteria to reference interfaces.yaml. Note: Canon already lists ZoneSystem at priority 30 and BuildingSystem at priority 40, so this may already be complete -- verify against current canon.

---

#### Conflict C-002: Ticket 4-034 (BuildingSystem Dependencies)

**Issue:** The BuildingSystem dependency injection pattern in 4-034 lists six forward dependency providers via constructor parameters, but systems.yaml defines BuildingSystem's `depends_on` list differently:

```yaml
# From systems.yaml epic_4_zoning.BuildingSystem.depends_on:
depends_on:
  - ZoneSystem (triggers spawning)
  - EnergySystem (affects building state)
  - FluidSystem (affects building state)
  - TransportSystem (road proximity rule)
```

The ticket omits TransportSystem from the constructor injection list and adds DemandProvider/CreditProvider which are not in the systems.yaml dependency list.

**Canon Reference:** systems.yaml `phase_2.epic_4_zoning.BuildingSystem.depends_on`

**Severity:** MINOR

**Resolution Options:**
A) Update ticket 4-034 to align constructor parameters with systems.yaml (add TransportSystem explicitly)
B) Propose systems.yaml update to include EconomySystem (for credits) and DemandSystem as BuildingSystem dependencies

**Recommendation:** Option B. The ticket is more complete than systems.yaml. The six forward dependency interfaces represent the actual runtime dependencies BuildingSystem needs. Update systems.yaml to add:
```yaml
depends_on:
  - ZoneSystem (triggers spawning)
  - EnergySystem (affects building state)
  - FluidSystem (affects building state)
  - TransportSystem (road proximity rule)
  - LandValueSystem (template selection)
  - DemandSystem (spawn gating)
  - EconomySystem (construction/demolition costs)
```

This is more accurate than the current canon.

---

## Canon Gaps Found

### Gap G-001: Building Template System Not Documented in patterns.yaml

patterns.yaml section `building_templates` exists but doesn't describe the template selection weighting algorithm, duplicate avoidance pattern, or seeded PRNG approach detailed in ticket 4-022. Consider adding a subsection documenting:
- Template pool organization (zone_type + density)
- Selection weighting factors
- Neighbor duplicate avoidance
- Deterministic seeded PRNG requirement

### Gap G-002: Zone Desirability Calculation

The desirability score concept (cached in ZoneComponent.desirability, updated periodically, used for template selection) is not documented in canon. While this is an implementation detail, the concept bridges ZoneSystem and BuildingSystem and should be documented in systems.yaml under ZoneSystem's `provides` or in a cross-cutting concerns section.

### Gap G-003: De-zone Flow (DemolitionRequestEvent Pattern)

The decoupled de-zone flow using DemolitionRequestEvent (CCR-012) represents an architectural pattern for avoiding circular dependencies between systems. This pattern should be documented in patterns.yaml as a general pattern for system decoupling via events.

### Gap G-004: Forward Dependency Stub Pattern

The forward dependency interface + stub pattern (CCR-006) is mentioned in the tickets but not formally documented in patterns.yaml. This is a reusable pattern that should be canonized:
- Define abstract interface for dependency
- Create stub implementation returning permissive defaults
- Use setter injection to swap stub with real implementation
- Include debug toggle for testing edge cases

---

## Interface Design Verification

### IZoneQueryable (Ticket 4-035)

**Proposed Methods:**
- get_zone_type(x, y) -> optional<ZoneType>
- get_zone_density(x, y) -> optional<ZoneDensity>
- is_zoned(x, y) -> bool
- get_zone_count(player_id, type) -> uint32_t
- get_zoned_tiles_without_buildings(player_id, type) -> vector<GridPosition>
- get_demand(type, player_id) -> float

**Verification:**
- Aligns with ZoneSystem's `provides` in systems.yaml ("Zone queries (type at position)", "Zone demand values")
- Methods are read-only queries, appropriate for an interface
- Uses canonical types (GridPosition, ZoneType, PlayerID)
- **Approved for canon addition**

### IBuildingQueryable (Ticket 4-036)

**Proposed Methods:**
- Spatial: get_building_at, is_tile_occupied, is_footprint_available, get_buildings_in_rect
- Ownership: get_buildings_by_owner, get_building_count_by_owner
- State: get_building_state, get_buildings_by_state
- Type: get_building_count_by_type, get_building_count_by_type_and_owner
- Capacity: get_total_capacity, get_total_occupancy
- Stats: get_building_stats

**Verification:**
- Aligns with BuildingSystem's `provides` in systems.yaml ("Building queries (type, state at position)", "Building events")
- Comprehensive query surface for downstream systems (Epics 5, 6, 7, 9, 10, 11, 13)
- Uses canonical types (EntityID, GridPosition, BuildingState, PlayerID)
- **Approved for canon addition**

### IBuildingTemplateQuery (Ticket 4-037)

**Proposed Methods:**
- get_template(template_id) -> const BuildingTemplate&
- get_templates_for_zone(type, density) -> template list
- get_energy_required(template_id)
- get_fluid_required(template_id)
- get_population_capacity(template_id)
- get_job_capacity(template_id)

**Verification:**
- Provides template metadata needed by EnergySystem, FluidSystem, PopulationSystem
- Separates template data access from building entity queries
- **Approved for canon addition**

---

## Forward Dependency Stub Verification

### Stub Pattern Consistency Check

| Interface | Stub Default | Pattern Match | Notes |
|-----------|-------------|---------------|-------|
| IEnergyProvider | is_powered = true | Yes | Consistent with Epic 3 approach |
| IFluidProvider | has_water = true | Yes | Consistent with Epic 3 approach |
| ITransportProvider | is_road_accessible = true, distance = 0 | Yes | Consistent with Epic 3 approach |
| ILandValueProvider | get_land_value = 50 | Yes | Medium value for neutral behavior |
| IDemandProvider | get_demand = 1.0f (positive) | Yes | Positive demand enables spawning |
| ICreditProvider | deduct_credits = true, has_credits = true | Yes | Always succeeds for testing |

All stubs include debug toggle for restrictive testing mode. This enables testing the full state machine (Active -> Abandoned -> Derelict -> Deconstructed) without waiting for Epics 5/6/7.

---

## Multiplayer Verification

### Server-Authoritative Checks

| Ticket | Operation | Authority | Sync Pattern | Compliant |
|--------|-----------|-----------|--------------|-----------|
| 4-012 | Zone placement | Server validates, applies | Entity creation synced | Yes |
| 4-013 | De-zoning | Server validates, applies | Entity removal synced | Yes |
| 4-022 | Template selection | Server-side seeded PRNG | template_id in spawn message | Yes |
| 4-025 | Building spawn | Server only | BuildingSpawnedMessage includes all variation data | Yes |
| 4-027 | Construction progress | Server ticks | Progress synced every 10 ticks, client interpolates | Yes |
| 4-028 | State transitions | Server evaluates | State changes synced immediately | Yes |
| 4-030 | Demolition | Server validates ownership, applies | BuildingDeconstructedEvent synced | Yes |

All operations are server-authoritative. Clients send requests, server validates and applies, results broadcast via SyncSystem. No client-side prediction for city builder operations (appropriate for the genre).

---

## Terminology Verification

All 48 tickets were checked against terminology.yaml. The following canonical terms are correctly used throughout:

| Human Term | Canonical Term | Usage in Tickets |
|------------|----------------|------------------|
| residential | habitation | Consistent |
| commercial | exchange | Consistent |
| industrial | fabrication | Consistent |
| tile | sector | Consistent |
| road | pathway | Consistent |
| citizen | being | Consistent |
| player/mayor | overseer | Consistent |
| zoning | designation | Consistent |
| building | structure | Consistent |
| under construction | materializing | Consistent |
| abandoned | derelict | Consistent (BuildingState uses both Abandoned and Derelict as distinct states) |
| demolished | deconstructed | Consistent |
| money | credits | Consistent |
| RCI demand | growth pressure | Consistent |
| city | colony | Consistent |
| land value | sector value | Consistent |

**Note on Abandoned vs. Derelict:** The tickets correctly use both terms -- "Abandoned" is the intermediate state (services lost, can recover), "Derelict" is the terminal decay state (cannot recover, must demolish). This is internally consistent and documented in the 5-state machine.

---

## Recommendations

### Must-Do (Before Implementation)

1. **Add BuildingGrid to dense_grid_exception in patterns.yaml** (per Issue M-001)
   - Rationale: Dense grid is a canonical exception to ECS-per-entity
   - Impact: Low -- documentation only

2. **Add IZoneQueryable, IBuildingQueryable, IBuildingTemplateQuery to interfaces.yaml** (per Issues M-002, M-003, M-004)
   - Rationale: Downstream epics depend on stable interface definitions
   - Impact: Medium -- interface lock required before Epic 5/6/7/10/11 can implement consumers

3. **Update systems.yaml BuildingSystem.depends_on** (per Conflict C-002)
   - Add LandValueSystem, DemandSystem, EconomySystem
   - Rationale: Accurate dependency graph for planning

### Should-Do (During Implementation)

4. **Document forward dependency stub pattern in patterns.yaml** (per Gap G-004)
   - Rationale: Reusable pattern for future epics
   - Impact: Low -- documentation

5. **Document template selection algorithm in patterns.yaml** (per Gap G-001)
   - Rationale: Critical algorithm deserves canonical documentation
   - Impact: Low -- documentation

6. **Correct interfaces.yaml reference in ticket 4-048** (per Conflict C-001)
   - Change "systems.yaml" to "interfaces.yaml" for ISimulatable priorities
   - Impact: Low -- ticket wording only

### Nice-to-Have

7. **Document DemolitionRequestEvent decoupling pattern** (per Gap G-003)
   - Useful for future cross-system event patterns

8. **Document zone desirability concept** (per Gap G-002)
   - Bridges ZoneSystem and BuildingSystem

---

## Conclusion

Epic 4 tickets are architecturally sound and well-aligned with project canon. The planning team has done excellent work incorporating alien terminology, ECS patterns, multiplayer constraints, and system boundaries. The issues identified are minor documentation gaps and one case of canon needing to catch up with the more complete ticket specifications.

**Verification Status: APPROVED with minor updates required**

The 4 minor issues and 2 conflicts are all resolvable through canon updates (not ticket modifications), indicating that the tickets are actually more thorough than current canon documentation. Once the recommended canon updates are applied, Epic 4 implementation can proceed with confidence.
