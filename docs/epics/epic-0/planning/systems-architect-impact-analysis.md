# Systems Architect Impact Analysis: Epic 0

**Epic:** Project Foundation
**Analyst:** Systems Architect
**Date:** 2026-01-28
**Previous Analysis Canon Version:** 0.3.0
**Current Canon Version:** 0.5.0

---

## Trigger
Canon update v0.3.0 -> v0.5.0

## Overall Impact: LOW

## Impact Summary

The canon v0.5.0 changes are overwhelmingly visual/content-oriented (art direction, terrain types, building templates, UI modes, audio) and target Epic 2+ concerns. Epic 0 is the invisible foundation layer and is minimally affected. The two concrete impacts are: (1) AssetManager paths/descriptions shift from sprites to models (already reflected in tickets from the v0.4.0 3D pivot), and (2) the Configuration System must accommodate the new configurable map size parameter, which the Server CLI must expose at game creation time.

---

## Item-by-Item Assessment

### UNAFFECTED

**APP-01: SDL3 Initialization/Shutdown** -- UNAFFECTED
No canon changes touch SDL initialization. Video, audio, events subsystems unchanged.

**APP-02: Window Management** -- UNAFFECTED
Window management is independent of art direction, terrain types, and map sizes. Resolution configuration is unchanged.

**APP-03: Main Game Loop** -- UNAFFECTED
Fixed timestep (50ms, 20 ticks/sec) and variable render rate are unchanged. Map size does not affect loop structure.

**APP-04: State Management** -- UNAFFECTED
Application states (Menu, Connecting, Playing, Paused) are unchanged by visual/content canon.

**APP-05: Server vs Client Mode Detection** -- UNAFFECTED
Server/client mode logic is unrelated to art direction or map configuration.

**INPUT-01 through INPUT-05: All InputSystem items** -- UNAFFECTED
Input polling, keyboard/mouse tracking, input mapping, input context stack, core actions -- none are affected by visual canon, terrain types, building templates, or map sizes. Dual UI modes may eventually introduce a toggle hotkey, but that is an Epic 12 concern, not Epic 0.

**COMP-01: PositionComponent** -- UNAFFECTED
Grid coordinates (grid_x, grid_y, elevation as int32_t) unchanged. Map size affects the valid range of coordinates but not the component definition itself. Range validation is a TerrainSystem concern (Epic 3).

**COMP-02: OwnershipComponent** -- UNAFFECTED
Ownership model unchanged by v0.5.0. Ghost town mechanics were already in v0.3.0.

**ECS-01: Component Registry Design** -- UNAFFECTED
No impact from visual/content changes.

**ECS-02: Entity Management Design** -- UNAFFECTED
Entity ID allocation unaffected.

**Risk 1: Fixed Timestep + Variable Render Interpolation** -- UNAFFECTED
Interpolation pattern unchanged.

**Risk 2: Async Asset Loading Thread Safety** -- UNAFFECTED
Threading model unchanged.

**Risk 3: Input System Coupling to Network** -- UNAFFECTED
InputMessage format unchanged.

**Risk 4: Headless Server Mode** -- UNAFFECTED
Server mode unaffected by visual canon.

**Risk 5: ECS Library vs Custom** -- UNAFFECTED
Already decided (EnTT). Canon changes don't affect this.

**Risk 6: Component Serialization Strategy** -- UNAFFECTED
Serialization pattern unchanged.

**Architectural Recommendations 1-4** -- UNAFFECTED
EnTT choice, core interfaces, logging, and configuration system design are structurally unchanged. (Configuration *content* is modified -- see MODIFIED section.)

### MODIFIED

**ASSET-01: Asset Loading Interface** -- MODIFIED
- **Change:** Canon v0.5.0 codifies `assets/models/` and `assets/textures/` as the directory structure (replacing the former `assets/sprites/`). The AssetManager description in systems.yaml now reads "Loading 3D models, textures, audio" (previously referenced sprites).
- **Impact on ticket:** Ticket 0-008 already reflects this from the v0.4.0 3D pivot (it lists glTF support and model loading library). The directory path constants used by AssetManager should reference `assets/models/` and `assets/textures/`. No structural change needed, but verify default asset search paths align with canon's `assets/` directory structure.
- **Classification:** Minor wording/path alignment. Already handled by existing ticket text.

**ASSET-05: SDL3 Asset Type Support** -- MODIFIED
- **Change:** Canon v0.5.0 adds detailed audio direction (user-sourced ambient music + engine SFX). While ticket 0-012 already covers audio loading (WAV, OGG), the canon now specifies playlist support, crossfade, and separate volume control for music vs. SFX. This is an Epic 15 concern, but the AssetManager's audio type support should anticipate the distinction between `Music` (long streaming tracks) and `AudioChunk` (short sound effects).
- **Impact on ticket:** Ticket 0-012 already distinguishes `load<AudioChunk>` and `load<Music>`. No structural change needed. The loading interface is sufficient; playback semantics are AudioSystem's responsibility (Epic 15).
- **Classification:** No change to ticket text required. Audio asset types already properly split.

