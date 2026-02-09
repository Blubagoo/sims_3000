# Epic 10: Simulation Core - Game Designer Review

**Reviewer:** Game Designer (GD)
**Date:** 2026-02-08
**Scope:** All implementation files across SimulationCore, PopulationSystem, DemandSystem, DisorderSystem, ContaminationSystem, LandValueSystem

---

## Summary Assessment

Epic 10 delivers a **well-structured simulation foundation** with clean separation of concerns, pure calculation functions, and thoughtful double-buffering for circular dependencies. The systems will create recognizable city-builder gameplay loops once fully integrated. However, several balance constants and interaction patterns need attention to ensure the simulation feels responsive, fair, and fun rather than opaque or punishing.

**Overall Grade: B+** -- Strong architecture, needs tuning passes on balance constants and a few design gaps that could undermine player agency.

---

## Strengths

### 1. Pure Calculation Architecture
Every formula (birth rate, death rate, demand, disorder generation, etc.) is implemented as a pure function with a result struct that exposes intermediate values. This is excellent for game design iteration because:
- Formulas can be tuned independently without touching system plumbing
- Result structs (e.g., `BirthRateResult`, `DeathRateResult`, `AttractivenessResult`) provide exactly the data the UI needs to explain "why" something is happening
- Unit testing of individual formulas is trivial

### 2. Factor Breakdown for Player Understanding
The `DemandFactors` struct (`include/sims3000/demand/DemandData.h`) and the `DemandFactorsUI` system (`src/demand/DemandFactorsUI.cpp`) provide per-factor breakdowns with human-readable descriptions ("Strong Growth", "Decline", etc.) and a `get_dominant_factor_name()` function. This gives players clear, actionable information about what is driving zone demand.

### 3. Sandbox Safety Rails
Several design-level guardrails prevent catastrophic collapse:
- Death rate capped at 5% per cycle (`DeathRateCalculation.h:29` -- `MAX_DEATH_PERCENT = 0.05f`)
- Migration out capped at 5% per cycle (`MigrationOut.h:21` -- `MAX_OUT_RATE = 0.05f`)
- Population never goes negative (`NaturalGrowth.cpp:38`)
- Migration never causes total exodus (`MigrationOut.cpp:49`)
- Minimum 1 birth if population > 0 and housing available (`BirthRateCalculation.cpp:72`)

These prevent the classic city-builder death spiral where a small mistake causes irreversible population collapse.

### 4. Double-Buffer Pattern for Circular Dependencies
The `GridSwapCoordinator` (`include/sims3000/sim/GridSwapCoordinator.h`) and the read-previous/write-current pattern across Disorder, Contamination, and LandValue systems is elegant. This avoids tick-order oscillation where disorder raises land value penalty which raises disorder in the same tick, creating an unstable feedback loop.

### 5. Milestone System
The milestone system (`include/sims3000/population/PopulationMilestones.h`) provides clear progression markers (Village at 100, Town at 500, City at 2000, Metropolis at 10000, Megalopolis at 50000). These give players satisfying "level up" moments and clear goals to aim for.

### 6. Phased Tick Frequencies
Different systems update at different frequencies -- demographics every 100 ticks (5s), migration every 20 ticks (1s), employment every tick, demand every 5 ticks. This prevents all changes from hitting simultaneously and creates a natural rhythm to city evolution.

---

## Concerns

### BALANCE: Disorder-LandValue Feedback Loop Is Too Aggressive

**Files:** `src/disorder/LandValueDisorderEffect.cpp:28-33`, `src/landvalue/DisorderPenalty.cpp:17-18`

The land value effect on disorder (`LandValueDisorderEffect.cpp:31-32`) applies `extra = disorder * (1.0 - land_value/255.0)`. At low land value (0), this **doubles** the disorder on a tile every tick. Meanwhile, the disorder penalty on land value (`DisorderPenalty.cpp:17`) subtracts up to 40 points from land value.

This creates a positive feedback loop:
1. Low land value doubles disorder
2. Higher disorder further lowers land value
3. Repeat

Even with double-buffering preventing same-tick oscillation, this will converge to maximum disorder / minimum land value within a few dozen ticks for any area that starts slightly bad. Players in a low-land-value industrial zone will see disorder spike to 255 very quickly with no way to recover without massive enforcer investment.

