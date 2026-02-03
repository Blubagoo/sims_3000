# Game Designer Analysis: Epic 17 - Polish & Alien Theming

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Canon Version:** 0.17.0
**Status:** Initial Analysis

---

## Executive Summary

Epic 17 is unique among all epics - it adds no new systems. Instead, it transforms ZergCity from a functional simulation into a **polished, cohesive alien world**. This is the epic that makes players feel like they're building an alien colony, not just a city with renamed labels.

Core Principle #5 demands: *"This is an alien colony builder, not a human city builder. All terminology, UI text, and naming must reflect the alien theme."*

This epic is the final guardian of that principle. Every player-facing string, every tooltip, every notification must pass through the alien terminology filter. Beyond terminology, this epic addresses balance tuning, UX friction, accessibility, and preserving the "cozy sandbox" vibe that makes ZergCity approachable for social/casual players.

**The Goal:** When Epic 17 is complete, a new player should feel like they're learning an alien civilization's systems, not translating human concepts.

---

## 1. Alien Terminology Audit

### 1.1 Audit Scope

Every player-facing string across 16 epics must be reviewed. This includes:

| Category | Source Systems | Estimated Strings |
|----------|----------------|-------------------|
| UI Labels | UISystem (Epic 12) | ~200 |
| Tooltips | All systems | ~500 |
| Notifications | EventBus consumers | ~100 |
| Error Messages | All systems | ~150 |
| Tutorial/Help | UISystem | ~300 |
| Settings Menus | SettingsManager (Epic 16) | ~80 |
| Sound Labels | AudioSystem (Epic 15) | ~50 |
| Save/Load UI | PersistenceSystem (Epic 16) | ~60 |
| **Total** | | **~1,440 strings** |

### 1.2 Terminology Categories

Based on `terminology.yaml`, all text must conform to these categories:

#### World & Population

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| city | colony | All references to player's settlement |
| citizen/people | being/beings | Population references |
| mayor | overseer | Player role, milestone names |
| town hall | command nexus | Building names, unlocks |
| neighborhood | district | Zone groupings |
| resident | inhabitant | Population breakdowns |
| worker | laborer | Employment stats |

#### Zones

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| residential | habitation | Zone designation, demand |
| commercial | exchange | Zone designation, demand |
| industrial | fabrication | Zone designation, demand |
| zoned/unzoned | designated/undesignated | Tile states |

#### Infrastructure

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| power/electricity | energy | Resource pools, coverage |
| water | fluid | Resource pools, coverage |
| road/street | pathway | Transportation |
| power plant | energy nexus | Building names |
| power line | energy conduit | Infrastructure |
| pipes | fluid conduits | Infrastructure |
| blackout | grid collapse | Crisis alerts |
| traffic/congestion | flow/flow blockage | Transport stats |
| train station | rail terminal | Transit buildings |
| subway | subterra rail | Transit systems |

#### Services

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| police | enforcers | Service type |
| fire station | hazard response post | Building names |
| hospital | medical nexus | Building names |
| school | learning center | Building names |
| health | vitality | Stats |
| education | knowledge quotient | Stats |
| park | green zone | Building names |

#### Economy

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| money/dollars | credits | Currency |
| taxes | tribute | Rate settings |
| budget | colony treasury | Financial windows |
| income | revenue | Financial stats |
| expenses | expenditure | Financial stats |
| debt/loan | deficit/credit advance | Financial mechanics |

#### Simulation

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| happiness | harmony | Stats, feedback |
| crime | disorder | Stats, overlays |
| pollution | contamination | Stats, overlays |
| land value | sector value | Stats, overlays |
| year/month/day | cycle/phase/rotation | Time references |
| demand | growth pressure | Zone demand UI |
| RCI demand | zone pressure | Demand bars |

#### Disasters

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| disaster | catastrophe | Events, notifications |
| earthquake | seismic event | Specific disaster |
| tornado | vortex storm | Specific disaster |
| fire | conflagration | Specific disaster |
| riot | civil unrest | Specific disaster |
| nuclear meltdown | core breach | Specific disaster |
| monster attack | titan emergence | Specific disaster |

#### Buildings

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| building | structure | Generic reference |
| house | dwelling | Low-density habitation |
| apartment | hab unit | High-density habitation |
| factory | fabricator | Fabrication buildings |
| under construction | materializing | Build states |
| abandoned | derelict | Building states |
| demolished | deconstructed | Building states |
| rubble | debris | Post-demolition |

