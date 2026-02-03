# Epic 5: Canon Verification Report

**Verified By:** Systems Architect Agent
**Date:** 2026-01-28
**Canon Version:** 0.6.0
**Status:** PASS_WITH_UPDATES

---

## Verification Summary

| Category | Status | Notes |
|----------|--------|-------|
| System Boundaries | PASS | EnergySystem owns correct scope per systems.yaml |
| Interfaces | PASS_WITH_UPDATES | IEnergyConsumer needs priority level update |
| Component Sizes | PASS | 12/24/4 bytes appropriate for ECS pattern |
| Tick Priority | PASS | Priority 10 matches ISimulatable definition |
| Terminology | PASS | Correct alien terms used throughout |
| Patterns | PASS_WITH_UPDATES | Pool model correct; CoverageGrid needs dense_grid_exception addition |
| Dependencies | PASS | Cross-epic dependencies correctly identified |

---

## Detailed Findings

### System Boundaries

**Status:** PASS

The tickets correctly scope EnergySystem to own:
- Energy nexus simulation (5-003, 5-010, 5-022, 5-023)
- Energy pool calculation per player (5-005, 5-012)
- Energy coverage zone calculation (5-007, 5-014, 5-015, 5-016, 5-017)
- Energy distribution to buildings (5-018, 5-019)
- Brownout/blackout detection (5-013)
- Plant efficiency and aging (5-022)

Per canon systems.yaml `epic_5_power.EnergySystem.owns`, the tickets correctly do NOT own:
- Building energy consumption amounts (handled via templates in 5-037)
- Energy UI (documented for Epic 12 in 5-040)
- Energy costs (deferred to EconomySystem integration)

**Canon Reference:** `systems.yaml` lines 269-303

---

### Interfaces

**Status:** PASS_WITH_UPDATES

#### IEnergyConsumer (interfaces.yaml)
- **Canon states:** Priority levels "1=critical (medical), 2=important (enforcer), 3=normal"
- **Tickets define:** 4 levels: 1=Critical, 2=Important, 3=Normal, 4=Low (CCR-003)

**Finding:** Canon mentions 3 priority levels, but tickets define 4 levels. The 4th level (Low for habitation) is a reasonable design decision that improves granularity during rationing.

**Required Update:** interfaces.yaml IEnergyConsumer.notes should be updated from:
```
"Priority levels: 1=critical (medical), 2=important (enforcer), 3=normal"
```
to:
```
"Priority levels: 1=critical (medical/command), 2=important (enforcer/hazard), 3=normal (exchange/fabrication), 4=low (habitation)"
```

#### IEnergyProducer (interfaces.yaml)
- Canon lists implemented_by: "Coal energy nexus, Oil energy nexus, Gas energy nexus..."
- Tickets use canonical alien names: Carbon, Petrochemical, Gaseous, Nuclear, Wind, Solar

**Finding:** Canon uses human terms (coal, oil, gas) in IEnergyProducer.implemented_by but should use alien terminology.

**Required Update:** interfaces.yaml IEnergyProducer.implemented_by should use canonical terms:
- "Coal energy nexus" -> "Carbon nexus"
- "Oil energy nexus" -> "Petrochemical nexus"
- "Gas energy nexus" -> "Gaseous nexus"

#### IEnergyProvider Interface (missing)
- Tickets define IEnergyProvider interface (5-009) for BuildingSystem integration
- Canon interfaces.yaml does NOT define this interface

**Required Update:** Add IEnergyProvider interface to interfaces.yaml under `utilities:` section

#### IContaminationSource (interfaces.yaml)
- Canon defines this interface correctly (lines 332-357)
- Ticket 5-025 correctly implements it for dirty nexuses
- Canon correctly lists "Certain energy nexuses (coal, oil)" - needs terminology update

**Finding:** Method signatures match. implemented_by list needs alien terminology.

**Required Update:** interfaces.yaml IContaminationSource.implemented_by should use:
- "Certain energy nexuses (carbon, petrochemical)" instead of "(coal, oil)"

---

### Component Sizes

**Status:** PASS

| Component | Ticket Size | Justification |
|-----------|-------------|---------------|
| EnergyComponent | 12 bytes | Fits ECS tight packing; trivially copyable |
| EnergyProducerComponent | 24 bytes | Contains aging/efficiency floats; reasonable |
| EnergyConduitComponent | 4 bytes | Minimal data for coverage extension |

All components follow canon ECS patterns:
- Pure data, no methods
- snake_case field names
- Trivially copyable for network serialization
- No pointers (uses entity IDs)

**Canon Reference:** `patterns.yaml` lines 24-50

---

### Tick Priority

**Status:** PASS

