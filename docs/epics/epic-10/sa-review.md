# Epic 10: Simulation Core - Systems Architect Review

**Reviewer:** Systems Architect
**Date:** 2026-02-08
**Scope:** All Epic 10 implementation files (sim, population, demand, disorder, contamination, landvalue, economy stubs, adapters, building interfaces)
**Files Reviewed:** ~120 header/source files across 10 directories

---

## Summary Assessment

Epic 10 is a solid, well-structured implementation of the simulation heartbeat. The code demonstrates strong separation of concerns, consistent patterns, and thoughtful interface design. The double-buffered grid pattern is correctly implemented. The decomposition into free functions for calculations (birth rate, employment matching, disorder generation, etc.) makes the system highly testable.

**Overall: PASS with 7 concerns requiring follow-up tickets.**

The architecture is sound for a single-threaded simulation at the target 512x512 grid size. The main risks are (1) heap allocations per tick in the spread algorithms, (2) the `GridSwapCoordinator` creating a hard coupling to concrete grid types, and (3) the disconnect between `SimulationCore` and `SimulationPipeline` (two parallel system-scheduling mechanisms).

---

## Architecture Strengths

### 1. Clean Interface Layering

The `ISimulatable` / `ISimulationTime` split (defined in `include/sims3000/core/`) is excellent. Systems receive read-only time via `const ISimulationTime&`, preventing accidental clock mutation. The interface is minimal: `getCurrentTick()`, `getTickDelta()`, `getInterpolation()`, `getTotalTime()`. No unnecessary methods.

**File:** `include/sims3000/core/ISimulatable.h:23-45`, `include/sims3000/core/ISimulationTime.h:27-63`

### 2. Pure Calculation Functions

All simulation formulas are implemented as pure, side-effect-free functions that take data structs as input and return result structs. This is the right pattern for testability and parallelization potential.

Examples:
- `calculate_birth_rate(const PopulationData&, uint32_t)` -> `BirthRateResult`
- `calculate_habitation_demand(const HabitationInputs&)` -> `HabitationDemandResult`
- `match_employment(uint32_t, uint32_t, uint32_t)` -> `EmploymentMatchingResult`
- `calculate_tile_value(const LandValueTileInput&)` -> `uint8_t`
- `calculate_disorder_amount(const DisorderSource&)` -> `uint8_t`

**Files:** `src/population/BirthRateCalculation.cpp`, `src/demand/HabitationDemand.cpp`, `src/population/EmploymentMatching.cpp`, `src/landvalue/FullValueRecalculation.cpp`, `src/disorder/DisorderGeneration.cpp`

### 3. Correct Double-Buffer Protocol

The double-buffering design is well-executed. Both `DisorderGrid` and `ContaminationGrid` implement the same swap pattern: `std::swap(m_grid, m_previous_grid)` for O(1) pointer swap. Readers call `get_level_previous_tick()`, writers call `set_level()`/`add_disorder()`/`add_contamination()`. The `LandValueGrid` correctly avoids double-buffering since it reads from other grids' previous buffers.

**Files:** `src/disorder/DisorderGrid.cpp:74-76`, `src/contamination/ContaminationGrid.cpp:104-107`, `include/sims3000/landvalue/LandValueGrid.h:1-9`

### 4. Saturating Arithmetic Throughout

All grid operations correctly use saturating arithmetic -- no overflow/underflow wraps. Example from `DisorderGrid::add_disorder()`:

```cpp
uint16_t sum = static_cast<uint16_t>(m_grid[idx]) + static_cast<uint16_t>(amount);
m_grid[idx] = sum > 255 ? 255 : static_cast<uint8_t>(sum);
```

**File:** `src/disorder/DisorderGrid.cpp:46-53`, `src/contamination/ContaminationGrid.cpp:54-71`

### 5. Well-Scoped Adapters for Cross-Epic Integration

The `EnergyContaminationAdapter` and `TrafficContaminationAdapter` cleanly bridge the energy and transport systems to the contamination system via `IContaminationSource`. They follow the adapter pattern correctly: owned data + interface implementation.

**Files:** `include/sims3000/energy/EnergyContaminationAdapter.h`, `include/sims3000/transport/TrafficContaminationAdapter.h`, `src/energy/EnergyContaminationAdapter.cpp`, `src/transport/TrafficContaminationAdapter.cpp`

### 6. Consistent Component Size Assertions