#### UI Elements

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| minimap | sector scan | Navigation element |
| toolbar | command array | Tool selection |
| info panel | data readout | Selected entity info |
| overlay | scan layer | Data visualization |
| query tool | probe function | Inspection tool |
| tooltip | hover data | Hover information |
| notification | alert pulse | Toast notifications |
| dialog | comm panel | Modal windows |
| budget window | colony treasury panel | Financial UI |
| slider | adjustment control | UI controls |
| status bar | colony status | Persistent HUD |
| demand meter | zone pressure indicator | RCI display |

#### Terrain

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| flat ground | substrate | Basic terrain |
| hills | ridges | Elevated terrain |
| ocean | deep void | Map edge water |
| river | flow channel | Water feature |
| lake | still basin | Water feature |
| forest | biolume grove | Vegetation |
| crystal fields | prisma fields | Alien terrain |
| spore plains | spore flats | Alien terrain |
| toxic marsh | blight mire | Alien terrain |
| volcanic rock | ember crust | Alien terrain |
| clear terrain | purge terrain | Terrain action |
| terraform | reclaim terrain | Late-game action |

#### Multiplayer

| Human Term | Alien Term | Usage Context |
|------------|------------|---------------|
| host | prime overseer | Session roles |
| player | overseer | Player references |
| lobby | staging area | Connection UI |
| territory | domain | Ownership |
| unclaimed | uncharted | Tile states |
| claimed | chartered | Tile states |
| abandoned | forsaken | Decay states |
| ghost town | remnant | Decay states |

### 1.3 High-Risk Areas for Terminology Errors

Based on analysis of previous epics, these areas are most likely to contain non-alien terminology:

| Area | Risk Level | Reason |
|------|------------|--------|
| Error messages | HIGH | Often written quickly during debugging |
| Debug logging (if exposed) | HIGH | Developer-focused, may leak to players |
| Tooltip help text | MEDIUM | Long form text is easy to miss in review |
| Dynamic string generation | MEDIUM | Concatenated strings may mix terms |
| Legacy code paths | MEDIUM | Early epics may predate full terminology |
| External library messages | HIGH | Third-party errors can't use our terms |

### 1.4 Terminology Audit Process

**Phase 1: Automated Scan**
1. Extract all string literals from source code
2. Cross-reference against human term list
3. Flag any occurrence of banned terms in player-facing contexts
4. Generate report with file, line, and suggested replacement

**Phase 2: Manual Review**
1. Review all UI text in context (screenshots/mockups)
2. Verify dynamic strings (string formatting, concatenation)
3. Check icon labels and button text
4. Review notification message queue
5. Verify settings menu labels
6. Check save/load UI strings

**Phase 3: Playtest Verification**
1. Fresh players (no prior ZergCity exposure) play the game
2. Note any moment they use human terms naturally ("the power plant")
3. If players default to human terms, the alien terms aren't resonating
4. Iterate on terminology that causes confusion

### 1.5 Terminology Consistency Guidelines

**Rule 1: Never Mix Terms**
```
BAD:  "Your city needs more power plants"
GOOD: "Your colony needs more energy nexuses"
```

**Rule 2: Be Consistent Across Contexts**
```
BAD:  UI says "Habitation Zone", tooltip says "Residential Area"
GOOD: Both say "Habitation Zone" or "Habitation"
```

**Rule 3: Use Full Terms in Formal Contexts, Short Terms in Casual**
```
Formal (tooltips, help): "Energy Nexus"
Casual (quick alerts): "Energy" or "Power" in isolation is OK if context is clear
```

**Rule 4: Error Messages Must Stay Alien**
```
BAD:  "Cannot build: insufficient funds"
GOOD: "Cannot construct: insufficient credits"
```

**Rule 5: External Terms Get Alien Wrappers**
```
BAD:  "Connection error: timeout"
GOOD: "Link disrupted: signal timeout"
```

---

## 2. Balance Tuning

### 2.1 Balance Philosophy

ZergCity is a **cozy sandbox**, not a punishing optimization puzzle. Balance should:

1. **Reward Engagement** - Active players prosper
2. **Forgive Mistakes** - Suboptimal choices aren't catastrophic
3. **Encourage Experimentation** - Multiple viable strategies
4. **Support Social Play** - Cooperation is viable, competition is fun but optional

