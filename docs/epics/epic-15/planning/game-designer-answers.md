# Game Designer Answers - Epic 15 Audio System

**Date:** 2026-02-01
**Discussion:** epic-15-planning.discussion.yaml
**Answering:** Systems Architect questions SA-1 through SA-8

---

## SA-1: Sound Variants Per Action

**Question:** Should there be multiple sound variants per action (e.g., 3-5 building complete sounds that play randomly)? This adds variety but increases asset requirements.

**Answer:** Yes, implement sound variants with a tiered approach based on frequency.

For **high-frequency sounds** (structure materialization, zone designation, pathway placement), use 3-4 variants. Players hear these constantly throughout a session, and repetition becomes grating quickly. The cozy sandbox vibe depends on these common actions feeling pleasant rather than monotonous.

For **medium-frequency sounds** (achievement unlocks, disaster warnings, milestone notifications), 2-3 variants suffice. These occur less often, so repetition is less noticeable.

For **low-frequency sounds** (transcendence achievement, arcology completion), a single distinctive sound is fine and even preferable - these should feel special and recognizable.

This tiered approach keeps asset requirements reasonable while eliminating the "same sound over and over" fatigue that breaks immersion in city builders.

---

## SA-2: Disaster Sound Scaling

**Question:** Should disaster sounds scale with severity? E.g., conflagration intensity affects sound intensity?

**Answer:** Yes, absolutely. Disaster sound intensity should scale with severity, but with careful attention to the player experience curve.

**Conflagration:** Start with a subtle crackling at low intensity (1-2 tiles), building to an ominous roar as it spreads. This creates a natural tension curve - the audio communicates danger level without requiring players to constantly check the map. The sound design should feel alien: think crystalline cracking mixed with organic burning rather than standard Earth fire sounds.

**Seismic events:** Low rumble that intensifies. The audio should precede visual impact slightly, giving players that "something's wrong" moment before tiles start taking damage.

**Inundation:** Rising water sounds that become more urgent as flooding spreads.

The key principle: disasters should feel dramatic but not punishing to listen to. Even at maximum severity, the audio should add tension without being unpleasant. We want players to feel engaged, not reach for the mute button during emergencies.

---

## SA-3: Milestone Achievement Sound Length

**Question:** How long should milestone achievement sounds be? Short confirmation (1-2 sec) or celebratory fanfare (5-10 sec)?

**Answer:** Use a **hybrid approach** based on milestone tier, not a single length for all.

**Minor milestones** (Colony Emergence at 2K, Colony Establishment at 10K): Short confirmation, 1.5-2 seconds. A satisfying chime or crystalline tone that acknowledges progress without interrupting flow. Players are still actively building; don't pull them out of the zone.

**Major milestones** (Colony Wonder at 90K, Colony Ascension at 120K): Medium fanfare, 3-4 seconds. These are significant accomplishments that deserve celebration. Include a distinct musical phrase that players will associate with reaching these thresholds.

**Transcendence** (150K): Full celebratory sequence, 6-8 seconds. This is the ultimate achievement in the game - the audio should feel momentous and memorable. Consider a building crescendo that reflects the alien aesthetic, perhaps with harmonic overtones that sound otherworldly.

The principle: match audio investment to player emotional investment. Small wins get quick acknowledgment; big wins get proper celebration.

---

## SA-4: Other Player Milestone Audio

**Question:** When another player achieves a milestone, should there be audio feedback? If so, what level - same as own, reduced, or subtle chime?

**Answer:** Yes, provide audio feedback for rival milestones, but **significantly reduced** - a subtle notification rather than celebration.

When a rival achieves a milestone, play a short (0.5-1 second) **distinct chime** that's clearly different from your own achievement sounds. This creates competitive awareness ("they're growing faster than me") without the full dopamine hit reserved for your own achievements.

**Design considerations:**
- The sound should be identifiable as "rival activity" without needing to check the UI
- Use a slightly lower-key tone than your own achievements - you shouldn't feel like celebrating their success
- Consider a spatial or panning element if the rival colony's direction is known (subtle audio positioning)
- Players should be able to disable this in settings if they find it distracting during focused building sessions

This supports the competitive multiplayer experience by maintaining awareness of rival progress while keeping the primary emotional focus on your own colony's achievements.

---

## SA-5: Ambient Audio During Pause

