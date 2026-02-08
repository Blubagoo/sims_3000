# Epic FF: Fast Follow - Tickets

**Epic:** FF - Fast Follow (Bugs & Issues Backlog)
**Created:** 2026-02-07
**Purpose:** Track bugs, issues, and small improvements discovered during development

---

## Ticket Summary Table

| ID | Title | Priority | Found In | Status |
|----|-------|----------|----------|--------|
| FF-001 | Multi-instance blank window bug | Medium | Epic 3 | Open |
| FF-002 | BuildingSpawningLoop not spawning buildings into placed zones | High | Epic 4 | Open |

---

## Tickets

---

### FF-001: Multi-instance blank window bug

**Priority:** Medium
**Found During:** Epic 3 terrain integration
**Date Reported:** 2026-02-07

#### Description

When launching a second instance of sims_3000.exe, the window appears blank (no rendering) even though the console shows successful initialization:
- Terrain shaders loaded successfully
- Terrain pipeline created
- 64 chunks initialized
- "Entering main loop" reached

The first instance renders correctly. Both instances use the same seed (325) and initialize without errors.

#### Reproduction Steps

1. Launch `sims_3000.exe` (first instance renders terrain correctly)
2. Launch `sims_3000.exe` again (second instance window is blank)
3. Both console windows show successful initialization

#### Suspected Causes

- Swapchain present failing silently when window not focused
- Shared config file conflict (`%AppData%/Sims3000/config.json`)
- Window creation timing issue
- SDL_GPU device/swapchain initialization order

#### Impact

- Cannot verify multiplayer visually on single machine
- Need two physical machines for multiplayer testing until fixed

#### Acceptance Criteria

- [ ] Two instances of sims_3000.exe can run simultaneously
- [ ] Both instances render terrain correctly
- [ ] Both instances respond to input independently

#### Notes

This is NOT an SDL_GPU resource limitation - SDL_GPU is efficient. This is a bug in our multi-instance handling code.

---

### FF-002: BuildingSpawningLoop not spawning buildings into placed zones

**Priority:** High
**Found During:** Epic 4 demo integration testing
**Date Reported:** 2026-02-07

#### Description

When zones are placed via the keyboard controls (4/5/6 + Z), the zone grid overlay renders correctly (green/blue/orange rectangles), but `BuildingSpawningLoop::scan_for_overseer()` never spawns buildings into the designated zones. All individual subsystem unit tests pass, but the end-to-end chain fails in the live application.

The rendering pipeline IS working — a manually spawned test building (via `BuildingFactory::spawn_building()` in init code) renders correctly as a wireframe box. The issue is specifically in the automatic spawn loop.

#### Investigation So Far

1. **Demand provider was disconnected** — `ZoneSystem` had no external demand provider set, so `get_demand_for_type()` returned 0. **Fixed:** added `m_zoneSystem->set_external_demand_provider(&m_stubDemand)` in `initZoneBuilding()`.

2. **Stub providers pass permissively** — `StubEnergyProvider`, `StubFluidProvider`, etc. all return permissive values. The `BuildingSpawnChecker` has `nullptr` for transport/energy/fluid (set at construction time), so those checks are skipped entirely.

3. **Template pool exists** — `register_initial_templates()` registers 5+ Habitation/Low templates (IDs 1-5). `get_templates_for_pool(Habitation, Low)` should return candidates.

4. **Enum mappings match** — `ZoneType::Habitation=0` ↔ `ZoneBuildingType::Habitation=0`, `ZoneDensity::LowDensity=0` ↔ `DensityLevel::Low=0`.

5. **Scan interval is 20 ticks (1 second)** — After 5+ seconds of waiting, multiple scans should have executed.

#### Suspected Remaining Causes

- `BuildingSpawnChecker` subsystem pointers are set at `BuildingSystem` construction time. The `set_*_provider()` methods update `BuildingSystem` members but NOT the internal `m_spawn_checker`'s pointers (see comment in `BuildingSystem::set_energy_provider()`). The demand check goes through `m_zone_system->get_demand_for_type()` which was fixed, but verify no other stale pointer issue exists.
- Zone state might not be `Designated` after `place_zones()` — verify with a unit test that calls `place_zones()` then checks `get_zone_state()`.
- The 256×256 grid full scan (65,536 cells) per `scan_for_overseer()` may have a performance or logic issue at scale.
- `m_clock.accumulate()` might return 0 ticks if the clock is paused or misconfigured in the client's Playing state.

#### Reproduction Steps

1. Build Release and launch via desktop shortcut (server + client)
2. Press 4 to select Habitation zone mode
3. Press Z to place a 5×5 zone at camera focus
4. Wait 10+ seconds
5. Observe: zone rectangles render, but no building wireframe boxes appear

#### Acceptance Criteria

- [ ] Placing a Habitation zone and waiting causes buildings to auto-spawn (cyan wireframe = Materializing)
- [ ] Buildings progress through construction and become Active (white wireframe)
- [ ] Multiple zone types (Exchange, Fabrication) also spawn buildings
- [ ] Spawning works without manual intervention beyond zone placement

#### Files of Interest

- `src/app/Application.cpp` — `initZoneBuilding()`, `tickZoneBuilding()`
- `src/building/BuildingSpawningLoop.cpp` — `tick()`, `scan_for_overseer()`
- `src/building/BuildingSpawnChecker.cpp` — `can_spawn_building()`
- `src/building/BuildingSystem.cpp` — constructor (subsystem pointer wiring)
- `src/building/TemplateSelector.cpp` — `select_template()`

---

## Adding New Tickets

Use this template:

```markdown
### FF-XXX: [Title]

**Priority:** High/Medium/Low
**Found During:** [Epic or feature]
**Date Reported:** YYYY-MM-DD

#### Description
[What's the bug/issue?]

#### Reproduction Steps
1. [Step 1]
2. [Step 2]

#### Acceptance Criteria
- [ ] [Criterion 1]
- [ ] [Criterion 2]
```