### 2.2 Economic Balance

#### Income vs. Expenses

| Metric | Target Range | Current (Estimated) | Notes |
|--------|--------------|---------------------|-------|
| Starting Credits | 20,000 | 20,000 | Enough for ~40 tiles and basic infrastructure |
| Default Tribute Rate | 7% | 7% | Neutral - neither punishing nor overpowered |
| Break-even Population | ~500 beings | Unknown | When tribute covers basic services |
| Comfortable Margin | +15-25% surplus | Unknown | Allows savings for expansion |
| Crisis Threshold | <-10% deficit | Unknown | When difficult choices required |

**Tuning Questions:**
- At what population does a colony become self-sustaining?
- How long can a colony survive at max deficit before bankruptcy?
- Are credit advances too punishing (interest rate vs. need)?

#### Building Costs

| Building Type | Target Cost Range | Tuning Goal |
|---------------|-------------------|-------------|
| Basic Pathway (per tile) | 10-20 credits | Cheap enough to build freely |
| Energy Nexus (coal equiv) | 3,000-5,000 credits | Significant investment |
| Energy Nexus (clean) | 8,000-15,000 credits | Premium for sustainability |
| Fluid Extractor | 1,000-2,000 credits | Cheaper than energy |
| Service Buildings | 2,000-8,000 credits | Scale with coverage radius |
| Arcology | 100,000-200,000 credits | Aspirational late-game |

**Tuning Questions:**
- Is there a clear cost progression that guides players?
- Are clean energy options viable, or do players always go cheap?

#### Maintenance Costs

| System | Target % of Income | Notes |
|--------|-------------------|-------|
| Infrastructure | 5-10% | Pathways, conduits, pipes |
| Energy Generation | 15-20% | Major expense |
| Services | 20-30% | Scales with coverage |
| Other | 5-10% | Miscellaneous |
| **Reserve** | **40-50%** | Available for expansion/savings |

### 2.3 Service Coverage Balance

#### Coverage Requirements

| Service | Coverage Radius | Effect if Missing |
|---------|-----------------|-------------------|
| Enforcers | 15-20 tiles | Disorder increases |
| Hazard Response | 15-20 tiles | Conflagration damage +50% |
| Medical Nexus | 25-30 tiles | Longevity -10%, migration -5% |
| Learning Center | 20-25 tiles | Knowledge quotient -20% |

**Tuning Questions:**
- Can a single service building cover reasonable district size?
- Is there overlap waste, or do players need precise placement?
- Do services scale appropriately with funding level?

### 2.4 Population Growth Balance

#### Growth Rate Targets

| Colony Size | Target Growth Rate | Notes |
|-------------|-------------------|-------|
| 0-1,000 | +5-10% per cycle | Fast early growth encourages engagement |
| 1,000-10,000 | +3-5% per cycle | Steady middle game |
| 10,000-50,000 | +1-3% per cycle | Requires infrastructure investment |
| 50,000-150,000 | +0.5-1% per cycle | Quality of life matters more |
| 150,000+ | +0.1-0.5% per cycle | Near cap, prestige play |

**Growth Factors (from systems.yaml):**

| Factor | Impact | Source System |
|--------|--------|---------------|
| Energy coverage | +/- 20% | EnergySystem |
| Fluid coverage | +/- 20% | FluidSystem |
| Pathway accessibility | +/- 10% | TransportSystem |
| Harmony (happiness) | +/- 15% | PopulationSystem |
| Disorder index | -5% to -20% | DisorderSystem |
| Contamination | -5% to -15% | ContaminationSystem |
| Sector value | +5% to +15% | LandValueSystem |
| Service coverage | +10% to +20% | ServicesSystem |

### 2.5 Disaster Balance

#### Frequency Tuning

| Colony Size | Catastrophe Frequency | Rationale |
|-------------|----------------------|-----------|
| <5,000 | None (tutorial protection) | Let players learn |
| 5,000-20,000 | 1 minor per 100 cycles | Introduction to disasters |
| 20,000-50,000 | 1 per 50 cycles | Regular challenge |
| 50,000-100,000 | 1 per 30 cycles | Keeps late-game interesting |
| 100,000+ | 1 per 20 cycles | High risk, high reward |

**Note:** Manual disaster trigger available for sandbox experimentation.

