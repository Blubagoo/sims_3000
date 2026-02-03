# ZergCity Project Confidence Assessment

**Generated:** 2026-02-01
**Canon Version:** 0.17.0
**Assessed By:** Claude (Opus 4.5)
**Purpose:** Identify areas requiring validation before implementation

---

## 1. Overall Project Confidence

**Rating: 6.5 / 10**

### Justification

The ZergCity project demonstrates exceptional planning discipline and documentation quality. The canon system, with its YAML-based specifications for systems, interfaces, patterns, and terminology, represents industry-leading project organization. However, several factors temper confidence:

**Strengths:**
- Comprehensive canon documentation with clear change process
- Well-defined system boundaries with explicit owns/does_not_own demarcation
- Detailed interface contracts between systems
- Realistic phasing (Foundation -> Core -> Services -> Advanced -> Polish)
- 400+ tickets with dependency graphs and size estimates
- Clear architectural patterns (ECS, dense grids, pool/network models)

**Concerns:**
- No implementation exists yet - all confidence is theoretical
- SDL_GPU is relatively new technology with limited community resources
- Multiplayer-first architecture adds complexity to every system
- Team size/velocity unknown - 18 epics is ambitious
- Some epics have implicit dependencies not fully captured
- 3D toon shading pipeline is non-trivial to implement correctly

---

## 2. Per-Epic Confidence Ratings

### Phase 1: Foundation

| Epic | Name | Confidence | Rationale |
|------|------|------------|-----------|
| **0** | Project Foundation | **8/10** | Well-scoped (34 tickets). SDL3 setup, ECS integration, game loop are standard patterns. EnTT is mature. Risk: SDL3 is newer than SDL2, may have subtle issues. |
| **1** | Multiplayer Networking | **5/10** | ENet is proven but snapshot sync at 20Hz with delta compression is complex. 22 tickets may underestimate edge cases. Rollback/reconciliation adds significant complexity. LZ4 integration straightforward. |
| **2** | Core Rendering | **6/10** | 50 tickets is substantial but SDL_GPU toon shading is underspecified in community resources. Camera system well-defined. Grid rendering with instancing is achievable. Outline rendering may require iteration. |
| **3** | Terrain System | **7/10** | Dense grid pattern is well-documented. Vegetation generation, contamination grids are straightforward. Heightmap handling clear. Dependency on Epic 2 rendering is reasonable. |

### Phase 2: Core Gameplay

| Epic | Name | Confidence | Rationale |
|------|------|------------|-----------|
| **4** | Zoning & Building | **7/10** | 48 tickets, well-structured. Zone placement, building spawning, development levels are core SimCity patterns. Demand system integration may need iteration. |
| **5** | Energy System | **7/10** | Pool model well-defined. Priority-based distribution (4 levels) is clear. Power plant types straightforward. CoverageGrid pattern established. |
| **6** | Fluid System | **7/10** | Mirrors energy system patterns. Fluid treatment, recycling mechanics add complexity but are contained. Double-buffering pattern documented. |
| **7** | Transport System | **6/10** | Network model more complex than pool model. Pathfinding, cross-ownership connectivity, traffic simulation add risk. RailSystem as separate subsystem increases scope. |
| **10** | Simulation Core | **5/10** | ~60 tickets, most complex epic. Population dynamics, demand calculation, land value, disorder all interact. Circular dependencies require careful double-buffering. LandValueSystem depends on many systems. |

### Phase 3: Services & UI

| Epic | Name | Confidence | Rationale |
|------|------|------------|-----------|
| **9** | Services System | **6/10** | Coverage-based services are well-understood. Emergency response adds complexity. Service quality effects on other systems create coupling. |
| **11** | Economy System | **6/10** | Treasury, taxation, expenses are clear. Credit advance system adds complexity. Balancing income/expenses against simulation is iterative. |
| **12** | UI System | **5/10** | Dual UI modes (Classic + Holographic) doubles the work. Event system defined but UI implementation highly iterative. Overlay rendering, notifications, tool modes complex. |

### Phase 4: Advanced

| Epic | Name | Confidence | Rationale |
|------|------|------------|-----------|
| **8** | Ports & Trade | **6/10** | Trade routes, import/export mechanics are defined. Spaceport capacity, supply/demand curves require balancing. Limited inter-system dependencies. |
| **13** | Disasters | **5/10** | 60 tickets. Fire spread simulation (ConflagrationGrid), damage propagation, recovery zones are complex. Emergency response integration with Epic 9. Cascade effects hard to test. |
| **14** | Progression | **6/10** | 52 tickets. Milestones, edicts, arcologies well-defined. TranscendenceAura effects add complexity. Balancing progression pacing is iterative. |

### Phase 5: Polish

| Epic | Name | Confidence | Rationale |
|------|------|------------|-----------|
| **15** | Audio System | **7/10** | 52 tickets. Positional audio, variant selection, crossfades are standard patterns. OGG Vorbis decoding straightforward. Music streaming well-understood. |
| **16** | Save/Load | **6/10** | 58 tickets. Binary format with LZ4 compression defined. Component versioning and migration framework adds complexity. Multiplayer load-while-connected is tricky. World Template export well-scoped. |
| **17** | Theming | **7/10** | Visual polish, UI skin variants are contained. Depends heavily on earlier rendering and UI work being stable. Lower technical risk. |