**Configuration System (Architectural Recommendation 4)** -- MODIFIED
- **Change:** Canon v0.5.0 introduces configurable map sizes (128x128, 256x256, 512x512) selected at game creation by the server host. This is a server-side configuration value that must be stored and communicated to clients.
- **Impact:** The Configuration System (ticket 0-014) should include map_size as a server-side configuration parameter. This is NOT a client-local preference (like window resolution or key bindings) -- it is a game-creation parameter set by the server host.
- **Classification:** Ticket 0-014 needs a minor addition to acceptance criteria. See Affected Tickets section.

### NEW CONCERNS

**NEW-01: Map Size as Server Game-Creation Parameter**
- **Source:** Canon v0.5.0 map_configuration section
- **Concern:** Map size is set at game creation and cannot change. This is fundamentally different from other configuration values (which are runtime-modifiable preferences). It is a server-authoritative, immutable game parameter. The Configuration System must distinguish between:
  1. **Client preferences** (window resolution, key bindings, log level) -- local, mutable, per-user
  2. **Server game parameters** (map size, game seed) -- set once at creation, immutable, server-authoritative
- **Epic 0 impact:** The Configuration System (0-014) and Server CLI (0-022) must both understand this distinction. The game parameter is set via the CLI at server startup/game creation, persisted, and sent to clients on connection.
- **Severity:** Low. This is a design distinction, not a structural change. The configuration system can handle both categories with a simple separation. The actual map size value is consumed by TerrainSystem (Epic 3), not by Epic 0 systems.

**NEW-02: Building Template System Awareness (No Epic 0 Impact)**
- **Source:** Canon v0.5.0 building_templates section
- **Concern:** The building template system defines a `model_source: procedural | asset` field and `model_path` for glTF files. This means the AssetManager will need to load building models referenced by templates. However, this is entirely an Epic 4 (Zoning) concern -- the AssetManager's generic `load<Model>(path)` interface (already in 0-008/0-034) is sufficient.
- **Epic 0 impact:** None. The AssetManager interface is generic enough.

**NEW-03: Expanded Terrain Types (No Epic 0 Impact)**
- **Source:** Canon v0.5.0 terrain_types section
- **Concern:** The terrain type set expanded from 6 types (ground, hills, water, forest) to 10 types (adding crystal_fields, spore_plains, toxic_marshes, volcanic_rock). Each has distinct gameplay effects and visual properties.
- **Epic 0 impact:** None. Terrain is Epic 3. PositionComponent does not encode terrain type. TerrainComponent (Epic 3) will handle this.

**NEW-04: Dual UI Modes (No Epic 0 Impact)**
- **Source:** Canon v0.5.0 ui_design section
- **Concern:** Two UI modes (Classic SimCity 2000 + Sci-fi Holographic) that are toggleable at runtime. This affects UISystem (Epic 12) and potentially InputSystem if a toggle hotkey is needed.
- **Epic 0 impact:** None. The InputMapping system (0-019) can add a UI_TOGGLE action in a future epic. No foundation changes needed.

**NEW-05: Bioluminescent Art Direction (No Epic 0 Impact)**
- **Source:** Canon v0.5.0 art_direction section
- **Concern:** The bioluminescent palette (dark base + glowing accents) with toon/cell shading, bloom post-processing, and emissive materials. This is entirely a RenderingSystem (Epic 2) and shader pipeline concern.
- **Epic 0 impact:** None.

---

## Questions for Other Agents

None required. The v0.5.0 changes are well-specified and their Epic 0 impact is minimal. The distinction between client preferences and server game parameters (NEW-01) is a straightforward design pattern that does not require cross-agent discussion.

---

## Affected Tickets

### UNCHANGED (no modifications needed)