#### Damage Scaling

| Catastrophe Type | Target Damage % | Recovery Time |
|------------------|-----------------|---------------|
| Conflagration (small) | 2-5% of district | 10-20 cycles |
| Conflagration (large) | 10-15% of colony | 50-100 cycles |
| Seismic Event (minor) | 5-10% structures | 20-30 cycles |
| Seismic Event (major) | 20-30% structures | 100+ cycles |
| Vortex Storm | 5-15% in path | 30-50 cycles |
| Titan Emergence | 10-20% of zone | 50-80 cycles |

**Design Goal:** Disasters should be setbacks, not game-enders. Recovery should feel achievable.

---

## 3. UX Polish

### 3.1 Tool Selection Flow

**Current Pain Points (Expected):**
1. Too many clicks to switch between zone types
2. Unclear which tool is currently active
3. No way to quickly return to last-used tool
4. Infrastructure tools (pathway, conduit, pipe) require menu diving

**Proposed Improvements:**

| Issue | Solution | Implementation |
|-------|----------|----------------|
| Zone switching | Keyboard shortcuts 1/2/3 for H/E/F | Already in spec, verify implemented |
| Active tool clarity | Persistent glow on selected tool + cursor change | Visual design task |
| Last-used tool | Right-click cancels current, returns to query | Input handler change |
| Infrastructure access | R/P/W keyboard shortcuts | Already in spec, verify implemented |

### 3.2 Build Confirmation Feedback

**Every build action needs a clear feedback loop:**

| Action | Visual Feedback | Audio Feedback | Timing |
|--------|-----------------|----------------|--------|
| Tool selected | Icon highlights, cursor changes | Soft click | Immediate |
| Valid placement hover | Green ghost preview | None | <50ms |
| Invalid placement hover | Red ghost + tooltip why | None | <50ms |
| Placement confirmed | Structure appears (materializing) | Construction sound | <100ms |
| Placement rejected | Red flash + error tooltip | Denial tone | Immediate |
| Construction complete | Structure activates (glow) | Completion chime | When done |

### 3.3 Error Messaging Clarity

**Error messages must answer: What happened? Why? What can I do?**

| Bad Error | Good Error |
|-----------|------------|
| "Cannot build here" | "Cannot construct: terrain is deep void (unbuildable)" |
| "Not enough money" | "Insufficient credits: need 5,000, have 3,200. Await tribute or take credit advance." |
| "Power required" | "Structure requires energy coverage. Construct energy conduit within 3 tiles." |
| "Invalid placement" | "Habitation zone requires pathway within 3 tiles. Nearest pathway: 5 tiles away." |

**Error Categories:**

| Category | Color | Icon | Example |
|----------|-------|------|---------|
| Resource | Amber | Credit symbol | Insufficient credits |
| Infrastructure | Cyan | Grid icon | No pathway access |
| Coverage | Blue | Wave icon | No energy coverage |
| Terrain | Brown | Ground icon | Unbuildable terrain |
| Ownership | Purple | Territory icon | Tile not chartered |
| State | Red | Warning | Structure derelict |

### 3.4 Information Density

**Data Readout (Info Panel) should layer information:**

**Layer 1: Essential (Always Visible)**
- Structure name and type
- Current state (Active/Derelict/Materializing)
- Owner (in multiplayer)

**Layer 2: Details (Click to Expand)**
- Energy requirement and status
- Fluid requirement and status
- Population/employment
- Contribution to economy

**Layer 3: Deep (Statistics Tab)**
- Historical performance
- Maintenance cost breakdown
- Neighbors affecting this tile
- Detailed component data

### 3.5 Notification Management

**Alert Pulse System Improvements:**

| Priority | Duration | Stacking | Dismiss |
|----------|----------|----------|---------|
| Critical | 8 seconds | Max 2, newest bumps oldest | Click only |
| Warning | 5 seconds | Max 3 | Auto-fade or click |
| Info | 3 seconds | Max 4 | Auto-fade |

**Notification Grouping:**
- Multiple same-type notifications combine: "3 structures completed" not 3 separate alerts
- Catastrophe notifications pin until acknowledged
- Achievement/milestone notifications get special celebratory treatment

---

## 4. Accessibility Features

### 4.1 Colorblind Support

**Required Colorblind Modes:**

