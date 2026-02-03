# Game Designer Analysis: Epic 15 - Audio System

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Canon Version:** 0.15.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 15 implements the audio experience for ZergCity - an alien colony builder with a cozy, social sandbox vibe. The audio system must reinforce the alien aesthetic while providing satisfying feedback for player actions.

Key design characteristics:
1. **Alien Aesthetic:** Sounds should be crystalline, organic-electronic, and otherworldly
2. **Cozy Atmosphere:** Relaxing ambient soundscape for casual play
3. **Clear Feedback:** Immediate audio response to player actions
4. **User Music:** Support for player-curated background music

---

## Table of Contents

1. [Audio Design Philosophy](#1-audio-design-philosophy)
2. [Sound Effect Design](#2-sound-effect-design)
3. [Music System Design](#3-music-system-design)
4. [Ambient Soundscape](#4-ambient-soundscape)
5. [Audio Feedback Timing](#5-audio-feedback-timing)
6. [Volume and Mixing](#6-volume-and-mixing)
7. [Accessibility Considerations](#7-accessibility-considerations)
8. [Multiplayer Audio](#8-multiplayer-audio)
9. [Questions for Other Agents](#9-questions-for-other-agents)
10. [Player Experience Goals](#10-player-experience-goals)

---

## 1. Audio Design Philosophy

### 1.1 Core Principles

**Alien, Not Human**
- Avoid traditional city builder sounds (human construction equipment, traffic noise)
- Use crystalline, resonant, organic-electronic tones
- Think: bioluminescent pulses, energy harmonics, organic growth sounds

**Cozy Over Stressful**
- Even disasters should sound "interesting" not "panic-inducing"
- Ambient soundscape should be relaxing
- No harsh, jarring sounds

**Informative Without Intrusive**
- Audio confirms actions without demanding attention
- Important events (milestones) get emphasized treatment
- Frequent actions (zone placement) get subtle confirmation

**Alien Terminology Reflection**
- Sound design should match canon terminology
- "Materializing" not "construction" - sounds more ethereal
- "Alert pulse" not "notification beep" - more organic tone

### 1.2 Sound Palette

| Category | Character | Examples |
|----------|-----------|----------|
| Construction | Crystalline growth, energy formation | Soft chimes, resonant harmonics |
| Infrastructure | Energy flow, connection establishment | Whooshes, electronic hums |
| UI | Clean, responsive | Soft clicks, subtle tones |
| Disasters | Otherworldly but not terrifying | Cosmic rumbles, energy distortion |
| Ambient | Organic, atmospheric | Wind through crystals, bioluminescent pulses |
| Milestones | Celebratory, triumphant | Harmonic fanfares, ascending tones |

---

## 2. Sound Effect Design

### 2.1 Construction Sounds

**Zone Designation**
- Brief, satisfying "confirmation" tone
- Crystalline ping with subtle reverb
- Duration: 0.3-0.5 seconds
- Pitch: Varies slightly by zone type (habitation = warm, fabrication = industrial)

**Building Materializing**
- Organic growth sound, like crystal forming
- Starts quiet, builds slightly over construction duration
- Subtle energy shimmer undertone
- Duration: 2-3 seconds (can loop for longer constructions)

**Building Complete**
- Triumphant completion chime
- Harmonic chord that resolves pleasantly
- Duration: 1-1.5 seconds
- Should feel rewarding without being distracting

**Demolition/Deconstruction**
- Reverse of materializing - energy dispersing
- Crystalline dissolution sound
- Not destructive/violent sounding
- Duration: 0.5-1 second

### 2.2 Infrastructure Sounds

**Pathway Placement**
- Quick, subtle connection sound
- Like energy flowing through a circuit
- Duration: 0.2-0.3 seconds
- Slightly different pitch for each segment (avoids monotony)

**Conduit/Pipe Placement**
- Similar to pathway but with more "energy" character
- Conduits: higher frequency electrical hum
- Pipes: lower frequency fluid flow undertone
- Duration: 0.2-0.3 seconds

**Infrastructure Connected**
- Satisfying "click" when networks link
- Brief harmonic resolution
- Feedback that systems are working together
- Duration: 0.3-0.5 seconds

### 2.3 UI Sounds

**Button Click**
- Soft, responsive tap
- Not mechanical - organic touch response
- Duration: 0.1 seconds
- Consistent across all buttons

**Tool Selected**
- Slightly more pronounced than click
- Brief tone that hints at tool function
- Zone tool: soft chime, Demolish: subtle warning tone
- Duration: 0.2-0.3 seconds

**Menu Open/Close**
- Gentle whoosh or slide sound
- Open: ascending pitch, Close: descending pitch
- Duration: 0.2-0.4 seconds

**Alert Pulse (Notification)**
- Attention-getting but not jarring
- Bioluminescent "pulse" sound
- Higher priority = more prominent tone
- Duration: 0.5-1 second

### 2.4 Simulation Sounds

**Disaster Warning**
- Atmospheric tension build
- Low rumble or cosmic disturbance
- Gives player time to react (matches warning duration)
- Duration: 3-5 seconds (loops until warning ends)

**Disaster Active**
- Type-specific ambient loop:
  - Conflagration: Crackling energy, not literal fire
  - Seismic: Low rumbling, ground disturbance
  - Inundation: Flowing, rushing fluid
  - Vortex: Swirling wind, energy distortion
- Duration: Loops during disaster

**Disaster Ended**
- Relief/resolution tone
- Energy settling down
- Brief musical motif
- Duration: 1-2 seconds

**Milestone Achieved**
- Triumphant fanfare!
- Full harmonic celebration
- This is the big payoff - make it feel special
- Duration: 3-5 seconds
- Music ducks to let fanfare shine

**Population Threshold**
- Subtle but positive
- Quick ascending chime
- Less prominent than milestone
- Duration: 1 second

---

## 3. Music System Design

### 3.1 Music Direction

**Ambient, Not Active**
- Background atmosphere, not foreground entertainment
- Should fade into consciousness during focused play
- Comes forward during idle moments

**User-Curated**
- Players provide their own music
- We provide default ambient tracks as fallback
- Music complements, doesn't define experience

**Crossfade Transition**
- 3-second crossfade between tracks (canon specification)
- No jarring stops or starts
- Maintains atmospheric continuity

### 3.2 Default Music Style

If we provide default tracks:
- Ambient electronica
- Minimal, spacey, atmospheric
- No strong beats or melodies that demand attention
- Artists for reference: Brian Eno, Tycho, Boards of Canada (ambient works)

### 3.3 Music Behavior

**Normal Gameplay**
- Music plays at moderate volume
- Loops through playlist continuously
- Shuffle recommended for variety

**Important Events**
- Music ducks (reduces volume) during:
  - Milestone fanfares (duck to 30%)
  - Disaster warnings (duck to 60%)
  - Active disasters (duck to 50%)
- Returns to normal after event

**Pause Menu**
- Music continues playing
- Slight filter effect optional (muffled/distant)
- Reinforces that world is still there

---

## 4. Ambient Soundscape

### 4.1 Layered Ambient Design

Three ambient layers that blend together:

**Layer 1: Environmental Base**
- Constant atmospheric bed
- Wind through crystal formations
- Subtle alien wildlife (distant, non-specific)
- Volume: Low, always present

**Layer 2: Bioluminescent Pulses**
- Periodic rhythmic element
- Soft "breathing" or pulsing sounds
- Varies with time of day in simulation
- Volume: Subtle, adds life

**Layer 3: Colony Activity Hum**
- Scales with population (canon specification)
- No population: Silent
- Small colony: Faint hum
- Large colony: Rich, layered activity sound
- Volume: 0% at 0 pop, scales to ~30% at 100K beings

### 4.2 Population Scaling

| Population | Activity Level | Character |
|------------|----------------|-----------|
| 0-1,000 | Minimal | Near silence, just environment |
| 1,000-10,000 | Emerging | Faint hum, occasional energy crackle |
| 10,000-50,000 | Active | Steady background activity |
| 50,000-100,000 | Bustling | Rich, layered colony sounds |
| 100,000+ | Thriving | Full ambient activity, harmonic undertones |

### 4.3 Zoom-Based Mixing

**Zoomed In**
- More local sounds (building-specific)
- Positional audio more pronounced
- Individual construction sounds audible

**Zoomed Out**
- More colony-wide ambient blend
- Positional audio less prominent
- Overview "city hum" dominates

---

## 5. Audio Feedback Timing

### 5.1 Response Latency

**Immediate (< 50ms)**
- UI clicks
- Tool selection
- Button presses

**Quick (50-100ms)**
- Zone placement
- Infrastructure placement
- Building selection

**Synchronized**
- Building complete (matches animation end)
- Disaster warning (matches visual warning)
- Milestone (matches UI popup)

### 5.2 Sound Duration Guidelines

| Category | Minimum | Maximum | Notes |
|----------|---------|---------|-------|
| UI Click | 0.05s | 0.15s | Quick and responsive |
| Zone Place | 0.2s | 0.5s | Satisfying but not long |
| Building Complete | 1.0s | 2.0s | Celebratory |
| Milestone | 3.0s | 5.0s | Special occasion |
| Disaster Warning | 3.0s | Loop | Matches warning duration |
| Ambient Layer | Loop | Loop | Continuous |

### 5.3 Cooldown and Spam Prevention

**Rapid Placement**
- Zone tool can be dragged across many tiles
- Don't play sound for every tile!
- Aggregate to single sound per drag action
- Or: Play first sound, then subtle "continuation" for additional tiles

**Batch Operations**
- Multiple buildings completing simultaneously
- Combine into single, richer sound
- Not N separate identical sounds

---

## 6. Volume and Mixing

### 6.1 Channel Balance

| Channel | Default Volume | Typical Range | Notes |
|---------|----------------|---------------|-------|
| Master | 100% | 0-100% | Global multiplier |
| Music | 70% | 0-100% | Lower default, background |
| SFX | 100% | 0-100% | Primary feedback |
| Ambient | 50% | 0-100% | Atmospheric, not prominent |
| UI | 100% | 0-100% | Clear feedback priority |

### 6.2 Ducking Relationships

| Trigger | Music Duck | Duration | Recovery |
|---------|------------|----------|----------|
| Milestone Achieved | 30% | 3 sec | 1 sec fade back |
| Disaster Warning | 60% | Warning duration | 1 sec fade back |
| Disaster Active | 50% | Active duration | 1 sec fade back |
| Disaster Ended | Normal | N/A | Already recovering |
| Population Threshold | 70% | 1 sec | 0.5 sec fade back |

### 6.3 Priority System

When too many sounds compete:

**Critical (Always Play)**
1. Disaster Warning
2. Milestone Achieved
3. UI Alerts

**High (Usually Play)**
4. Building Complete
5. UI Clicks
6. Tool Selection

**Normal (Play If Room)**
7. Zone Placement
8. Infrastructure Placement
9. Construction Sounds

**Low (Play If Quiet)**
10. Ambient Variations
11. Environmental Details

---

## 7. Accessibility Considerations

### 7.1 Visual Alternatives

For players with hearing impairment:
- All audio feedback has corresponding visual feedback
- UI confirms all actions visually (not audio-only)
- Option for enhanced visual effects when audio is low/off

### 7.2 Audio Sensitivity Options

**Reduced Audio Mode**
- Removes sudden/sharp sounds
- Gentler volume curves
- No low-frequency rumble effects

**Mono Audio**
- Combine stereo to mono for single-ear hearing
- Maintain all information in mono mix

**Volume Per Category**
- Granular control over each audio channel
- Can mute categories individually
- Presets: "Music Only", "SFX Only", "All", "Minimal"

### 7.3 Subtitle/Captions

For important audio:
- "Disaster Warning" text accompanies sound
- Milestone name displayed
- Not needed for routine SFX

---

## 8. Multiplayer Audio

### 8.1 Whose Actions Do You Hear?

**Own Actions: Full Audio**
- All your placement sounds
- Your building completions
- Your disasters

**Other Players' Actions:**
- Disasters: Hear if in your view or affects your tiles
- Milestones: Subtle notification (optional)
- Construction: Not heard (too spammy)

### 8.2 Shared Experiences

**Disasters Crossing Boundaries**
- If disaster affects you, you hear it
- Distance attenuation based on your view
- Your hazard response sounds are local

**Population Events**
- Only hear your own milestone sounds
- Subtle "other player milestone" chime (optional)
- Chat message accompanies

### 8.3 Voice Chat Ducking (Future)

If voice chat is added:
- Music ducks when voice active
- SFX continues normally
- Ambient slightly reduced

---

## 9. Questions for Other Agents

### @systems-architect

1. **AudioSystem as ECS or Core?** systems.yaml says ecs_system, but audio doesn't really participate in simulation tick. What's the recommendation?

2. **Event-based triggering:** What interface do systems use to trigger sounds? EventBus subscription or direct IAudioProvider calls?

3. **SDL3 Audio Capabilities:** What audio formats does SDL3 support? What's recommended for SFX vs music?

4. **Positional audio extent:** How far should positional sounds travel? Is there a max range?

5. **Stream vs preload:** Should large music files stream, or preload everything?

### @graphics-engineer (Future)

6. **Audio-visual sync:** How do we ensure building completion sound matches animation end?

7. **Camera-based listener:** How does CameraSystem provide listener position?

---

## 10. Player Experience Goals

### 10.1 Emotional Journey

**First Play**
- Immediate understanding of action feedback
- "Oh, that sounds alien and interesting"
- Comfortable but novel

**Regular Play**
- Sounds fade into background during focus
- Still notice important events
- Music becomes personal (user tracks)

**Long Sessions**
- No audio fatigue
- Ambient soundscape remains pleasant
- Optional music cycling prevents repetition

### 10.2 Key Moments

**First Building Complete**
- Satisfying, memorable sound
- "I built something!"

**First Milestone**
- Celebratory fanfare
- "I achieved something big!"

**First Disaster**
- Interesting, not terrifying
- "Something dramatic is happening!"

**Quiet Moment Watching Colony**
- Ambient hum of activity
- Music creates mood
- "This is cozy"

### 10.3 Success Metrics

**Audio Contributes to Experience When:**
- Players notice when music is off (it adds to vibe)
- Action feedback feels satisfying
- Important events feel appropriately weighted
- Extended play doesn't cause audio fatigue
- Alien theme is reinforced through sound

---

## Summary

Epic 15 audio design should:

1. **Sound Alien:** Crystalline, organic-electronic, bioluminescent aesthetic
2. **Feel Cozy:** Relaxing ambient soundscape, non-stressful feedback
3. **Provide Feedback:** Clear audio confirmation for all actions
4. **Support User Music:** Playlist with crossfade, user folder
5. **Scale with Population:** Colony hum grows with colony
6. **Respect Player Attention:** Important sounds emphasized, routine sounds subtle

The audio system reinforces ZergCity's identity as a cozy, social alien colony builder.

---

**End of Game Designer Analysis: Epic 15 - Audio System**