**Recommendation:** Cap the land-value-to-disorder amplification at +50% rather than +100%. Or make the effect sublinear (e.g., square root scaling). The current linear doubling is too harsh.

---

### BALANCE: Contamination Spread vs. Decay Equilibrium

**Files:** `include/sims3000/contamination/ContaminationDecay.h:23` (BASE_DECAY_RATE = 2), `src/contamination/ContaminationSpread.cpp:70-71` (cardinal spread = level/8), `include/sims3000/contamination/IndustrialContamination.h:36` (base output 50-200)

A level-3 fabrication building outputs 200 contamination per tick at full occupancy. Base decay is 2 per tick. Even with water (+3) and forest (+3) bonuses, maximum decay is 8 per tick. At 8-neighbor spread with cardinal level/8 and diagonal level/16, contamination will propagate vastly faster than it decays.

Running the math: a single high-density factory at 200/tick will contaminate surrounding tiles at 200/8=25 per tick cardinally. With only 2-8 decay, contamination will spread outward indefinitely. Players building industry will face a contamination crisis that is nearly impossible to contain without simply not building fabrication zones.

**Recommendation:** Consider one or more of:
- Increase BASE_DECAY_RATE to 4-5
- Add a contamination resistance factor for tiles far from sources
- Make spread reduce the source proportionally (like disorder spread does -- `DisorderSpread.cpp:66` subtracts from source but contamination spread does not)
- Add a "cleanup" building type that actively removes contamination

---

### BALANCE: Attractiveness Weights Don't Sum to 1.0

**File:** `include/sims3000/population/AttractivenessCalculation.h:19-27`

Positive weights sum to: 0.20 + 0.15 + 0.10 + 0.15 + 0.15 = 0.75
Negative weights sum to: 0.10 + 0.10 + 0.03 + 0.02 = 0.25

With all factors at 50 (neutral), the net attraction would be:
- Positive: 50 * 0.75 = 37.5
- Negative: 50 * 0.25 = 12.5
- Net: +25

This means a perfectly neutral city has a strong positive attraction bias. Migration will always be positive in a "meh" city, which removes the challenge of making a city attractive. The player never has to work to attract migrants; they just need to avoid terrible conditions.

**Recommendation:** Either normalize weights so positive and negative sum to equal values, or rebalance the default factor values so that a neutral city nets approximately 0 attraction.

---

### PACING: Migration Runs Too Frequently Relative to Demographics

**File:** `include/sims3000/population/PopulationSystem.h:62-63`

Demographics (births/deaths) run every 100 ticks (5 seconds), but migration runs every 20 ticks (1 second). With `BASE_MIGRATION = 50` beings per cycle plus a colony size bonus, migration will dominate population change. A city of 10,000 with neutral attraction would gain ~60 migrants per migration tick (50 base + 10 colony bonus * ~1.0 multiplier), meaning 300 migrants per 100-tick demographic cycle. Meanwhile, at default 15/1000 birth rate, a 10,000-population city produces only ~150 births per cycle.

Migration being 2x faster than natural growth feels wrong for a city builder. Players should feel like they are growing their city, not just being a migration magnet.

**Recommendation:** Reduce migration frequency to every 50 ticks, or divide migration amounts by the ratio of migration frequency to demographic frequency (currently 5x). Alternatively, make BASE_MIGRATION scale with city size more gradually (diminishing returns).

---

### BALANCE: Unemployment Effect Is Applied Every Tick Without Smoothing

**File:** `src/population/UnemploymentEffects.cpp:34-43`

`apply_unemployment_effect()` directly sets harmony_index based on current unemployment rate. At 60% unemployment, the penalty is -30 harmony (capped). Since this is applied every tick via `update_employment()`, harmony will instantly snap to the penalty-modified value. There is no smoothing or gradual change.

Combined with the fact that harmony affects birth rate (0.5x-1.5x range), unemployment causes an immediate cascade: jobs lost -> harmony drops instantly -> births slow -> less labor -> less demand -> more buildings abandon -> more job loss.

**Recommendation:** Apply the unemployment harmony modifier as a gradual blend toward the target (e.g., `harmony += (target - current) * 0.1` per tick) rather than an immediate set. This gives players time to react to unemployment before it spirals into demographic collapse.

---

