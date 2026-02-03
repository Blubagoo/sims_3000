# Game Designer Answers: Epic 2 Discussion

**Date:** 2026-01-27
**Agent:** Game Designer
**Focus:** Player experience, alien theme consistency, and fun

---

## Answers to Systems Architect Questions

### SA-Q6: Building Idle Animations

**Decision:** Yes, buildings should have idle animations, but with clear purpose and restraint.

**Recommended idle animations:**
- **Energy nexuses (power plants):** Pulsing glow and occasional energy discharge effects. This communicates "active and producing" at a glance.
- **Energy conduits:** Subtle traveling light pulses along the conduit path. Players should be able to "read" power flow direction.
- **Fabrication zones (industrial):** Rhythmic mechanical motion, venting steam/exhaust. Shows productivity.
- **Habitation zones:** Occasional window illumination changes, subtle movement indicators. Suggests life inside without being distracting.

**What to avoid:**
- Constant frenetic motion (causes visual fatigue)
- Animations that draw attention away from problems (a building on fire should be more noticeable than a happy building)
- Beings walking inside visible through windows (too much detail, performance concern, save for high zoom)

**Experience rationale:** Idle animations create the "living colony" feeling. When a player pauses to admire their work, they should see a world that breathes. But when they're problem-solving (grid collapse, flow blockage), static buildings shouldn't distract from issues that need attention.

---

### SA-Q7: Isometric Camera Angle

**Decision:** Use the **SimCity 2000 angle (2:1 diamond, approximately 26.57 degrees)** for initial implementation.

**Rationale:**
1. **Nostalgia value:** Our inspiration is SimCity 2000. That camera angle IS the genre for many players.
2. **Readability:** The 2:1 ratio creates clean pixel alignment and makes tile boundaries obvious.
3. **Building silhouettes:** This angle shows two visible faces of buildings (front and side), which is ideal for recognizing structure types.
4. **Alien differentiation:** We differentiate through art style (toon shading, alien architecture), not camera angle. The familiar angle helps players feel grounded while the content feels alien.

**Note:** If we add camera rotation later, we're rotating around this angle - not changing the pitch. The isometric pitch stays constant.

---

### SA-Q8: Camera Rotation Conditions

**Decision:** Camera rotation should be added when **players experience occlusion frustration** - specifically when tall structures block view of areas they're actively trying to work on.

**Trigger conditions for adding rotation:**
1. When we implement multi-story/tall towers that can block significant tile areas
2. When players are building in areas surrounded by existing tall structures
3. When underground infrastructure becomes a gameplay focus (rotation helps with layer transitions)

**Modeling implication:** **Model all 4 sides of buildings from the start.** Here's why:
- Art pipeline should assume rotation is coming
- Cheaper to do it right initially than retrofit hundreds of models
- Even without rotation, players may see other angles in tooltips, info panels, or promotional materials
- 4-sided modeling is standard practice anyway

**Player experience:** Rotation should be 90-degree snaps with a quick, smooth animation (200-300ms). Free rotation causes nausea and loses the clean isometric grid feel.

---

### SA-Q9: Day/Night Effect on Toon Shader

**Decision:** **Palette shift approach, NOT lighting direction change.**

**How it should work:**
1. **Light direction stays constant** - shadows always fall the same way. Changing shadow direction would be disorienting and make the colony look different at different times.
2. **Color temperature shifts** - warm golden tones at "day peak," cool blue-purple at night
3. **Ambient color changes** - the "fill" color adjusts to simulate indirect lighting changes
4. **Emission becomes visible** - at night, building windows, energy conduits, and powered indicators should glow visibly. This creates beautiful nighttime colony vistas.

**Experience rationale:**
- Players learn building silhouettes by their shadow patterns. Changing light direction breaks this.
- The toon shader bands should remain stable - just the colors shift.
- Night should feel magical, not just darker. Glowing alien colonies at night = shareable screenshots, pride in creation.

**Implementation detail:** Consider 4-6 time-of-day presets rather than continuous interpolation. Distinct "dawn, day, dusk, night" feels more intentional.

---

### SA-Q10: Zoom Level Detail Requirements

**Decision:**

**At maximum zoom IN:**
- Individual structure details visible (window patterns, antenna, signage)
- Animated elements on buildings clearly readable
- **NO individual being faces** - beings should be stylized shapes/silhouettes, not detailed characters
- Tile selection and grid lines clearly visible
- Construction progress on materializing buildings

**At maximum zoom OUT:**
- Entire player domain visible (their owned sectors)
- Neighboring player colonies visible (creates sense of shared world)
- Zone types distinguishable by color/pattern
- Major infrastructure (transit corridors, energy matrix) readable
- Individual buildings become simplified shapes/icons
- Target: at least 128x128 tiles viewable, ideally 256x256 if multiple colonies share a map

**Zoom behavior:**
- Smooth continuous zoom (mousewheel)
- UI zoom presets for quick navigation ("my colony" / "region" / "tile detail")
- Current zoom level should NOT affect simulation - purely visual

**Thematic note:** Beings are never the focus. This is about colony management, not individual aliens. Even zoomed in, beings are ambient detail, like cars in a city builder. Stylized shapes moving along pathways, not characters with faces and expressions.

---

## Answers to Graphics Engineer Questions

### GE-Q6: Orthographic vs Perspective (Experience Perspective)

**Decision:** **Orthographic projection.**

