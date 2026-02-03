# Canon & Epic Plan Review Report

**Date:** 2026-01-25
**Reviewer:** Systems Architect Agent
**Canon Version Reviewed:** 0.1.0
**Epic Plan Version:** Initial

---

## Executive Summary

The canonical documentation and epic plan demonstrate **strong overall cohesion** with well-defined system boundaries, consistent terminology, and clear dependency hierarchies. However, there are notable gaps in **interface definitions for several systems**, some **multiplayer edge cases not addressed**, and a few **dependency sequencing issues** that should be resolved before implementation begins. The foundation is solid but needs refinement in cross-system contracts.

---

## Confidence Ratings

| Area | Confidence | Notes |
|------|------------|-------|
| Dependency Graph | 7/10 | Most dependencies clear; some circular risks and missing links |
| Interface Contracts | 5/10 | Only utility interfaces defined; transport, services, disasters lack contracts |
| System Boundaries | 8/10 | Well-defined; minor overlaps in pollution/contamination |
| Multiplayer Coverage | 6/10 | Core patterns good; edge cases (reconnection, conflict resolution) undefined |
| Terminology Consistency | 9/10 | Epic plan mostly uses canonical terms; minor inconsistencies |
| Phase Ordering | 7/10 | Generally sound; EPIC 7 should precede EPIC 10 per dependencies |

**Overall Confidence: 7/10**

---

## Gaps Identified

### Critical (Must Fix Before Implementation)

1. **Missing ITransportable Interface**
   - **Location:** `interfaces.yaml`
   - **Impact:** BuildingSystem depends on TransportSystem for the "3-tile road proximity rule" but no interface contract exists
   - **Recommendation:** Define `ITransportConnectable` interface with methods like `get_nearest_road_distance()`, `is_road_accessible()`

2. **Missing IServiceProvider Interface**
   - **Location:** `interfaces.yaml`
   - **Impact:** ServicesSystem owns service effects but no contract for how buildings query coverage
   - **Recommendation:** Define `IServiceProvider` with `get_coverage_at(position)`, `get_effectiveness()`

3. **Circular Dependency Risk: LandValueSystem**
   - **Location:** `systems.yaml` Phase 2
   - **Impact:** LandValueSystem depends on DisorderSystem, which depends on LandValueSystem ("low value = more crime")
   - **Recommendation:** Break cycle by having DisorderSystem use previous tick's land value (one-tick delay)

4. **Missing PopulationSystem Definition**
   - **Location:** `systems.yaml`
   - **Impact:** Referenced in 8+ places but never formally defined with owns/provides/depends_on
   - **Recommendation:** Add PopulationSystem to Phase 2 or Phase 3 with clear boundaries

5. **EPIC 7 Listed Before EPIC 10 in Canon, But EPIC 10 Dependencies Are Wrong**
   - **Location:** `systems.yaml` epic_10_simulation
   - **Impact:** DemandSystem depends on TransportSystem (accessibility), but epic plan shows EPIC 10 (Simulation Core) before EPIC 7 (Transportation) in Phase 2
   - **Recommendation:** Either reorder phases or clarify that basic DemandSystem can work without transport

### Important (Should Fix Soon)

6. **Missing Disaster Interfaces**
   - **Location:** `interfaces.yaml`
   - **Impact:** DisasterSystem needs contracts with BuildingSystem (damage), ServicesSystem (response), and RenderingSystem (effects)
   - **Recommendation:** Define `IDamageable`, `IEmergencyResponder` interfaces

7. **Ambiguous ContaminationSystem Sources**
   - **Location:** `systems.yaml` Phase 3
   - **Impact:** ContaminationSystem "does not own what produces contamination" but no interface for reporting pollution
   - **Recommendation:** Define `IContaminationSource` interface or clarify event-based reporting