---

## 3. Architecture Confidence

### ECS Architecture: **8/10**
- EnTT is battle-tested
- Clear component/system separation in canon
- Dense grid exception pattern is well-justified
- Tick priority ordering (5-90) provides clear execution order

### Multiplayer Architecture: **5/10**
- Dedicated server model is correct choice
- Snapshot sync with delta compression is proven pattern
- **Risk:** Authority model for actions not fully specified
- **Risk:** Reconciliation of client predictions unclear
- **Risk:** Database persistence + networking interaction complex

### Rendering Architecture: **6/10**
- SDL_GPU provides modern GPU abstraction
- Toon shading pipeline documented in patterns.yaml
- **Risk:** SDL_GPU is new, community examples limited
- **Risk:** Outline rendering techniques may need R&D
- **Risk:** LOD system for 512x512 maps not detailed

### Persistence Architecture: **7/10**
- Section-based binary format is sensible
- LZ4 compression reuses Epic 1 infrastructure
- Component versioning/migration framework well-designed
- **Risk:** Multiplayer rollback authority voting is complex

---

## 4. Technical Risk Areas

### High Risk

| Risk | Impact | Mitigation |
|------|--------|------------|
| **SDL_GPU maturity** | Rendering system may hit undocumented limitations | Prototype toon shader pipeline early in Epic 2 |
| **Snapshot sync performance** | 20Hz updates with delta compression may not scale | Benchmark with 512x512 map, 4 players early |
| **Circular simulation dependencies** | Land value, demand, population create feedback loops | Double-buffering pattern defined, but needs careful testing |
| **Fire spread simulation** | ConflagrationGrid (3 bytes/tile) performance at scale | Profile 512x512 with max conflagration scenario |

### Medium Risk

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Transport pathfinding** | Network model more complex than pool model | Consider simpler connectivity heuristics before A* |
| **UI dual modes** | Classic + Holographic doubles UI implementation | Consider shipping with one mode first |
| **Multiplayer load-while-connected** | Coordinating pause/clear/load/resume across clients | Extensive integration testing required |
| **Component migration** | Save format changes require migration functions | Establish migration testing early, version all components from start |

### Lower Risk

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Audio positional falloff** | 50-tile max, zoom-aware attenuation | Standard techniques, well-understood |
| **LZ4 compression** | Performance characteristics well-known | Library is mature, minimal risk |
| **ECS performance** | EnTT is optimized | Proven at scale in other projects |

---

## 5. Scope/Estimation Confidence

### Ticket Count Analysis

| Epic | Tickets | Size Breakdown | Concern Level |
|------|---------|----------------|---------------|
| Epic 0 | 34 | Reasonable for foundation | Low |
| Epic 1 | 22 | May underestimate edge cases | Medium |
| Epic 2 | 50 | Appropriate for rendering | Low |
| Epic 4 | 48 | Reasonable for core gameplay | Low |
| Epic 10 | ~60 | Most complex, may need more | Medium |
| Epic 13 | 60 | Disaster systems are complex | Medium |
| Epic 14 | 52 | Progression may need iteration | Low |
| Epic 15 | 52 | Audio is well-scoped | Low |
| Epic 16 | 58 | Save/load is comprehensive | Low |

### Estimation Concerns

1. **Integration tickets underrepresented:** Cross-epic integration work often discovered late
2. **Testing tickets light:** Multiplayer testing especially requires more tickets
3. **Art asset creation not ticketed:** Hybrid procedural/hand-modeled pipeline needs asset work
4. **Performance optimization implicit:** No explicit optimization phase tickets
5. **Balance/tuning not scoped:** Simulation balance is highly iterative

### Recommended Additions

- Add 5-10 integration tickets per phase boundary
- Add explicit performance profiling tickets for Epics 1, 2, 10, 13
- Consider dedicated "hardening" sprint between phases
- Budget 20% buffer for discovered work

---

## 6. Integration Complexity

### Critical Integration Points

| Systems | Complexity | Notes |
|---------|------------|-------|
| **Simulation <-> Networking** | **Very High** | Every simulation tick must be network-synchronized |
| **Rendering <-> Terrain <-> Zoning** | **High** | Visual representation of all game state |
| **Services <-> Simulation <-> Disasters** | **High** | Emergency response, coverage effects, cascade damage |
| **Economy <-> All Systems** | **High** | Income/expense affects everything |
| **UI <-> Every System** | **High** | All systems need UI representation |

### Dependency Depth Concerns

1. **Epic 10 (Simulation)** depends on: 0, 1, 2, 3, 4, 5, 6, 7, 9
   - Late in dependency chain, integration issues surface late

2. **Epic 12 (UI)** depends on: 0, 1, 2, 4, 5, 6, 7, 9, 10, 11
   - Must visualize all prior systems
   - Changes to any system may require UI updates