| Ticket | Title | Reason |
|--------|-------|--------|
| 0-001 | Project Build Infrastructure | No impact from visual/content canon |
| 0-002 | SDL3 Initialization and Shutdown | No impact |
| 0-003 | Window Management | No impact |
| 0-004 | Main Game Loop with Fixed Timestep | No impact |
| 0-005 | Application State Management | No impact |
| 0-006 | Server vs Client Mode | No impact |
| 0-007 | Interpolation Pattern | No impact |
| 0-009 | Asset Reference Counting | No impact |
| 0-010 | Async Asset Loading | No impact |
| 0-011 | Asset Loading Progress | No impact |
| 0-013 | Logging Infrastructure | No impact |
| 0-015 | SDL Event Polling | No impact |
| 0-016 | Keyboard State Tracking | No impact |
| 0-017 | Mouse State Tracking | No impact |
| 0-018 | Input Context Stack | No impact |
| 0-019 | Input Mapping System | No impact |
| 0-020 | Core Input Actions for Epic 0 | No impact |
| 0-021 | Deterministic RNG Utility | No impact |
| 0-023 | PositionComponent Definition | No impact (map size doesn't change component definition) |
| 0-024 | OwnershipComponent Definition | No impact |
| 0-025 | ECS Framework Integration | No impact |
| 0-026 | Core Type Definitions | No impact |
| 0-027 | InputMessage Structure | No impact |
| 0-028 | Serialization Pattern | No impact |
| 0-029 | Unit Testing Framework | No impact |
| 0-030 | Debug Console Output | No impact |
| 0-031 | SyncPolicy Metadata Pattern | No impact |
| 0-032 | Application Shutdown and Cleanup | No impact |
| 0-034 | 3D Model Loading Support | No impact (already created for 3D pipeline) |

### MODIFY (minor updates needed)

**Ticket 0-008: AssetManager Core Interface** -- MODIFY (MINOR)
- **What changed:** Canon now explicitly specifies `assets/models/` for 3D models and `assets/textures/` for textures. AssetManager description changed from "Loading sprites" to "Loading 3D models."
- **Required update:** Verify that acceptance criteria reference the canonical directory structure (`assets/models/`, `assets/textures/`, `assets/audio/`). The ticket already mentions glTF support from the v0.4.0 update, so the core interface is fine.
- **Specific change:** Add acceptance criterion: "Default asset search paths match canon directory structure: `assets/models/`, `assets/textures/`, `assets/audio/`, `assets/fonts/`"

**Ticket 0-012: SDL3 Asset Type Support** -- MODIFY (MINOR)
- **What changed:** Canon v0.5.0 distinguishes music (user-sourced ambient, playlist/crossfade) from SFX (engine-provided game actions). The ticket already lists `load<AudioChunk>` and `load<Music>` as separate types.
- **Required update:** Add a note clarifying that Music assets may be large (streaming) vs AudioChunk (fully loaded). The AssetManager's async loading must handle both patterns. This is already implied but should be explicit.
- **Specific change:** Add note: "Music assets may require streaming support (large files). AudioChunk assets are fully loaded into memory. Both use async loading but different memory strategies."

**Ticket 0-014: Configuration System** -- MODIFY (MODERATE)
- **What changed:** Canon v0.5.0 introduces configurable map sizes (128/256/512) as a server game-creation parameter.
- **Required update:** The configuration system must support two categories of configuration:
  1. **Client preferences** (window resolution, keybindings, log level) -- local JSON file
  2. **Server game parameters** (map size, game seed) -- set at game creation, immutable
- **Specific change:** Add acceptance criteria:
  - "Configuration system distinguishes client preferences from server game parameters"
  - "Server game parameters: map_size (128, 256, 512; default 256)"
  - "Server game parameters are set at game creation and are immutable during gameplay"
  - "Server game parameters are transmitted to clients on connection (consumed by Epic 1)"

**Ticket 0-022: Server CLI Interface** -- MODIFY (MODERATE)
- **What changed:** Canon v0.5.0 specifies map size is selected by the server host at game creation.
- **Required update:** Server CLI should accept map size as a startup parameter or game creation command.
- **Specific change:** Add acceptance criteria:
  - "Server accepts --map-size <small|medium|large> (or 128/256/512) at startup or game creation"
  - "Default map size: medium (256x256)"
  - "`status` command displays current map size"
  - "Map size is logged on server startup"

**Ticket 0-033: Integration Smoke Test** -- MODIFY (MINOR)
- **What changed:** Canon v0.5.0 solidifies 3D model loading as primary asset type. The smoke test should verify model loading, not just texture loading.
- **Required update:** Smoke test should load a test 3D model (.glb) in addition to texture and audio.
- **Specific change:** Update acceptance criterion from "Asset manager can load a test texture and audio file" to "Asset manager can load a test 3D model (.glb), texture, and audio file"

### NEW (no new tickets needed)

No new tickets are required for Epic 0. All v0.5.0 canon additions (expanded terrain, building templates, bioluminescent art, dual UI modes, audio direction) are consumed by Epic 2+ systems. The minor modifications to existing tickets fully cover the Epic 0 impact.

---

## Conclusion

Canon v0.5.0 is a content-heavy, visually-focused update. Its impact on Epic 0 (Project Foundation) is minimal because Epic 0 builds the invisible plumbing -- game loop, asset loading interfaces, input handling, ECS framework -- not the content that flows through it. The two meaningful changes are:

1. **Map size configuration** (0-014, 0-022): The configuration system and server CLI need to handle a new game-creation parameter. This is a moderate but straightforward addition.
2. **Asset path alignment** (0-008, 0-033): Default asset directories should match canon's `assets/models/` + `assets/textures/` structure, and the smoke test should verify 3D model loading.

No architectural risks are introduced. No new tickets are needed. The foundation design remains sound.