### EDGE CASE: Birth Rate Minimum 1 Creates Infinite Growth for Tiny Cities

**File:** `src/population/BirthRateCalculation.cpp:72`

`result.births = std::max(1u, rounded_births)` guarantees at least 1 birth whenever population > 0 and housing > 0, regardless of modifiers. For a city of 1 being with terrible conditions (harmony 0, health 0, housing barely available), the effective birth rate could be near zero, but 1 birth will still occur.

Combined with the "never cause total exodus" rule, a city of 1 being can never die and always grows. This removes consequence from poor city management for very small cities.

**Recommendation:** Only guarantee minimum 1 birth if `rounded_births > 0` OR if population is above some minimum threshold (e.g., 10 beings). A city of 1 should be able to die off if conditions are terrible.

---

### PLAYER FEEDBACK: Disorder Events Scan Every Tile Every Tick

**File:** `src/disorder/DisorderEvents.cpp:40-81`

`detect_disorder_events()` scans every tile comparing current and previous levels. On a 512x512 grid, this generates a potentially huge event vector (up to 262,144 events in the worst case -- every tile could spike simultaneously). More practically, in a spreading disorder crisis, hundreds of HighDisorderWarning events would fire simultaneously.

This would flood the UI event queue and desensitize players. In SimCity, the newspaper headline "CRIME ON THE RISE" works because it is one message. Here, getting 200 separate "tile (x, y) has high disorder" events is noise, not signal.

**Recommendation:** Rate-limit event detection to run every N ticks (e.g., every 20 ticks). Aggregate tile-level events into region-level events (e.g., "Disorder crisis in northwest quadrant" with tile count and average severity). The `CityWideDisorder` event type is the right idea; extend this approach to quadrant/district-level warnings.

---

### BALANCE: Energy Contamination Is Binary and Extreme

**Files:** `include/sims3000/contamination/EnergyContamination.h:35` (200, 120, 40), `src/contamination/EnergyContamination.cpp:27`

A carbon nexus outputs a flat 200 contamination per tick regardless of its utilization or output level. This is the same as a level-3 fabrication building at full capacity. Since energy plants are required infrastructure (not optional), players who rely on carbon power face an unavoidable 200/tick contamination bomb.

There is no production scaling -- an idle carbon plant pollutes just as much as one at full capacity. This removes a potential strategic lever (underutilize dirty power to reduce pollution).

**Recommendation:** Scale energy contamination by production ratio (e.g., `output * production_ratio`). An idle plant should produce minimal contamination. This gives players a reason to build excess clean capacity and transition away from dirty power gradually.

---

### EDGE CASE: Disorder Spread Reads from Current Buffer, Not Previous

**File:** `src/disorder/DisorderSpread.cpp:31`

`apply_disorder_spread()` calls `grid.get_level(x, y)` -- reading from the **current** buffer, not the previous tick buffer. However, the disorder system's tick sequence is: swap_buffers -> generate -> apply_land_value -> apply_spread -> apply_suppression. After swap, the current buffer is the old previous buffer (reset by swap). Then generate and land_value write new values into current. Then spread reads those just-written values.

This means spread is reading a mix of freshly-generated disorder and values that were just swapped in. This is internally consistent within the disorder system (all operations work on the same buffer within one tick), but it means spread is not using the clean double-buffer semantics that the architecture promises.

More importantly: the spread is order-dependent within a single tick because it reads from the buffer it is also writing to. The delta buffer mitigates this for the spread step itself, but the land-value amplification applied in the previous step (`LandValueDisorderEffect`) already modified the values that spread reads.

**Recommendation:** This is acceptable as-is since the delta buffer handles order-independence for the spread step. But document that the disorder system uses single-buffer-with-delta semantics internally, with double-buffering only at the system-to-system boundary.

---

### PACING: No Maximum Tick Catch-Up Limit

**File:** `src/sim/SimulationCore.cpp:41`

The `while (m_accumulator >= SIMULATION_TICK_DELTA)` loop has no maximum iteration count. If the game stalls for 5 seconds (e.g., loading, lag spike), the accumulator will be 5.0 at 3x speed = 15.0 seconds. At 20 ticks/second, that is 300 ticks in a single frame. All simulation systems will execute 300 times before rendering resumes, causing a freeze.