- Ticket 5-008 specifies `get_priority() returns 10`
- Canon interfaces.yaml ISimulatable.implemented_by lists "EnergySystem (priority: 10)"

The priority ordering is correct:
- TerrainSystem: 5
- **EnergySystem: 10** (runs before zones/buildings)
- FluidSystem: 20
- ZoneSystem: 30
- BuildingSystem: 40

This ensures energy state is settled before building systems query it.

**Canon Reference:** `interfaces.yaml` lines 35-45

---

### Terminology

**Status:** PASS

The tickets correctly use canonical alien terminology:

| Human Term | Canonical Term | Used in Tickets |
|------------|----------------|-----------------|
| power | energy | Yes (throughout) |
| power plant | energy nexus | Yes (5-003, 5-010, etc.) |
| power lines | energy conduits | Yes (5-004, 5-027, etc.) |
| power grid | energy matrix | Yes (epic goal) |
| blackout | grid collapse | Yes (5-013, 5-021) |
| brownout | energy deficit | Yes (5-013, 5-021) |
| coal | carbon | Yes (5-001, 5-023) |
| oil | petrochemical | Yes (5-001, 5-023) |
| gas | gaseous | Yes (5-001, 5-023) |
| player | overseer | Yes (5-005, descriptions) |

**Canon Reference:** `terminology.yaml` lines 101-109

**Minor Note:** terminology.yaml does not explicitly define:
- carbon_nexus (for carbon energy nexus)
- petrochemical_nexus (for petrochemical energy nexus)
- gaseous_nexus (for gaseous energy nexus)

**Suggested Update:** Add these compound terms to terminology.yaml infrastructure section for completeness.

---

### Patterns

**Status:** PASS_WITH_UPDATES

#### Pool Model
The tickets correctly implement the canon pool model:
- Per-player energy pools (5-005)
- Generators add to pool (5-010)
- Consumers draw from pool (5-011, 5-012)
- Infrastructure defines coverage, not transport (5-004, 5-014)
- "Tile must be in coverage zone AND pool must have surplus" (5-018)

**Canon Reference:** `patterns.yaml` lines 285-309

#### Dense Grid Exception
Ticket 5-007 (CoverageGrid) uses dense 2D array storage, which falls under the `dense_grid_exception` pattern.

**Canon states applies_to:**
- "TerrainGrid (Epic 3): 4 bytes/tile dense 2D array"
- "Future: contamination grid, land value grid, pathfinding grid"

**Finding:** CoverageGrid is not listed in dense_grid_exception.applies_to but follows the same pattern rationale:
- Data covers every grid cell
- Spatial locality critical for O(1) queries
- Per-entity overhead prohibitive for 65k+ cells

**Required Update:** Add CoverageGrid to patterns.yaml dense_grid_exception.applies_to:
```yaml
applies_to:
  - "TerrainGrid (Epic 3): 4 bytes/tile dense 2D array"
  - "CoverageGrid (Epic 5): 1 byte/tile dense 2D array for energy coverage"
  - "Future: contamination grid, land value grid, pathfinding grid"
```

---

### Dependencies

**Status:** PASS

#### Correctly Identified External Dependencies:

**Epic 3 (Terrain):**
- ITerrainQueryable for placement validation (5-026, 5-027)
- ITerrainQueryable.is_buildable() for conduit/nexus placement
- Terrain elevation for Wind nexus bonus (5-024)

**Epic 4 (Zoning/Building):**
- BuildingConstructedEvent (5-032)
- BuildingDeconstructedEvent (5-033)
- IEnergyProvider stub (4-019) to be replaced by 5-009

#### Correctly Identified Downstream Dependents:

| Epic | System | Dependency |
|------|--------|------------|
| Epic 6 | FluidSystem | Shares pool model pattern (no direct dependency) |
| Epic 7 | RailSystem | Queries IEnergyProvider.is_powered() |
| Epic 9 | ServicesSystem | Service buildings query IEnergyProvider |
| Epic 10 | ContaminationSystem | Queries IContaminationSource |
| Epic 10 | PopulationSystem | Pool state affects habitability |
| Epic 11 | EconomySystem | Nexus build/maintenance costs |
| Epic 12 | UISystem | Energy overlay, pool status display |

**Canon Reference:** `systems.yaml` epic_5_power.EnergySystem.depends_on

---

## Canon Update Requirements

The following canon updates should be applied based on epic planning:

### 1. interfaces.yaml - IEnergyConsumer priority levels

**Current:**
```yaml
notes:
  - "Priority levels: 1=critical (medical), 2=important (enforcer), 3=normal"
```

**Update to:**
```yaml
notes:
  - "Priority levels: 1=critical (medical/command), 2=important (enforcer/hazard), 3=normal (exchange/fabrication), 4=low (habitation)"
```

