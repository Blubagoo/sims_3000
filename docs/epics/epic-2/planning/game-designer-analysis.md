# Game Designer Analysis: Epic 2 (3D Rendering)

**Epic:** Core Rendering Engine (3D with Toon Shading)
**Agent:** Game Designer
**Date:** 2026-01-27
**Canon Version:** 0.4.0

---

## Context: The 3D Pivot

This analysis replaces the previous 2D sprite-based approach. ZergCity now uses **3D rendering with a fixed isometric-style camera** and **toon/cell-shading**. This is a fundamental shift in how the game looks and feels, bringing us closer to modern city builders like Cities Skylines and Tropico while maintaining our distinctive alien aesthetic.

The key insight: 3D is not about realism. It is about using 3D geometry with stylized shading to create a clean, readable, and visually distinctive alien colony.

---

## Experience Requirements

### 3D Visual Feel with Toon Shading

The shift to 3D opens entirely new possibilities for visual expression. Toon shading lets us have 3D depth and form while keeping the stylized, illustrative quality that defines our alien aesthetic.

- [ ] **REQ-01: 3D Clarity, Not Realism** - The toon shader must make structures read clearly from the isometric angle. Silhouettes matter. Buildings should pop against terrain and each other through clean edge work and controlled color palettes. We are not simulating reality; we are creating an illustrated alien world in three dimensions.

- [ ] **REQ-02: Depth Becomes Tangible** - In 3D, depth is real. Tall towers actually loom over smaller structures. Valleys have actual depth. This physical presence should make the colony feel more substantial than any 2D approximation. Players should feel like they are looking down into a miniature world, not at a flat image.

- [ ] **REQ-03: Living World Through Movement** - 3D enables subtle ambient animation at low cost: energy conduits with flowing particles, habitation zones with beings walking paths, fabrication zones with moving machinery. These should feel organic, not mechanical. The colony breathes.

- [ ] **REQ-04: Toon Shading as Brand Identity** - The cell-shaded look IS ZergCity's visual brand. Every 3D model, every shader pass, every lighting decision must reinforce this aesthetic. When a player sees a screenshot, they should immediately recognize it as ZergCity. Not realistic. Not pixel art. Not flat colors. A distinctive toon-shaded 3D style.

- [ ] **REQ-05: Scale Communicates Hierarchy** - High-density structures should tower over low-density. Energy nexuses should dominate their district. The Command Nexus should be visible from anywhere on the map. 3D scale creates natural visual hierarchy that 2D sprites could only approximate.

### Fixed Isometric Camera Experience

The camera is our player's eye. With a fixed isometric-style angle (no rotation initially), we must ensure that this single viewpoint delivers a complete, satisfying experience.

- [ ] **REQ-06: The Isometric Sweet Spot** - The ~35 degree pitch and 45 degree rotation creates the classic isometric feel while being true 3D. This angle must show building fronts, roofs, and ground clearly. The player should feel like they are observing from a comfortable vantage point - close enough to care, far enough to command.

- [ ] **REQ-07: Pan Feels Like Flying** - Camera pan should feel like smoothly gliding over the colony, not jerking a technical viewport. WASD and edge-scroll should have subtle momentum (ease-out on stop). The camera is an extension of the player's attention - it should move the way attention moves.

- [ ] **REQ-08: Zoom Reveals Worlds** - Zooming in 3D is fundamentally different from 2D. As the camera approaches, models gain detail. At far zoom, see the colony as infrastructure network. At close zoom, see individual beings, structural details, the texture of alien technology. Each zoom level should feel like a discovery.

- [ ] **REQ-09: Zoom Centers on Intent** - Mouse wheel zoom MUST center on cursor position. This is non-negotiable in 3D as in 2D. The player is saying "I want to look at THIS closer" - the camera must honor that intent.

- [ ] **REQ-10: No Rotation Is a Feature** - Initially, no camera rotation. This constraint creates simplicity: players never need to reorient themselves. Building designs can assume a single viewing angle. Art direction is simpler. If rotation is added later, it is an expansion, not a fix. The fixed view must be complete in itself.

