# Epic 17: Polish & Alien Theming - Ticket Breakdown

**Generated:** 2026-02-01
**Canon Version:** 0.17.0
**Status:** READY FOR VERIFICATION
**Total Tickets:** 98

---

## Overview

Epic 17 is unique among all epics: **it adds no new systems**. Instead, it focuses on polishing and optimizing the 16 previously implemented epics into a cohesive, performant, and thematically consistent experience. This is the final epic before release.

### Key Characteristics
- **System Type:** No new systems - optimization, integration, and polish only
- **Focus Areas:** Performance, terminology, accessibility, balance, testing
- **Core Principle Enforcement:** #5 (Alien Theme Consistency)
- **Success Criteria:** 60 FPS, <100 KB/s network, zero terminology violations, all integration tests passing

### Design Decisions (from Discussion)
- **DD-1:** LOD transitions use dithered blending over 10-15 tile ranges
- **DD-2:** 60 FPS is non-negotiable; implement automatic quality scaling
- **DD-3:** Alien terminology for game-world content; human terms for software UI (Settings, Quit, etc.)
- **DD-10:** Terminology lint: hard fail on player-facing code, warn on internal code
- **DD-12:** 30 FPS absolute floor on 512x512 with auto quality scaling
- **DD-13:** No explicit difficulty settings; sandbox modifiers without judgment labels
- **DD-14:** No mandatory tutorial; optional contextual hints with searchable help panel
- **DD-15:** Disasters always recoverable, never colony-ending
- **DD-16:** Accessibility P0: colorblind modes, key rebinding, text scaling, pause anytime
- **DD-17:** Release: 0 Critical/High bugs, <10 Medium, <25 Low

---

## Group A: Performance - Rendering (12 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-A01 | LOD system framework | M | Implement LODManager with 3-level system (LOD0/1/2), distance thresholds at 50/150/300 tiles | None |
| 17-A02 | LOD building models | L | Create simplified geometry for buildings at LOD1 (major details) and LOD2 (billboard/impostor) | 17-A01 |
| 17-A03 | LOD dithered transitions | M | Screen-door dithering during LOD transitions over 10-15 tile range, preserve bioluminescent glow | 17-A01 |
| 17-A04 | LOD vegetation | M | LOD levels for vegetation: full detail, reduced density, billboard sprites | 17-A01 |
| 17-A05 | Hierarchical frustum culling | M | Spatial hash-based culling with chunk-level early-out before entity-level | None |
| 17-A06 | Instanced rendering - buildings | L | GPU instancing for zone buildings, group by building type/material | 17-A05 |
| 17-A07 | Instanced rendering - vegetation | M | GPU instancing for all vegetation types with billboard fallback | 17-A05 |
| 17-A08 | Instanced rendering - infrastructure | M | GPU instancing for pathways, conduits, pipes with batched draw calls | 17-A05 |
| 17-A09 | Overlay texture rendering | M | GPU-based overlay rendering instead of per-tile, quarter-res option | None |
| 17-A10 | Draw call batching | M | Material-based batching, merge static geometry where possible | 17-A06 |
| 17-A11 | Incremental terrain mesh | M | Dirty chunk tracking, update only changed terrain chunks | None |
| 17-A12 | Automatic quality scaling | M | Dynamic resolution/LOD targeting 60 FPS, reduce vegetation/shadows/particles first | 17-A01, 17-A06 |

---

## Group B: Performance - Simulation (10 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-B01 | ECS spatial partitioning | L | Spatial hash for ContaminationSystem, DisorderSystem queries, dirty chunk tracking | None |
| 17-B02 | Tick distribution - LandValueSystem | M | Spread calculation across 16 ticks (32 rows/tick for 512x512) | None |
| 17-B03 | Tick distribution - ContaminationSystem | M | Process dirty chunks only, skip stable regions | 17-B01 |
| 17-B04 | Tick distribution - DisorderSystem | M | Cached query results, spatial partitioning integration | 17-B01 |
| 17-B05 | Sparse grid - PathwayGrid | M | Convert to sparse storage with chunk-based allocation, 50-80% memory savings | None |
| 17-B06 | Sparse grid - BuildingGrid | M | Convert to sparse storage, maintain fast lookup for occupied tiles | None |
| 17-B07 | ConflagrationGrid lazy allocation | S | Allocate only during active disasters, release when complete | None |
| 17-B08 | SIMD contamination decay | M | AVX2/SSE4 vectorized decay for contamination grid processing | None |
| 17-B09 | SIMD disorder spread | M | Vectorized neighbor averaging for disorder calculations | None |
| 17-B10 | Tick budget monitoring | M | Real-time tick time tracking, per-system profiling infrastructure | None |