8. **Missing Network Message Types**
   - **Location:** `patterns.yaml` multiplayer section
   - **Impact:** Only 6 message types defined; disasters, progression, services need sync
   - **Recommendation:** Add `DisasterMessage`, `MilestoneMessage`, `ServiceCoverageMessage`

9. **PortSystem Dependencies Incomplete**
   - **Location:** `systems.yaml` epic_8_ports
   - **Impact:** PortSystem depends on ZoneSystem, TerrainSystem, TransportSystem but not EconomySystem (trade income)
   - **Recommendation:** Add EconomySystem to dependencies

10. **AudioSystem Component Missing**
    - **Location:** `systems.yaml` epic_15_audio
    - **Impact:** AudioSystem listed but no component defined; unclear what triggers sounds
    - **Recommendation:** Define `AudioTriggerComponent` or clarify event-based audio

### Minor (Nice to Have)

11. **Terminology Inconsistency: "RCI" vs "Zone Pressure"**
    - **Location:** Epic plan uses "RCI Demand" extensively; canon uses "zone_pressure"
    - **Impact:** Minor confusion for implementers
    - **Recommendation:** Add "RCI" as alias in terminology.yaml or update epic plan

12. **Missing SimulationCore System Priority**
    - **Location:** `interfaces.yaml` ISimulatable
    - **Impact:** SimulationCore orchestrates systems but has no defined priority
    - **Recommendation:** Clarify that SimulationCore is the orchestrator, not a participant

13. **Tile Size Undefined**
    - **Location:** `patterns.yaml` grid section
    - **Impact:** `tile_sizes.base` and `elevation_step` are "TBD"
    - **Recommendation:** Define before rendering implementation

14. **Missing Chat/Communication Interface**
    - **Location:** `interfaces.yaml`
    - **Impact:** EPIC 1 mentions in-game chat but no contract defined
    - **Recommendation:** Add `IChatHandler` interface

15. **Victory Conditions Not Formalized**
    - **Location:** Both documents mention victory but no canonical rules
    - **Impact:** Competitive mode needs clear win states
    - **Recommendation:** Add victory_conditions section to canon

---

## Dependency Analysis

### System Dependency Graph

```
EPIC 0 (Foundation)
    |
    +---> EPIC 1 (Multiplayer)
    |         |
    |         +---> EPIC 2 (Rendering)
    |                   |
    |                   +---> EPIC 3 (Terrain)
    |                             |
    |                             +---> EPIC 4 (Zoning/Building)
    |                                       |
    +---> EPIC 15 (Audio)                   +---> EPIC 5 (Power)
                                            |         |
                                            |         +---> EPIC 6 (Water)*
                                            |         |
                                            |         +---> EPIC 10 (Simulation)**
                                            |                   |
                                            +---> EPIC 7 (Transport)***
                                            |         |
                                            |         +---> EPIC 8 (Ports)
                                            |
                                            +---> EPIC 9 (Services)
                                                      |
                                                      +---> EPIC 11 (Financial)
                                                                |
                                                                +---> EPIC 12 (UI)
                                                                          |
                                                                          +---> EPIC 13 (Disasters)
                                                                          |
                                                                          +---> EPIC 14 (Progression)
                                                                                    |
                                                                                    +---> EPIC 16 (Save/Load)
                                                                                              |
                                                                                              +---> EPIC 17 (Polish)
```

### Dependency Issues Identified

| Issue | Systems Involved | Problem | Resolution |
|-------|------------------|---------|------------|
| **Circular** | LandValueSystem <-> DisorderSystem | Each depends on the other | Use previous-tick values |
| **Missing** | BuildingSystem -> TransportSystem | No interface defined | Add ITransportConnectable |
| **Ordering** | EPIC 10 depends on EPIC 7 | Phase 2 lists 10 before 7 | Reorder or stub transport |
| **Undefined** | PopulationSystem | Referenced but not defined | Add to systems.yaml |
| **Implicit** | EconomySystem -> All cost-bearing systems | Costs mentioned but no interface | Define IMaintenanceCost |

