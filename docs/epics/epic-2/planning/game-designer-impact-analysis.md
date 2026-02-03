# Game Designer Impact Analysis: Epic 2

**Epic:** Core Rendering Engine (3D)
**Agent:** Game Designer
**Date:** 2026-01-28
**Trigger:** Canon update v0.4.0 -> v0.5.0 (includes v0.4.1)

---

## Trigger
Canon update v0.4.0 -> v0.5.0. Two major changes: (1) Camera changed from fixed isometric to free camera with isometric preset snap views. (2) Bioluminescent art direction formalized as core identity, configurable map sizes, expanded terrain (10 types), building template system, bloom for glow, emissive materials, dual UI modes.

## Overall Impact: HIGH

## Impact Summary
The camera model change from fixed isometric to free camera fundamentally alters the player's relationship with the 3D world -- rotation is now core functionality rather than explicitly prohibited, requiring new experience requirements for orbit, tilt, and snap-to-preset transitions. The bioluminescent art direction formalizing what was already present in the analysis amplifies and validates many existing requirements (especially around emissive materials and glow), while configurable map sizes add scaling concerns for camera speed and LOD that were only implicit before.

---

## Item-by-Item Assessment

### UNAFFECTED

- **REQ-01: 3D Clarity, Not Realism** -- The toon shader clarity mandate applies regardless of camera model or art direction specifics. Still valid as written.

- **REQ-02: Depth Becomes Tangible** -- True 3D depth is unchanged by camera or art direction changes. Still valid.

- **REQ-03: Living World Through Movement** -- Ambient animation requirements are independent of camera model. The bioluminescent art direction reinforces this (pulsing glow, spore clouds) but does not change the requirement itself.

- **REQ-05: Scale Communicates Hierarchy** -- Visual hierarchy through 3D scale is camera-independent. Still valid.

- **REQ-12: Elevation Is Visible Height** -- 32 elevation levels as real 3D height is unchanged. Still valid.

- **REQ-13: Occlusion Is Automatic (And Correct)** -- Depth buffer occlusion works regardless of camera model. Still valid.

- **REQ-15: Tall Structures Create Presence** -- Visual weight of tall structures applies in any camera mode. Still valid.

- **REQ-16: Underground Is Below** -- Underground view mechanics are camera-independent. Still valid.

- **REQ-17: Two-Tone Minimum** -- Toon shading band count is unaffected. Still valid.

- **REQ-21: No Specular (Or Minimal)** -- Specular policy unchanged. Still valid.

- **REQ-22: Rim Lighting Optional But Powerful** -- Rim lighting guidance unchanged. Still valid.

- **REQ-23: Fresnel for Depth** -- Fresnel effect guidance unchanged. Still valid.

