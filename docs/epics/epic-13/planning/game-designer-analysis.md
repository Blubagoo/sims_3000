# Epic 13: Disasters - Game Designer Analysis

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Canon Version:** 0.13.0
**Status:** Initial Analysis

---

## Executive Summary

Catastrophes in ZergCity must create **memorable moments of tension and recovery** without undermining the casual, social sandbox experience. Unlike competitive games where disasters punish players, our endless sandbox design means catastrophes should be **exciting shared events** that bring overseers together, create stories worth telling, and offer meaningful recovery gameplay.

The alien theme provides unique opportunities: void anomalies, titan emergences, and cosmic radiation events can feel genuinely novel compared to traditional city builder disasters.

---

## 1. Experience Goals

### 1.1 Target Emotions

| Emotion | When | How |
|---------|------|-----|
| **Anticipation** | Warning phase | Environmental cues, early alerts, time to prepare |
| **Tension** | Active catastrophe | Decisions matter, resources strain, damage accumulates |
| **Relief** | Catastrophe ends | Immediate danger passes, assessment begins |
| **Determination** | Recovery | Rebuilding feels purposeful, not punishing |
| **Pride** | Rebuilt | Colony restored, maybe even improved |
| **Connection** | Throughout | Other overseers witness, help, or commiserate |

### 1.2 Emotional Arc

```
ANTICIPATION → TENSION → RELIEF → DETERMINATION → PRIDE
   (Warning)    (Active)   (End)    (Rebuild)    (Restored)
```

**Critical:** The arc must be complete. A catastrophe that creates tension but frustrates recovery fails the experience.

### 1.3 What Makes Catastrophes Memorable

1. **Visual spectacle** - Bioluminescent effects going haywire, structures collapsing with particle effects
2. **Sound design** - Warning sirens, impact sounds, ambient shifts
3. **Stakes that feel real** - Buildings we spent time creating are at risk
4. **Stories to share** - "Remember when the vortex_storm hit right after I finished my exchange district?"
5. **Recovery triumphs** - "I rebuilt better than before"

### 1.4 Anti-Frustration Principles

- **Never instant-kill a whole colony** - Even the worst catastrophe leaves something salvageable
- **Warnings are essential** - Time to save/prepare (even if brief)
- **No permanent loss of unlockables** - Milestones, progression, achievements remain
- **Insurance/recovery assistance** - Treasury bonds, aid mechanics
- **Recovery should be faster than initial build** - Debris clearance cheaper than fresh construction

---

## 2. Catastrophe Design

### 2.1 Canonical Terminology

Per `terminology.yaml`, we use alien terms:

| Human Term | Alien Term | Notes |
|------------|------------|-------|
| Disaster | Catastrophe | Generic term for all events |
| Earthquake | Seismic Event | Ground shaking |
| Flood | Inundation | Water level rise |
| Tornado | Vortex Storm | Localized wind funnel |
| Hurricane | Mega Storm | Wide-area wind damage |
| Fire | Conflagration | Structure burning/spread |
| Riot | Civil Unrest | Population disorder spike |
| Nuclear Meltdown | Core Breach | Energy nexus failure |
| Monster Attack | Titan Emergence | Giant creature event |
| Cosmic Event | Void Anomaly | Space-based disaster |

### 2.2 Catastrophe Categories

#### Natural Catastrophes (Translated from SimCity)

| Catastrophe | Trigger | Area | Duration | Damage Type |
|-------------|---------|------|----------|-------------|
| **Seismic Event** | Random, terrain-based | Regional radius | 5-15 seconds | Structural |
| **Inundation** | Random near water | Low elevation tiles | Progressive | Water damage |
| **Mega Storm** | Random, seasonal | Map-wide (wind path) | 30-60 seconds | Wind + water |
| **Vortex Storm** | Random, rare | Narrow path across map | 15-30 seconds | Severe structural |
| **Conflagration** | Spreading from source | Tile-by-tile spread | Until contained | Fire damage |

#### Colony-Induced Catastrophes

| Catastrophe | Trigger | Area | Duration | Damage Type |
|-------------|---------|------|----------|-------------|
| **Core Breach** | Nuclear nexus failure | Large radius from plant | Instant + fallout | Radiation |
| **Civil Unrest** | High disorder + low harmony | District-wide | Until resolved | Vandalism |
| **Chemical Spill** | Industrial accident | Spreading from source | Until cleaned | Contamination |
| **Microwave Misfire** | Microwave receiver malfunction | Random beam target | Instant | Fire + structural |

#### Alien-Themed Catastrophes (Unique to ZergCity)