**Recommendation:** Cap catch-up ticks to a reasonable maximum (e.g., 5-10 ticks per frame). Excess accumulated time should be discarded. Players expect the simulation to slow down during lag, not to fast-forward 15 seconds of simulation in a single frame freeze.

---

### BALANCE: Water Proximity Bonus for Terrain Is Overly Generous

**Files:** `include/sims3000/landvalue/TerrainValueFactors.h:26-28`

Water adjacent gives +30, 1 tile gives +20, 2 tiles gives +10. Since water_distance <= 1 counts as "adjacent" (`src/landvalue/TerrainValueFactors.cpp:48`), any tile next to water gets +30 on top of the 128 base. Combined with crystal fields (+25), a waterfront crystal tile starts at 128 + 30 + 25 = 183 land value before any roads.

This means waterfront property is worth 43% more than inland property before the player does anything. In a 4-player multiplayer map, the player who gets the waterfront starts with a massive land value advantage, which reduces disorder, increases migration attractiveness, and compounds into a snowball advantage.

**Recommendation:** Reduce water bonuses to +15/+10/+5 to keep waterfront desirable but not dominant. Or make the bonus apply only to habitation zones (commercial and industrial buildings do not care about views).

---

### PLAYER FEEDBACK: PopulationStats Uses Hardcoded Contamination/Disorder Defaults

**File:** `src/population/PopulationStats.cpp:47-49`

When calculating life expectancy via `get_population_stat()`, contamination and disorder levels are hardcoded to 50:
```cpp
input.contamination_level = 50;  // Default, should be provided by caller
input.disorder_level = 50;       // Default, should be provided by caller
```

This means the life expectancy stat shown to players will always be incorrect -- it will not reflect actual city conditions. A heavily polluted city will show the same life expectancy as a pristine one.

**Recommendation:** Wire the actual contamination and disorder averages from the grid systems into this stat calculation. This is noted as a TODO in the code but should be elevated to a blocking follow-up ticket, since displaying incorrect stats is worse than displaying no stat at all.

---

### BALANCE: Milestone Thresholds Are Missing Intermediate Steps

**File:** `include/sims3000/population/PopulationMilestones.h:35`

Milestones are: 100, 500, 2000, 10000, 50000. The ticket spec (`tickets.md:691`) calls for: 100, 500, 1000, 2000, 5000, 10000, 30000, 60000, 90000, 120000. The implementation has only 5 milestones instead of the specified 10.

The gap from 2000 to 10000 is very large -- players will go potentially several minutes without any milestone reward. The gap from 10000 to 50000 is even worse.

**Recommendation:** Implement the full 10-milestone set from the ticket specification. More frequent milestones maintain engagement during the mid-game plateau.

---

### EMERGENT BEHAVIOR: Positive Feedback Loops Summary

The following cross-system feedback loops exist and need monitoring during playtesting:

1. **Disorder Death Spiral** (aggressive): Low land value -> doubled disorder -> lower land value -> more disorder -> buildings abandon -> less tax revenue -> less services -> more disorder
2. **Contamination Spread Doom** (moderate): Industry built -> contamination spreads -> land value drops -> disorder rises -> buildings abandon -> but contamination persists due to low decay
3. **Employment-Harmony Cascade** (moderate): Job loss -> harmony drops immediately -> birth rate drops -> labor force shrinks -> demand falls -> more closures
4. **Migration Snowball** (positive): Good city -> migration surplus -> more workers -> more demand -> more buildings -> higher land value -> even more attractive

Loop 1 is the most dangerous because the doubling effect in `LandValueDisorderEffect` can move faster than the player can build enforcers. Loops 2-3 are manageable with the sandbox caps. Loop 4 is the "virtuous cycle" that rewards good play -- this is desirable.

---

## Recommended Follow-Up Tickets

### GD-001: Cap Disorder Amplification from Low Land Value
**Priority:** P0 (affects core balance)
**Description:** Reduce the land-value-to-disorder amplification in `LandValueDisorderEffect.cpp` from 100% doubling to 50% maximum. Consider square-root or logarithmic scaling.
**Files:** `src/disorder/LandValueDisorderEffect.cpp:31-32`