- [ ] **REQ-11: Boundaries Feel Like World Edges** - When hitting map edges, the camera should decelerate gently, not hard-stop. This creates the feeling that the colony exists at the edge of explored territory, not that an invisible wall exists.

### 3D Depth and Scale

True 3D depth changes everything about how players read the world.

- [ ] **REQ-12: Elevation Is Visible Height** - The 32 elevation levels are now real 3D height. Cliffs have geometry. Slopes are actual ramps. A being walking uphill visibly climbs. This is not a painted illusion - it is genuine verticality.

- [ ] **REQ-13: Occlusion Is Automatic (And Correct)** - Unlike 2D where depth sorting is complex, 3D with depth buffer Just Works. Buildings in front occlude buildings behind. The GPU handles this. But we must ensure the toon shader does not break depth testing. Perfect occlusion is table stakes.

- [ ] **REQ-14: Shadows Ground Objects** - Even with toon shading, subtle shadows help "ground" structures on the terrain. A building without shadow feels like it is floating. Shadow style should match the toon aesthetic - clean, maybe slightly exaggerated, not photorealistic.

- [ ] **REQ-15: Tall Structures Create Presence** - High-rise towers should cast visual weight on their surroundings. In 3D this can be literal (shadows) or suggested through model design (broader bases, imposing silhouettes). A tower district should feel dense and imposing.

- [ ] **REQ-16: Underground Is Below** - When switching to underground view, we are now looking at actual underground geometry. Pipes and subterra-rail are spatially below the surface. The transition should feel like X-ray vision or peeling back a layer - the surface becomes translucent while the underworld becomes visible.

### Toon Shader Aesthetic Guidelines

The toon shader is the artistic heart of the 3D approach. It transforms standard 3D into our distinctive look.

- [ ] **REQ-17: Two-Tone Minimum** - Every surface should have clear lit/shadow distinction. Toon shading typically uses discrete color bands rather than smooth gradients. Two bands minimum (lit + shadow), three for more complex surfaces (lit + mid + shadow).

- [ ] **REQ-18: Edge Lines Define Form** - Outline rendering (silhouette edges and/or crease edges) gives toon-shaded models their illustration quality. Edge weight can vary: thicker on silhouettes, thinner on internal edges. The outline IS the drawing.

- [ ] **REQ-19: Single Light Source** - All objects should be lit from a consistent direction (the alien sun). This creates unified shadows and readable forms. Light direction should come from "above and behind camera" to illuminate building fronts.

- [ ] **REQ-20: Shadow Color Has Character** - Shadows are not just "darker." They shift toward purple or teal, reinforcing the alien palette. This color shift in shadows is a key part of the visual identity.

- [ ] **REQ-21: No Specular (Or Minimal)** - Toon shading typically avoids specular highlights or uses very controlled specular bands for metal/glass. Specular makes things look wet or plastic. We want structures to feel solid and alien, not shiny.

- [ ] **REQ-22: Rim Lighting Optional But Powerful** - A rim light (backlighting) can separate objects from backgrounds and add drama. If used, it should be subtle and consistent - a thin line of brightness at object edges.

- [ ] **REQ-23: Fresnel for Depth** - Fresnel effects (edges brighter than faces) can add depth perception. Subtle fresnel works well with toon shading to make objects feel more three-dimensional.

### Alien Theme Color Palette (3D Updated)

The alien color palette now applies to materials and lighting, not just sprite colors.