**Question:** Should ambient audio continue during pause, or fade out?

**Answer:** Ambient audio should **fade to approximately 30-40% volume** during pause, not cut completely.

Complete silence during pause feels jarring and sterile. The cozy sandbox vibe benefits from maintaining atmosphere even when the simulation is stopped. However, full-volume ambience during pause fights for attention when players are trying to think or plan.

**Recommended behavior:**
- On pause: Fade ambient audio to 30-40% over 0.5 seconds
- Music: Continue at reduced volume (50%) or transition to a calmer "planning" track
- UI sounds: Play at full volume (players are still interacting)
- On unpause: Fade back to full ambient volume over 0.3 seconds

This creates a subtle "thinking mode" atmosphere where the colony still feels alive but less demanding of attention. Players can plan road layouts or zone placement without the audio pressure of active simulation, while still feeling connected to their colony.

---

## SA-6: Unique Sounds Per Disaster Type

**Question:** Should each disaster type have unique sounds (conflagration = crackling, seismic = rumbling, inundation = rushing water)?

**Answer:** Yes, each catastrophe type should have **distinct audio signatures**, and this is critical for gameplay clarity, not just polish.

**Conflagration:** Crackling with an alien twist - incorporate crystalline splintering sounds mixed with organic burning. The alien terrain (prisma fields, biolume groves) should sound like it burns differently than Earth vegetation.

**Seismic Event:** Deep rumbling with harmonic overtones. Consider adding a subtle "frequency shift" quality that feels otherworldly - not quite an earthquake, but a seismic event on an alien world.

**Inundation:** Rushing water with an emphasis on rising intensity. Include splashing and flowing sounds, but consider adding a slight echo quality that suggests alien atmospheric properties.

**Vortex Storm:** Howling wind with an electrical crackle element - these alien storms might carry charged particles.

Distinct audio serves a gameplay function: experienced players should be able to identify disaster type by sound alone, allowing them to react before fully processing the visual notification. Audio becomes an early warning system, which feels satisfying and rewards player familiarity.

---

## SA-7: Building Density Completion Sounds

**Question:** Should different building densities have different completion sounds?

**Answer:** Yes, building density should affect completion sounds, but **subtly** rather than dramatically.

**Low-density habitation/exchange/fabrication:** Lighter, softer materialization sounds. Quick crystalline chimes or gentle construction tones. These are smaller structures and shouldn't demand attention.

**High-density structures:** Fuller, more substantial completion sounds. Deeper tones with more layers - the audio should communicate "something significant just materialized."

**Towers (skyscrapers):** The most impressive completion sounds. Consider a brief ascending tone that suggests vertical scale, culminating in a satisfying resolution.

**Implementation note:** This can be achieved through sound layering rather than completely separate assets. Use a base "materialization" sound with density-specific layers on top. This keeps asset requirements reasonable while providing meaningful audio differentiation.

The key is that players should subconsciously register density differences through audio without these sounds becoming distracting. A small dwelling materializing should feel appropriately humble; a tower should feel like an accomplishment.

---

## SA-8: Music State Changes

**Question:** Should music change based on game state (e.g., disaster music during catastrophe)?

**Answer:** Yes, implement **dynamic music that responds to game state**, but with careful transitions to maintain the cozy sandbox atmosphere.

**Normal gameplay:** Ambient, relaxed soundtrack with alien aesthetic - crystalline tones, unusual harmonics, a sense of otherworldly calm. This is the baseline that players spend most of their time with.

**Active disaster:** Music shifts to heightened tension - faster tempo, minor keys, urgency. BUT this should layer over the existing track rather than hard-cutting. A jarring music change adds stress rather than engagement. Fade in tension elements over 2-3 seconds as disaster spreads.

**Post-disaster recovery:** Transition to a more melancholic variant that gradually returns to baseline as debris is cleared and structures are repaired. This creates a satisfying arc from crisis to recovery.

**Major milestone achievement:** Brief musical flourish that harmonizes with the current track, then returns to normal. Don't interrupt the flow for too long.

**Critical balance:** ZergCity is a cozy sandbox first. Even during disasters, the music should add drama without becoming stressful. Think "exciting challenge" not "anxiety-inducing emergency." Players should want to engage with disasters, not dread them.

Consider allowing players to disable dynamic music in settings for those who prefer consistent background audio.
