# QA Engineer Answers: Epic 17 Cross-Agent Questions

**Author:** QA Engineer Agent
**Date:** 2026-02-01
**In Response To:** Systems Architect and Game Designer Questions
**Status:** COMPLETE

---

## Answers to Systems Architect Questions

### SA-7: Test Coverage Targets

**Question:** "What percentage of code coverage should we target for integration tests?"

**Answer:**

For Epic 17, I recommend a tiered coverage approach:

| Test Type | Coverage Target | Rationale |
|-----------|-----------------|-----------|
| Unit Tests | 80% line coverage | Industry standard for critical systems |
| Integration Tests | 60% code coverage, 100% interface coverage | Focus on cross-system boundaries |
| End-to-End Tests | 40% path coverage, 100% critical paths | Cover happy paths and main error paths |

**Specific Recommendations:**

1. **Integration Test Focus Areas (100% coverage required):**
   - All interface contracts (IEnergyProvider, IFluidProvider, etc.)
   - Cross-system event chains (Building -> Energy -> Zone)
   - Dense grid double-buffering synchronization
   - Multiplayer state sync boundaries

2. **Integration Test Acceptable Gaps (60% is fine):**
   - Cosmetic animation code
   - Debug/logging code paths
   - Platform-specific fallback paths
   - Error message formatting

3. **Coverage Tooling:**
   - Use gcov/lcov for C++ coverage
   - Track coverage trends per build (should never decrease)
   - Generate HTML reports for CI artifacts

**Why not 100%?** Diminishing returns. The last 20% of coverage often tests trivial code (getters, error formatting) while the bugs hide in integration boundaries.

---

### SA-8: Performance Benchmark Baselines

**Question:** "Should benchmarks compare against previous release, or fixed targets?"

**Answer:**

**Use both approaches in a layered system:**

| Baseline Type | Purpose | Failure Threshold |
|---------------|---------|-------------------|
| Fixed Targets | Ensure minimum quality bar | Hard fail if violated |
| Previous Release | Detect regressions | Warning at 5%, fail at 10% |
| Previous Build | Catch immediate regressions | Warning at 3%, fail at 5% |

**Fixed Targets (from my analysis Section 5.1):**

```yaml
performance_gates:
  tick_time:
    empty_map: 5ms
    small_map_128: 15ms
    medium_map_256: 30ms
    large_map_512: 45ms
  frame_time_p99: 16.67ms  # 60 FPS
  memory_512_map: 100MB
  network_bandwidth_per_client: 100KB/s
  save_time_512: 2000ms
  load_time_512: 3000ms
```

**Why Both?**
- Fixed targets catch slow decay over many releases
- Previous-release comparison catches sudden regressions
- Previous-build comparison provides immediate feedback in CI

**Implementation:**
1. Store performance baselines in `benchmarks/baselines/` as JSON
2. CI runs benchmarks and compares against all three
3. Dashboard shows trend over last 30 builds
4. Automatic baseline update on tagged releases

---

### SA-9: Stress Test Duration

**Question:** "How long should continuous stress tests run? 1 hour? 8 hours? 24 hours?"

**Answer:**

**Tiered stress testing based on trigger:**

| Trigger | Duration | Configuration |
|---------|----------|---------------|
| Per-Commit (CI) | Skip stress tests | Too slow for commit feedback |
| Hourly Build | 1 hour | Single server, 2 simulated clients |
| Nightly Build | 8 hours | Full 4-player, 512x512 map |
| Pre-Release | 24 hours minimum | Full stress + memory profiling |
| Release Candidate | 72 hours | Soak test with production config |

**Rationale:**

1. **1 Hour (Hourly):** Catches memory leaks with >60MB/hour growth rate. Fast feedback loop for developers.

2. **8 Hours (Nightly):** Represents a long play session. Catches:
   - Memory leaks >7MB/hour
   - Component pool fragmentation
   - Network buffer accumulation
   - Most race conditions under load