| Mode | Affects | Population |
|------|---------|------------|
| Deuteranopia | Red-green confusion | ~6% of males |
| Protanopia | Red-green confusion | ~2% of males |
| Tritanopia | Blue-yellow confusion | ~0.01% |

**Implementation Strategy:**

| UI Element | Standard | Colorblind Alt |
|------------|----------|----------------|
| Valid/Invalid placement | Green/Red | Green/Striped pattern |
| Health/Danger | Green/Red | Shapes + patterns |
| Disorder overlay | Red gradient | Red + hatch pattern |
| Contamination overlay | Purple-yellow | Distinct icons per level |
| Ownership colors | Faction colors | Faction colors + border patterns |

**Additional Features:**
- High contrast mode (boosted saturation)
- Pattern overlays in addition to colors
- Icon-based status indicators (not color-only)

### 4.2 Text Scaling

**Requirements:**
- UI text scalable from 80% to 150% of default
- All UI elements must reflow gracefully
- No truncation of critical information
- Tooltips expand to fit scaled text

**Implementation Notes:**
- Use relative sizing, not absolute pixels
- Test at 150% scale with longest localized strings
- Consider separate "Large UI" mode for severe vision impairment

### 4.3 Audio Cues for Visual Events

**Every critical visual event needs an audio equivalent:**

| Visual Event | Audio Cue | Notes |
|--------------|-----------|-------|
| Structure completes | Completion chime | Already in spec |
| Catastrophe warning | Alarm tone (distinct) | Must be unmistakable |
| Catastrophe damage | Impact sound | Spatial audio at location |
| Energy collapse | Power-down hum | Colony-wide |
| Milestone achieved | Triumphant fanfare | Celebratory |
| Timer/cycle change | Subtle tick | Optional, can disable |
| Alert pulse appears | Notification ping | Different by priority |

**Audio Description Mode (Future Consideration):**
- Announce state changes via text-to-speech
- Describe selected structure properties
- Report overlay data verbally

### 4.4 Keyboard Navigation

**Full keyboard control without mouse:**

| Context | Keys | Action |
|---------|------|--------|
| General | Arrow keys | Pan camera |
| General | +/- | Zoom in/out |
| General | Q/E | Rotate camera (preset mode) |
| General | Tab | Cycle tools |
| General | Shift+Tab | Cycle overlays |
| Menu | Arrow keys | Navigate options |
| Menu | Enter | Select |
| Menu | Escape | Back/Cancel |
| Building | Space | Confirm placement |
| Building | Escape | Cancel tool |

**Focus Indicators:**
- Visible focus rectangle on all interactive elements
- Focus must be keyboard-trappable in dialogs
- Tab order must be logical (left-to-right, top-to-bottom)

### 4.5 Reduced Motion Mode

**For users sensitive to motion:**
- Disable parallax effects
- Reduce animation speeds by 50%
- Option to disable camera smooth transitions
- Static alternatives to animated UI elements
- Disable holographic flicker effects

### 4.6 Subtitles and Captions

**If voice is ever added (future consideration):**
- All spoken content must have subtitles
- Important sound effects should have visual indicators
- Caption positioning should not block gameplay

---

## 5. "Cozy Sandbox" Vibe Preservation

### 5.1 Core Principle Reminder

From CANON.md Core Principle #4:
> "This is NOT a competitive game with victory conditions. No win/lose states. Endless play - build, prosper, hang out with friends. Social/casual vibe. Optional 'immortalize' feature to freeze a city as a monument."

### 5.2 Stress-Reducing Design Decisions

| Stressor | Mitigation | Implementation |
|----------|------------|----------------|
| Time pressure | Pause anytime, no penalties | Pause is always available |
| Catastrophe anxiety | Tutorial protection, recovery support | No disasters <5K pop |
| Financial ruin | Credit advances, slow decline | Never instant bankruptcy |
| Competition | Optional, visual only | No forced PvP mechanics |
| Information overload | Progressive disclosure | Layer information depth |
| Mistake punishment | Undo pathway demolition, cheap rebuild | Low demolition costs |

### 5.3 Achievement Celebration

**Milestone Achievements (from Epic 14):**

