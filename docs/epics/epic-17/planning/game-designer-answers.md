# Game Designer Answers: Epic 17 - Polish & Alien Theming

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Canon Version:** 0.17.0
**Status:** Answers Complete

---

## Questions from Systems Architect

### SA-1: LOD Visual Impact

**Question:** At what zoom distance should buildings transition to simplified models? Should the transition be noticeable, or prioritize smoothness?

**Answer:**

Prioritize **smoothness over sharp transitions**. The "cozy sandbox" philosophy means players should never be jarred by visual pop-in.

**Recommended LOD Distance Thresholds:**

| LOD Level | Distance (tiles) | Content | Transition Style |
|-----------|-----------------|---------|------------------|
| LOD 0 (Full) | 0-50 tiles | Full detail with bioluminescent glow | N/A |
| LOD 1 (Medium) | 50-150 tiles | Simplified geometry, major details | 10-tile dithered blend |
| LOD 2 (Low) | 150-300 tiles | Billboard/impostor with silhouette | 15-tile dithered blend |
| Culled | 300+ tiles | Not rendered | N/A |

**Key Design Decisions:**

1. **Dithered transitions**: Use screen-door dithering during LOD transitions to avoid visible "pop". This technique blends LOD levels over distance rather than hard-swapping.

2. **Preserve bioluminescent identity**: Even at LOD 2, buildings should retain their characteristic glow color. The glow is the visual signature of ZergCity - never lose it entirely.

3. **User control**: Provide a "Visual Quality" setting that adjusts these distances:
   - Low: Aggressive LOD (halve all distances)
   - Medium: Default distances
   - High: Extended full-detail range (multiply by 1.5x)

4. **Camera zoom awareness**: At maximum zoom-out, most buildings will be LOD 1 or 2 - this is expected and acceptable. Players zoomed out are viewing the colony macro-structure, not individual building details.

---

### SA-2: Performance vs Visual Quality Trade-off

**Question:** If we must choose between 60 FPS and maximum visual detail on large maps, which is the priority?

**Answer:**

**60 FPS is the priority.** This is non-negotiable for the cozy sandbox experience.

**Rationale:**

1. **Responsiveness = Player Control**: A city builder lives or dies by how responsive building placement feels. Input lag at 30 FPS makes placement feel sluggish and frustrating.

2. **Long Sessions**: Players spend hours in ZergCity. 60 FPS reduces eye strain compared to 30 FPS over extended play.

3. **Multiplayer Fairness**: All players should have similar responsiveness regardless of hardware (within reason).

**Implementation Guidance:**

1. **Automatic Quality Scaling**: Implement dynamic resolution/LOD that targets 60 FPS. If frame time exceeds 14ms, aggressively reduce quality. Players won't notice reduced vegetation density at 512x512 scale.

2. **Quality Presets**:
   - "Performance": Target 60 FPS on minimum spec, visual compromises accepted
   - "Balanced": Target 60 FPS on recommended spec
   - "Quality": Target 60 FPS on high-end, maximum detail
   - "Uncapped": For players who want maximum visuals regardless of FPS

3. **What to Cut First (in order):**
   1. Vegetation density (80% -> 40% -> 20%)
   2. Shadow quality (soft -> hard -> none)
   3. LOD distances (halve them)
   4. Overlay resolution (quarter-resolution overlays)
   5. Particle effects (celebrations, construction sparkles)

4. **What to Never Cut:**
   - Bioluminescent glow on buildings (the visual identity)
   - UI responsiveness
   - Building readability (players must identify building types)

---

### SA-3: Terminology Strictness

**Question:** Should debug/developer-facing strings also use alien terminology, or only player-facing content?

**Answer:**

**Player-facing content only.** Debug/developer strings should use clear, unambiguous technical language.

**Rationale:**

1. **Debugging Efficiency**: When hunting bugs, developers need to understand log output instantly. "EnergySystem: brownout_cascade triggered" is clearer than "EnergySystem: grid_collapse_cascade triggered" for debugging purposes.

2. **External Communication**: Bug reports, stack traces, and technical documentation shared with external developers (mods, community) should use industry-standard terminology.

3. **Consistency with Code**: Variable names, function names, and internal APIs use technical terms. Debug output should match code structure.

**Boundary Definition:**