3. **24 Hours (Pre-Release):** Catches:
   - Slow memory leaks (>4MB/hour)
   - Database connection pool exhaustion
   - Timer overflow bugs (if using 32-bit timers)
   - Rare race conditions

4. **72 Hours (Release Candidate):** Represents a weekend server uptime. Final validation before shipping.

**Specific Stress Scenarios (from my analysis Section 4):**

```
Scenario: 4-player_max_activity
Duration: 8 hours (nightly) / 72 hours (RC)
Config:
  - 512x512 map, procedurally filled to 50% capacity
  - 4 AI clients performing continuous actions
  - Actions: zone placement, building demolition, overlay toggling
  - Injected disasters every 30 minutes
  - Save/load cycle every 2 hours
Success Criteria:
  - Memory growth < 50MB over 8 hours
  - Zero crashes
  - Zero desync events
  - Tick time p99 < 50ms
```

---

### SA-10: Performance Regression Criteria

**Question:** "What performance regression percentage should trigger a build failure?"

**Answer:**

**Tiered thresholds based on metric criticality:**

| Metric Category | Warning Threshold | Failure Threshold | Rationale |
|-----------------|-------------------|-------------------|-----------|
| Frame Time | 5% regression | 10% regression | Player-visible, high sensitivity |
| Tick Time | 10% regression | 20% regression | Simulation accuracy critical |
| Memory Usage | 15% increase | 30% increase | Affects min-spec users |
| Network Bandwidth | 20% increase | 50% increase | Affects multiplayer experience |
| Save/Load Time | 15% regression | 25% regression | Player perception sensitive |

**Specific Thresholds (matching my analysis Section 5.3):**

```yaml
regression_thresholds:
  frame_time:
    warning: 1.05x  # 5% slower
    failure: 1.10x  # 10% slower
  tick_time:
    warning: 1.10x
    failure: 1.20x
  peak_memory:
    warning: 1.15x
    failure: 1.30x
  network_bandwidth:
    warning: 1.20x
    failure: 1.50x
  save_time:
    warning: 1.15x
    failure: 1.25x
```

**Grace Period for Large Changes:**

When a known-large change lands (e.g., new system, major refactor):
1. Allow manual override with justification in PR description
2. Require new baseline to be explicitly committed
3. Track the regression in a "known performance debt" document

**P99 vs. Average:**

Always compare P99 (99th percentile) values, not averages. Average hides spikes that cause visible stutters. A 20% P99 regression with stable average often indicates a new worst-case code path.

---

### SA-11: Platform Testing Matrix

**Question:** "Should performance benchmarks run on multiple hardware configurations?"

**Answer:**

**Yes, but with a focused matrix:**

| Configuration | Priority | Benchmark Frequency |
|---------------|----------|---------------------|
| CI Server (Linux, headless) | P0 | Every commit |
| Minimum Spec (Win10, integrated GPU) | P0 | Nightly |
| Recommended Spec (Win10, discrete GPU) | P1 | Nightly |
| High-End (Win11, current-gen GPU) | P2 | Weekly |
| Linux Desktop (Ubuntu 22.04) | P1 | Nightly |

**Minimum Test Hardware (must pass):**

```
Minimum Spec Reference:
- CPU: Intel Core i5-6500 (4-core, 3.2GHz)
- RAM: 8 GB DDR4
- GPU: Intel UHD Graphics 630 (integrated)
- Storage: HDD (not SSD - worst case)
- OS: Windows 10 21H2
```

**Recommended Spec Reference:**

```
Recommended Spec Reference:
- CPU: Intel Core i7-9700 (8-core, 3.6GHz)
- RAM: 16 GB DDR4
- GPU: NVIDIA GTX 1060 6GB
- Storage: SSD
- OS: Windows 10/11
```

**Why Multiple Configurations?**