### GD-002: Rebalance Contamination Spread vs. Decay Rates
**Priority:** P0 (affects core balance)
**Description:** Either increase BASE_DECAY_RATE to 4-5, or make contamination spread reduce the source (like disorder spread does), or both. Current equilibrium makes contamination uncontrollable.
**Files:** `include/sims3000/contamination/ContaminationDecay.h:23`, `src/contamination/ContaminationSpread.cpp`

### GD-003: Fix Attractiveness Weight Imbalance
**Priority:** P1 (affects migration balance)
**Description:** Normalize positive and negative attractiveness weights so a neutral city produces ~0 net attraction instead of +25. Either reweight or adjust default factor values.
**Files:** `include/sims3000/population/AttractivenessCalculation.h:19-27`

### GD-004: Add Tick Catch-Up Limit to SimulationCore
**Priority:** P1 (affects player experience during lag)
**Description:** Cap the tick catch-up loop at 5-10 ticks per frame to prevent freeze-then-fast-forward behavior.
**Files:** `src/sim/SimulationCore.cpp:41`

### GD-005: Smooth Unemployment Harmony Effects
**Priority:** P1 (affects cascade behavior)
**Description:** Apply unemployment harmony modifier as a gradual blend (e.g., 10% per tick toward target) rather than instant application.
**Files:** `src/population/UnemploymentEffects.cpp:34-43`

### GD-006: Scale Energy Contamination by Production Ratio
**Priority:** P1 (affects player agency)
**Description:** Multiply energy contamination output by the plant's production ratio so idle plants pollute less than active ones.
**Files:** `src/contamination/EnergyContamination.cpp:27`, `include/sims3000/contamination/EnergyContamination.h`

### GD-007: Rate-Limit and Aggregate Disorder Events
**Priority:** P1 (affects UI usability)
**Description:** Run disorder event detection every 20 ticks instead of every tick. Aggregate tile-level events into region-level summaries.
**Files:** `src/disorder/DisorderEvents.cpp`

### GD-008: Implement Full 10-Milestone Set
**Priority:** P2 (affects engagement pacing)
**Description:** Expand milestones from the current 5 to the spec's 10 thresholds: 100, 500, 1000, 2000, 5000, 10000, 30000, 60000, 90000, 120000.
**Files:** `include/sims3000/population/PopulationMilestones.h:35`

### GD-009: Wire Real Grid Stats into Life Expectancy Calculation
**Priority:** P2 (affects stat accuracy)
**Description:** Replace hardcoded contamination/disorder defaults (50) in `PopulationStats::get_population_stat()` with actual city averages from ContaminationGrid and DisorderGrid.
**Files:** `src/population/PopulationStats.cpp:47-49`

### GD-010: Reduce Water Proximity Land Value Bonus
**Priority:** P2 (affects multiplayer balance)
**Description:** Reduce water bonus from +30/+20/+10 to +15/+10/+5 to prevent waterfront snowball advantage. Alternatively, make the bonus zone-type-dependent.
**Files:** `include/sims3000/landvalue/TerrainValueFactors.h:26-28`

### GD-011: Remove Guaranteed Minimum Birth at Population 1
**Priority:** P2 (affects edge case fairness)
**Description:** Only guarantee minimum 1 birth when rounded_births > 0, or when population exceeds a small threshold (e.g., 10). A single being with terrible conditions should be able to die off.
**Files:** `src/population/BirthRateCalculation.cpp:72`

---

## System Interaction Matrix (For Reference)

| Producer System | Consumer System | Data Flow | Buffer Pattern |
|----------------|----------------|-----------|----------------|
| DisorderSystem | LandValueSystem | disorder level | Previous tick (double-buffer) |
| ContaminationSystem | LandValueSystem | contamination level | Previous tick (double-buffer) |
| LandValueSystem | DisorderSystem | land value | Current tick (read directly) |
| PopulationSystem | DemandSystem | population, employment | Per-player components |
| DemandSystem | BuildingSystem | demand values, caps | IDemandProvider interface |
| DisorderSystem | PopulationSystem | average disorder | Grid stat query |
| ContaminationSystem | PopulationSystem | average contamination | Grid stat query |
| PopulationSystem | AttractivenessCalc | harmony, jobs, housing | MigrationFactors component |
| UnemploymentEffects | PopulationSystem | harmony modifier | Direct write |

---

*End of GD Review -- Epic 10: Simulation Core*