All data structs include `static_assert` on their sizes, preventing accidental bloat:
- `DisorderCell`: 1 byte
- `ContaminationCell`: 2 bytes
- `LandValueCell`: 2 bytes
- `DisorderComponent`: 12 bytes
- `ContaminationComponent`: 16 bytes
- `BuildingOccupancyComponent`: <= 12 bytes
- `SimulationStateHeader`: 28 bytes

### 7. Frequency Gating

The `PopulationSystem` correctly gates expensive calculations: demographics every 100 ticks (5s), migration every 20 ticks (1s), employment every tick. `DemandSystem` gates at every 5 ticks (250ms). This is a good pattern for performance.

**File:** `src/population/PopulationSystem.cpp:26-48`, `src/demand/DemandSystem.cpp:25-38`

---

## Technical Concerns

### CONCERN-1: Duplicate System Scheduling Mechanisms (Medium Severity)

There are two independent system schedulers that do essentially the same thing:

1. **`SimulationCore`** (`include/sims3000/sim/SimulationCore.h`) - Manages `ISimulatable*` systems with priority sorting, accumulator pattern, speed control, and time tracking.
2. **`SimulationPipeline`** (`include/sims3000/sim/SimulationPipeline.h`) - Manages `std::function<void(float)>` callbacks with priority sorting, from Epic 4.

These are redundant. `SimulationCore` was designed for Epic 10 and supersedes `SimulationPipeline`. However, `SimulationPipeline` is still referenced by existing Epic 4 code (building/zone systems register via function callbacks). This creates confusion about which mechanism is canonical.

Additionally, `SimulationCore::update()` calls `system->tick(*this)`, passing itself as `ISimulationTime`. But `SimulationPipeline::tick()` calls `entry.tick_fn(delta_time)`, passing only the delta. Systems registered with `SimulationPipeline` do not receive `ISimulationTime`.

**Files:** `include/sims3000/sim/SimulationCore.h:43-201`, `include/sims3000/sim/SimulationPipeline.h:38-85`

**Recommendation:** Create a migration ticket to consolidate `SimulationPipeline` into `SimulationCore`, converting all Epic 4 systems to `ISimulatable`.

---

### CONCERN-2: Heap Allocation Every Tick in Spread Algorithms (Medium Severity)

Both disorder and contamination spread algorithms allocate delta buffers on the heap every tick:

**Disorder spread** (`src/disorder/DisorderSpread.cpp:22`):
```cpp
std::vector<int16_t> delta(total_cells, 0);  // 512*512 = 524,288 int16_t = 1MB per tick
```

**Contamination spread** (`src/contamination/ContaminationSpread.cpp:54`):
```cpp
std::vector<SpreadDelta> deltas(grid_size);  // 512*512 = 524,288 SpreadDelta (2 bytes each) = 1MB per tick
```

At 20 Hz, this is ~40MB/s of allocation and deallocation. While modern allocators handle this acceptably, it creates GC pressure and cache pollution. At Fastest speed (60 Hz effective), this becomes ~120MB/s.

**Recommendation:** Pre-allocate delta buffers as members of `DisorderSystem` and `ContaminationSystem` respectively. Clear with `memset` each tick instead of reallocating. This is a simple optimization that eliminates heap churn.

---

### CONCERN-3: GridSwapCoordinator Hard-Couples to Concrete Types (Low Severity)

`GridSwapCoordinator` (`include/sims3000/sim/GridSwapCoordinator.h`) directly includes and references `DisorderGrid` and `ContaminationGrid` by concrete type. Its `swap_all()` method is hardcoded to exactly two grids.

```cpp
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/contamination/ContaminationGrid.h>
```

If a new double-buffered grid is added in future epics (e.g., temperature, radiation), this class must be modified. A more extensible design would use an `IDoubleBufferedGrid` interface with a `swap_buffers()` method, and `GridSwapCoordinator` would manage a vector of those.

However, the current design is simple and correct for the current requirements. The concern is purely about future extensibility.

**Files:** `include/sims3000/sim/GridSwapCoordinator.h:29-30`, `include/sims3000/sim/GridSwapCoordinator.h:79-86`

**Recommendation:** Low priority. Document in code that adding new double-buffered grids requires updating this class. Optionally, extract an `ISwappable` interface in a future refactoring ticket.

---

### CONCERN-4: DisorderSystem::tick() Swaps Buffers Internally Despite GridSwapCoordinator (Medium Severity)