1. **CI Server:** Fast feedback, catches obvious regressions
2. **Minimum Spec:** Ensures game is playable for target audience
3. **Recommended Spec:** Validates target experience quality
4. **High-End:** Catches GPU driver compatibility issues
5. **Linux:** Cross-platform validation

**Practical Implementation:**

- Use cloud-based testing (AWS EC2 or Azure) with fixed instance types
- Min-spec tests on c5.xlarge (roughly equivalent)
- GPU tests on g4dn.xlarge (NVIDIA T4)
- Self-hosted runners for specific GPU models if needed

---

### SA-12: Terminology Test Automation

**Question:** "Should CI fail on terminology violations, or just warn?"

**Answer:**

**Fail on terminology violations in player-facing code. Warn in internal/debug code.**

| Code Location | Violation Response | Rationale |
|---------------|-------------------|-----------|
| UI strings (*.ui, strings/*.yaml) | Hard fail | Player-visible, Core Principle #5 |
| Tooltip text | Hard fail | Player-visible |
| Notification messages | Hard fail | Player-visible |
| Error messages shown to players | Hard fail | Player-visible |
| Log messages (internal) | Warning only | Developer-focused, may need human terms for debugging |
| Code comments | Ignore | Not player-visible |
| Test code | Warning only | Test descriptions may reference both terms |

**Implementation Approach:**

```yaml
# ci/terminology_check.yaml
terminology_lint:
  strict_paths:  # Fail on violation
    - src/ui/strings/
    - src/ui/tooltips/
    - src/notifications/
    - assets/text/
  warn_paths:  # Warn but don't fail
    - src/**/*.cpp  # Log messages
    - tests/
  ignore_paths:
    - docs/
    - comments in code

  forbidden_terms:
    - { human: "city", alien: "colony" }
    - { human: "citizen", alien: "being" }
    - { human: "residential", alien: "habitation" }
    # ... full list from terminology.yaml
```

**Recommended Workflow:**

1. Pre-commit hook runs terminology lint (fast, local feedback)
2. CI runs full terminology scan (authoritative)
3. PR cannot merge if strict-path violations exist
4. Weekly report of warning-path violations for gradual cleanup

**Edge Case Handling:**

- Allow `// TERMINOLOGY_EXCEPTION: <reason>` comments to bypass specific instances
- Exception comments require justification (e.g., external API reference)
- Track exception count - should trend toward zero

---

### SA-13: Multiplayer Test Configuration

**Question:** "How many concurrent sessions should the stress test support?"

**Answer:**

**Canon specifies 4-player maximum per server. Stress testing should validate 4 concurrent players, plus connection churn.**

| Test Scenario | Concurrent Sessions | Duration |
|---------------|---------------------|----------|
| Standard Stress | 4 players, stable | 8 hours (nightly) |
| Connection Churn | 4 slots, 2 stable + 2 cycling | 4 hours |
| Maximum Load | 4 players + pending connections | 2 hours |
| Reconnection Storm | 4 players disconnect/reconnect every 5 min | 1 hour |

**Detailed Test Configurations:**

```yaml
multiplayer_stress_tests:
  standard_4_player:
    players: 4
    duration: 8h
    map_size: 512x512
    activity: continuous_random
    success_criteria:
      - zero_desyncs
      - bandwidth_per_client: <100KB/s
      - tick_time_p99: <50ms

  connection_churn:
    stable_players: 2
    cycling_players: 2
    cycle_interval: 60s  # disconnect and reconnect every minute
    duration: 4h
    success_criteria:
      - zero_crashes
      - reconnection_time_p99: <5s
      - no_memory_leaks_per_reconnect

  reconnection_storm:
    players: 4
    reconnect_interval: 300s  # All 4 reconnect every 5 min
    duration: 1h
    success_criteria:
      - snapshot_transfer_time: <5s
      - state_consistency: verified
      - server_memory_stable: true

  pending_queue:
    connected_players: 4
    pending_connections: 4  # 4 clients trying to join full server
    duration: 2h
    success_criteria:
      - graceful_rejection: true
      - no_server_impact: true
      - clear_error_message: true
```

**Why Not More Than 4?**

Canon explicitly limits to 4 players per server. Testing beyond this is unnecessary for game functionality, but we should verify:
- Server correctly rejects 5th connection
- Rejection is graceful (no crash, clear message)
- Pending clients don't consume server resources

---

### SA-14: Save Compatibility Testing

**Question:** "How many prior versions should we maintain save compatibility with?"

**Answer:**

**Maintain compatibility with all saves from the same major version (0.x.x).**

| Version Range | Compatibility Requirement |
|---------------|---------------------------|
| Same minor version (0.17.x) | Full compatibility, no migration |
| Previous minor versions (0.16.x, 0.15.x, etc.) | Migration support required |
| Development builds | Best-effort, warn on load |
| Pre-1.0 to Post-1.0 | Not guaranteed, explicit migration tool |

**Specific Recommendations for Epic 17:**

```
Save Compatibility Matrix:
- 0.17.x saves: Full compatibility (always)
- 0.16.x saves: Migrate automatically, no user action
- 0.14.x-0.15.x saves: Migrate with user confirmation
- 0.1.x-0.13.x saves: Migrate with warning about potential issues
- Pre-0.1.x development saves: Reject with clear message
```

**Implementation Requirements:**

1. **Version Header:** Every save file must include version metadata
2. **Migration Chain:** Implement v16->v17, v15->v16, etc. (chained migration)
3. **Validation:** After migration, verify component checksums
4. **Rollback Save:** Before migration, create backup of original
5. **Test Matrix:** CI tests load saves from each supported version

**Testing Strategy:**

```yaml
save_compatibility_tests:
  test_saves:
    - version: "0.16.0"
      file: test_saves/v0.16_small_colony.sav
      expected: migrate_success
    - version: "0.15.0"
      file: test_saves/v0.15_medium_colony.sav
      expected: migrate_success_with_warning
    - version: "0.10.0"
      file: test_saves/v0.10_legacy.sav
      expected: migrate_success_with_warning
    - version: "0.5.0"
      file: test_saves/v0.5_ancient.sav
      expected: migrate_success_with_warning
    - version: "corrupted"
      file: test_saves/corrupted.sav
      expected: graceful_rejection

  validation_after_migration:
    - entity_count_preserved: true
    - component_checksums_valid: true
    - simulation_runs_100_ticks: true
    - resave_matches_migrated: true
```

**How Many Test Saves?**

- Minimum: 1 save per supported minor version (0.10 through 0.17 = 8 saves)
- Recommended: 3 saves per minor version (small/medium/large colony) = 24 saves
- Edge cases: Corrupted files, truncated files, bit-flipped files = 10+ variants

---

## Answers to Game Designer Questions

### GD-5: Terminology Testing Strategy

**Question:** "How do we automate scanning for non-alien terms? Should we have a 'terminology lint' in CI/CD? What's the test matrix for localization (if applicable)?"

**Answer:**

#### Automated Terminology Scanning

**Yes, implement a terminology lint tool as a CI gate.**

**Implementation Approach:**

```python
# tools/terminology_lint.py (conceptual)
class TerminologyLinter:
    def __init__(self, terminology_yaml):
        self.forbidden_terms = load_forbidden_terms(terminology_yaml)
        self.exceptions = load_exception_patterns()

    def scan_file(self, filepath):
        violations = []
        for line_num, line in enumerate(read_file(filepath)):
            # Skip comments
            if is_comment(line):
                continue
            # Skip exception-marked lines
            if has_exception_marker(line):
                continue
            # Check each forbidden term
            for human_term, alien_term in self.forbidden_terms:
                if human_term in line.lower():
                    violations.append({
                        'file': filepath,
                        'line': line_num,
                        'term': human_term,
                        'suggested': alien_term,
                        'context': line.strip()
                    })
        return violations
```

**CI Integration:**

```yaml
# .github/workflows/terminology.yml
terminology_check:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v3
    - name: Run Terminology Lint
      run: |
        python tools/terminology_lint.py \
          --config docs/canon/terminology.yaml \
          --strict-paths "src/ui/,assets/text/" \
          --warn-paths "src/" \
          --output report.json
    - name: Fail on Strict Violations
      run: |
        if [ $(jq '.strict_violations | length' report.json) -gt 0 ]; then
          echo "Terminology violations found in player-facing code!"
          jq '.strict_violations' report.json
          exit 1
        fi
```

#### Localization Test Matrix

**If localization is planned (even English-only for now, prepare for future):**

| Test | Purpose | Implementation |
|------|---------|----------------|
| String extraction verification | All UI text externalized | Scan for hardcoded strings in UI code |
| Format string safety | No broken concatenation | Parse format strings for placeholders |
| Text expansion room | UI handles 150% text length | Render with expanded test strings |
| Unicode handling | Font supports target character sets | Render test strings from each locale |
| Number/date formatting | Locale-aware display | Test with different locale settings |

**Test Matrix:**

```yaml
localization_tests:
  en_US:  # Primary
    - all_strings_render: true
    - no_truncation: true
    - format_strings_work: true

  pseudo_locale:  # For testing expansion
    # "Colony" -> "[~Cooloonyy~]" (50% longer + markers)
    - expansion_fits_ui: true
    - no_hardcoded_strings: true

  unicode_test:  # For future i18n
    # Test with CJK characters
    - font_renders_correctly: true
    - line_wrapping_works: true
```

---

### GD-6: Accessibility Testing

**Question:** "Do we have access to screen reader testing tools? How do we validate colorblind modes without colorblind testers? What devices/resolutions do we need to test text scaling on?"

**Answer:**

#### Screen Reader Testing

**Tools Available:**

| Tool | Platform | Cost | Use Case |
|------|----------|------|----------|
| NVDA | Windows | Free | Primary screen reader testing |
| Narrator | Windows | Built-in | Secondary verification |
| Orca | Linux | Free | Linux screen reader testing |
| JAWS | Windows | Paid | Gold standard (if budget allows) |

**Practical Approach:**

1. **NVDA Testing (Primary):** Free, widely used, good for initial validation
2. **Focus on Semantic Structure:** Ensure UI elements have proper labels, roles
3. **Keyboard Navigation First:** If keyboard works, screen reader is 80% there
4. **Test Key Flows:**
   - Main menu navigation
   - Tool selection
   - Building placement confirmation
   - Error message announcement
   - Milestone celebration announcements

**What We Can Test Without Full Screen Reader Support:**

- Tab order correctness
- Focus visibility
- ARIA-equivalent labels present (if using a UI framework)
- Keyboard-only playability

#### Colorblind Mode Validation

**Simulation Tools (No Colorblind Testers Needed):**

| Tool | Type | Use Case |
|------|------|----------|
| Color Oracle | Desktop app | Real-time screen simulation |
| Coblis | Web tool | Upload screenshots for simulation |
| Sim Daltonism | macOS | Live simulation with camera |
| Photoshop/GIMP | Image editor | Simulate in design phase |

**Testing Process:**

1. **Capture Screenshots:** All UI states, overlays, icons
2. **Apply Simulation:** Deuteranopia, Protanopia, Tritanopia
3. **Verify Distinguishability:**
   - Can valid/invalid placement be distinguished?
   - Can ownership colors be differentiated?
   - Can overlay data be read?
4. **Pattern Verification:** Ensure patterns/shapes supplement colors

**Automated Colorblind Testing:**

```python
# Conceptual automated test
def test_colorblind_distinguishability():
    screenshot = capture_game_screenshot()

    for mode in ['deuteranopia', 'protanopia', 'tritanopia']:
        simulated = apply_colorblind_filter(screenshot, mode)

        # Check that critical UI elements remain distinguishable
        assert contrast_ratio(
            get_color(simulated, 'valid_placement'),
            get_color(simulated, 'invalid_placement')
        ) >= 3.0  # WCAG AA minimum
```

#### Text Scaling Device Matrix

**Minimum Test Matrix:**

| Resolution | Scale Factor | Device Equivalent |
|------------|--------------|-------------------|
| 1920x1080 | 100% | Standard desktop |
| 1920x1080 | 125% | Common Windows default |
| 1920x1080 | 150% | Large text preference |
| 2560x1440 | 100% | High-res monitor |
| 2560x1440 | 150% | High-res with scaling |
| 1366x768 | 100% | Older laptops |
| 1366x768 | 125% | Older laptops with scaling |
| 3840x2160 | 200% | 4K with default scaling |

**Key Verification Points:**

1. **No Truncation:** All text visible at 150% scale on 1920x1080
2. **No Overlap:** UI elements don't collide at max scale
3. **Readable Minimum:** Text legible at minimum supported resolution
4. **Tooltip Expansion:** Tooltips grow to fit scaled content

---

### GD-7: Balance Validation

**Question:** "Can we automate playthroughs to detect balance issues? What metrics should we track during QA playtests? How do we detect 'feels wrong' without specific bug reports?"

**Answer:**

#### Automated Balance Playthroughs

**Yes, implement AI-driven playthrough simulation:**

```cpp
// Conceptual automated player
class BalanceTestBot {
    void run_simulation(int cycles) {
        while (current_cycle < cycles) {
            // Make decisions like a player would
            if (treasury > threshold_expand) {
                expand_colony();
            }
            if (energy_deficit) {
                build_energy_nexus();
            }
            if (population_stagnant) {
                zone_more_habitation();
            }

            advance_simulation();
            record_metrics();
        }
    }
};
```

**Automated Balance Scenarios:**

| Scenario | Bot Behavior | Expected Outcome |
|----------|--------------|------------------|
| Passive Growth | Build minimum infrastructure, wait | Colony reaches 10K in 200 cycles |
| Aggressive Expansion | Spend all credits immediately | Survive first 50 cycles without bankruptcy |
| Balanced Play | Maintain 20% treasury buffer | Reach 50K in 500 cycles |
| Neglect Services | Never build services | Growth caps at ~20K due to disorder |
| Max Services | Build excess services | Slower growth due to expense, but stable |

#### Metrics to Track During QA Playtests

**Quantitative Metrics (Auto-Captured):**

| Metric | Target Range | Red Flag If |
|--------|--------------|-------------|
| Time to 1K population | 10-20 minutes | >30 min (too slow) or <5 min (too easy) |
| Time to break-even | 20-40 cycles | >60 cycles (frustrating) |
| Treasury volatility | +/- 20% per cycle | >50% swings (feels unstable) |
| Average growth rate | 2-5% per cycle | <0.5% (stagnant) or >10% (trivial) |
| Service coverage efficiency | 70-90% | <50% (placement unclear) |
| Death rate | 1-3% annually | >5% (punishing) |
| Disaster recovery time | 20-50 cycles | >100 cycles (game-ending) |

**Qualitative Metrics (Survey-Based):**

| Question | Scale | Target |
|----------|-------|--------|
| "Rate your stress level (1-10)" | 1-10 | 3-5 (engaged, not anxious) |
| "Was money a constant worry?" | Yes/No | 70% No |
| "Did disasters feel fair?" | Yes/No | 80% Yes |
| "Did you feel rewarded for playing well?" | Yes/No | 90% Yes |
| "Would you play again?" | Yes/No | 90% Yes |

#### Detecting "Feels Wrong"

**Behavioral Telemetry (Opt-In):**

| Signal | What It Indicates | Example |
|--------|-------------------|---------|
| Repeated failed placements | Unclear placement rules | 5+ attempts to place same building |
| Rapid tool switching | Confusion about workflow | >10 tool changes per minute |
| Pause immediately after event | Processing/stress response | Pause within 2s of disaster |
| Session abandonment after event | Frustration point | Quit within 5 min of treasury hitting 0 |
| Help/tooltip hover duration | Information seeking | >3s hover = confusion |
| Undo usage frequency | Mistake recovery | High undo = unclear consequences |

**Heuristic Detection Rules:**

```yaml
friction_detection:
  placement_frustration:
    trigger: 5+ failed placements in 30 seconds
    flag: "Placement rules unclear for {building_type}"

  economic_anxiety:
    trigger: treasury < 0 for > 3 cycles
    flag: "Player in financial distress"

  tutorial_gap:
    trigger: > 2 minutes without any action
    flag: "Player may be stuck"

  rage_pattern:
    trigger: demolish 3+ buildings within 10 seconds
    flag: "Possible frustration demolition"
```

**Playtest Observation Protocol:**

1. **Watch for Sighs/Frowns:** Non-verbal frustration cues
2. **Note Questions Asked:** What wasn't clear from the game?
3. **Track Verbalized Confusion:** "Wait, why can't I...?"
4. **Record Feature Discovery Order:** What did they find vs. miss?
5. **Time to First Milestone:** Is pacing satisfying?

---

### GD-8: UX Friction Identification

**Question:** "Should we track click counts per action? Can we instrument 'rage quit' moments (repeated failed placements)? How do we gather qualitative feedback on 'cozy' feeling?"

**Answer:**

#### Click Count Tracking

**Yes, implement action telemetry:**

```cpp
// Action telemetry system
struct ActionEvent {
    ActionType type;        // zone_place, tool_select, menu_open
    int click_count;        // Clicks to complete action
    float time_to_complete; // Seconds from intent to completion
    bool succeeded;         // Did action complete?
    std::string context;    // Current tool, game state
};

class TelemetrySystem {
    void record_action(ActionEvent event) {
        // Store for later analysis
        action_log_.push_back(event);

        // Flag if action took too many clicks
        if (event.click_count > expected_clicks(event.type) * 1.5) {
            flag_friction_point(event);
        }
    }
};
```

**Target Click Counts:**

| Action | Target Clicks | Flag If |
|--------|---------------|---------|
| Select zone tool | 1 | >2 |
| Place single zone tile | 1 | >2 |
| Place zone area (drag) | 2 (click+release) | >3 |
| Open building info | 1 | >2 |
| Switch overlay | 1 | >2 |
| Access budget | 2 | >4 |
| Save game | 2-3 | >5 |

#### Rage Quit Instrumentation

**Implement friction pattern detection:**

```cpp
class FrictionDetector {
    struct PlacementAttempt {
        Vec2i position;
        BuildingType type;
        Timestamp time;
        bool success;
    };

    std::deque<PlacementAttempt> recent_attempts_;

    void on_placement_attempt(PlacementAttempt attempt) {
        recent_attempts_.push_back(attempt);

        // Keep last 30 seconds of attempts
        prune_old_attempts(30s);

        // Detect frustration patterns
        int failed_count = count_if(recent_attempts_,
            [](auto& a) { return !a.success; });

        if (failed_count >= 5) {
            emit_friction_event("repeated_failed_placement", {
                {"building_type", attempt.type},
                {"position", attempt.position},
                {"failure_count", failed_count}
            });
        }
    }

    void on_session_end(SessionEndReason reason) {
        if (reason == QUIT) {
            // Check if quit happened shortly after friction
            if (time_since_last_friction() < 60s) {
                emit_event("potential_rage_quit", {
                    {"last_friction", last_friction_event_},
                    {"session_duration", session_duration_}
                });
            }
        }
    }
};
```

**Friction Patterns to Detect:**

| Pattern | Definition | Response |
|---------|------------|----------|
| Repeated Failed Placement | 5+ failed placements in 30s | Log and flag |
| Rapid Demolition | 3+ demolitions in 10s | Log and flag |
| Menu Thrashing | 5+ menu opens/closes in 20s | Log and flag |
| Tool Confusion | 10+ tool switches in 1 minute | Log and flag |
| Save Spam | 3+ saves in 5 minutes | Log (anxiety indicator) |
| Quit After Failure | Session end within 2 min of major failure | Log as potential rage quit |

#### Gathering Qualitative "Cozy" Feedback

**Multi-Method Approach:**

**1. In-Game Sentiment Prompts (Non-Intrusive):**

```
After 30 minutes of play:
"How are you feeling about your colony?"
[ :) Relaxed ] [ :| Neutral ] [ :( Stressed ]

After milestone achievement:
"Quick check: Is the pace feeling good?"
[ Too Slow ] [ Just Right ] [ Too Fast ]

On session end:
"Would you describe this session as cozy?"
[ Yes ] [ Somewhat ] [ No ]
```

**2. Post-Session Survey (Optional, Detailed):**

| Question | Type | Purpose |
|----------|------|---------|
| "Rate your stress level during play" | 1-10 scale | Quantify cozy |
| "Did you feel rushed at any point?" | Yes/No/Sometimes | Pacing check |
| "Were disasters frustrating or exciting?" | Scale | Disaster balance |
| "Did multiplayer feel cooperative?" | Scale | Social vibe |
| "Three words to describe the experience" | Free text | Qualitative themes |

**3. Behavioral Proxy Metrics:**

| Metric | Indicates Cozy If |
|--------|-------------------|
| Session length | Longer than intended ("just one more turn") |
| Return rate | Players come back within 24 hours |
| Pause frequency | Low (not needing stress breaks) |
| Zoom-out frequency | High (enjoying the view) |
| Music volume | Kept on (ambient appreciated) |
| Simulation speed | Not always fast-forwarded |

**4. Playtest Observation Checklist:**

```
Observer notes during playtest:
- [ ] Player smiled during session
- [ ] Player verbalized positive emotion
- [ ] Player was surprised by time passed
- [ ] Player mentioned wanting to continue
- [ ] Player asked about future features (engaged)
- [ ] Player exhibited relaxed posture
- [ ] Player explored without goal pressure
```

**Aggregating Cozy Score:**

```python
def calculate_cozy_score(session_data):
    score = 100  # Start at max cozy

    # Subtract for friction
    score -= session_data.friction_events * 5
    score -= session_data.rage_patterns * 15

    # Subtract for stress indicators
    if session_data.quit_after_disaster:
        score -= 20
    if session_data.high_pause_frequency:
        score -= 10

    # Add for positive indicators
    if session_data.session_longer_than_expected:
        score += 10
    if session_data.returned_within_24h:
        score += 15
    if session_data.milestone_achieved:
        score += 5

    return max(0, min(100, score))
```

---

## Summary

These answers provide actionable, implementable strategies for Epic 17 QA concerns:

**For Systems Architect:**
- Tiered coverage targets focusing on integration boundaries
- Dual baseline system (fixed targets + regression detection)
- Tiered stress test durations based on trigger context
- Specific regression thresholds by metric category
- Focused hardware matrix prioritizing min-spec
- Terminology lint as CI gate with strict/warn paths
- 4-player stress testing with churn scenarios
- Version-based save compatibility with migration chain

**For Game Designer:**
- Automated terminology linting with CI integration
- Colorblind simulation tools instead of requiring testers
- Comprehensive text scaling test matrix
- AI-driven balance validation with behavioral metrics
- Friction detection through action telemetry
- Multi-method cozy feedback gathering

All recommendations are designed to be practical, automatable where possible, and aligned with the project's cozy sandbox philosophy.

---

*End of QA Engineer Answers for Epic 17 Cross-Agent Questions*