3. **Epic 13 (Disasters)** depends on: 0, 1, 2, 3, 4, 5, 9
   - Disaster effects cascade through multiple systems
   - Fire affects buildings (4), requires emergency response (9), may cut power (5)

### Interface Stability Risk

- **ISimulatable:** Many systems implement, changes ripple widely
- **Dense grid interfaces:** Multiple grids (10+), format changes expensive
- **Network message formats:** Must be versioned from start
- **Component schemas:** Migration required for any change post-launch

---

## 7. Assumptions Log

### Confirmed Assumptions (Low Risk)

| Assumption | Confidence | Source |
|------------|------------|--------|
| SDL3 is production-ready | High | SDL3 officially released |
| EnTT supports required patterns | High | Well-documented, widely used |
| LZ4 compression adequate | High | Proven library |
| 2-4 players is target | High | Canon states explicitly |
| ECS is appropriate architecture | High | Industry standard for simulation games |

### Unvalidated Assumptions (Medium Risk)

| Assumption | Confidence | Validation Needed |
|------------|------------|-------------------|
| SDL_GPU toon shading achievable | Medium | Prototype shader pipeline |
| 20Hz snapshot sync performs at scale | Medium | Benchmark with 512x512, 4 players |
| Dense grid pattern performs at 512x512 | Medium | Profile grid operations |
| Delta compression ratio sufficient | Medium | Measure actual save sizes |
| Component migration can be automated | Medium | Build migration framework, test |

### Questionable Assumptions (High Risk)

| Assumption | Confidence | Concern |
|------------|------------|---------|
| Team can deliver 18 epics | Unknown | No team size/velocity specified |
| Art pipeline can produce assets | Unknown | No artist mentioned |
| Simulation balance achievable | Low | Highly iterative, no balance pass planned |
| Players want multiplayer sandbox | Unknown | Market validation not mentioned |
| Single codebase for server/client | Medium | May need separation |

---

## 8. Recommendations

### Immediate Actions (Before Implementation)

1. **Prototype SDL_GPU toon shader pipeline**
   - Build minimal toon shader with outlines
   - Test on target hardware
   - Validate SDL_GPU capabilities match requirements

2. **Prototype multiplayer snapshot sync**
   - Build minimal ENet connection with snapshot at 20Hz
   - Measure bandwidth with simulated entity counts
   - Test delta compression ratio

3. **Define team capacity**
   - 18 epics at ~50 tickets average = 900 tickets
   - At 3 tickets/week/developer, need 6 developers for 1-year timeline
   - Adjust scope or timeline based on actual team

4. **Validate dense grid performance**
   - Create 512x512 test grids
   - Profile iteration, update, serialization performance
   - Confirm assumptions about memory layout benefits

### Short-Term (During Phase 1)

5. **Establish component versioning from day one**
   - Every component gets version number
   - Migration framework built before first save file

6. **Build integration test harness early**
   - Multiplayer tests from Epic 1 forward
   - Automated join/leave/sync verification

7. **Create performance benchmark suite**
   - Run after each epic
   - Track regression early

8. **Consider single UI mode for MVP**
   - Classic OR Holographic, not both
   - Second mode is Phase 5 polish

### Medium-Term (During Implementation)

9. **Add explicit integration sprints**
   - End of Phase 1: Foundation integration
   - End of Phase 2: Core gameplay integration
   - End of Phase 3: Full gameplay loop

10. **Budget for balance iteration**
    - Simulation tuning is unbounded
    - Define "good enough" criteria early
    - Use config files for all balance values

11. **Plan for late-discovered work**
    - 20% buffer on all estimates
    - Quarterly scope review

### Documentation Gaps to Address

12. **Network message format specification**
    - Define versioning strategy
    - Document all message types upfront

13. **Art asset pipeline documentation**
    - Procedural generation parameters
    - Hand-modeled asset requirements
    - glTF export conventions

14. **Deployment architecture**
    - Server hosting requirements
    - Database (which one?) specifications
    - Client distribution method

---

## Summary

The ZergCity project is **well-planned but ambitious**. The canon system and documentation quality are exceptional - among the best project planning I've seen. However, several factors require attention:

- **Validate SDL_GPU early** - it's the highest technical risk
- **Prototype multiplayer sync** - networking affects all systems
- **Define team capacity** - 18 epics is substantial work
- **Plan for iteration** - simulation balance cannot be designed, only discovered

The 6.5/10 confidence rating reflects strong planning offset by unvalidated technical assumptions and unknown team capacity. With targeted prototyping of high-risk areas, confidence could rise to 8/10.

**Recommended focus areas for validation:**
1. SDL_GPU toon shader prototype (Epic 2 risk)
2. Multiplayer sync benchmark (Epic 1 risk)
3. Simulation feedback loop testing (Epic 10 risk)
4. Dense grid performance at 512x512 (architectural risk)

---

*This assessment is based on documentation review only. Confidence ratings should be updated as implementation validates or invalidates assumptions.*