`DisorderSystem::tick()` calls `m_grid.swap_buffers()` at line 22 of `src/disorder/DisorderSystem.cpp`. Similarly, `ContaminationSystem::tick()` calls `m_grid.swap_buffers()` at line 20 of `src/contamination/ContaminationSystem.cpp`.

But `GridSwapCoordinator::swap_all()` also calls `swap_buffers()` on both grids. If both are called during the same tick, the buffers get swapped twice, which is equivalent to not swapping at all -- a correctness bug.

The intended design (documented in `GridSwapCoordinator.h` comments) is that `swap_all()` is called BEFORE any system ticks. But each system also swaps internally. One of these must be removed.

**Files:**
- `src/disorder/DisorderSystem.cpp:22` -- calls `m_grid.swap_buffers()`
- `src/contamination/ContaminationSystem.cpp:20` -- calls `m_grid.swap_buffers()`
- `include/sims3000/sim/GridSwapCoordinator.h:79-86` -- calls `swap_buffers()` on both

**Recommendation:** HIGH PRIORITY fix. Either:
(a) Remove `swap_buffers()` from `DisorderSystem::tick()` and `ContaminationSystem::tick()`, relying solely on `GridSwapCoordinator::swap_all()` called from `SimulationCore`.
(b) Remove `GridSwapCoordinator` entirely and let each system manage its own swap.

Option (a) is preferred because it ensures all grids swap atomically before any system writes, which is the documented design intent.

---

### CONCERN-5: ContaminationGrid::add_contamination() Dominant Type Logic Bug (Low Severity)

In `src/contamination/ContaminationGrid.cpp:66`:

```cpp
if (cell.dominant_type == 0 || amount > 0) {
    cell.dominant_type = type;
}
```

The condition `amount > 0` is always true when this code is reached (since `add_contamination` is called with nonzero amounts in practice). This means the dominant type is always overwritten to the last caller's type, regardless of whether the new contribution is actually dominant. If two sources contribute to the same cell -- say Industrial contributes 100 and Traffic contributes 5 -- Traffic will be marked as dominant because it was applied last.

The correct logic should compare the new amount against the existing level to determine true dominance, or track per-type levels.

**File:** `src/contamination/ContaminationGrid.cpp:64-68`

**Recommendation:** Fix the dominant type tracking. Options:
(a) Track the largest single contribution and update dominant type only when a new contribution exceeds the previous largest.
(b) Accept "last write wins" as a deliberate simplification and document it.

---

### CONCERN-6: DisorderEvents::detect_disorder_events() Generates O(N) Events (Medium Severity)

`detect_disorder_events()` in `src/disorder/DisorderEvents.cpp` scans every tile and emits a `DisorderEvent` for each tile that crosses a threshold. On a 512x512 grid with widespread disorder changes, this could generate hundreds of thousands of events in a single tick, each allocated as a `push_back` into a returned `std::vector`.

**File:** `src/disorder/DisorderEvents.cpp:40-81`

This is not called from `DisorderSystem::tick()` yet (it is a utility function), but when it is integrated, it will be a performance problem.

**Recommendation:** Add event batching or rate-limiting:
- Only emit events for a sampled subset of tiles per tick.
- Or aggregate into region-level events (e.g., "District X has high disorder") rather than per-tile.
- Or emit a single `DisorderCrisisBeganEvent` with a tile count, matching the E10-079 ticket spec.

---

### CONCERN-7: Mutable Fallback Pattern in PopulationSystem/DemandSystem (Low Severity)

Both `PopulationSystem::get_population_mut()` and `DemandSystem::get_demand_data_mut()` have a questionable fallback pattern:

```cpp
// src/population/PopulationSystem.cpp:62-66
if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
    return m_population[0];  // Returns slot 0 as fallback
}
```

If the caller passes an invalid player ID, they silently receive a mutable reference to player 0's data, which they could then corrupt. The const accessor correctly returns a static empty instance, but the mutable accessor returns live data belonging to a potentially different player.

**Files:** `src/population/PopulationSystem.cpp:58-67`, `src/demand/DemandSystem.cpp:79-86`

**Recommendation:** Either:
(a) Return a static mutable dummy (which is also unsafe in multithreaded contexts).
(b) Add an assertion/log and still return the dummy, but not player 0's real data.
(c) Return `std::optional<std::reference_wrapper<T>>` or a pointer that can be null-checked.

---

## Additional Observations (Non-Blocking)