### Notes on Dependency Graph

- `*` EPIC 6 (Water): Listed in Phase 3 in canon but dependencies suggest Phase 2
- `**` EPIC 10 (Simulation): DemandSystem depends on TransportSystem
- `***` EPIC 7 (Transport): Should be implemented before or in parallel with EPIC 10

---

## Cross-System Data Flow Analysis

### Data Producers and Consumers

| Data Type | Producer(s) | Consumer(s) | Sync Required |
|-----------|-------------|-------------|---------------|
| Power State | EnergySystem | BuildingSystem, FluidSystem, RailSystem | Yes |
| Fluid State | FluidSystem | BuildingSystem | Yes |
| Land Value | LandValueSystem | BuildingSystem, ZoneSystem, DemandSystem | Yes |
| Zone Demand | DemandSystem | ZoneSystem, BuildingSystem, UISystem | Yes |
| Traffic Density | TransportSystem | ContaminationSystem, LandValueSystem | Yes |
| Crime Rate | DisorderSystem | LandValueSystem, UISystem | Yes |
| Pollution Level | ContaminationSystem | LandValueSystem, UISystem | Yes |
| Population | PopulationSystem* | EconomySystem, DemandSystem, ProgressionSystem | Yes |
| Treasury | EconomySystem | BuildingSystem (build checks), UISystem | Yes |

`*` PopulationSystem not defined in systems.yaml

### Multiplayer Data Authority

| System | Authority Model | Notes |
|--------|-----------------|-------|
| EnergySystem | Server | Grid state server-authoritative |
| FluidSystem | Server | Network state server-authoritative |
| ZoneSystem | Server | Zone placement validated by server |
| BuildingSystem | Server | Building spawns are deterministic on server |
| TransportSystem | Server | Traffic simulation server-only |
| EconomySystem | Server | Per-player budgets, server tracks all |
| DisorderSystem | Server | Crime affects all players |
| DisasterSystem | Server | Disasters synchronized from server |

---

## Multiplayer Gap Analysis

### Addressed in Canon

- Server authority model
- Delta state synchronization
- Ownership component
- Player IDs and boundaries
- Basic message types
- Tick rate (20/sec)

### NOT Addressed (Gaps)

| Gap | Impact | Question |
|-----|--------|----------|
| **Reconnection Protocol** | What happens to a player's city during disconnect? | Does simulation pause? Do AI take over? |
| **Conflict Resolution** | Two players build on same tile simultaneously | First-come-first-served? Server timestamp? |
| **Shared Infrastructure Rules** | Can players connect roads/power across boundaries? | Always allowed? Mutual consent? Cost sharing? |
| **Competitive Mode Victory** | "Highest pop, first to milestone" mentioned | Formal conditions undefined |
| **Cooperative Mode** | Mentioned as alternative but no details | Shared budget? Shared zones? |
| **Late Joiner** | Can a player join mid-game? | What resources do they start with? |
| **Host Migration** | Marked "optional" in epic plan | If host disconnects, what happens? |
| **Player Color Conflicts** | What if two players want same color? | Auto-assign? First-choice? |

---

## Terminology Consistency Check

### Terms Used Correctly (Epic Plan -> Canon)

| Epic Plan Term | Canonical Term | Status |
|----------------|----------------|--------|
| Residential | Habitation | Mostly correct (EPIC 4 uses both) |
| Commercial | Exchange | Mostly correct |
| Industrial | Fabrication | Mostly correct |
| Power plant | Energy nexus | Inconsistent in EPIC 5 |
| Power lines | Energy conduit | Inconsistent in EPIC 5 |
| Water pump | Fluid extractor | Correct in EPIC 6 |
| Police | Enforcer | Not used in epic plan |
| Crime | Disorder | Not used in epic plan |
| Citizens | Beings | Mentioned in EPIC 17 only |
| Mayor | Overseer | Mentioned in EPIC 17 only |