| Population | Milestone | Celebration |
|------------|-----------|-------------|
| 2,000 | Colony Emergence | Alert pulse + subtle fanfare |
| 10,000 | Colony Establishment | Alert pulse + moderate fanfare |
| 30,000 | Colony Identity | Alert + camera flourish + fanfare |
| 60,000 | Colony Security | Alert + camera flourish + fanfare |
| 90,000 | Colony Wonder | Alert + camera flourish + dramatic fanfare |
| 120,000 | Colony Ascension | Alert + camera flourish + triumphant fanfare |
| 150,000 | Colony Transcendence | Full celebration sequence |

**Celebration Elements:**
1. **Alert Pulse**: Special "achievement" style, gold/cyan, longer duration
2. **Audio**: Triumphant tones, scale with milestone tier
3. **Camera Flourish**: Brief zoom-out to show full colony (optional, can disable)
4. **Population Counter Animation**: Numbers tick up dramatically
5. **Confetti/Particle Effect**: Subtle bioluminescent sparkles (optional)

**Daily/Session Achievements:**
- "First structure of the session" - welcome back message
- "Cycle milestone" (every 100 cycles) - subtle acknowledgment
- "Population personal best" - celebrate growth

### 5.4 Multiplayer Cooperation Feel

**Encouraging Positive Social Interaction:**

| Feature | Purpose | Vibe |
|---------|---------|------|
| Shared pathway networks | Mutual benefit | "We're connected" |
| Resource trading | Win-win exchanges | "Let's help each other" |
| Milestone announcements | Celebrate others' success | "Congrats on 30K!" |
| No land stealing | Safe ownership | "My stuff is safe" |
| Ghost town reclaim | Fresh starts | "New opportunities" |