### OBS-1: Serialization Header Only, No Grid Data Serialization

`SimulationSerializer` only handles the header (28 bytes). The ticket E10-111 specifies serializing grid data and per-player components, but only the header is implemented. This is acceptable if E10-111 is marked as incomplete/deferred.

**File:** `src/sim/SimulationSerializer.cpp`

### OBS-2: Tick Starts at 1, Not 0

`SimulationCore::update()` increments `m_tick` before ticking systems:

```cpp
// src/sim/SimulationCore.cpp:43-44
m_accumulator -= SIMULATION_TICK_DELTA;
m_tick++;
```

So the first tick that systems see is tick 1, not tick 0. The frequency gating in `PopulationSystem` uses `current_tick % 100 == 0`, which means demographics run at tick 100, 200, etc. -- but NOT at tick 0 (game start). This is fine for ongoing simulation, but means there is no initial demographics calculation. New players start with the default `PopulationData{}` values until tick 100.

**Files:** `src/sim/SimulationCore.cpp:43-44`, `src/population/PopulationSystem.cpp:37`

### OBS-3: Contamination Spread Reads Previous Tick, Disorder Spread Reads Current Tick

The contamination spread algorithm reads from `get_level_previous_tick()` (correct for double-buffered pattern):

```cpp
// src/contamination/ContaminationSpread.cpp:60
const uint8_t level = grid.get_level_previous_tick(x, y);
```

But the disorder spread algorithm reads from `get_level()` (the current buffer):

```cpp
// src/disorder/DisorderSpread.cpp:31
uint8_t level = grid.get_level(x, y);
```

This is technically correct because `DisorderSpread` uses a delta buffer pattern that is independent of double-buffering. But it creates an inconsistency: if `GridSwapCoordinator::swap_all()` has already been called, the disorder spread reads from what was just written (the new current buffer after generation). This means disorder spread operates on partially-computed data (generation has happened, spread has not). The contamination spread, by contrast, reads from the previous tick's fully-computed data.

The disorder approach is actually valid (it spreads based on the current tick's generated values), but the inconsistency should be documented.

**Files:** `src/disorder/DisorderSpread.cpp:31`, `src/contamination/ContaminationSpread.cpp:60`

### OBS-4: IGridOverlay Interface Location

The `IGridOverlay` interface is in `include/sims3000/services/IGridOverlay.h` (under the services namespace), but it is used by disorder, contamination, and land value overlays which are not in the services domain. This is a minor namespace mismatch.

### OBS-5: `static_assert` Discrepancy on MigrationFactors

`MigrationFactors` has a static_assert checking `sizeof(MigrationFactors) == 11` with a comment saying "approximately 12 bytes." The actual size is 11 bytes, which is fine, but the comment is misleading.

**File:** `include/sims3000/population/MigrationFactors.h:53`

---

## Recommended Follow-Up Tickets

| ID | Title | Severity | Related Concern |
|----|-------|----------|-----------------|
| E10-F01 | Consolidate SimulationPipeline into SimulationCore | Medium | CONCERN-1 |
| E10-F02 | Pre-allocate spread delta buffers to avoid per-tick heap allocation | Medium | CONCERN-2 |
| E10-F03 | Fix double-swap bug: remove swap_buffers() from system tick() methods | High | CONCERN-4 |
| E10-F04 | Fix ContaminationGrid dominant type tracking | Low | CONCERN-5 |
| E10-F05 | Add event batching/rate-limiting to disorder event detection | Medium | CONCERN-6 |
| E10-F06 | Fix mutable data fallback returning player 0 data on invalid ID | Low | CONCERN-7 |
| E10-F07 | Complete grid data serialization (E10-111 remainder) | Medium | OBS-1 |

---

## Conclusion

The Epic 10 Simulation Core is architecturally well-designed. The choice to decompose all calculations into pure functions, use double-buffered grids for circular dependencies, and gate calculation frequencies are all correct design decisions that will pay dividends in testability and performance.

The one high-priority issue (CONCERN-4: double-swap bug) should be addressed before any integration testing, as it will cause incorrect simulation behavior if `GridSwapCoordinator` is used alongside the systems' internal swaps. The medium-priority heap allocation concern (CONCERN-2) should be addressed before performance profiling, as it represents unnecessary overhead that is trivial to eliminate.

The remaining concerns are low-severity cleanup items that can be addressed as part of normal development.

---

*End of SA Review*