- [ ] **REQ-24: Bioluminescent Teal (#00D4AA)** - The signature ZergCity color. Used for energy conduits (emissive material), powered state indicators, UI highlights. In 3D, this can be actual emissive glow on models.

- [ ] **REQ-25: Deep Purple (#2A1B3D)** - Shadow color. Ambient occlusion should trend toward purple. Night cycle pushes toward purple. This creates the alien twilight atmosphere.

- [ ] **REQ-26: Amber Alert (#FFB627)** - Hazard/attention color. Contamination zones, alerts, warnings. Warm contrast against cool palette.

- [ ] **REQ-27: Terrain: Alien Earth** - Not green grass. Not brown dirt. Blue-gray terrain, purple-tinted rocks, crystalline formations. The toon shader should make these colors pop with clean color bands.

- [ ] **REQ-28: Fluid: Not Blue Water** - Mercury silver, iridescent green, or bioluminescent pools. In 3D this can be actual reflective/animated material. One choice that screams "alien planet."

- [ ] **REQ-29: Zone Color Families** - Habitation (teal-green variants), Exchange (gold-amber variants), Fabrication (purple-gray variants). These should be visible in roof colors, structural accents, or ground plane tinting.

### Zoom Experience in 3D

3D zoom is not scaling; it is camera distance. This fundamentally changes the feel.

- [ ] **REQ-30: Zoom Range: Strategic to Detail** - Far zoom: 0.25x (camera far away) sees broad colony structure. Close zoom: 2.0x (camera close) sees individual beings, structural detail. This range should feel expansive.

- [ ] **REQ-31: LOD Transitions Must Be Invisible** - If Level of Detail is used (simpler models at distance), transitions must be imperceptible. Pop-in is immersion-breaking. Either smooth crossfade or aggressive LOD distances.

- [ ] **REQ-32: Far Zoom: Infrastructure Network** - At maximum zoom out, see pathways as veins, districts as organs, the whole colony as an organism. Individual buildings become shapes; their arrangement is what matters.

- [ ] **REQ-33: Close Zoom: Intimate Details** - At maximum zoom in, see window patterns, structural ornamentation, beings walking. This is where "being there" happens. Model detail must reward close inspection.

- [ ] **REQ-34: Zoom Speed Feels Natural** - Zoom should accelerate at extremes. Near max zoom, small scroll = big perceived change. Near min zoom, small scroll = small perceived change. Perceptual consistency.

- [ ] **REQ-35: Atmosphere at Distance** - Optional: distant objects can have subtle atmospheric haze (depth fog). This adds depth cues and reinforces the "alien sky" atmosphere. Not required but impactful.

### Visual Feedback and "Juice" in 3D

3D enables new forms of feedback and polish.

- [ ] **REQ-36: Build Placement Preview** - Ghost-transparent version of the 3D model at cursor position. Pulsing/animated to show "about to be placed." Position snapping should feel magnetic.

- [ ] **REQ-37: Placement Confirmation** - When structure is placed, brief scale animation (pop up, settle down). Particle burst from ground. Sound synced to visual. The moment of creation should feel powerful.

- [ ] **REQ-38: Materializing Construction** - Structures under construction should "materialize" from bottom to top. In 3D this can be a dissolve effect, an energy field growing upward, or a particle swarm assembling the building. Much more dramatic than 2D scaffolding.

- [ ] **REQ-39: Demolition Is Destruction** - Deconstructing in 3D can have actual collapse physics (or faked collapse animation). Dust particles, debris, the structure crumbling. Make destruction feel consequential.

- [ ] **REQ-40: Power State Glow** - Powered buildings can have subtle emissive glow on energy-related elements. Unpowered buildings: dim, desaturated, dormant. The energy matrix should be VISIBLE across the colony.

- [ ] **REQ-41: Activity Animation** - Fabrication zones with moving parts, smoke, particles. Exchange districts with being crowds. Habitation with window lights cycling. Each zone type has characteristic ambient motion.

- [ ] **REQ-42: Selection in 3D** - Selected structure: outline glow (post-process or geometry-based), ground indicator ring, subtle scale increase. Selection should feel like "spotlighting" in theatrical sense.

- [ ] **REQ-43: Hover Feedback** - Before clicking, hoverable structures should respond: subtle brighten, edge highlight at reduced intensity. Shows "this is interactive."

### Multiplayer Visual Requirements

- [ ] **REQ-44: Other Overseer Cursors** - Render other players' cursors as 3D objects or billboarded sprites at world position. Faction-colored. Shows presence and activity.

- [ ] **REQ-45: Faction Color Integration** - Each overseer's structures should have subtle faction color: building base plates, flag/banner elements, tile borders. Visible but not overwhelming.

- [ ] **REQ-46: Real-Time Build Visibility** - Other overseers' construction should be visible as it happens. Their materializing buildings appear in your view (after server confirmation).

- [ ] **REQ-47: Territory Visualization** - Boundary between overseer domains: subtle ground tint, border glow, or fence geometry. Clear ownership without ugly hard lines.

---

## Questions for Other Agents

### For Systems Architect

- @systems-architect: For the toon shader, should shader parameters (number of color bands, edge line width, shadow color) be configurable at runtime or baked into the shader? What is the interface between shader configuration and game systems?

- @systems-architect: How does underground view work in 3D? Is it camera position change (move camera down), transparency on surface objects, or culling/layer system? What is the state machine?

- @systems-architect: For model LOD (Level of Detail), is this managed per-entity via a LODComponent, or is it a global RenderingSystem policy based on camera distance? Who decides which LOD to display?

- @systems-architect: 3D model animation (ambient motion, construction effects) - does this live in AnimationSystem or is it shader-driven? What is the boundary between skeletal/transform animation and visual effects?

- @systems-architect: For the fixed isometric camera, should camera parameters (pitch angle, rotation) be truly constant or configurable-but-locked? Thinking about future rotation feature.

### For Graphics Engineer

- @graphics-engineer: What toon shader technique will we use? Sobel edge detection + stepped lighting? Inverted hull for outlines? What does SDL_GPU support for custom fragment shaders?

- @graphics-engineer: For model format (glTF), how do we specify toon shader materials? Custom material extensions? Texture-based ramps? UV2 for emission maps?

- @graphics-engineer: What is the expected performance budget for toon shader? How many draw calls can we afford with outline rendering (often requires extra passes)?

- @graphics-engineer: How do we handle transparency in 3D with toon shading? Construction ghost previews, underground view, selection overlays - these all need alpha and can break depth sorting.

- @graphics-engineer: For emissive materials (energy conduits, powered indicators), does SDL_GPU support bloom/glow post-processing? Or do we fake it with additive geometry?

---

## Risks & Concerns

### Toon Shader Risks

1. **Shader complexity vs. performance** - Good toon shading often requires multiple render passes (geometry for outline, main pass for shading, possibly post-process for effects). This can multiply draw call cost. Must profile early.

2. **Outline consistency at distance** - Screen-space outline techniques can become too thick or disappear entirely at different zoom levels. Need LOD-aware outline scaling or geometry-based outlines.

3. **Shadow color banding** - Stepped/banded shadows are core to toon look but can create ugly artifacts at certain angles. Need careful light direction and band threshold tuning.

4. **Toon shader + transparency** - Alpha-blended objects (construction preview, selection effects) often break with outline shaders. Need fallback rendering path or creative solutions.

### Camera Experience Risks

5. **Fixed angle limitations** - Some building arrangements may be unreadable from the fixed isometric angle. Buildings that look great from one side may hide important detail. Art direction must account for this constraint.

6. **Zoom extremes feel different** - Far zoom feels like strategy game. Close zoom feels like diorama. These are very different player mindsets. Need smooth psychological transition, not jarring mode change.

7. **No rotation can frustrate** - Players accustomed to city builders with rotation may feel constrained. Must ensure the fixed view is genuinely complete - no "hidden" building faces that matter.

### 3D Model Risks

8. **Model complexity creep** - It is easy to add detail to 3D models that will never be visible at typical zoom. Poly budget discipline required. Toon shading can hide low-poly with good design.

9. **Silhouette readability** - Building types must be distinguishable by silhouette alone, since toon shading flattens detail. Habitation vs. Exchange vs. Fabrication must read from outline.

10. **Animation cost** - 3D animation (even simple ambient motion) has CPU/GPU cost. With many animated buildings, this accumulates. Need culling strategy for off-screen animation.

### Multiplayer Visual Risks

11. **Synchronization artifacts** - Other players' building placement must appear at correct time. Network delay + visual delay = potential for jarring "teleport" of constructions.

12. **Faction color clashes** - In 3D with toon shading, faction colors interact with lighting. Some colors may become unreadable in shadow or at night. Need careful palette design.

---

## Visual Style Guidelines (3D Updated)

### The ZergCity 3D Look

**One-line vision:** "A bioluminescent alien colony rendered in clean 3D with confident toon shading - simultaneously futuristic, organic, and unmistakably stylized."

### Toon Shading Rules

| Element | Treatment |
|---------|-----------|
| Base surfaces | 2-3 color bands (lit, shadow, optional mid) |
| Edges/Outlines | Visible on silhouettes, subtle on internal creases |
| Shadow color | Shift toward purple/teal, not just darker |
| Specular | None or minimal (metal/glass only) |
| Emission | Teal glow for powered/active elements |
| Light direction | From above-behind camera, illuminating building fronts |

### Model Design Principles

1. **Silhouette first** - Design recognizable outlines before adding detail
2. **Clean UV** - Large color regions rather than busy textures
3. **Geometry for detail** - Use actual geometry for important edges (outline shader will catch them)
4. **Vertical emphasis** - Taller feels more important; exaggerate height where appropriate
5. **Base and cap** - Buildings should have clear base (grounded) and cap (top/roof) for readability

### Alien Theme in 3D

| Element | 3D Expression |
|---------|---------------|
| Terrain | Blue-gray/purple geometry with crystalline rock formations |
| Fluids | Animated mercury/iridescent shader, NOT blue water |
| Energy | Emissive teal materials, particle effects along conduits |
| Habitation | Organic curves, translucent panels, warm window glow |
| Fabrication | Angular geometry, visible machinery, exhaust particles |
| Exchange | Modular/busy silhouettes, gold accents, crowd activity |

### Animation Philosophy

1. **Ambient motion is background** - Never distracting, always present
2. **Construction is event** - Materializing should feel like watching something being born
3. **State changes are transitions** - Powered/unpowered, active/inactive get smooth transitions
4. **Beings move with purpose** - Not random wandering; following implied pathways

---

## Mood Board Concepts (3D)

### Terrain
- **Not this:** Flat green planes with texture, blue water polygons
- **This:** Sculpted alien landscape with visible elevation geometry, crystalline formations catching light, mercury pools with animated surface

### Structures
- **Not this:** Realistic skyscrapers, brick textures, photorealistic materials
- **This:** Toon-shaded buildings with clean silhouettes, organic curves meeting hard-tech edges, visible energy systems glowing teal, structures that feel grown as much as built

### Beings
- **Not this:** Realistic humanoid characters, complex skeletal animation
- **This:** Stylized alien silhouettes with smooth animation, purposeful movement on pathways, scale emphasizing they are inhabitants of YOUR colony

### Energy Infrastructure
- **Not this:** Gray utility poles, underground pipes invisible
- **This:** Visible energy conduits with flowing teal particles, crystalline energy nexuses with pulsing light, the power grid as a VISIBLE network of light

### Day/Night
- **Not this:** Realistic day/night cycle with standard Earth colors
- **This:** Alien day with slightly off-color sunlight, twilight phases with purple sky, night mode where the colony becomes a constellation of teal glowing points

---

## Summary

Epic 2 in 3D is a different challenge than 2D - and a much more powerful opportunity. True 3D depth, toon shading as artistic statement, and modern rendering capabilities let us create something visually distinctive.

**Key Experience Priorities (3D):**

1. **Toon shader is identity** - Get the shader right and everything built on it inherits quality
2. **Camera must feel perfect** - Fixed angle means every pan/zoom is critical; no rotation to escape to
3. **Silhouette readability** - Buildings must be recognizable from outline alone
4. **Depth creates presence** - Use real 3D depth to make the colony feel substantial
5. **Movement creates life** - Ambient animation makes static geometry feel like a living world

The alien theme now has 3D expression: glowing energy systems, otherworldly terrain geometry, structures that feel simultaneously organic and technological. This is not SimCity in 3D. This is ZergCity - an alien colony that looks like nothing else in the genre.

The toon shader is our artistic commitment. It says: we are not trying to look realistic. We are creating an illustrated world that happens to have 3D depth. This clarity of vision must inform every technical and artistic decision in this epic.