### Recommendation

The epic plan was written before terminology was finalized. EPIC 17 defers theming to polish phase, which is acceptable. However, all technical documents (including epic plan feature lists) should be updated to use canonical terms to avoid confusion during implementation.

---

## Missing Systems Check

### Systems Mentioned in Epic Plan But Not in systems.yaml

| System | Epic Reference | Status |
|--------|----------------|--------|
| **PopulationSystem** | EPIC 10 (Population Dynamics) | **MISSING** - Critical gap |
| **NewspaperSystem** | EPIC 12 (Newspaper System) | Missing - Could be part of UISystem |
| **TutorialSystem** | EPIC 17 (Tutorial/Help) | Missing - Could be part of UISystem |
| **VictorySystem** | EPIC 1, EPIC 14 (Victory conditions) | Missing - Could be part of ProgressionSystem |

### Systems in systems.yaml But Not Explicitly in Epic Plan

| System | systems.yaml Location | Epic Coverage |
|--------|----------------------|---------------|
| SyncSystem | epic_1_multiplayer | Implicit in EPIC 1 |
| LandValueSystem | epic_10_simulation | Part of EPIC 10 |
| DemandSystem | epic_10_simulation | Part of EPIC 10 (RCI Demand) |
| ContaminationSystem | epic_9_services | Part of EPIC 10 (Pollution Simulation) |
| DisorderSystem | epic_9_services | Part of EPIC 10 (Crime Simulation) |

---

## Phase Ordering Analysis

### Current Phase Order (from canon CANON.md)

| Phase | Epics | Focus |
|-------|-------|-------|
| Phase 1 | 0, 1, 2, 3 | Foundation |
| Phase 2 | 4, 5, 7, 10 | Core Gameplay |
| Phase 3 | 6, 9, 11, 12 | Infrastructure |
| Phase 4 | 8, 13, 14 | Advanced |
| Phase 5 | 15, 16, 17 | Polish |

### Issues with Phase Order

1. **EPIC 7 vs EPIC 10 in Phase 2**
   - DemandSystem (EPIC 10) depends on TransportSystem (EPIC 7) for accessibility
   - Current order: "4, 5, 7, 10" suggests parallel development
   - **Recommendation:** Clarify that EPIC 7's basic road system must exist before full demand calculation

2. **EPIC 6 (Water) in Phase 3**
   - BuildingSystem (EPIC 4) may need water for full building logic
   - EPIC 6 depends on EPIC 2 and EPIC 4, not EPIC 7 or 10
   - **Recommendation:** Consider moving EPIC 6 to late Phase 2

3. **EPIC 9 (Services) in Phase 3**
   - DisorderSystem and ContaminationSystem are in EPIC 9
   - But EPIC 10 (Phase 2) references "Crime Simulation" and "Pollution Simulation"
   - **Conflict:** Either EPIC 10 contains basic crime/pollution, or EPIC 9 must be Phase 2
   - **Recommendation:** Split simulation systems - basic versions in EPIC 10, full service integration in EPIC 9

4. **EPIC 15 (Audio) in Phase 5**
   - Audio depends only on EPIC 0
   - Could be developed in parallel much earlier
   - **Recommendation:** No issue if resources permit parallel development

### Recommended Phase Order Adjustments

```
Phase 1 (unchanged): 0, 1, 2, 3
Phase 2 (reordered): 4, 7, 5, 6, 10
  - Reason: Roads (7) before Demand (10); Water (6) moved earlier
Phase 3 (adjusted): 9, 11, 12
  - Reason: Services completes simulation dependencies
Phase 4 (unchanged): 8, 13, 14
Phase 5 (unchanged): 15, 16, 17
```

---

## Recommendations

### Priority 1: Before Any Implementation