### 2. interfaces.yaml - IEnergyProducer implemented_by terminology

**Current:**
```yaml
implemented_by:
  - Coal energy nexus
  - Oil energy nexus
  - Gas energy nexus
  - Nuclear energy nexus
  - Wind turbines
  - Hydro energy nexus
  - Solar collectors
  - Fusion reactor (future tech)
  - Microwave receiver (future tech)
```

**Update to:**
```yaml
implemented_by:
  - Carbon nexus
  - Petrochemical nexus
  - Gaseous nexus
  - Nuclear nexus
  - Wind turbines
  - Hydro nexus
  - Solar collectors
  - Fusion reactor (future tech)
  - Microwave receiver (future tech)
```

### 3. interfaces.yaml - Add IEnergyProvider interface

Add under `utilities:` section:
```yaml
IEnergyProvider:
  description: "Provides energy state queries for other systems"
  purpose: "Allows BuildingSystem and other consumers to query power state"

  methods:
    - name: is_powered
      params:
        - name: entity
          type: EntityID
      returns: bool
      description: "Whether entity is currently receiving power"

    - name: is_powered_at
      params:
        - name: x
          type: int32_t
        - name: y
          type: int32_t
        - name: owner
          type: PlayerID
      returns: bool
      description: "Whether position has power coverage and pool surplus"

    - name: get_pool_state
      params:
        - name: owner
          type: PlayerID
      returns: EnergyPoolState
      description: "Current pool state (Healthy/Marginal/Deficit/Collapse)"

  implemented_by:
    - EnergySystem (Epic 5)

  notes:
    - "Replaces Epic 4 stub (ticket 4-019)"
    - "is_powered_at combines coverage check + pool surplus check"
```

### 4. interfaces.yaml - IContaminationSource terminology

**Current:**
```yaml
implemented_by:
  - Fabrication buildings (industrial pollution)
  - Certain energy nexuses (coal, oil)
  - High-traffic roads (traffic pollution)
  - Terrain: blight_mires / toxic_marshes (natural contamination)
```

**Update to:**
```yaml
implemented_by:
  - Fabrication buildings (industrial pollution)
  - Certain energy nexuses (carbon, petrochemical, gaseous)
  - High-traffic roads (traffic pollution)
  - Terrain: blight_mires / toxic_marshes (natural contamination)
```

### 5. patterns.yaml - dense_grid_exception applies_to

**Current:**
```yaml
applies_to:
  - "TerrainGrid (Epic 3): 4 bytes/tile dense 2D array"
  - "Future: contamination grid, land value grid, pathfinding grid"
```

**Update to:**
```yaml
applies_to:
  - "TerrainGrid (Epic 3): 4 bytes/tile dense 2D array"
  - "CoverageGrid (Epic 5): 1 byte/tile dense 2D array for energy coverage"
  - "Future: contamination grid, land value grid, pathfinding grid"
```

### 6. systems.yaml - EnergySystem tick priority

Add explicit priority documentation to epic_5_power.EnergySystem:
```yaml
tick_priority: 10  # Runs before zones (30) and buildings (40)
```

### 7. terminology.yaml - Energy nexus type terms (optional)

Add to infrastructure section:
```yaml
# Energy Nexus Types (canonical names for power plants)
carbon_nexus: carbon_nexus           # Coal-equivalent dirty energy
petrochemical_nexus: petrochemical_nexus  # Oil-equivalent dirty energy
gaseous_nexus: gaseous_nexus         # Gas-equivalent dirty energy
```

---

## Conflicts Requiring Decisions

**None identified.**

All discrepancies between tickets and canon are minor terminology/documentation updates that do not require architectural decisions. The 4-priority-level system (CCR-003) is a reasonable refinement of the 3-level system mentioned in canon.

---

## Conclusion

Epic 5 tickets are **well-aligned with canon** and ready for implementation after the documented canon updates are applied.

**Key Strengths:**
1. Correctly implements the pool model (no physical flow simulation)
2. Proper use of alien terminology throughout
3. Correct tick priority (10) for system ordering
4. Appropriate component sizes for ECS performance
5. Clean interface boundaries with Epic 3 and Epic 4
6. CoverageGrid correctly follows dense_grid_exception pattern rationale

**Required Actions Before Implementation:**
1. Apply the 7 canon updates listed above via `/update-canon`
2. Verify Epic 4 ticket 4-019 IEnergyProvider stub matches the interface defined here

**Risk Assessment:** LOW
- No fundamental conflicts with canon principles
- All updates are additive or terminology corrections
- No architectural decisions required

---

**Verification Complete**