| Catastrophe | Description | Area | Duration | Damage Type |
|-------------|-------------|------|----------|-------------|
| **Void Anomaly** | Spatial distortion appears, warping/damaging structures | Expanding sphere | 20-40 seconds | Reality warping |
| **Cosmic Radiation** | Solar flare equivalent, disables electronics | Map-wide | 15-30 seconds | System shutdown |
| **Titan Emergence** | Giant creature rises from terrain | Path across map | 45-90 seconds | Physical destruction |
| **Spore Cascade** | Bioluminescent flora goes hyperactive | Spreading from source | Until contained | Overgrowth |
| **Crystal Resonance** | Prisma fields resonate destructively | Near crystal terrain | 10-20 seconds | Sonic/structural |
| **Blight Surge** | Toxic marshes expand aggressively | From blight_mires | Progressive | Contamination |

### 2.3 Severity Tiers

| Tier | Frequency | Warning Time | Damage Scale | Examples |
|------|-----------|--------------|--------------|----------|
| **Minor** | Common | 30+ seconds | 5-15% of affected area | Small conflagration, minor seismic |
| **Major** | Uncommon | 15-30 seconds | 25-40% of affected area | Vortex storm, civil unrest |
| **Catastrophic** | Rare | 10-15 seconds | 50-70% of affected area | Core breach, titan emergence |
| **Extinction-Level** | Extremely rare | 5-10 seconds | 70-90% of affected area | Void anomaly (extreme) |

**Design Note:** Extinction-level events should be **optional** and **toggleable separately** from regular catastrophes. Some players want the ultimate challenge; others want to build in peace.

### 2.4 Catastrophe Fit Analysis

#### SimCity Classics That Translate Well
- **Seismic Event** - Universal, dramatic, fits alien theme
- **Conflagration** - Classic gameplay loop (spread prevention, response)
- **Vortex Storm** - Visually spectacular, path-based destruction
- **Core Breach** - High stakes, consequence of player choices (nuclear energy)

#### SimCity Classics That Need Rethinking
- **UFO Attack** - We ARE the aliens. Replace with **titan_emergence** (native creature)
- **Flood** - Works, but rename to **inundation** for alien feel
- **Riot** - Becomes **civil_unrest**, tied to disorder/harmony systems

#### New Alien Catastrophes
- **Void Anomaly** - The crown jewel. Spatial distortion with unique visual effects (reality tearing, chromatic aberration, structure warping). Truly alien.
- **Cosmic Radiation** - Electronics fail, structures lose power temporarily. Creates interesting infrastructure vulnerability.
- **Titan Emergence** - Not a kaiju movie monster, but a massive native creature disturbed by colony activity. Thematically fits "aliens living on alien world."
- **Spore Cascade** - Bioluminescent flora overgrowing structures. Visually stunning, mechanically interesting (removal gameplay).

---

## 3. Player Agency

### 3.1 Prevention & Mitigation

| Catastrophe | Prevention | Mitigation |
|-------------|------------|------------|
| Seismic Event | None (natural) | Earthquake-resistant structures (upgrade) |
| Inundation | Avoid building at low elevation | Flood barriers, drainage |
| Conflagration | Hazard response coverage | Fire breaks, response dispatch |
| Civil Unrest | Maintain harmony, enforcer coverage | Rapid response, harmony boost |
| Core Breach | Plant maintenance, shutdown protocols | Evacuation, containment |
| Titan Emergence | Avoid disturbing deep terrain? | Defense installation, distraction |
| Void Anomaly | None (cosmic) | Anomaly containment tech? |

**Key Principle:** Some catastrophes are preventable (player fault), some are mitigable (player response), some are inevitable (nature happens). The mix creates varied experiences.

### 3.2 During-Catastrophe Decisions

Players should have **meaningful choices** during active catastrophes:

1. **Dispatch Resources**
   - Send hazard responders to conflagration
   - Send enforcers to civil unrest
   - Prioritize which districts to protect

2. **Triage Decisions**
   - Save the exchange district or the fabrication zone?
   - Protect the energy nexus or the fluid extractors?
   - These create genuine tension

3. **Emergency Spending**
   - Emergency fund activation
   - Rapid response bonuses (speed vs. cost)
   - Mutual aid requests to other overseers

4. **Evacuation**
   - Order beings to evacuate (reduces casualties)
   - Structural damage continues, but population preserved

### 3.3 Recovery Gameplay Loop

Recovery should be a **satisfying loop**, not a chore:

```
DAMAGE ASSESSMENT → DEBRIS CLEARANCE → INSURANCE/AID → REBUILD → IMPROVED
```