1. **Define PopulationSystem** in systems.yaml with full owns/provides/depends_on
2. **Add ITransportConnectable interface** for road proximity queries
3. **Resolve LandValue <-> Disorder circular dependency** with timing rules
4. **Clarify EPIC 10 minimal dependencies** - can it work without transport?

### Priority 2: Before Phase 2

5. **Add IServiceProvider interface** for coverage queries
6. **Add IDamageable interface** for disaster system
7. **Define multiplayer reconnection protocol**
8. **Define shared infrastructure rules** for competitive mode

### Priority 3: Before Phase 3

9. **Add missing network message types** for disasters, milestones
10. **Define IContaminationSource interface**
11. **Formalize victory conditions** in canon

### Priority 4: Cleanup

12. Update epic plan to use canonical terminology consistently
13. Define tile_sizes in patterns.yaml
14. Add AudioTriggerComponent or clarify audio event system
15. Document cooperative mode rules (or explicitly cut it)

---

## Questions for Product/User

1. **Shared Infrastructure:** In competitive mode, can players connect their road/power networks across boundaries? If yes, how is cost/maintenance split?

2. **Reconnection Behavior:** If a player disconnects, should their city continue simulating? Pause? Be taken over by AI?

3. **Late Join:** Can new players join an in-progress game? If yes, what starting resources/territory do they get?

4. **Cooperative Mode:** Is this a firm requirement? The canon and epic plan mention it but no details exist.

5. **Victory Conditions:** What are the formal win conditions for competitive mode?
   - First to X population?
   - First to build Y milestone?
   - Highest score at time limit?
   - All of the above as options?

6. **Host Migration:** If the host disconnects, should the game end, pause, or migrate to another player?

7. **EPIC 6 Timing:** Can buildings in EPIC 4 function without water, or should EPIC 6 come earlier?

8. **Basic vs Full Simulation:** Should EPIC 10 include basic crime/pollution, with EPIC 9 adding service-based mitigation? Or should all simulation be in EPIC 10?

---

## Appendix: Component Inventory

### Defined in Canon

| Component | System Owner | Interface |
|-----------|--------------|-----------|
| EnergyComponent | EnergySystem | IEnergyConsumer |
| EnergyProducerComponent | EnergySystem | IEnergyProducer |
| EnergyConduitComponent | EnergySystem | - |
| FluidComponent | FluidSystem | IFluidConsumer |
| FluidProducerComponent | FluidSystem | IFluidProducer |
| FluidConduitComponent | FluidSystem | - |
| ZoneComponent | ZoneSystem | - |
| BuildingComponent | BuildingSystem | IBuildable, IDemolishable |
| ConstructionComponent | BuildingSystem | - |
| TerrainComponent | TerrainSystem | - |
| RoadComponent | TransportSystem | - |
| TrafficComponent | TransportSystem | - |
| RailComponent | RailSystem | - |
| SubterraComponent | RailSystem | - |
| ServiceProviderComponent | ServicesSystem | - |
| ServiceCoverageComponent | ServicesSystem | - |
| TaxableComponent | EconomySystem | - |
| MaintenanceCostComponent | EconomySystem | - |
| DamageableComponent | DisasterSystem | - |
| OnFireComponent | DisasterSystem | - |
| OwnershipComponent | Cross-cutting | - |
| PositionComponent | Cross-cutting | - |

### Missing (Should Be Defined)

| Component | Suggested Owner | Purpose |
|-----------|-----------------|---------|
| PopulationComponent | PopulationSystem | Track beings in buildings |
| HealthComponent | PopulationSystem | Life expectancy tracking |
| EducationComponent | PopulationSystem | Education quotient |
| ValueComponent | LandValueSystem | Cached land value |
| DisorderComponent | DisorderSystem | Crime level per tile |
| ContaminationComponent | ContaminationSystem | Pollution level per tile |
| MilestoneComponent | ProgressionSystem | Achievement tracking |
| VictoryComponent | ProgressionSystem | Win condition tracking |

---

*End of Report*