---

## Group C: Performance - Network (8 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-C01 | Field-level delta encoding | L | Component field-level deltas instead of full component, 30-40% bandwidth reduction | None |
| 17-C02 | Grid region delta encoding | M | Region-based (16x16) delta sync for dense grids, 50-60% reduction | None |
| 17-C03 | Message batching | M | Batch state changes within tick, single compressed packet per tick | None |
| 17-C04 | Adaptive send rate | M | Reduce sync frequency for stable state, increase during active changes | 17-C03 |
| 17-C05 | Visual interpolation tuning | M | Smooth interpolation for cosmetic entities (0.8-0.9 smoothing factor) | None |
| 17-C06 | Bandwidth monitoring UI | S | Real-time bandwidth display (optional, developer mode) | None |
| 17-C07 | Protocol version negotiation | M | Client-server version exchange on connect, graceful fallback | None |
| 17-C08 | Network stress validation | M | Verify <100 KB/s per client under normal conditions | 17-C01, 17-C02, 17-C03 |

---

## Group D: Technical Debt Resolution (12 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-D01 | Code pattern audit | M | Scan for naming violations (camelCase fields, wrong suffixes), generate report | None |
| 17-D02 | Pattern violation fixes | M | Fix identified naming/pattern violations from audit | 17-D01 |
| 17-D03 | TODO/FIXME audit | S | Catalog all markers, categorize by severity (Critical/High/Medium/Low) | None |
| 17-D04 | Critical TODO resolution | M | Fix all Critical and High priority TODOs | 17-D03 |
| 17-D05 | Medium TODO resolution | M | Fix Medium priority TODOs as time permits | 17-D03 |
| 17-D06 | Dead code removal | M | Remove stub implementations, unused components, orphaned event types | None |
| 17-D07 | Error handling audit | M | Verify consistent error patterns across all systems | None |
| 17-D08 | Error handling fixes | M | Fix inconsistent error handling from audit | 17-D07 |
| 17-D09 | Memory leak audit | M | ASAN/Valgrind scan, verify RAII compliance, check EventBus subscriptions | None |
| 17-D10 | Memory leak fixes | M | Fix identified leaks, ensure weak_ptr for subscriptions | 17-D09 |
| 17-D11 | Thread safety audit | M | TSan scan, verify double-buffering for render-visible state | None |
| 17-D12 | Thread safety fixes | M | Fix identified race conditions, add missing synchronization | 17-D11 |

---

## Group E: Integration Polish (10 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-E01 | Event ordering audit | M | Document all cross-system event chains, identify circular dependencies | None |
| 17-E02 | Event ordering fixes | M | Implement phased event processing (Infrastructure -> State -> Simulation -> Notification) | 17-E01 |
| 17-E03 | Multiplayer sync edge case audit | M | Test late join, disconnect during action, reconnection scenarios | None |
| 17-E04 | Multiplayer sync fixes | L | Fix identified sync issues, improve reconnection handling | 17-E03 |
| 17-E05 | Interface contract verification | M | Verify all interfaces match canon contracts (return types, signatures) | None |
| 17-E06 | Interface contract fixes | M | Fix any interface mismatches from verification | 17-E05 |
| 17-E07 | Cross-system state consistency audit | M | Verify state coherence after multi-system operations | None |
| 17-E08 | State consistency fixes | M | Fix identified state inconsistencies | 17-E07 |
| 17-E09 | Save/load multiplayer integration | M | Verify save/load with connected clients, full resync after load | None |
| 17-E10 | Disaster + active construction edge cases | M | Test disaster during build, verify damage and construction state | None |

---

## Group F: Alien Terminology (12 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-F01 | StringTable class | M | Centralized string storage with ID-based lookup, format placeholder support | None |
| 17-F02 | String ID enum generation | M | Generate StringID enum from string table YAML files | 17-F01 |
| 17-F03 | String externalization - UI panels | L | Extract all UI panel text to string tables (~200 strings) | 17-F01 |
| 17-F04 | String externalization - tooltips | L | Extract all tooltip text to string tables (~500 strings) | 17-F01 |
| 17-F05 | String externalization - notifications | M | Extract notification text to string tables (~100 strings) | 17-F01 |
| 17-F06 | String externalization - error messages | M | Extract error/warning messages to string tables (~150 strings) | 17-F01 |
| 17-F07 | Terminology lint tool | M | Scan source for banned human terms, generate violation report | None |
| 17-F08 | Terminology CI integration | S | Add terminology lint to CI, hard fail on player-facing violations | 17-F07 |
| 17-F09 | UI string audit | M | Manual review of all UI text in context, verify alien terminology | 17-F03 |
| 17-F10 | Notification string audit | S | Verify all notifications use alien terms | 17-F05 |
| 17-F11 | Loading screen content | M | Implement rotating tips (60%) and lore (40%), bioluminescent animation | 17-F01 |
| 17-F12 | Final terminology verification | M | Full automated scan + manual review, zero violations | 17-F07, 17-F09, 17-F10 |