| Context | Terminology | Examples |
|---------|-------------|----------|
| UI text (panels, buttons, labels) | Alien | "Colony Treasury", "Habitation Zone" |
| Tooltips and help text | Alien | "This structure generates energy" |
| Notifications (player-facing) | Alien | "Energy deficit detected" |
| Error dialogs | Alien | "Cannot construct: insufficient credits" |
| Console/log output | Technical | "EnergySystem: coverage update failed" |
| Debug overlays | Technical | "FPS: 60, Tick: 23ms, Entities: 12,847" |
| Crash reports | Technical | "NullPointerException in BuildingSystem" |

**Exception**: If debug overlays are ever exposed to players (F3-style debug screen), use hybrid terminology: "Population: 12,500 beings (EntityCount: 12500)".

---

### SA-4: Loading Screen Content

**Question:** What should players see during the initial asset load? Tips? Alien lore? Colony statistics?

**Answer:**

**A rotating mix of tips and lore snippets**, with progress indication. No colony statistics during initial load (the colony doesn't exist yet).

**Loading Screen Design:**

1. **Visual Design:**
   - Dark background with subtle bioluminescent particle animation
   - ZergCity logo (glowing, animated)
   - Progress bar with percentage (styled as "energy conduit filling")
   - Rotating content panel

2. **Content Rotation (5-second intervals):**

   **Gameplay Tips (60% of rotation):**
   - "Pathways connect structures to resources. No pathway, no growth."
   - "Energy conduits transmit power. Energy nexuses generate it."
   - "Harmony rises when beings have energy, fluid, and services nearby."
   - "The sector scan (minimap) shows your colony at a glance."
   - "Press [H/E/F] to quickly switch zone designation modes."
   - "Catastrophes can be recovered from. Stay calm, rebuild."
   - "Higher tribute rates fill your treasury faster, but reduce harmony."

   **Lore Snippets (40% of rotation):**
   - "The First Overseers arrived from the void, seeking a new home..."
   - "Beings are drawn to harmony. A well-managed colony thrives."
   - "Prisma fields contain valuable crystalline structures."
   - "The biolume groves glow with ancient energy."
   - "Credits are the lifeblood of expansion. Spend wisely."
   - "Even the mightiest colony began with a single pathway."

3. **For Reconnection/Load (not fresh start):**
   - Show colony name
   - Show population count
   - Show last played date
   - "Restoring colony state..."

4. **Never Show:**
   - Developer names or credits (save for dedicated credits screen)
   - Marketing messages
   - External links
   - Countdown timers ("Loading complete in 5...4...3...")

---

### SA-5: Performance Indicators

**Question:** Should players see FPS/ping in the UI? If so, how prominent?

**Answer:**

**Optional, and subtle by default.** Performance anxiety is the enemy of cozy.

**Design:**

1. **Default Behavior:**
   - FPS/ping hidden entirely
   - No performance indicators visible during normal play

2. **When Enabled (Settings toggle):**
   - Small, semi-transparent display in a corner (user-selectable corner)
   - Use colony-themed labels:
     - "Render Rate: 60" (not "FPS: 60")
     - "Link Latency: 45ms" (not "Ping: 45ms")
   - Muted colors - don't draw attention

3. **Automatic Warning Display:**
   - If FPS drops below 30 for 5+ seconds: brief "Performance degraded - consider lowering visual quality" notification
   - If ping exceeds 300ms for 10+ seconds: "Connection unstable" notification
   - These warnings auto-dismiss and don't repeat within 5 minutes

4. **Debug Mode (F3 or similar):**
   - Full technical overlay for power users
   - Entity count, tick time, memory usage, network bandwidth
   - Uses technical terminology (as per SA-3)
   - Only accessible via keyboard shortcut, not in settings menu

**Rationale:**

The cozy sandbox philosophy means players should focus on their colony, not performance metrics. Showing FPS constantly turns relaxation into optimization anxiety. However, some players genuinely need this information, so it should be available as an opt-in.

---

### SA-6: Terminology Exceptions

**Question:** Are there any terms that should remain human (e.g., "Tutorial", "Settings", "Quit")?

**Answer:**

**Yes.** Meta-UI terms that reference the game-as-software (not the game-as-world) should remain human-readable.

**Terms That Stay Human:**

| Term | Reason |
|------|--------|
| Settings | Universal software term, users expect this |
| Options | Alternative to "Settings", also universal |
| Quit / Exit | Universal, safety-critical (users must understand they're closing the game) |
| Save / Load | Universal game concepts |
| New Game | Universal |
| Continue | Universal |
| Main Menu | Universal navigation |
| Pause / Resume | Universal |
| Tutorial | Universal, important for onboarding |
| Help | Universal |
| Credits | Universal |
| Version | Technical information |
| OK / Cancel / Yes / No | Dialog buttons, universal |

**Terms That Get Alien Treatment:**

| Human Term | Alien Term | Context |
|------------|------------|---------|
| Difficulty | Challenge Level | If difficulty settings exist |
| Map Size | Colony Expanse | World creation |
| Player Name | Overseer Designation | Multiplayer identity |
| Server | Colony Hub | Multiplayer connection |
| Host Game | Establish Colony Hub | Multiplayer |
| Join Game | Link to Colony Hub | Multiplayer |
| Delete Save | Decommission Colony | Save management |
| Autosave | Auto-Archive | Settings |
| Checkpoint | Colony Checkpoint | Save system |

**Edge Cases:**

- "Volume" (audio) - stays human (universal)
- "Brightness" - stays human (universal)
- "Resolution" - stays human (technical)
- "Fullscreen" - stays human (universal)
- "Language" - stays human (accessibility-critical)

**The Rule:** If a term is about interacting with the game software, use human terms. If a term is about interacting with the game world, use alien terms.

---

## Questions from QA Engineer

### QA-9: Acceptable Performance Trade-offs

**Question:** If we can't hit 60fps on large maps, what's the minimum acceptable? Is 30fps with visual scaling acceptable?

**Answer:**

**30 FPS is the absolute floor**, but only with automatic visual scaling, not as a default target.

**Performance Tiers:**

| Map Size | Minimum Spec | Recommended Spec | High-End |
|----------|--------------|------------------|----------|
| 128x128 | 60 FPS | 60 FPS | 60 FPS |
| 256x256 | 45 FPS | 60 FPS | 60 FPS |
| 512x512 | 30 FPS | 60 FPS | 60 FPS |

**Design Decisions:**

1. **Warn Players at World Creation:**
   - When selecting 512x512 on minimum spec: "Large colony expanse recommended for higher-end systems. Performance may be reduced."
   - Don't prevent them, just inform.

2. **Automatic Quality Scaling:**
   - If FPS drops below 45, automatically reduce visual quality
   - If FPS drops below 30, show a one-time notification: "Visual quality reduced to maintain playability"
   - If FPS still below 30 at minimum quality, suggest smaller map size

3. **Never Below 30 FPS:**
   - Below 30 FPS, input latency becomes unacceptable
   - If we can't maintain 30 FPS at minimum quality, the map size shouldn't be available on that hardware

4. **Simulation Tick Independence:**
   - Simulation must maintain 20 ticks/sec regardless of render FPS
   - If tick time exceeds budget, simulation becomes the bottleneck (more serious than render)
   - If tick time consistently >50ms, show warning: "Simulation struggling - colony may be too complex"

---

### QA-10: Terminology Exceptions (QA Perspective)

**Question:** Are there any human terms that should intentionally remain (e.g., "arcology" is already sci-fi)? Any new terminology needed for Epic 17 UI changes?

**Answer:**

**Yes to both.**

**Terms That Remain as "Arcology":**

"Arcology" is already a science fiction term (coined by Paolo Soleri). It should remain as-is because:
1. It's not a "human city" term - arcologies don't exist
2. Changing it would reduce recognition for sci-fi fans
3. It fits the alien theme already

**Other Sci-Fi Terms That Stay:**

| Term | Reason |
|------|--------|
| Arcology | Already sci-fi |
| Nexus | Common sci-fi term (Energy Nexus is fine) |
| Conduit | Already technical/sci-fi |
| Terminal | Already technical/sci-fi |

**New Terminology for Epic 17 UI:**

| Feature | Proposed Term | Usage |
|---------|---------------|-------|
| Quality Settings | Visual Fidelity | Settings menu |
| Performance Mode | Efficiency Mode | Quality preset |
| Accessibility | Overseer Assistance | Settings category |
| Colorblind Mode | Chromatic Adaptation | Accessibility option |
| Text Size | Glyph Magnitude | Accessibility option |
| Reduced Motion | Stabilized Display | Accessibility option |
| Key Bindings | Command Mapping | Settings category |
| Loading... | Materializing... | Loading screens |
| "Tip:" | "Intel:" | Loading screen tips |
| Warning dialog | Caution Alert | Error handling |
| Error dialog | Anomaly Alert | Error handling |

**Testing Implication:**

QA should maintain a terminology checklist that includes both:
1. Terms that MUST change (city -> colony)
2. Terms that MUST NOT change (arcology, settings, quit)

---

### QA-11: Balance Testing Criteria

**Question:** What metrics define "balanced" gameplay? Population growth rate? Time to first milestone? Treasury stability?

**Answer:**

**Multiple metrics across the player journey.**

**Early Game (0-30 minutes):**

| Metric | Target Range | Why |
|--------|--------------|-----|
| Time to first building | 1-2 minutes | Players should see results quickly |
| Time to 500 population | 10-15 minutes | First sense of growth |
| Time to first milestone (2,000) | 25-35 minutes | Reward for sustained play |
| Treasury at 30 minutes | Positive (500-5,000 credits) | Players shouldn't be broke early |

**Mid Game (30 minutes - 2 hours):**

| Metric | Target Range | Why |
|--------|--------------|-----|
| Time to 10,000 population | 45-75 minutes | Steady progression |
| Time to 30,000 population | 90-120 minutes | Achievement feels earned |
| Treasury stability | +5% to +15% per cycle | Not stressful, not trivial |
| Service coverage gaps | <10% of buildings uncovered | Services feel manageable |

**Late Game (2+ hours):**

| Metric | Target Range | Why |
|--------|--------------|-----|
| Time to 60,000 population | 2-3 hours | Meaningful investment |
| Arcology affordability | 3-4 hours for first | Aspirational but achievable |
| Treasury at late game | +10% to +25% surplus | Wealth accumulation feels good |

**Balance Health Indicators:**

| Symptom | Problem | Fix Direction |
|---------|---------|---------------|
| Treasury always <0 | Economy too punishing | Reduce costs or increase tribute rate default |
| Treasury always >50% surplus | Economy too easy | Increase costs or reduce income rates |
| Population stalls at X | Bottleneck at X | Check resource requirements at that tier |
| Players never use service Y | Service Y too expensive or unclear | Reduce cost or improve tutorialization |
| Players always spam strategy Z | Strategy Z overpowered | Nerf Z or buff alternatives |

**"Fun Factor" Proxies:**

- Session length (longer = more engaged)
- Return rate (players come back)
- Colony complexity (players experiment)
- Milestone achievement rate (players progress)

---

### QA-12: Difficulty Perception

**Question:** Should there be difficulty settings, or is single-balance acceptable? How do we test "fun factor"?

**Answer:**

**Single-balance by default, with sandbox modifiers available.** No explicit "difficulty settings."

**Rationale:**

ZergCity is a cozy sandbox, not a competitive game. Traditional difficulty settings (Easy/Normal/Hard) imply there's a "correct" way to play. Instead, we offer sandbox modifiers that let players customize their experience without judgment.

**Sandbox Modifiers (all optional):**

| Modifier | Default | Range | Effect |
|----------|---------|-------|--------|
| Starting Credits | 20,000 | 5,000-100,000 | Initial treasury |
| Tribute Rate | 7% | 3%-15% | Base taxation |
| Growth Speed | 1.0x | 0.5x-2.0x | Population growth multiplier |
| Catastrophe Frequency | 1.0x | 0x-3.0x | Disaster occurrence rate |
| Construction Cost | 1.0x | 0.5x-2.0x | Building costs |
| Maintenance Cost | 1.0x | 0.5x-2.0x | Ongoing expenses |

**Important:** These are presented as "Sandbox Options," not "Difficulty Settings." There's no "Easy Mode" - there's "Generous Starting Funds" or "Peaceful (No Catastrophes)."

**Testing "Fun Factor":**

1. **Observational Playtesting:**
   - Watch testers play without guiding them
   - Note moments of frustration (sighs, repeated failures)
   - Note moments of delight (smiles, "ooh!", taking screenshots)
   - Track when testers ask "how do I...?" (confusion points)

2. **Survey Questions (post-session):**
   - "Did you feel in control of your colony's development?"
   - "Were there any moments that felt unfair?"
   - "Did you want to keep playing when the session ended?"
   - "Rate your stress level during play (1-10)"
   - "Rate your satisfaction with your colony (1-10)"

3. **Behavioral Metrics:**
   - Average session length (target: 45+ minutes)
   - Session quit point (where do players stop?)
   - Feature discovery rate (which features do players find?)
   - Retry rate after failure (do players give up or try again?)

---

### QA-13: Tutorial/Onboarding

**Question:** Does Epic 17 include any tutorial or help system? If so, what needs QA validation?

**Answer:**

**Yes, Epic 17 should include contextual help, but NOT a mandatory tutorial.**

**Onboarding Philosophy:**

ZergCity's cozy vibe means we don't force players through a scripted tutorial. Instead:

1. **First-Time Welcome:**
   - Brief modal on first launch: "Welcome, Overseer. Would you like guidance as you build?" [Yes/No]
   - If Yes: Enable hint system
   - If No: Player is on their own (hints available in settings)

2. **Contextual Hints (if enabled):**
   - Non-blocking tooltips that appear during relevant actions
   - Example: First time selecting zone tool: "Hint: Habitation zones need pathway access and energy coverage to develop."
   - Hints dismiss after 5 seconds or on click
   - Each hint shows only once (tracked in player settings)

3. **Help Panel (F1 or ? button):**
   - Searchable help topics
   - Topics organized by system (Zones, Energy, Services, etc.)
   - Each topic: 2-3 sentence summary + "Show me" button that highlights relevant UI

**QA Validation Needed:**

| Area | What to Test |
|------|--------------|
| First-time experience | Does welcome modal appear? Is choice remembered? |
| Hint triggers | Does each hint appear at the right moment? |
| Hint dismissal | Do hints stop appearing after being shown once? |
| Help panel coverage | Does every major feature have a help topic? |
| Help panel search | Can players find topics by searching? |
| "Show me" functionality | Does each "Show me" correctly highlight UI elements? |
| Settings persistence | Are hint preferences saved and restored? |
| Terminology | Does all help text use alien terminology? |

---

### QA-14: Accessibility Requirements

**Question:** Any accessibility features (colorblind modes, screen reader support, key rebinding) that need testing?

**Answer:**

**Yes. ZergCity should be accessible to a wide audience.**

**P0 - Must Have for Release:**

| Feature | Description | Test Focus |
|---------|-------------|------------|
| Colorblind modes | Deuteranopia, Protanopia, Tritanopia | All UI elements distinguishable in each mode |
| Key rebinding | All actions rebindable | No conflicts, all bindings saveable/loadable |
| Text scaling | 80%-150% UI scale | No clipping, all text readable |
| Pause anytime | Game pausable at any moment | No exceptions, no time-critical sequences |
| Subtitles/captions | For any narration (if added) | Readable, properly timed |

**P1 - Should Have:**

| Feature | Description | Test Focus |
|---------|-------------|------------|
| Reduced motion mode | Disable/reduce animations | No essential info conveyed only via animation |
| High contrast mode | Boosted visual contrast | All elements visible |
| Audio cues for critical events | Sound alerts for disasters, milestones | Events never silent in critical moments |
| Mouse-free navigation | Full keyboard control | All features accessible without mouse |

**P2 - Nice to Have (Post-Launch):**

| Feature | Description |
|---------|-------------|
| Screen reader support | Full ARIA-like support for UI |
| Custom color schemes | User-defined UI colors |
| Dyslexia-friendly font | Alternative font option |

**QA Validation:**

1. **Colorblind Mode Testing:**
   - Use color blindness simulation tools (e.g., Color Oracle)
   - Verify all overlays distinguishable
   - Verify zone types distinguishable
   - Verify status indicators distinguishable

2. **Key Rebinding Testing:**
   - Rebind all keys to non-default
   - Test gameplay with rebindings
   - Verify no hardcoded keys remain
   - Test loading rebindings from saved settings

3. **Text Scaling Testing:**
   - Test at 80%, 100%, 125%, 150%
   - All resolutions
   - No text overflow or clipping
   - All tooltips readable

---

### QA-15: Acceptable Bug Count

**Question:** What level of "rough edges" is acceptable for initial release vs post-launch patches? Any areas where we'd accept known issues?

**Answer:**

**Release should be polished, but not perfect.** Known issues are acceptable if documented and non-critical.

**Release Criteria:**

| Severity | Definition | Acceptable Count |
|----------|------------|------------------|
| Critical | Crash, data loss, security | 0 |
| High | Major feature broken | 0 |
| Medium | Feature degraded, workaround exists | <10 (with documentation) |
| Low | Minor visual/audio issues | <25 |
| Trivial | Cosmetic, rarely noticed | No limit |

**Areas Where Known Issues Are Acceptable:**

| Area | Acceptable Issues | Not Acceptable |
|------|-------------------|----------------|
| 512x512 maps | Performance warnings, quality scaling | Crashes, unplayable FPS |
| Rare edge cases | "If you do X while Y during Z, tooltip misaligns" | Crashes, data corruption |
| Visual polish | Minor z-fighting, occasional particle glitch | Unreadable UI, missing textures |
| Audio | Occasional audio priority issues in chaos | No audio, wrong audio playing |
| Multiplayer (4 players, 512x512) | Occasional lag spikes, brief desync | Persistent desync, crashes |

**Documentation Requirements:**

All known Medium or higher bugs at release must have:
1. Description in release notes
2. Workaround if applicable
3. Expected fix timeline (patch 1, patch 2, future)

**Post-Launch Patch Philosophy:**

- Patch 1 (Week 1-2): Critical/High bugs discovered in the wild
- Patch 2 (Week 3-4): Medium bugs, performance improvements
- Patch 3+ (Monthly): Lower priority fixes, QoL improvements

---

### QA-16: Disaster Balance

**Question:** What's the acceptable disaster damage range? Should disasters ever be colony-ending, or always recoverable?

**Answer:**

**Always recoverable.** Disasters are setbacks, not game-overs.

**Disaster Design Philosophy:**

1. **No Colony-Ending Events:**
   - Core Principle #4: "No win/lose states. Endless play."
   - Disasters can damage, but never destroy everything
   - Even the worst disaster leaves enough for recovery

2. **Damage Caps:**

| Disaster | Max Damage | Recovery Time | Notes |
|----------|------------|---------------|-------|
| Conflagration (small) | 5% of nearby structures | 10-20 cycles | Local event |
| Conflagration (large) | 15% of colony structures | 50-100 cycles | Devastating but survivable |
| Seismic Event (minor) | 10% of structures | 20-30 cycles | Structural damage |
| Seismic Event (major) | 25% of structures | 100+ cycles | Requires significant rebuild |
| Vortex Storm | 15% of structures in path | 30-50 cycles | Localized path of destruction |
| Titan Emergence | 20% of structures in zone | 50-80 cycles | Dramatic but contained |

3. **Protective Factors:**

| Factor | Protection Effect |
|--------|-------------------|
| Small colony (<5,000) | No disasters (tutorial protection) |
| Hazard Response coverage | -30% to -50% damage in covered areas |
| Multiple Hazard Response Posts | Faster recovery time |
| High treasury | Faster rebuilding (can afford it) |

4. **Recovery Support:**

After any disaster:
- Affected area highlighted on sector scan (minimap)
- "Recovery Zone" designation provides passive bonuses
- Emergency services auto-prioritize damaged areas
- Notification: "The colony endures. Rebuilding can begin."

5. **Player Control:**

- Sandbox modifier to disable disasters entirely (Catastrophe Frequency: 0x)
- Manual disaster trigger for players who want chaos
- No mandatory disasters in tutorial or early game

**QA Validation:**

- Trigger every disaster type at various colony sizes
- Verify damage never exceeds caps
- Verify recovery is possible from max damage scenario
- Verify tutorial protection works (<5,000 population)
- Verify Hazard Response coverage reduces damage correctly

---

## Summary

These answers reflect ZergCity's core design philosophy:

1. **Cozy Sandbox First**: Performance, accessibility, and player comfort over visual flashiness
2. **Alien Theme Consistency**: Use alien terminology for game-world elements, human terms for software UI
3. **Recoverable Challenges**: Disasters and setbacks, never game-overs
4. **Player Control**: Options for customization, not forced tutorials or difficulty gates
5. **Polish Over Features**: Epic 17 is about refinement, not new mechanics

The goal is a game where players can relax, build, and enjoy their alien colony without anxiety about performance, balance, or irreversible mistakes.

---

*End of Game Designer Answers: Epic 17 - Polish & Alien Theming*