- **REQ-26: Amber Alert (#FFB627)** -- Hazard/attention color unchanged. Canon adds amber to formal glow accent palette, reinforcing this.

- **REQ-29: Zone Color Families** -- Zone color families unchanged by these canon updates. Still valid.

- **REQ-34: Zoom Speed Feels Natural** -- Perceptual zoom consistency is camera-model-independent. Still valid.

- **REQ-36: Build Placement Preview** -- Ghost placement preview unchanged. Still valid.

- **REQ-37: Placement Confirmation** -- Placement animation/feedback unchanged. Still valid.

- **REQ-38: Materializing Construction** -- Construction animation unchanged. Building template system formalizes this but does not alter the experience requirement.

- **REQ-39: Demolition Is Destruction** -- Demolition feedback unchanged. Still valid.

- **REQ-41: Activity Animation** -- Zone activity animation unchanged. Still valid.

- **REQ-42: Selection in 3D** -- Selection feedback unchanged. Still valid.

- **REQ-43: Hover Feedback** -- Hover interaction unchanged. Still valid.

- **REQ-44: Other Overseer Cursors** -- Multiplayer cursor rendering unchanged. Still valid.

- **REQ-45: Faction Color Integration** -- Faction colors unchanged. Still valid.

- **REQ-46: Real-Time Build Visibility** -- Multiplayer build visibility unchanged. Still valid.

- **REQ-47: Territory Visualization** -- Territory boundary visualization unchanged. Still valid.

- **Risk 1: Shader complexity vs. performance** -- Unchanged. Still valid.

- **Risk 2: Outline consistency at distance** -- Unchanged. Still valid.

- **Risk 3: Shadow color banding** -- Unchanged, though now lighting must work from multiple angles (see MODIFIED items).

- **Risk 4: Toon shader + transparency** -- Unchanged. Still valid.

- **Risk 8: Model complexity creep** -- Unchanged. Still valid.

- **Risk 10: Animation cost** -- Unchanged. Still valid.

- **Risk 11: Synchronization artifacts** -- Unchanged. Still valid.

- **Risk 12: Faction color clashes** -- Unchanged. Still valid.

---

### MODIFIED

- **REQ-04: Toon Shading as Brand Identity**
  - Previous: "The cell-shaded look IS ZergCity's visual brand."
  - Revised: The brand identity is now more precisely defined: "Bioluminescent toon-shaded 3D" -- the toon shader combined with the dark-base-plus-glow-accents palette IS the brand. The toon shader alone is not sufficient; the bioluminescent color language is inseparable from it. Screenshots should read as "dark world with glowing structures" at a glance.
  - Reason: Canon v0.5.0 explicitly establishes bioluminescence as core art direction, not just an accent. The brand is toon shading PLUS bioluminescence.

- **REQ-06: The Isometric Sweet Spot**
  - Previous: "The ~35 degree pitch and 45 degree rotation creates the classic isometric feel" as the single camera mode.
  - Revised: The isometric sweet spot (~35.264 degree pitch, 45/135/225/315 degree yaw) now describes one of four preset snap views, plus the default starting mode. It is the "home base" camera feel -- the mode players start in and snap back to. The sweet spot requirement still applies but is now one mode among several rather than the only mode.
  - Reason: Canon v0.4.1 changed camera from fixed isometric to free camera with isometric preset snaps. The isometric angle is preserved as a preset, not eliminated.

- **REQ-07: Pan Feels Like Flying**
  - Previous: Referenced "WASD and edge-scroll" with smooth momentum in the context of a fixed-angle camera.
  - Revised: Pan still applies and should feel like flying, but now it must work well in BOTH free camera and isometric preset modes. In free camera mode, pan operates relative to the current view direction (not always world-axis-aligned). Pan momentum and ease-out must feel correct regardless of current camera pitch and yaw. Middle-mouse drag should remain intuitive even when the camera is tilted away from the default isometric angle.
  - Reason: Free camera means pan is no longer always along fixed world axes. Player expectation of smooth, directional movement must hold at arbitrary angles.

- **REQ-08: Zoom Reveals Worlds**
  - Previous: "Zooming in 3D is fundamentally different from 2D. As the camera approaches, models gain detail."
  - Revised: Same core requirement, but zoom behavior should adapt slightly to camera mode. In isometric preset mode, zoom adjusts orthographic width (feels like SimCity zoom). In free camera mode, zoom may use perspective distance or dolly (depending on projection mode). The "discovery at each zoom level" feel must be maintained in both modes. Additionally, with configurable map sizes (128/256/512), zoom range may need to scale -- a 512x512 map needs a wider max zoom-out to see the full colony as infrastructure network.
  - Reason: Free camera and configurable map sizes both affect zoom behavior and range requirements.

- **REQ-09: Zoom Centers on Intent**
  - Previous: "Mouse wheel zoom MUST center on cursor position. This is non-negotiable in 3D as in 2D."
  - Revised: Still non-negotiable. Cursor-centered zoom must work correctly in both isometric preset and free camera modes. In free camera mode, "centering on cursor" means the world point under the cursor stays visually stable, which requires different math when the camera is orbited to a non-standard angle.
  - Reason: Free camera rotation changes the unprojection math for cursor-to-world mapping during zoom.

- **REQ-11: Boundaries Feel Like World Edges**
  - Previous: "When hitting map edges, the camera should decelerate gently."
  - Revised: Same deceleration requirement, but now applies to all camera movement modes (pan, orbit, tilt). Orbit should not allow looking "off the edge" of the map into void. The world-edge feeling must hold when the player is orbiting freely -- the camera should gently resist viewing angles that would expose empty space beyond the map. With configurable map sizes, boundary behavior applies at different scales (128 vs 512 edge distances).
  - Reason: Free camera orbit adds new ways to encounter map boundaries. Map size variability means boundary distances change per game.

- **REQ-14: Shadows Ground Objects**
  - Previous: "Even with toon shading, subtle shadows help ground structures."
  - Revised: Shadow direction must remain consistent and readable regardless of camera angle. Since the player can now orbit freely, shadows should be cast from a fixed world-space light direction (the alien sun), not relative to the camera. Previously, "from above-behind camera" worked because the camera was fixed. Now, the light source must be a world-space directional light whose position does not change with camera orbit.
  - Reason: Free camera rotation means "above-behind camera" is no longer a fixed direction. Light must be world-space.

- **REQ-18: Edge Lines Define Form**
  - Previous: "Outline rendering gives toon-shaded models their illustration quality."
  - Revised: Edge lines must read correctly from all viewing angles, not just the fixed isometric view. Screen-space Sobel edge detection naturally handles this, but edge thickness tuning may need to account for steep viewing angles (high tilt) or near-overhead views where silhouettes change character.
  - Reason: Free camera means structures are viewed from angles the art team did not originally optimize for.

- **REQ-19: Single Light Source**
  - Previous: "Light direction should come from above and behind camera to illuminate building fronts."
  - Revised: Light is now a fixed world-space directional light (the alien sun) that does not rotate with the camera. When the player orbits to a different angle, they will see different shadow patterns on buildings. This is correct and expected -- it adds depth. The light direction should be chosen so that in the default isometric preset view (N at 45 degrees yaw), building fronts are well-illuminated. Other angles will naturally show side and back lighting, which is a feature of free camera, not a problem.
  - Reason: Camera rotation decouples light direction from view direction. Light must be world-space to avoid disorienting shadow shifts during orbit.

- **REQ-20: Shadow Color Has Character**
  - Previous: "Shadows shift toward purple or teal."
  - Revised: Same color shift, but now formally aligned with the canonical bioluminescent palette. Shadow shift toward deep purple (#2A1B3D) or dark teal is confirmed by canon. Additionally, with the dark base environment now canonical, shadow areas will be more prominent in the overall scene. Shadow color becomes even more important for maintaining visual interest in the darker parts of the world.
  - Reason: Canon v0.5.0 formalizes dark base tones, making shadow treatment more visually impactful.

- **REQ-24: Bioluminescent Teal (#00D4AA)**
  - Previous: "The signature ZergCity color. Used for energy conduits, powered state indicators, UI highlights."
  - Revised: Still the signature color, but now part of a formally defined glow accent palette: cyan/teal, green, amber, magenta. Teal is the PRIMARY glow color but no longer the ONLY one. Energy systems use teal; healthy zones use green; energy/warnings use amber; landmarks/special structures use magenta. The original requirement is expanded, not replaced.
  - Reason: Canon v0.5.0 defines a full bioluminescent glow palette with four accent families, expanding beyond teal-only.

- **REQ-25: Deep Purple (#2A1B3D)**
  - Previous: "Shadow color. Ambient occlusion should trend toward purple."
  - Revised: Deep purple now serves double duty: shadow color AND part of the dark base environment. The entire world has dark base tones (deep blues, greens, purples). Purple is not just "shadow" -- it IS the ambient environment. This reinforces the existing requirement and makes it more important.
  - Reason: Canon v0.5.0 establishes dark base tones as the environment foundation, making purple a world-level color, not just a shadow shift.

- **REQ-27: Terrain: Alien Earth**
  - Previous: "Blue-gray terrain, purple-tinted rocks, crystalline formations."
  - Revised: Terrain now has 10 distinct types, each with specific visual identity and glow properties. Flat ground has "dark soil/rock with subtle bioluminescent moss." Hills have "layered rock with glowing vein patterns." Crystal fields have "bright magenta/cyan crystal spires with strong glow emission." Spore plains have "pulsing green/teal spore clouds." Toxic marshes have "sickly yellow-green glow, bubbling surface." Volcanic rock has "dark obsidian with orange/red glow cracks." The requirement now specifies distinct visual identity per terrain type, not a single alien-earth description.
  - Reason: Canon v0.5.0 defines 10 terrain types (up from 4) with specific glow properties each.

- **REQ-28: Fluid: Not Blue Water**
  - Previous: "Mercury silver, iridescent green, or bioluminescent pools."
  - Revised: Water is now canonically "dark water with bioluminescent particles, soft glow" with three subtypes (ocean, river, lake). The "not blue water" mandate remains, but the specific look is now defined: dark water surface with glowing particles. This is more constrained than the original "choose one alien fluid type" guidance.
  - Reason: Canon v0.5.0 specifies water visual as "dark water with bioluminescent particles."

- **REQ-30: Zoom Range: Strategic to Detail**
  - Previous: "Far zoom: 0.25x sees broad colony structure. Close zoom: 2.0x sees individual beings."
  - Revised: Zoom range must now account for configurable map sizes. A 512x512 map may need a wider max zoom-out than a 128x128 map. The "strategic" zoom level on a large map should show the entire colony as a glowing constellation in the dark environment. The "detail" level is unchanged. Consider whether zoom range should be fixed or scale with map size.
  - Reason: Canon v0.5.0 introduces configurable map sizes (128/256/512), affecting zoom range needs.

- **REQ-31: LOD Transitions Must Be Invisible**
  - Previous: "If Level of Detail is used, transitions must be imperceptible."
  - Revised: LOD is no longer optional -- it is REQUIRED for large (512x512) maps per canon. "Larger maps require distance-based LOD and frustum culling" is now explicit. This elevates the requirement from "if used, be invisible" to "must be implemented and must be invisible." LOD is critical path for large maps. The dark bioluminescent environment may actually help hide LOD transitions, as distant objects naturally fade into the dark base.
  - Reason: Canon v0.5.0 explicitly requires LOD for large maps, upgrading this from optional to mandatory.

- **REQ-32: Far Zoom: Infrastructure Network**
  - Previous: "At maximum zoom out, see pathways as veins, districts as organs."
  - Revised: On large maps (512x512), the far zoom infrastructure network view becomes even more dramatic. The bioluminescent art direction means the far-zoom view is literally a constellation of glowing points and lines on a dark surface. Energy conduit networks should read as glowing veins. Powered districts should be bright clusters. Unpowered areas should be visibly dark. The "colony as organism" metaphor is now powerfully literal in the bioluminescent palette.
  - Reason: Dark base environment + glow accents + larger maps make the infrastructure network view more impactful and more important.

- **REQ-33: Close Zoom: Intimate Details**
  - Previous: "See window patterns, structural ornamentation, beings walking."
  - Revised: Close zoom now reveals bioluminescent material detail -- the subtle glow of active structures, the pulsing of crystal terrain, the particle effects of spore plains. Close inspection should reward players with the full bioluminescent experience. Building templates include construction animations that should be visible at close zoom. Terrain glow properties (crystal emission, spore pulsing, toxic bubbling) should be most visible at close zoom.
  - Reason: Bioluminescent art direction adds glow-specific detail that rewards close inspection. Building template system formalizes construction animations.

- **REQ-35: Atmosphere at Distance**
  - Previous: "Optional: distant objects can have subtle atmospheric haze (depth fog)."
  - Revised: With the dark base environment now canonical, depth fog should be dark (deep blue/purple), not light haze. Distant objects fade into darkness, not into bright sky. This creates a natural bioluminescent falloff where distant glow dims into shadow. Atmospheric depth cue is no longer just "optional but impactful" -- with 512x512 maps, it becomes a practical necessity for readability and performance (hiding LOD transitions).
  - Reason: Dark base environment changes atmospheric haze from "light fog" to "dark fade." Large maps make it more important.

- **REQ-40: Power State Glow**
  - Previous: "Powered buildings can have subtle emissive glow. Unpowered buildings: dim, desaturated, dormant."
  - Revised: Power state glow is now THE CORE visual feedback mechanism. In the bioluminescent art direction, powered = glowing = alive. Unpowered = dark = dead. This is no longer "subtle emissive glow" -- it is the primary visual language. A powered colony is a constellation of light. An unpowered colony is a dark ruin. The powered/unpowered contrast is dramatically amplified by the dark base environment. Canon specifies: "Structures should have subtle glow when active (powered + watered). Unpowered/abandoned structures lose glow -- visually 'dead'."
  - Reason: Canon v0.5.0 makes glow the primary visual indicator of structure state. Dark environment amplifies the powered/unpowered contrast from "subtle" to "dramatic."

- **Risk 5: Fixed angle limitations**
  - Previous: "Some building arrangements may be unreadable from the fixed isometric angle."
  - Revised: This risk is fundamentally reduced. Free camera allows players to orbit to see any side of any building. However, a new related risk replaces it: buildings must now look good from ALL angles, not just one. Art direction can no longer assume a single viewing angle. Building templates must be designed for 360-degree viewing.
  - Reason: Free camera removes the fixed-angle readability risk but introduces the all-angle quality risk.

- **Risk 6: Zoom extremes feel different**
  - Previous: "Far zoom feels like strategy game. Close zoom feels like diorama."
  - Revised: Still valid, but now amplified by map size variability. On a 512x512 map, far zoom is much farther out, increasing the psychological distance between "strategy" and "diorama" modes. The dark bioluminescent environment may actually help unify these modes -- far zoom is "glowing constellation" and close zoom is "glowing detail" -- same visual language at different scales.
  - Reason: Configurable map sizes extend the zoom range, amplifying the mode-shift concern.

- **Risk 7: No rotation can frustrate**
  - Previous: "Players accustomed to city builders with rotation may feel constrained."
  - Revised: This risk is RESOLVED. Free camera with orbit directly addresses the frustration concern. However, a new concern emerges: players may get disoriented with full rotation freedom. Isometric preset snap views serve as "orientation anchors" to mitigate this.
  - Reason: Free camera eliminates rotation frustration. New disorientation risk replaces it.

- **Risk 9: Silhouette readability**
  - Previous: "Building types must be distinguishable by silhouette alone."
  - Revised: Still critical, but now silhouettes must be readable from multiple angles, not just the fixed isometric view. Buildings need strong silhouette identity from N/E/S/W preset views AND from arbitrary orbit angles. This makes silhouette design harder but more rewarding.
  - Reason: Free camera means silhouettes are viewed from many angles.

- **Visual Style: One-line vision**
  - Previous: "A bioluminescent alien colony rendered in clean 3D with confident toon shading -- simultaneously futuristic, organic, and unmistakably stylized."
  - Revised: "A dark alien world alive with bioluminescent glow, rendered in toon-shaded 3D with free camera exploration -- simultaneously futuristic, organic, and unmistakably stylized." The dark base environment and free camera are now part of the vision statement.
  - Reason: Canon v0.5.0 formalizes the dark environment and v0.4.1 adds free camera.

- **Visual Style: Toon Shading Rules Table**
  - Previous: Emission row listed "Teal glow for powered/active elements."
  - Revised: Emission row should read "Bioluminescent glow (cyan, green, amber, magenta) for active elements -- color varies by function." Light direction row previously read "From above-behind camera." Should read "Fixed world-space directional light (alien sun); default isometric preset illuminates building fronts."
  - Reason: Multi-color glow palette and world-space lighting.

- **Visual Style: Alien Theme in 3D Table**
  - Previous: Terrain row listed "Blue-gray/purple geometry with crystalline rock formations."
  - Revised: Terrain row should list 10 distinct terrain types with glow properties: ground (bioluminescent moss), hills (glowing veins), water (bioluminescent particles), forest (glowing alien trees), crystal fields (magenta/cyan crystal spires), spore plains (pulsing spore clouds), toxic marshes (yellow-green glow), volcanic rock (orange/red glow cracks). Fluids row should specify "Dark water with bioluminescent particles" rather than open options.
  - Reason: Canon v0.5.0 specifies 10 terrain types with distinct visuals.

- **Mood Board: Day/Night**
  - Previous: "Night mode where the colony becomes a constellation of teal glowing points."
  - Revised: Night mode where the colony becomes a constellation of multi-colored glow: teal energy systems, green healthy zones, amber warnings, magenta landmarks. The base environment is already dark, so "night" deepens the darkness further, making glow even more prominent. Day is "alien twilight" with visible glow. Night is "full bioluminescence" where glow IS the only light.
  - Reason: Multi-color glow palette and dark base environment.

---

### INVALIDATED

- **REQ-10: No Rotation Is a Feature**
  - Reason: Canon v0.4.1 explicitly adds full camera rotation (orbit/tilt). Rotation is now core functionality, not a deliberately excluded feature. The entire premise -- "no camera rotation creates simplicity" -- is reversed. The new camera model embraces rotation with isometric presets as orientation anchors.
  - Recommendation: Replace with new requirements for orbit/tilt feel, snap-to-preset transitions, and rotation-aware art direction (see NEW CONCERNS below).

---

### NEW CONCERNS

- **NEW-01: Orbit Controls Must Feel Grounded**
  - Reason: Free camera orbit is new functionality with no prior experience requirement. The orbit must feel like rotating around the colony, not spinning in space. Orbit should have a world-space pivot point (the focus point on the terrain surface), not an abstract center. Orbit speed should be comfortable -- not too fast (disorienting) or too slow (frustrating). Right-mouse-drag is the expected orbit binding.
  - Recommendation: Add as REQ-48. Define orbit pivot, speed, and input binding. Orbit should feel like walking around a diorama, looking at the colony from different angles.

- **NEW-02: Tilt Limits Protect Readability**
  - Reason: Free camera tilt without limits could allow players to look straight down (overhead/plan view) or nearly horizontal (ground-level). Both extremes change how the colony reads dramatically. Tilt should be constrained to a useful range (e.g., 15-75 degree pitch) to prevent extreme views that break visual clarity.
  - Recommendation: Add as REQ-49. Define tilt range limits that maintain toon-shaded readability. Overhead view may flatten toon shading. Ground-level view may obscure grid layout.

- **NEW-03: Snap-to-Preset Transitions Must Be Smooth and Fast**
  - Reason: The isometric preset snap views (N/E/S/W at 45 degree increments, ~35.264 degree pitch) are new. Snapping from free camera to a preset must be a smooth animated transition, not an instant cut. The transition should take ~0.3-0.5 seconds with an ease-in-out curve. Players should feel the camera "settling" into a known position, like clicking a ratchet.
  - Recommendation: Add as REQ-50. Define preset snap animation timing, easing, and feel. Should feel satisfying, like a mechanical click into place.

- **NEW-04: Mode Switching UX Must Be Clear**
  - Reason: Players need to understand when they are in "free camera" vs "isometric preset" mode. A subtle HUD indicator showing current camera mode and available preset keys would prevent confusion. Default should start in isometric preset (per canon: "default_camera_mode: Isometric preset").
  - Recommendation: Add as REQ-51. Define camera mode indicator, default mode, and switching affordances. Note: this is a UI concern but affects rendering (compass/mode indicator overlay).

- **NEW-05: Camera Speed Should Scale with Map Size**
  - Reason: Configurable map sizes (128/256/512) mean a 512x512 map is 16x the area of a 128x128 map. Pan speed at a given zoom level should account for map size so that crossing the map does not take unreasonably long on large maps. This may mean pan speed scales with map diagonal, or that zoom-out is more aggressive on large maps.
  - Recommendation: Add as REQ-52. Define relationship between map size and camera movement speed. Consider fast-travel (click minimap to jump) as a complement for large maps.

- **NEW-06: 360-Degree Building Design Requirement**
  - Reason: Free camera orbit means buildings are seen from all angles. The previous analysis assumed a single fixed viewing angle and noted that art direction could optimize for it. Now, building templates must be designed to look good from any angle. No "ugly back" is acceptable. This affects model design principles -- buildings need 4-sided design at minimum.
  - Recommendation: Update Model Design Principles to add "360-degree readability" rule. Buildings must look good from all four isometric preset angles AND from arbitrary orbit positions. This was already hinted at in tickets ("models should be 4-sided ready") but is now a hard requirement, not future-proofing.

- **NEW-07: Dark Environment Affects Readability at Low Glow**
  - Reason: The bioluminescent art direction uses dark base tones. In early game, when the player has few powered structures, the world may feel too dark and hard to read. There must be a baseline ambient light level (from terrain bioluminescence, sky, or ambient sources) that keeps the world readable even without player-built structures. Unpowered areas should be dark but not black.
  - Recommendation: Define minimum ambient light level for dark base environment. Terrain itself should have subtle natural glow (canon specifies "bioluminescent moss" on flat ground) that provides baseline readability. Sky dome or ambient light should prevent pure-black areas.

- **NEW-08: Terrain Glow Must Not Compete with Structure Glow**
  - Reason: With 10 terrain types having their own glow properties (crystal spires with strong emission, spore clouds with pulsing glow, toxic marshes with yellow-green glow), there is a risk that terrain glow competes with or overwhelms structure glow. Player-built structures must always be the brightest, most visually prominent elements. Terrain glow should be subtle/ambient, not competing for attention.
  - Recommendation: Establish glow intensity hierarchy: player structures (brightest, most saturated) > terrain features (subtle, ambient) > background/base (minimal). This hierarchy ensures built infrastructure always reads as the dominant visual layer.

- **NEW-09: Bloom Intensity Must Be Carefully Tuned for Dark Environment**
  - Reason: Bloom post-processing on bright emissive surfaces against a dark background can be visually dramatic but also overwhelming if over-tuned. Excessive bloom in a dark environment creates "glow soup" where everything bleeds together. Bloom must be tuned conservatively so that glow effects enhance readability rather than destroy it.
  - Recommendation: Bloom should be subtle -- a soft halo around emissive elements, not an overwhelming wash. Bloom radius and intensity should be configurable. Consider player-facing bloom intensity slider in settings.

- **NEW-10: Perspective vs Orthographic in Free Camera Mode**
  - Reason: The original analysis assumed orthographic projection for isometric view. Free camera with orbit and tilt may benefit from perspective projection to feel natural. Canon does not explicitly resolve whether free camera mode uses perspective or orthographic projection. Orthographic maintains grid clarity but feels unusual when orbiting freely. Perspective feels natural for free camera but distorts the grid. This needs a clear decision.
  - Recommendation: Raise as a question for Systems Architect. Possible approach: orthographic in isometric preset mode, perspective in free camera mode, with smooth projection transition when switching modes. Or: perspective always, with near-orthographic FOV.

---

## Questions for Other Agents

### For Systems Architect

- @systems-architect: With the free camera model, should the projection switch between orthographic (isometric presets) and perspective (free camera), or remain orthographic throughout? Orthographic free-orbit feels unusual to players. Perspective free-orbit feels natural but distorts the grid. What is the approach?

- @systems-architect: Camera speed scaling with map size -- should this be automatic (derived from map dimensions) or a player-adjustable setting? What is the CameraState structure modification needed?

- @systems-architect: Snap-to-preset animation -- does this live in CameraSystem as an animation state, or in CameraAnimation (ticket 2-027)? What triggers the snap (keybind, button, both)?

### For Graphics Engineer

- @graphics-engineer: With the dark base environment, what is the minimum ambient light level needed to keep terrain readable? Should this come from a sky dome, an ambient light term in the toon shader, or emissive terrain materials?

- @graphics-engineer: Bloom on dark backgrounds -- do we need to adjust bloom threshold dynamically, or can a single conservative threshold work across all zoom levels and lighting conditions?

- @graphics-engineer: With 10 terrain types having distinct glow properties, how many emissive material variants do we need? Can this be parameterized (emission color + intensity per terrain type) or does each terrain need a separate material?

---

## Affected Tickets

- **2-017: Camera State and Configuration** -- MODIFY. CameraState must now include: camera mode (free vs preset), orbit angle, tilt angle, active preset (N/E/S/W/none), and transition animation state. Pitch is no longer fixed at 26.57 degrees. Yaw is no longer fixed at 45 degrees. Both are now player-controllable within limits.

- **2-020: View Matrix Calculation** -- MODIFY. View matrix must now account for arbitrary pitch/yaw from free camera orbit, not just fixed isometric angles. Camera position calculation from spherical coordinates already supports this, but acceptance criteria should reflect variable angles.

- **2-021: Projection Matrix Calculation** -- MODIFY. May need to support both orthographic (isometric preset) and perspective (free camera) projection, or the decision on projection type needs to be documented. Acceptance criteria about "parallel lines stay parallel" may not hold in perspective mode.

- **2-023: Pan Controls** -- MODIFY. Pan must work relative to current camera orientation in free mode, not just world-axis-aligned. Add orbit controls (right-mouse drag). Add tilt controls. Add snap-to-preset keybinds (e.g., Numpad 1/3/7/9 or similar).

- **2-024: Zoom Controls** -- MODIFY. Zoom center-on-cursor must work at arbitrary camera angles. Zoom range may need to scale with map size (configurable 128/256/512).

- **2-025: Viewport Bounds and Map Clamping** -- MODIFY. Boundary clamping must handle orbit (don't orbit to see void beyond map edges). Map size is now configurable, so boundary values are dynamic.

- **2-027: Camera Animation** -- MODIFY. Add snap-to-preset animation (smooth transition to N/E/S/W preset angles with easing). This is a primary use case for camera animation, not just "go to" and camera shake.

- **2-037: Emissive Material Support** -- MODIFY. Emissive materials now serve the CORE art direction (bioluminescence), not just energy conduits. Multiple emission colors needed (cyan, green, amber, magenta), not just teal. Terrain types also need emissive properties (crystal, spore, toxic, volcanic glow). Emission intensity hierarchy should be defined.

- **2-038: Bloom Post-Process** -- MODIFY. Bloom is now more critical (core visual feature, not just accent effect). Must be tuned for dark environments. Bloom threshold and intensity should be configurable. Consider player-facing bloom intensity setting.

- **2-005: Toon Shader Implementation** -- MODIFY. Light direction must be world-space fixed (alien sun), not relative to camera. Shadow color formally confirmed as purple (#2A1B3D). Ambient term must provide minimum readability in dark base environment.

- **NEW: 2-046: Free Camera Orbit and Tilt Controls** -- New ticket needed. Implement orbit (right-mouse drag) and tilt controls for free camera mode. Define pitch limits (e.g., 15-75 degrees), orbit speed, input bindings. Separate from pan controls (2-023) as this is a distinct input modality.

- **NEW: 2-047: Isometric Preset Snap System** -- New ticket needed. Implement four preset snap views (N/E/S/W at 45-degree yaw increments, ~35.264 degree pitch). Smooth animated transition from current camera state to target preset (0.3-0.5s ease-in-out). Keybind support. Mode indicator. Default to isometric preset on game start.

- **NEW: 2-048: Camera Mode Management** -- New ticket needed. State machine for camera modes: Preset (locked to isometric angle), Free (full orbit/pan/zoom/tilt), Animating (transitioning between modes). Track current mode, handle mode switches, provide mode query for other systems. Integrate with CameraState (2-017).

---

## Summary of Key Priorities (Updated)

1. **Bioluminescent toon shading is identity** -- Toon shader + dark environment + multi-color glow = ZergCity's visual brand
2. **Camera must support both modes flawlessly** -- Free orbit for exploration, isometric presets for efficient building, smooth transitions between them
3. **Dark world must remain readable** -- Minimum ambient light, glow intensity hierarchy, conservative bloom
4. **Silhouette readability from all angles** -- Buildings must be recognizable from 360 degrees, not just one fixed view
5. **Map size affects camera experience** -- Speed, zoom range, LOD, and boundary behavior must scale with configurable map sizes