**Rationale:**
1. **Grid clarity:** Orthographic means tiles at the back are the same size as tiles at the front. This is essential for accurate placement and spatial reasoning.
2. **Genre expectation:** Classic isometric city builders (SimCity 2000, Pharaoh, Caesar III) all used orthographic. Players expect this.
3. **Clean aesthetics:** No barrel distortion, no vanishing points. The toon shader + orthographic = very clean, almost graphic-novel look.
4. **Multiplayer fairness:** Perspective could make some players' views feel "closer" to action. Orthographic is neutral.

**Counterargument addressed:** Yes, perspective adds depth perception. But our depth comes from:
- Z-sorting (tiles behind rendered first)
- Shadows
- Height variation in terrain/buildings
- Atmospheric effects (slight fog/haze on distant tiles)

We don't need perspective for depth. We need orthographic for clarity.

---

### GE-Q7: Camera Rotation in Initial Release

**Decision:** **Defer camera rotation. Not in initial release.**

**Rationale:**
1. **Reduces Epic 2 scope** - rotation requires handling occlusion, UI repositioning, and player orientation tracking
2. **Early buildings aren't tall enough** - occlusion won't be a problem until we have multi-story towers
3. **Players can plan around it** - experienced players know to leave sightlines
4. **We can add it right** - rather than rush a janky rotation, we wait and do smooth 90-degree snaps properly

**What we DO need in Epic 2:**
- Camera system that CAN support rotation later (don't paint into a corner)
- Buildings modeled from all 4 sides (as discussed in SA-Q8)
- UI that doesn't assume fixed camera direction

**Timeline:** Revisit rotation in Phase 2 when tall structures become gameplay-relevant.

---

### GE-Q8: Day/Night Cycle Approach

**Decision:** **Palette shift approach, with emissive materials activated at night.**

**Detailed breakdown:**
1. **Base lighting direction: Static.** The sun doesn't move. This is alien - maybe their world has a different rotation pattern, or their species prefers consistent lighting.
2. **Color palette: Time-based interpolation.**
   - Dawn: Warm pink/orange ambient
   - Day: Neutral with warm key light
   - Dusk: Deep orange/red ambient
   - Night: Blue-purple ambient, cool shadows
3. **Emissive materials: Intensity scales with darkness.**
   - Energy conduits glow
   - Powered buildings show lit windows
   - Hazard response posts have pulsing beacons
   - Creates gorgeous night cityscapes

**Player benefit:** Night becomes a reward, not a hindrance. Players will want to stay up and watch their colony glow. This is an emotional hook - pride in creation.

---

### GE-Q9: Building Construction Animation Style

**Decision:** **Keep the "materializing" ghost effect, enhanced for 3D.**

**How it should look in 3D:**
1. **Phase 1 - Designation:** Holographic wireframe/outline appears (semi-transparent blue/green tint)
2. **Phase 2 - Materialization:** Building fades in from bottom to top, with particle/energy effects along the "construction line"
3. **Phase 3 - Completion:** Flash of light, particles disperse, building is fully solid and begins idle animation

**Why NOT "rise from ground" or "assembly" animation:**
- We're aliens. We don't use human construction methods.
- "Materializing" fits the alien theme - advanced technology, teleportation-like building
- Simpler to implement (fade + shader effect) vs. rigging animated assembly
- Construction progress is readable at a glance (how far up is it materialized?)

**Experience note:** The materialization effect should feel magical and advanced. When a player zones a new district and watches structures materialize across it, they should feel powerful - an overseer commanding a colony into existence.

---

### GE-Q10: Real-Time Shadows Worth the Cost

**Decision:** **Yes, real-time shadows are worth it, with performance tiers.**

**Why shadows matter:**
1. **Depth perception:** In orthographic view, shadows are our primary depth cue
2. **Time of day:** Even with fixed light direction, shadow intensity creates atmosphere
3. **Spatial relationships:** Shadows show building heights, help players understand terrain
4. **Visual polish:** Shadows make everything look grounded and real

**Performance approach:**
1. **Shadow quality tiers:**
   - High: Full real-time shadows, soft edges
   - Medium: Real-time shadows, hard edges
   - Low: Baked/fake shadows (flat circles under buildings)
   - Potato: No shadows

2. **Shadow distance:** Shadows only calculate for visible area + small buffer
3. **Shadow cascades:** Nearby = detailed, far = chunky
4. **Building shadow LOD:** Simplified shadow geometry for distant/small buildings

**Experience guarantee:** Default should be Medium shadows. Even Low (fake shadows) provides spatial grounding. Only completely disable for truly struggling hardware.

**Cost justification:** For a 3D game with toon shading, shadows are not optional - they're fundamental to visual clarity. Budget for them from the start.

---

## Summary of Key Decisions

| Topic | Decision | Rationale |
|-------|----------|-----------|
| Building idle animations | Yes, purposeful | Living colony feel |
| Camera angle | 26.57 degrees (SimCity 2000) | Nostalgia + readability |
| Camera rotation | Defer, but model 4 sides | Reduce scope, prepare for future |
| Day/night shaders | Palette shift, not light direction | Consistency + night beauty |
| Max zoom in | Structure detail, no being faces | Colony focus, not characters |
| Max zoom out | 128-256 tiles | Domain + neighbor visibility |
| Projection | Orthographic | Grid clarity, genre fit |
| Construction animation | Materializing ghost | Alien theme, readable progress |
| Real-time shadows | Yes, with quality tiers | Depth perception, essential |

---

*These decisions prioritize player experience and alien theme consistency while being mindful of performance. Technical implementation details should be worked out with Systems Architect and Graphics Engineer.*