**Anti-Griefing Measures:**
- Cannot buy tiles from other players (only from Game Master)
- Cannot block pathway connections maliciously
- Rollback requires consent (time-gated voting)
- No negative interactions possible (can't attack, steal, sabotage)

### 5.5 Session Pacing

**The "One More Turn" Flow:**

| Session Length | Expected Progress | Feeling |
|----------------|-------------------|---------|
| 5 minutes | Check colony, collect tribute, minor adjustments | "Quick peek" |
| 15 minutes | Build a district, expand infrastructure | "Getting stuff done" |
| 30 minutes | Significant expansion, see growth | "Satisfying session" |
| 1 hour | Major milestone progress, strategic planning | "Deep engagement" |
| 2+ hours | Transformation of colony, potential achievement | "Dedicated play" |

**Session Bookends:**
- On launch: "Welcome back, Overseer. 47 cycles have passed."
- On quit: "Colony state synchronized. Your beings await your return."

### 5.6 Atmosphere Maintenance

**The world should feel alive and calm:**

| Element | Contribution | Implementation |
|---------|--------------|----------------|
| Ambient music | Relaxation | Looping, crossfading, user-sourced |
| Colony hum | Presence | Scales with population |
| Being movement | Activity | Cosmetic entities on pathways |
| Bioluminescent glow | Magic | Terrain and structures pulse subtly |
| Day/night cycle | Rhythm | Optional, cosmetic only |
| Weather effects | Variety | Rain, fog (cosmetic, no gameplay impact) |

---

## 6. Questions for Other Agents

### 6.1 For Systems Architect

1. **Terminology Enforcement Infrastructure:**
   - Is there a centralized string table or localization system?
   - How are dynamic strings (formatting, concatenation) handled?
   - Can we add compile-time checks for banned terms?

2. **Balance Data Externalization:**
   - Are balance values (costs, rates, thresholds) in config files or hardcoded?
   - Can we hot-reload balance changes without recompiling?
   - Is there a tuning override system for playtesting?

3. **Accessibility System Support:**
   - Is there a global settings system that persists colorblind/text scale preferences?
   - Can shaders be swapped at runtime for colorblind modes?
   - How do we handle audio-only mode for screen readers?

4. **Performance Budget for Polish:**
   - What's the frame time budget for UI animations?
   - Can we add particle effects for celebrations without impacting simulation?
   - Are there rendering passes available for outline/glow effects on focus?

### 6.2 For QA Engineer

5. **Terminology Testing Strategy:**
   - How do we automate scanning for non-alien terms?
   - Should we have a "terminology lint" in CI/CD?
   - What's the test matrix for localization (if applicable)?

6. **Accessibility Testing:**
   - Do we have access to screen reader testing tools?
   - How do we validate colorblind modes without colorblind testers?
   - What devices/resolutions do we need to test text scaling on?

7. **Balance Validation:**
   - Can we automate playthroughs to detect balance issues?
   - What metrics should we track during QA playtests?
   - How do we detect "feels wrong" without specific bug reports?

8. **UX Friction Identification:**
   - Should we track click counts per action?
   - Can we instrument "rage quit" moments (repeated failed placements)?
   - How do we gather qualitative feedback on "cozy" feeling?

### 6.3 For UI Developer

9. **Animation Performance:**
   - What's the budget for UI animations (panel slides, fades)?
   - Can celebration effects be disabled for low-spec machines?
   - How do we handle animation stacking (multiple celebrations)?

10. **Accessibility Implementation:**
    - Is there a UI framework feature for focus management?
    - How do we implement high-contrast mode (CSS-like theming)?
    - Can tooltips adapt to text scaling automatically?

---

## 7. Recommendations

### 7.1 MVP Polish (P0 - Must Have)

| Feature | Rationale |
|---------|-----------|
| Complete terminology audit | Core Principle #5 compliance |
| Error message clarity | Player frustration reduction |
| Critical alert audio | Accessibility baseline |
| Basic colorblind mode | Accessibility baseline |
| Milestone celebrations | Reward engagement |

### 7.2 Full Polish (P1 - Should Have)

| Feature | Rationale |
|---------|-----------|
| Text scaling | Accessibility requirement |
| Keyboard navigation | Accessibility requirement |
| Balance tuning pass | Gameplay quality |
| Notification grouping | Information overload reduction |
| Tool feedback improvements | UX smoothness |

### 7.3 Extended Polish (P2 - Nice to Have)

| Feature | Rationale |
|---------|-----------|
| Reduced motion mode | Expanded accessibility |
| High contrast mode | Expanded accessibility |
| Advanced colorblind modes | Full accessibility coverage |
| Session bookend messages | Cozy feel |
| Weather effects | Atmosphere enhancement |

### 7.4 Canon Update Proposals

#### New Terminology for Epic 17

```yaml
# Additions to terminology.yaml

polish:
  settings: colony_preferences        # Settings menu
  accessibility: overseer_assistance  # Accessibility options
  colorblind_mode: chromatic_adaptation
  text_scaling: glyph_magnitude
  reduced_motion: stabilized_display
  audio_cues: sonic_indicators
  keyboard_mode: tactile_interface

feedback:
  error: anomaly_alert               # Error messages
  warning: caution_signal            # Warning messages
  success: confirmation_pulse        # Success messages
  progress: advancement_indicator    # Progress bars

celebration:
  achievement: attainment            # Generic achievement
  fanfare: triumph_tone              # Celebratory sound
  confetti: luminous_particles       # Visual celebration
```

### 7.5 Testing Strategy

**Terminology Verification:**
1. Automated grep for banned terms in all `.cpp`, `.h` files
2. Manual review of all UI screenshots
3. Fresh player playtest (do they use alien terms naturally?)

**Balance Verification:**
1. Automated 1000-cycle simulation runs
2. Metric collection: time to break-even, growth curves, treasury trends
3. Player survey: "Did you feel stressed about money/disasters?"

**Accessibility Verification:**
1. Screen reader compatibility test
2. Color blindness simulation tool review
3. Keyboard-only playthrough attempt
4. Text scaling at 150% full gameplay test

**Cozy Feel Verification:**
1. Player survey: "Rate the stress level 1-10"
2. Session length tracking: Do players play longer than intended?
3. Return rate: Do players come back?
4. Qualitative feedback: "How did the game make you feel?"

---

## 8. Summary

Epic 17 is the finishing touch that transforms ZergCity from a functional city builder into a cohesive alien colony experience. Through meticulous terminology enforcement, thoughtful balance tuning, UX polish, accessibility features, and careful preservation of the cozy sandbox vibe, this epic ensures:

1. **Every word feels alien** - Players never encounter "city" or "citizens"
2. **The simulation is fair** - Challenge without frustration
3. **The interface is smooth** - Friction is minimized, feedback is clear
4. **Everyone can play** - Accessibility is not an afterthought
5. **It feels cozy** - Stress is managed, achievements are celebrated, cooperation is encouraged

This is not a technical epic. It's an experience epic. The measure of success is not "did it compile?" but "does it feel right?"

---

*This analysis establishes Game Designer priorities for Epic 17. Technical implementation details require input from Systems Architect, UI Developer, and QA Engineer.*