---

## Group G: Accessibility (14 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-G01 | AccessibilitySettings struct | S | Colorblind mode, text scale, high contrast, reduce motion, screen reader flags | None |
| 17-G02 | Settings persistence - accessibility | S | Add accessibility settings to ClientSettings, persist to JSON | 17-G01 |
| 17-G03 | Colorblind mode - Deuteranopia | M | LUT shader for red-green colorblind (6% of males) | 17-G01 |
| 17-G04 | Colorblind mode - Protanopia | M | LUT shader for red-green colorblind (2% of males) | 17-G01 |
| 17-G05 | Colorblind mode - Tritanopia | S | LUT shader for blue-yellow colorblind (0.01%) | 17-G01 |
| 17-G06 | Pattern overlays for colors | M | Add patterns/shapes to supplement color-only information | 17-G03 |
| 17-G07 | Text scaling system | M | 80%-150% UI scale, dynamic layout reflow, tooltip expansion | 17-G01 |
| 17-G08 | High contrast mode | M | Color palette swapping, boosted contrast for all UI elements | 17-G01 |
| 17-G09 | Reduced motion mode | M | Disable parallax, reduce animation speeds 50%, static alternatives | 17-G01 |
| 17-G10 | Key rebinding system | L | Full rebinding for all actions, persist to settings, conflict detection | None |
| 17-G11 | Keyboard-only navigation | M | Full tab order, visible focus indicators, focus trapping in dialogs | None |
| 17-G12 | Audio cues for critical events | M | Distinct sounds for disasters, milestones, alerts; never silent for critical | None |
| 17-G13 | Pause anytime guarantee | S | Verify pause works in all states, no time-critical sequences | None |
| 17-G14 | Accessibility settings UI | M | Settings panel for all accessibility options with live preview | 17-G01 |

---

## Group H: Balance & Tuning (10 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-H01 | Balance data externalization | M | Extract costs, rates, thresholds to YAML config files | None |
| 17-H02 | Balance hot-reload | M | File watcher for balance config, emit BalanceChangedEvent | 17-H01 |
| 17-H03 | Tuning override system | M | Layered config files for playtest profiles | 17-H01 |
| 17-H04 | Economic balance pass | M | Verify early game (first milestone 25-35 min), mid game growth curves | 17-H01 |
| 17-H05 | Service coverage balance | M | Verify single building covers reasonable district, funding scaling | None |
| 17-H06 | Population growth balance | M | Verify growth rates match targets (5-10% early, 0.1-0.5% late) | None |
| 17-H07 | Disaster damage caps | M | Implement max damage percentages (15-25%), tutorial protection <5K pop | None |
| 17-H08 | Disaster recovery support | M | Recovery zone designation, emergency service prioritization | 17-H07 |
| 17-H09 | Sandbox modifier UI | M | Starting credits, growth speed, catastrophe frequency sliders | None |
| 17-H10 | Balance validation tests | M | Automated playthrough bots to verify balance targets | 17-H04 |

---

## Group I: UX Polish (10 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-I01 | Tool selection feedback | M | Persistent glow on selected tool, cursor change, active state clarity | None |
| 17-I02 | Build placement feedback | M | Green/red ghost preview (<50ms), construction sound, completion chime | None |
| 17-I03 | Error message clarity | M | What/Why/How format for all errors, category colors and icons | None |
| 17-I04 | Notification grouping | M | Combine same-type notifications, catastrophe pins until acknowledged | None |
| 17-I05 | Notification priority | M | Critical/Warning/Info tiers with different durations (8s/5s/3s) | 17-I04 |
| 17-I06 | Information panel layering | M | Essential always visible, Details on expand, Deep in Statistics tab | None |
| 17-I07 | Help panel implementation | M | Searchable help topics by system, "Show me" button to highlight UI | None |
| 17-I08 | Contextual hint system | M | Non-blocking tooltips on first action, dismiss after 5s, show once | None |
| 17-I09 | First-time welcome modal | S | Opt-in for hints, remember choice, accessible from settings | 17-I08 |
| 17-I10 | Session bookend messages | S | Welcome back with cycle count, colony state sync message on quit | None |

---