1. **Damage Assessment**
   - Survey overlay shows damaged structures
   - Estimated costs displayed
   - Insurance payout calculated

2. **Debris Clearance**
   - Debris must be cleared before rebuilding
   - Faster/cheaper than original demolition
   - Can be automated (toggle)

3. **Insurance/Aid**
   - Treasury bonds available at low interest
   - Mutual aid from other overseers (optional)
   - "Colony Relief Fund" for catastrophic events

4. **Rebuild**
   - Rebuilding same structure type = 50% cost discount
   - Zone persists under debris (don't re-zone)
   - Fast-track construction option

5. **Improvement**
   - Opportunity to upgrade while rebuilding
   - "Silver lining" mechanics (new building variety unlocked?)

---

## 4. Multiplayer Dynamics

### 4.1 Catastrophe Scope

| Scope | Description | Use Cases |
|-------|-------------|-----------|
| **Local** | Affects one overseer only | Most catastrophes by default |
| **Regional** | Affects adjacent overseers | Spreading events (conflagration at border) |
| **Global** | Affects all overseers simultaneously | Cosmic radiation, mega storms |

**Recommendation:** Default most catastrophes to **local** scope. Global events should be rare and serve as **shared experiences** that bring overseers together.

### 4.2 Cooperation Opportunities

- **Mutual Aid**
  - Send credits to affected overseer
  - "Loan" hazard responders (if adjacent)
  - Share resources during recovery

- **Shared Response**
  - Coordinate containment of spreading events
  - Prevent cross-border conflagration spread
  - Joint titan_emergence defense

- **Empathy Moments**
  - See other overseers' catastrophes in real-time
  - Notification: "Overseer X is experiencing a vortex_storm"
  - Creates social connection, stories to share

### 4.3 Griefing Potential

**Manual Catastrophe Toggle** is essential for sandbox multiplayer:

| Setting | Description | Recommendation |
|---------|-------------|----------------|
| `catastrophes_enabled` | Random catastrophes occur | Default ON |
| `manual_catastrophes` | Players can trigger catastrophes | Default OFF |
| `cross_boundary` | Catastrophes can spread across ownership | Default ON (limited) |
| `pvp_catastrophes` | Players can target catastrophes at others | **NEVER** (not in game) |

**Critical Decision:** Manual catastrophes should be **self-targeting only** in multiplayer. You can trigger a catastrophe in YOUR colony for testing/fun, but never in another player's colony. This prevents griefing entirely.

### 4.4 Shared Events as Social Glue

**"Colony-Wide Emergency" Events:**
- Server announces incoming global catastrophe
- All overseers see countdown
- Creates shared anticipation and preparation
- Post-event: Shared recovery, stories, commiseration

These events are memorable because they affect everyone. "Remember the Great Cosmic Radiation of Cycle 247?"

---

## 5. Sandbox vs. Challenge

### 5.1 Essential Toggles

```yaml
catastrophe_settings:
  enabled: true/false        # Master toggle
  random_events: true/false  # Whether random catastrophes occur
  manual_trigger: true/false # Whether manual trigger is available
  severity_cap: minor/major/catastrophic/extinction  # Max severity
  frequency: low/medium/high # How often they occur
  warning_time_bonus: 0/50/100%  # Extra warning time
```

### 5.2 Sandbox Mode Expectations

For players who want a **pure building sandbox**:
- Catastrophes fully disabled
- No surprise interruptions
- Focus on creative expression
- Still can manually trigger for "what if" scenarios

### 5.3 Challenge Mode Expectations

For players who want **tension and stakes**:
- Full catastrophe system enabled
- Random events at chosen frequency
- Emergency response as core gameplay
- Recovery loop as part of the experience

### 5.4 Difficulty Scaling

| Difficulty | Warning Time | Damage Scale | Frequency | Recovery Aid |
|------------|--------------|--------------|-----------|--------------|
| Relaxed | +100% | -50% | Low | High |
| Standard | Normal | Normal | Medium | Normal |
| Challenging | -25% | +25% | High | Low |
| Brutal | -50% | +50% | Very High | None |

---

## 6. Alien Theme Integration

### 6.1 Visual Language

Catastrophes should look **alien**, not just Earth-like:

| Catastrophe | Alien Visual Element |
|-------------|---------------------|
| Seismic Event | Ground glows along fault lines before rupture |
| Conflagration | Fire is blue-green bioluminescent, not orange |
| Vortex Storm | Swirling with glowing particles, chromatic shifts |
| Inundation | Water rises with eerie glow, alien creatures visible |
| Civil Unrest | Disorder visualized as disrupted bioluminescence |
| Void Anomaly | Reality tears, chromatic aberration, geometry warping |
| Titan Emergence | Massive bioluminescent creature, awe-inspiring scale |
| Cosmic Radiation | Aurora-like effects across sky, electronics flicker |
| Spore Cascade | Explosive bioluminescent growth, pulsing patterns |

### 6.2 Thematic Coherence

**All catastrophes should feel like part of the alien world:**

- **Natural** catastrophes = alien planet's nature acting up
- **Colony-induced** = consequences of our alien civilization
- **Cosmic** = we're in a hostile universe as aliens too

**Avoid:** Generic Earth-like disasters that feel transplanted. Make every catastrophe feel like it belongs to this world.

### 6.3 Audio Design Implications

| Catastrophe Type | Sound Design Direction |
|------------------|----------------------|
| Natural | Deep rumbles, alien wildlife sounds, environmental shifts |
| Colony-induced | Industrial tones, alarms, structural stress |
| Cosmic | Otherworldly hums, reality-distortion sounds, void echoes |

---

## 7. Questions for Other Agents

### @systems-architect:

1. **Catastrophe System Architecture**
   - Should DisasterSystem use a state machine for each active catastrophe?
   - How do we handle multiple simultaneous catastrophes?
   - What's the tick_priority for DisasterSystem relative to BuildingSystem?

2. **Cross-System Interactions**
   - How does disaster damage interact with BuildingSystem's state machine (Active -> Damaged -> Derelict -> Debris)?
   - Should there be a DamageableComponent or is damage calculated from existing components?
   - How does conflagration spread interact with ContaminationSystem?

3. **Multiplayer Sync**
   - Catastrophe RNG seeding for deterministic replay across clients?
   - Delta sync for catastrophe progression state?
   - How do we handle catastrophe effects crossing player boundaries?

4. **Dense Grid for Catastrophe Effects?**
   - Should catastrophe effects (fire spread, damage zones) use a dense grid like other systems?
   - DamageGrid as transient data structure during active catastrophes?

### @services-engineer:

1. **Emergency Response Integration**
   - How does hazard response dispatch work during catastrophe events?
   - Can multiple service buildings respond simultaneously?
   - Response effectiveness calculation during catastrophe vs normal calls?

2. **Recovery Economy**
   - Insurance payout calculation formula needed
   - Treasury bond special terms during catastrophe recovery?
   - Cost reduction mechanics for rebuilding same structure type

3. **Database Persistence**
   - Active catastrophe state persistence (server restart mid-catastrophe?)
   - Historical catastrophe log for newspaper/statistics?

---

## 8. Canon Update Proposals

Based on this analysis, I propose the following canon additions:

### 8.1 Terminology Additions

```yaml
disasters:
  # Additional terms
  disaster_warning: catastrophe_alert
  emergency_response: rapid_response
  evacuation: exodus_protocol
  rubble_clearance: debris_reclamation
  insurance: catastrophe_indemnity

  # New alien catastrophes
  spore_cascade: spore_cascade      # Keep as-is, already alien
  crystal_resonance: prisma_resonance
  blight_surge: blight_surge        # Keep as-is
```

### 8.2 Notification Priorities

```yaml
notifications:
  priorities:
    critical:
      additions:
        - catastrophe_imminent
        - core_breach
        - titan_emergence
    warning:
      additions:
        - catastrophe_approaching
        - civil_unrest_building
        - contamination_spreading
```

---

## 9. Summary & Recommendations

### Key Design Principles

1. **Catastrophes are events, not punishments** - They create stories, not frustration
2. **Always give warning** - No unfair instant-death scenarios
3. **Recovery is gameplay** - Make rebuilding satisfying, not tedious
4. **Alien theme throughout** - Every catastrophe should feel like it belongs to this world
5. **Multiplayer-aware** - Shared experiences bond players; griefing is prevented
6. **Toggleable everything** - Sandbox players have full control

### Priority Catastrophe List (MVP)

1. **Conflagration** - Core spreading mechanic, hazard response integration
2. **Seismic Event** - Classic, dramatic, easy to implement
3. **Core Breach** - Player consequence, energy system integration
4. **Void Anomaly** - Unique alien catastrophe, visual showpiece
5. **Titan Emergence** - Memorable set piece, differentiator from other builders

### Deferred Catastrophes (Post-MVP)

- Mega Storm (requires weather system)
- Cosmic Radiation (requires electronics vulnerability system)
- Spore Cascade (requires overgrowth mechanics)
- Blight Surge (contamination expansion)

---

*This analysis focuses on player experience and thematic coherence. Technical implementation details require Systems Architect input.*