## Group J: Celebration & Feedback (6 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-J01 | Celebration particle system | M | Client-only particles for milestones, bioluminescent sparkles | None |
| 17-J02 | Celebration queue | M | Priority-based queue, max 2 concurrent effects, deduplication | 17-J01 |
| 17-J03 | Celebration quality scaling | S | Disable/reduce particles on low-spec, auto-scale based on framerate | 17-J01 |
| 17-J04 | Milestone celebration effects | M | Tiered fanfares (subtle -> moderate -> dramatic -> triumphant) | 17-J01 |
| 17-J05 | Achievement alert styling | M | Gold/cyan achievement pulse, longer duration, special audio | None |
| 17-J06 | Focus highlight effects | M | Emissive boost for focused entity, optional stencil glow | None |

---

## Group K: Testing Infrastructure (14 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 17-K01 | Integration test suite | L | Cross-system integration tests for all epic pairs, 100% interface coverage | None |
| 17-K02 | Performance benchmark framework | L | Automated benchmarks for tick, frame, memory, network, save/load | None |
| 17-K03 | Benchmark baseline tracking | M | Store baselines in JSON, compare against fixed targets + previous release | 17-K02 |
| 17-K04 | Performance regression alerts | M | CI alerts on regression (5% frame, 10% tick, 15% memory thresholds) | 17-K02 |
| 17-K05 | Stress test framework | M | Configurable stress scenarios, automated AI players | None |
| 17-K06 | 72-hour soak test | L | Release candidate soak test with 4 players, 512x512, memory monitoring | 17-K05 |
| 17-K07 | Multiplayer test harness | M | Spawn N clients programmatically, connection churn scenarios | None |
| 17-K08 | Test world generator | M | Procedural test data generation with configurable density | None |
| 17-K09 | CI benchmark integration | M | Run benchmarks on every commit, dashboard with 30-build trends | 17-K02 |
| 17-K10 | Memory profiling integration | M | Automated memory audits in CI, leak detection | None |
| 17-K11 | Screenshot comparison tests | M | UI regression detection via automated screenshot comparison | None |
| 17-K12 | Save compatibility test suite | M | Load saves from all supported versions (current + 2 prior releases) | None |
| 17-K13 | Platform test matrix | M | CI tests on Windows + Linux, min-spec + recommended hardware profiles | None |
| 17-K14 | Telemetry instrumentation | M | Click counts, friction pattern detection, session metrics | None |

---

## Ticket Summary

| Group | Name | Tickets |
|-------|------|---------|
| A | Performance - Rendering | 12 |
| B | Performance - Simulation | 10 |
| C | Performance - Network | 8 |
| D | Technical Debt Resolution | 12 |
| E | Integration Polish | 10 |
| F | Alien Terminology | 12 |
| G | Accessibility | 14 |
| H | Balance & Tuning | 10 |
| I | UX Polish | 10 |
| J | Celebration & Feedback | 6 |
| K | Testing Infrastructure | 14 |
| **Total** | | **98** |

---

## Size Distribution

| Size | Count | Percentage |
|------|-------|------------|
| S | 12 | 12% |
| M | 68 | 69% |
| L | 18 | 18% |

---

## Critical Path

The following tickets are on the critical path for release:

1. **Performance Foundation:**
   - 17-A01 (LOD framework) -> 17-A02, 17-A03, 17-A12
   - 17-A05 (Frustum culling) -> 17-A06, 17-A07, 17-A08
   - 17-B01 (Spatial partitioning) -> 17-B02, 17-B03, 17-B04

2. **Terminology Compliance:**
   - 17-F01 (StringTable) -> 17-F03, 17-F04, 17-F05, 17-F06
   - 17-F07 (Lint tool) -> 17-F08 -> 17-F12

3. **Quality Gates:**
   - 17-K02 (Benchmark framework) -> 17-K03 -> 17-K04 -> 17-K09
   - 17-K01 (Integration tests) -> Release validation

4. **Accessibility Baseline:**
   - 17-G01 (Settings struct) -> 17-G02 -> 17-G03, 17-G07, 17-G08
   - 17-G10 (Key rebinding) -> Release requirement

---

## Release Criteria Mapping

| Release Criterion | Tickets | Validation |
|-------------------|---------|------------|
| 60 FPS recommended spec | 17-A01 - 17-A12, 17-B01 - 17-B10 | 17-K02, 17-K04 |
| <100 KB/s network | 17-C01 - 17-C08 | 17-C08, 17-K02 |
| Zero terminology violations | 17-F01 - 17-F12 | 17-F12 |
| All integration tests passing | 17-E01 - 17-E10 | 17-K01 |
| P0 accessibility features | 17-G01 - 17-G14 | Manual testing |
| 0 Critical/High bugs | 17-D01 - 17-D12 | 17-K01, 17-K06 |
| <10 Medium bugs | All groups | Pre-release audit |

---

**End of Epic 17 Ticket Breakdown**
