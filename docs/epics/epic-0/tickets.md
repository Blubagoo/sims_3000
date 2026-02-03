# Epic 0: Project Foundation - Tickets

Generated: 2026-01-25
Canon Version: 0.13.0
Planning Agents: systems-architect, game-designer

## Revision History

| Date | Trigger | Tickets Modified | Tickets Removed | Tickets Added |
|------|---------|------------------|-----------------|---------------|
| 2026-01-28 | canon-update (v0.3.0 → v0.5.0) | 0-008, 0-012, 0-014, 0-020, 0-022, 0-033, 0-034 | — | — |
| 2026-01-29 | canon-verification (v0.5.0 → v0.13.0) | — | — | — |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. No changes required - Epic 0 is foundational; interfaces added in v0.7.0-v0.13.0 (ISimulationTime, IGridOverlay, UI events) are consumed by later epics, not Epic 0.

## Overview

Epic 0 establishes the foundational systems that all subsequent epics depend on: the Application lifecycle (SDL3, game loop, state management), AssetManager (loading, caching, async operations), and InputSystem (event polling, keyboard/mouse state, input mapping). Additionally, we define the first ECS components (PositionComponent, OwnershipComponent) and establish the ECS framework itself. This is the "invisible foundation" - when done right, players never notice it; when done wrong, everything feels broken.

## Key Decisions

- **ECS Framework:** EnTT (header-only, high-performance)
- **Configuration Format:** JSON with nlohmann/json
- **3D Asset Support:** glTF format with cgltf library

## Tickets

### Ticket 0-001: Project Build Infrastructure

**Type:** infrastructure
**System:** Build
**Estimated Scope:** M

**Description:**
Set up the CMake build system with vcpkg integration for dependency management. Configure SDL3 and SDL3_image as dependencies. Establish debug/release build configurations and platform-specific settings for Windows/Linux/macOS.

**Acceptance Criteria:**
- [ ] CMake builds successfully on Windows
- [ ] vcpkg installs SDL3 and SDL3_image automatically
- [ ] Debug and Release configurations work correctly
- [ ] Build produces executable that initializes SDL3 without errors

**Dependencies:**
- Blocked by: None
- Blocks: 0-002 (SDL3 initialization needs working build)

**Agent Notes:**
- Systems Architect: vcpkg already present in repo. Need to pin SDL3 version for stability.
- Game Designer: N/A (infrastructure)

---

### Ticket 0-002: SDL3 Initialization and Shutdown

**Type:** feature
**System:** Application
**Estimated Scope:** S

**Description:**
Initialize SDL3 with video, audio, and events subsystems. Implement proper cleanup on shutdown in reverse order of initialization. Establish error handling patterns for SDL failures with meaningful error messages using alien terminology ("Subsystem Initialization Anomaly" not "SDL Init Failed").

**Acceptance Criteria:**
- [ ] SDL3 initializes successfully with video, audio, and events
- [ ] Shutdown cleans up all SDL resources in reverse order
- [ ] SDL failures produce clear, logged error messages
- [ ] No memory leaks on startup/shutdown cycle (verified with sanitizers)

**Dependencies:**
- Blocked by: 0-001 (build infrastructure)
- Blocks: 0-003, 0-008, 0-015 (window, asset loading, and input all need SDL)

**Agent Notes:**
- Systems Architect: Consider SDL_INIT_NOPARACHUTE for cleaner error handling. Server mode may skip video init.
- Game Designer: Error messages should use alien terminology from the start.

---

### Ticket 0-003: Window Management

**Type:** feature
**System:** Application
**Estimated Scope:** S

**Description:**
Create SDL3 window with configurable resolution. Handle resize events, fullscreen/windowed toggle, and expose window handle for future RenderingSystem (Epic 2). Window creation should skip entirely in server mode.

**Acceptance Criteria:**
- [ ] Window creates at configurable resolution (default 1280x720)
- [ ] Resize events are handled and logged
- [ ] Fullscreen toggle works (F11 or Alt+Enter)
- [ ] Window handle is accessible for renderer creation
- [ ] Server mode (--server flag) skips window creation entirely
- [ ] Window title displays "ZergCity" with version

**Dependencies:**
- Blocked by: 0-002 (SDL initialization)
- Blocks: 0-004, 0-017 (game loop needs window, mouse input needs window coords)

**Agent Notes:**
- Systems Architect: Single executable with --server flag, not separate executables. Application detects mode at startup.
- Game Designer: Window should feel professional from day one - proper title, icon when available.

---

### Ticket 0-004: Main Game Loop with Fixed Timestep

**Type:** feature
**System:** Application
**Estimated Scope:** M

**Description:**
Implement the core game loop using fixed timestep simulation (50ms = 20 ticks/sec per canon) with variable timestep rendering. Use accumulator-based tick scheduling to ensure consistent simulation regardless of frame rate. Include frame timing and delta time calculation.

**Acceptance Criteria:**
- [ ] Simulation runs at fixed 20 ticks/second (50ms per tick)
- [ ] Rendering runs at variable rate (target 60fps, uncapped or vsync)
- [ ] Frame time capped to prevent "spiral of death" (max 250ms/frame)
- [ ] Accumulator correctly schedules simulation ticks
- [ ] Alpha value calculated for interpolation (accumulator / TICK_DURATION)
- [ ] Render always happens at least once per frame (no "dead frames")
- [ ] Frame timing metrics available for debug overlay

**Dependencies:**
- Blocked by: 0-003 (needs window for render target)
- Blocks: 0-005, 0-007 (state management and interpolation need loop running)

**Agent Notes:**
- Systems Architect: Cap catch-up ticks per frame (max 5) to prevent simulation lag from blocking render. Log warning if this happens.
- Game Designer: REQ-01 (60fps target), REQ-04 (no dead frames). The loop must feel buttery smooth. Any stutter here propagates everywhere.

---

### Ticket 0-005: Application State Management

**Type:** feature
**System:** Application
**Estimated Scope:** M

**Description:**
Implement application state machine with Menu, Connecting, Playing, and Paused states. Each state has enter/exit hooks and controls which systems are active. Paused state in multiplayer is local-only (simulation continues on server).

**Acceptance Criteria:**
- [ ] AppState enum with Menu, Connecting, Playing, Paused states
- [ ] State transitions with on_enter() and on_exit() hooks
- [ ] Each state defines which systems run (see Q010 answer for matrix)
- [ ] Paused state allows camera movement and UI interaction
- [ ] Paused state freezes local simulation display but server continues
- [ ] State transitions feel smooth, not jarring
- [ ] Server mode has no Paused state (runs continuously)

**Dependencies:**
- Blocked by: 0-004 (needs game loop to have states)
- Blocks: None directly (enables future state-dependent features)

**Agent Notes:**
- Systems Architect: Per Q010, use "State Machine with System Activation Flags" pattern. Each state handler controls its own update/render behavior.
- Game Designer: REQ-13 (menu-to-game feels like "arrival"), REQ-14 (pause maintains atmosphere). Per Q005 answer: Local pause only in multiplayer - simulation NEVER stops.

---

### Ticket 0-006: Server vs Client Mode

**Type:** feature
**System:** Application
**Estimated Scope:** S

**Description:**
Application detects mode at startup via --server command-line flag. Server mode runs headless (no window, no rendering, no input handling). Client mode has full window, rendering, and input. Both share the same executable.

**Acceptance Criteria:**
- [ ] --server flag enables headless server mode
- [ ] Server mode: No window creation, no SDL video, no InputSystem
- [ ] Server mode: Simulation runs, logging works
- [ ] Client mode: Full window, rendering, input (default)
- [ ] Mode detection happens before any SDL initialization
- [ ] Server outputs periodic heartbeat to console so operators know it's alive

**Dependencies:**
- Blocked by: 0-002 (needs to conditionally init SDL)
- Blocks: 0-022 (server CLI interface)

**Agent Notes:**
- Systems Architect: Single executable simplifies build and deployment. Consider SDL_InitSubSystem for selective initialization.
- Game Designer: Per Q006, server operators need to feel in control. Basic console output from the start.

---

### Ticket 0-007: Interpolation Pattern for Smooth Rendering

**Type:** feature
**System:** Application/Components
**Estimated Scope:** M

**Description:**
Establish the double-buffered state pattern for smooth visual interpolation between 20Hz simulation ticks and 60fps rendering. Create Interpolatable<T> template wrapper that stores previous and current state for lerping.

**Acceptance Criteria:**
- [ ] Interpolatable<T> template provides previous/current state storage
- [ ] Pre-tick hook rotates buffers (previous = current)
- [ ] Rendering calculates interpolation alpha from accumulator
- [ ] Linear interpolation helper function for common types (position, float)
- [ ] Pattern documented for future component authors
- [ ] Verified smooth motion at various frame rates

**Dependencies:**
- Blocked by: 0-004 (needs game loop with accumulator)
- Blocks: 0-023 (PositionComponent will use this pattern)

**Agent Notes:**
- Systems Architect: Per Q007 answer, this is component-level storage with rendering-layer execution. Not all components need interpolation - OwnershipComponent is discrete.
- Game Designer: REQ-02 (consistent tick rate feel). Entities should never "teleport" or stutter between states. This is critical for feel.

---

### Ticket 0-008: AssetManager Core Interface

**Type:** feature
**System:** AssetManager
**Estimated Scope:** M

**Description:**
Implement the core AssetManager with generic load<T>(path) interface returning AssetHandle<T>. Handle basic synchronous loading of textures via SDL_image. Establish the caching pattern with path-based lookup. Asset directory structure follows canon: `assets/models/` for 3D models (.glb), `assets/textures/` for textures, `assets/audio/` for sound.

**Acceptance Criteria:**
- [ ] load<Texture>(path) loads SDL_Texture and returns handle
- [ ] AssetHandle<T> provides access to loaded asset
- [ ] Same path returns same cached asset (no duplicate loading)
- [ ] Clear error handling for missing/corrupt files
- [ ] Placeholder asset returned for failed loads (pink texture or bright cube)
- [ ] Asset statistics available (loaded count, cache size)
- [ ] Can load glTF 3D models (.glb format)
- [ ] Model loading library added to vcpkg.json (cgltf or tinygltf)
- [ ] Asset paths follow canonical directory structure: models/, textures/, audio/

> **Revised 2026-01-28:** Added canonical asset directory structure (assets/models/, assets/textures/, assets/audio/) per canon v0.5.0 art pipeline and file organization update. (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-002 (needs SDL initialized for texture loading)
- Blocks: 0-009, 0-010, 0-011, 0-012, 0-034 (all asset features build on this)

**Agent Notes:**
- Systems Architect: Generic interface hides implementation details. Textures need SDL_Renderer for creation.
- Game Designer: REQ-12 (graceful missing assets). Pink placeholder is better than crash. "Asset Retrieval Anomaly" in logs.

---

### Ticket 0-009: Asset Reference Counting and Lifetime

**Type:** feature
**System:** AssetManager
**Estimated Scope:** M

**Description:**
Implement reference counting for asset lifetime management. AssetHandle uses RAII pattern - assets are released when all handles are destroyed. Support explicit unload for memory pressure situations.

**Acceptance Criteria:**
- [ ] AssetHandle increments ref count on copy, decrements on destruction
- [ ] Asset unloaded when ref count reaches zero
- [ ] Explicit unload() method forces unload regardless of refs (logs warning if refs exist)
- [ ] Cache can be cleared entirely for memory pressure
- [ ] No dangling pointers when asset is unloaded
- [ ] Memory usage tracking for cache

**Dependencies:**
- Blocked by: 0-008 (needs core AssetManager)
- Blocks: None

**Agent Notes:**
- Systems Architect: Thread-safe reference counting needed for async loading. Consider std::atomic or mutex. Weak references for optional caching.
- Game Designer: N/A (internal implementation)

---

### Ticket 0-010: Async Asset Loading

**Type:** feature
**System:** AssetManager
**Estimated Scope:** L

**Description:**
Implement background thread asset loading using two-phase pattern: file I/O and decoding on background thread, GPU resource creation on main thread. Priority queue allows urgent assets to load first.

**Acceptance Criteria:**
- [ ] Background loader thread reads and decodes files
- [ ] SDL_Surface created on background thread (CPU memory)
- [ ] Main thread creates SDL_Texture from surface (GPU upload)
- [ ] Completion callbacks notify when asset is ready
- [ ] Priority levels (High/Normal/Low) for load ordering
- [ ] Main thread upload budget (max 2ms/frame) to maintain framerate
- [ ] Thread-safe queue for pending loads and completed loads

**Dependencies:**
- Blocked by: 0-008 (needs core AssetManager structure)
- Blocks: None

**Agent Notes:**
- Systems Architect: Per Q009, SDL3 GPU operations must be on main thread. Two-phase pattern is mandatory. Consider std::future for async results.
- Game Designer: REQ-09 (no hard loading screens). Gameplay must continue during loading. Progressive loading enables good reconnection experience.

---

### Ticket 0-011: Asset Loading Progress Indication

**Type:** feature
**System:** AssetManager
**Estimated Scope:** S

**Description:**
Provide progress information for asset loading operations. Track total pending loads, completed loads, and estimated completion. Enable UI to show meaningful loading progress.

**Acceptance Criteria:**
- [ ] Query total pending load count
- [ ] Query completed load count
- [ ] Progress percentage available (completed / total)
- [ ] Optional callback for load progress updates
- [ ] Current loading asset path available (for "Loading..." display)

**Dependencies:**
- Blocked by: 0-010 (async loading must exist to have progress)
- Blocks: None

**Agent Notes:**
- Systems Architect: Simple counters with mutex protection. Don't over-engineer.
- Game Designer: REQ-11 (loading progress indication). "Loading..." with no context feels broken. Show what's happening.

---

### Ticket 0-012: SDL3 Asset Type Support

**Type:** feature
**System:** AssetManager
**Estimated Scope:** M

**Description:**
Add support for common SDL3 asset types beyond textures: audio files (WAV, OGG) via SDL3_mixer or native SDL3 audio, and 3D models (glTF format). Establish patterns for future asset types (fonts, data files). Per canon audio direction, music is user-sourced ambient tracks played as looping playlists with crossfade — consider streaming for large music files rather than full load-into-memory.

**Acceptance Criteria:**
- [ ] load<AudioChunk>(path) loads sound effects (fully loaded into memory)
- [ ] load<Music>(path) loads background music (consider streaming for large files)
- [ ] load<Model>(path) loads 3D models/meshes (glTF/GLB format)
- [ ] Audio loading follows same async pattern as textures
- [ ] Audio decoding on background thread, registration on main thread
- [ ] Music supports playlist/crossfade (canon: user-sourced ambient music)
- [ ] Common formats supported: WAV, OGG for audio; PNG, JPG for textures; glTF/GLB for 3D models

> **Revised 2026-01-28:** Added music streaming consideration and playlist/crossfade support per canon v0.5.0 audio direction. (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-010 (needs async loading pattern)
- Blocks: None

**Agent Notes:**
- Systems Architect: SDL3_mixer may have different threading requirements. Test carefully. May need audio-specific loader.
- Game Designer: Audio foundation enables alien soundscapes. Menu music is the first thing players hear.

---

### Ticket 0-013: Logging Infrastructure

**Type:** infrastructure
**System:** Core
**Estimated Scope:** M

**Description:**
Establish logging infrastructure before any system can fail gracefully. Support log levels (ERROR, WARN, INFO, DEBUG), output to console and file, include timestamps and source location. Use alien terminology in player-facing messages.

**Acceptance Criteria:**
- [ ] Log levels: ERROR, WARN, INFO, DEBUG
- [ ] Log to console (stdout/stderr appropriate to level)
- [ ] Log to file (configurable path)
- [ ] Timestamp and source location in log entries
- [ ] Log level configurable at runtime
- [ ] Macro or function for easy logging: LOG_INFO("message")
- [ ] Server mode log output visible to operators

**Dependencies:**
- Blocked by: 0-001 (build infrastructure)
- Blocks: 0-002 (SDL init should log errors properly)

**Agent Notes:**
- Systems Architect: Consider spdlog or custom lightweight solution. Thread-safe logging required for async operations.
- Game Designer: Error messages should use alien terminology: "Synchronization Anomaly Detected" not "Connection Error".

---

### Ticket 0-014: Configuration System

**Type:** feature
**System:** Core
**Estimated Scope:** M

**Description:**
Establish configuration system for runtime-configurable values using JSON with nlohmann/json library. Support window resolution, asset cache limits, log levels. Platform-specific config file locations. Distinguish between client preferences (mutable, local) and server game parameters (immutable after game creation). Map size is a server game parameter.

**Acceptance Criteria:**
- [ ] nlohmann/json added to vcpkg.json
- [ ] Load configuration from JSON file on startup
- [ ] Configurable: window resolution, fullscreen, vsync
- [ ] Configurable: log level, log file path
- [ ] Configurable: asset cache limits
- [ ] Configurable: UI mode preference (classic | holographic, persisted locally)
- [ ] Server game parameter: map_size (small=128 | medium=256 | large=512, default: medium)
- [ ] Default values used if no config file exists
- [ ] Platform-specific paths: %APPDATA% (Windows), ~/.config (Linux), ~/Library (macOS)

> **Revised 2026-01-28:** Added map_size as server game parameter and UI mode preference as client config per canon v0.5.0 (configurable map sizes + dual UI modes). (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-001 (build infrastructure)
- Blocks: 0-019 (input mapping uses config system)

**Agent Notes:**
- Systems Architect: Keep it simple. Don't over-engineer versioning yet.
- Game Designer: N/A (infrastructure, but enables user preferences)
- Decision record: /plans/decisions/epic-0-config-format.md

---

### Ticket 0-015: SDL Event Polling

**Type:** feature
**System:** InputSystem
**Estimated Scope:** S

**Description:**
Poll all SDL events each frame and route to appropriate handlers. Handle window events (focus, resize, close), quit events, and input events (keyboard, mouse). Establish the event routing architecture.

**Acceptance Criteria:**
- [ ] All SDL events polled each frame
- [ ] Window events routed to Application (resize, focus, close)
- [ ] Quit events trigger graceful shutdown
- [ ] Input events routed to InputSystem
- [ ] Focus loss/gain handled (pause input on focus loss?)
- [ ] Event polling is efficient (no unnecessary copies)

**Dependencies:**
- Blocked by: 0-002 (needs SDL initialized)
- Blocks: 0-016, 0-017 (keyboard and mouse need event polling)

**Agent Notes:**
- Systems Architect: Single event loop in Application, dispatch to subsystems. Consider event queue for deferred processing.
- Game Designer: Focus loss should be graceful. Alt-tabbing shouldn't break things.

---

### Ticket 0-016: Keyboard State Tracking

**Type:** feature
**System:** InputSystem
**Estimated Scope:** S

**Description:**
Track keyboard state with current and previous frame comparison for edge detection. Support pressed (just pressed), held (continuously pressed), and released (just released) states. Handle key combinations (Ctrl+Z, etc.).

**Acceptance Criteria:**
- [ ] Current frame key states tracked
- [ ] Previous frame key states for edge detection
- [ ] IsKeyPressed(key) - true only on frame key went down
- [ ] IsKeyHeld(key) - true while key is down
- [ ] IsKeyReleased(key) - true only on frame key went up
- [ ] Modifier key detection (Ctrl, Alt, Shift)
- [ ] Key combination support (Ctrl+Z, Ctrl+S, etc.)

**Dependencies:**
- Blocked by: 0-015 (needs event polling)
- Blocks: 0-019 (input mapping uses key states)

**Agent Notes:**
- Systems Architect: SDL3 provides key scancode and keycode. Use scancodes for bindings (layout-independent).
- Game Designer: REQ-03 (responsive input). Under 100ms from keypress to visible response.

---

### Ticket 0-017: Mouse State Tracking

**Type:** feature
**System:** InputSystem
**Estimated Scope:** S

**Description:**
Track mouse position (screen coordinates), button states with edge detection, and scroll wheel input. Support cursor confinement to window (optional). World coordinates require RenderingSystem (Epic 2) so defer that.

**Acceptance Criteria:**
- [ ] Mouse position in screen coordinates
- [ ] Button states: left, right, middle with edge detection
- [ ] Scroll wheel delta (up/down, and horizontal if available)
- [ ] IsButtonPressed/Held/Released for each button
- [ ] Cursor show/hide capability
- [ ] Cursor confinement to window (optional setting)
- [ ] Middle-click drag detection for camera panning

**Dependencies:**
- Blocked by: 0-015 (needs event polling), 0-003 (needs window for coordinates)
- Blocks: 0-019 (input mapping uses mouse states)

**Agent Notes:**
- Systems Architect: World coordinates require camera transform - defer to Epic 2. Screen coords are sufficient for Epic 0.
- Game Designer: REQ-06 (zoom feels natural). Scroll wheel needs proper sensitivity. Middle-drag for panning is essential.

---

### Ticket 0-018: Input Context Stack

**Type:** feature
**System:** InputSystem
**Estimated Scope:** M

**Description:**
Implement input context stack for modal input handling. Per Q003 answer, Escape should be contextual: close dialog > deselect tool > cancel input > open menu. Each UI layer can push/pop input contexts.

**Acceptance Criteria:**
- [ ] Input context stack with push/pop operations
- [ ] Each context can consume or pass through input events
- [ ] Escape key routes to topmost context that handles it
- [ ] Contexts have names for debugging
- [ ] Default context always exists (base game input)
- [ ] Context stack state queryable (for UI display)

**Dependencies:**
- Blocked by: 0-016, 0-017 (needs keyboard and mouse input)
- Blocks: None

**Agent Notes:**
- Systems Architect: Stack pattern allows UI layers to capture input. Pass-through enables background input when appropriate.
- Game Designer: Per Q003, Escape is contextual. This architecture enables that pattern. Critical for UX.

---

### Ticket 0-019: Input Mapping System

**Type:** feature
**System:** InputSystem
**Estimated Scope:** M

**Description:**
Abstract input actions (PAN_LEFT, ZOOM_IN, PAUSE_TOGGLE) from physical inputs. Support configurable key/mouse bindings that persist locally. Include default bindings and ability to reset to defaults.

**Acceptance Criteria:**
- [ ] Action enum for all bindable actions (per Q002 answer)
- [ ] Map actions to one or more physical inputs
- [ ] Query action state: IsActionPressed/Held/Released
- [ ] Load bindings from config file
- [ ] Save bindings to config file
- [ ] Reset to defaults function
- [ ] Multiple inputs can trigger same action (W and Up for pan up)

**Dependencies:**
- Blocked by: 0-016, 0-017, 0-014 (needs input states and config system)
- Blocks: None

**Agent Notes:**
- Systems Architect: Per Q011, input mappings are 100% player-local, never synced via server. Use config system for persistence.
- Game Designer: REQ-07 (input mapping flexibility). Per Q004, preserve R/C/I for zones, modernize camera controls. All shortcuts rebindable.

---

### Ticket 0-020: Core Input Actions for Epic 0

**Type:** feature
**System:** InputSystem
**Estimated Scope:** S

**Description:**
Define and implement the minimal input actions needed for Epic 0 testing. Per Q002 answer: camera controls, game flow controls, debug actions, and UI mode toggle.

**Acceptance Criteria:**
- [ ] Camera: PAN_UP/DOWN/LEFT/RIGHT, ZOOM_IN/OUT, ROTATE_LEFT/RIGHT (if rotation supported)
- [ ] Game Flow: PAUSE_TOGGLE, OPEN_MENU, SPEED_1/2/3
- [ ] UI: UI_MODE_TOGGLE (switch between Classic and Holographic UI modes)
- [ ] Debug: TOGGLE_DEBUG_OVERLAY (F3), SCREENSHOT (F12)
- [ ] Default bindings: WASD/arrows for pan, scroll for zoom, Space for pause, Escape for menu
- [ ] Mouse: middle-drag for pan, scroll wheel for zoom

> **Revised 2026-01-28:** Added UI_MODE_TOGGLE action for dual UI mode switching per canon v0.5.0. Handler is a no-op until UISystem (Epic 12) exists. (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-019 (needs input mapping system)
- Blocks: None

**Agent Notes:**
- Systems Architect: This is the minimal viable input set. More actions added in future epics.
- Game Designer: Per Q002, this proves the foundation works. Camera should feel buttery smooth - this is the player's "hands".

---

### Ticket 0-021: Deterministic RNG Utility

**Type:** feature
**System:** Core
**Estimated Scope:** S

**Description:**
Introduce a seeded RNG utility that will be server-controlled. Per canon, all random values come from server. Establish the pattern now even if not heavily used in Epic 0. No rand() calls anywhere.

**Acceptance Criteria:**
- [ ] Seeded RNG class with deterministic output
- [ ] Server provides seed at game start
- [ ] RNG state can be serialized for save/load
- [ ] No std::rand() or rand() anywhere in codebase
- [ ] Convenience methods: random int in range, random float, random bool

**Dependencies:**
- Blocked by: 0-001 (build infrastructure)
- Blocks: None

**Agent Notes:**
- Systems Architect: Use std::mt19937 or PCG. Server sets seed, clients use same seed for prediction if needed later.
- Game Designer: N/A (infrastructure for determinism)

---

### Ticket 0-022: Server CLI Interface

**Type:** feature
**System:** Application
**Estimated Scope:** M

**Description:**
Per Q006 answer, implement basic CLI for server operators. Essential commands: status, players, kick, say, save, shutdown, help. Server should feel empowering to run, not scary. Server accepts --map-size flag at startup for game creation.

**Acceptance Criteria:**
- [ ] Server reads commands from stdin
- [ ] Server accepts --map-size flag at startup (small | medium | large, default: medium)
- [ ] status: shows tick rate, connected players, uptime, map size
- [ ] players: lists connected overseers (player names/IDs)
- [ ] kick <player>: removes player (placeholder until networking)
- [ ] say <message>: broadcasts message (placeholder until networking)
- [ ] save: forces database checkpoint (placeholder)
- [ ] shutdown: graceful shutdown with warning
- [ ] help: lists available commands
- [ ] Periodic heartbeat output so operators know server is alive

> **Revised 2026-01-28:** Added --map-size startup flag and map size display in status per canon v0.5.0 configurable map sizes. (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-006 (needs server mode working)
- Blocks: None

**Agent Notes:**
- Systems Architect: Simple stdin reader with command parsing. Commands are placeholders until Epic 1 (networking).
- Game Designer: Per Q006, server operators are players too. Clear feedback prevents frustration.

---

### Ticket 0-023: PositionComponent Definition

**Type:** feature
**System:** ECS/Components
**Estimated Scope:** S

**Description:**
Define PositionComponent per canon: grid_x, grid_y, elevation (int32_t). Trivially copyable with no pointers. Include serialization support for network sync. Consider Interpolatable wrapper for smooth rendering.

**Acceptance Criteria:**
- [ ] PositionComponent with grid_x, grid_y, elevation as int32_t
- [ ] Trivially copyable (verified with std::is_trivially_copyable)
- [ ] Serialization to/from byte buffer
- [ ] Works with Interpolatable<T> wrapper for visual smoothing
- [ ] GridPosition helper type for convenience
- [ ] SyncPolicy::OnChange metadata (syncs when changed, not every tick)

**Dependencies:**
- Blocked by: 0-025 (needs ECS framework), 0-007 (needs interpolation pattern)
- Blocks: None

**Agent Notes:**
- Systems Architect: First component defined. Establishes patterns for all future components. Serialization is critical for Epic 1.
- Game Designer: N/A (internal data structure)

---

### Ticket 0-024: OwnershipComponent Definition

**Type:** feature
**System:** ECS/Components
**Estimated Scope:** S

**Description:**
Define OwnershipComponent per canon: owner (PlayerID), state (OwnershipState enum), state_changed_at (tick). OwnershipState: Available, Owned, Abandoned, GhostTown. GAME_MASTER (ID=0) is initial owner of all tiles.

**Acceptance Criteria:**
- [ ] OwnershipComponent with owner, state, state_changed_at fields
- [ ] OwnershipState enum: Available, Owned, Abandoned, GhostTown
- [ ] PlayerID type defined (from interfaces.yaml)
- [ ] GAME_MASTER constant = 0 (owns unclaimed tiles)
- [ ] Trivially copyable, serializable
- [ ] SyncPolicy::OnChange metadata

**Dependencies:**
- Blocked by: 0-025 (needs ECS framework)
- Blocks: None

**Agent Notes:**
- Systems Architect: Uses PlayerID from interfaces.yaml. GAME_MASTER = 0 per canon 0.3.0 decisions.
- Game Designer: N/A (internal data structure, but enables tile ownership gameplay)

---

### Ticket 0-025: ECS Framework Integration

**Type:** infrastructure
**System:** ECS
**Estimated Scope:** L

**Description:**
Integrate EnTT as the ECS framework for entity-component management. EnTT is a mature, header-only, high-performance library. Set up component registration, entity creation/destruction, and basic querying.

**Acceptance Criteria:**
- [ ] EnTT added to vcpkg.json
- [ ] EnTT integrated and compiling
- [ ] Entity creation and destruction API
- [ ] Component registration and attachment
- [ ] Component queries (get all entities with ComponentA and ComponentB)
- [ ] Entity ID allocation and recycling
- [ ] Basic usage patterns documented
- [ ] Integration verified with PositionComponent and OwnershipComponent

**Dependencies:**
- Blocked by: 0-001 (build infrastructure for vcpkg/package management)
- Blocks: 0-023, 0-024, 0-026 (components need ECS)

**Agent Notes:**
- Systems Architect: Risk 5 identified this as high-impact decision. EnTT is mature, header-only, high-performance.
- Game Designer: N/A (infrastructure)
- Decision record: /plans/decisions/epic-0-ecs-framework.md

---

### Ticket 0-026: Core Type Definitions

**Type:** feature
**System:** Core
**Estimated Scope:** S

**Description:**
Define core data types from canon interfaces.yaml: EntityID, PlayerID, GridPosition, SimulationTime. These types are used throughout the codebase and in network serialization.

**Acceptance Criteria:**
- [ ] EntityID type (probably uint32_t or uint64_t)
- [ ] PlayerID type (uint8_t per canon, max 4 players)
- [ ] GridPosition struct (grid_x, grid_y)
- [ ] SimulationTime type (uint64_t tick count)
- [ ] All types are trivially copyable
- [ ] Serialization support for all types

**Dependencies:**
- Blocked by: 0-001 (build infrastructure)
- Blocks: 0-023, 0-024, 0-027 (components and messages use these types)

**Agent Notes:**
- Systems Architect: Per interfaces.yaml. These are the vocabulary types for the entire codebase.
- Game Designer: N/A (internal types)

---

### Ticket 0-027: InputMessage Structure

**Type:** feature
**System:** Core/Input
**Estimated Scope:** S

**Description:**
Define InputMessage structure for client-to-server communication. Even without networking (Epic 1), the format must be established. Per Systems Architect analysis, includes player, tick, action type, and target position.

**Acceptance Criteria:**
- [ ] InputMessage struct with player, tick, action, target
- [ ] ActionType enum for all player actions (BUILD, ZONE, etc.)
- [ ] Serializable to byte buffer
- [ ] InputSystem can produce InputMessages from mapped actions
- [ ] Messages queued for network transmission (Epic 1 will consume)

**Dependencies:**
- Blocked by: 0-026 (needs PlayerID, GridPosition types), 0-019 (needs action types from input mapping)
- Blocks: None

**Agent Notes:**
- Systems Architect: Risk 3 mitigation - define message format now without network layer. NetworkManager (Epic 1) serializes and transmits.
- Game Designer: N/A (internal structure for multiplayer)

---

### Ticket 0-028: Serialization Pattern

**Type:** feature
**System:** Core
**Estimated Scope:** M

**Description:**
Establish serialization pattern for components and messages. Per canon, components must be serializable for network sync. Choose approach: manual, reflection, or code-gen. Define ISerializable interface if used.

**Acceptance Criteria:**
- [ ] Serialization pattern documented
- [ ] Binary serialization to/from byte buffer
- [ ] Size calculation before serialization
- [ ] PositionComponent and OwnershipComponent serialize correctly
- [ ] InputMessage serializes correctly
- [ ] Round-trip test: serialize -> deserialize -> compare

**Dependencies:**
- Blocked by: 0-023, 0-024, 0-027 (need things to serialize)
- Blocks: None (but Epic 1 networking depends on this)

**Agent Notes:**
- Systems Architect: Risk 6 - this pattern affects every component. Manual serialization is simplest for MVP. Consider future migration to reflection/code-gen.
- Game Designer: N/A (infrastructure)

---

### Ticket 0-029: Unit Testing Framework

**Type:** infrastructure
**System:** Testing
**Estimated Scope:** S

**Description:**
Integrate unit testing framework for foundation code. Establish testing patterns for systems, components, and utilities. Options: Catch2, GoogleTest, doctest.

**Acceptance Criteria:**
- [ ] Testing framework integrated (recommend Catch2 or doctest for simplicity)
- [ ] Test executable builds and runs
- [ ] Example tests for core utilities (RNG, serialization)
- [ ] Tests can run in CI pipeline (GitHub Actions)
- [ ] Test coverage for critical paths (game loop timing, asset loading)

**Dependencies:**
- Blocked by: 0-001 (build infrastructure)
- Blocks: None (but all tickets benefit from tests)

**Agent Notes:**
- Systems Architect: Open question identified. Testing is crucial for foundation code. Recommend Catch2 for header-only simplicity.
- Game Designer: N/A (infrastructure)

---

### Ticket 0-030: Debug Console Output

**Type:** feature
**System:** Application
**Estimated Scope:** S

**Description:**
Console/terminal debug output showing frame timing, tick rate, and system stats. Toggle with F3 to enable periodic console output. Outputs to stdout for developer monitoring. No visual overlay or text rendering required.

**Acceptance Criteria:**
- [ ] F3 toggles debug output on/off
- [ ] Output to console: FPS, frame time (ms)
- [ ] Output to console: Simulation tick rate, ticks behind
- [ ] Output to console: Asset cache stats (loaded count, memory)
- [ ] Output to console: Entity count
- [ ] Configurable output interval (e.g., every 1 second)
- [ ] Minimal performance impact when enabled

**Dependencies:**
- Blocked by: 0-004 (needs frame timing), 0-008 (needs asset stats)
- Blocks: None

**Agent Notes:**
- Systems Architect: Simple diagnostic tool using console output only. No text rendering dependencies.
- Game Designer: Per Q002, F3 for debug info is expected. Operators need visibility into system health.
- Visual debug overlay deferred to Epic 2 (requires RenderingSystem)

---

### Ticket 0-031: SyncPolicy Metadata Pattern

**Type:** feature
**System:** ECS/Components
**Estimated Scope:** S

**Description:**
Establish component metadata pattern for network synchronization behavior. Components can be EveryTick, OnChange, or Never (client-only). This metadata guides Epic 1 networking.

**Acceptance Criteria:**
- [ ] SyncPolicy enum: EveryTick, OnChange, Never
- [ ] Component metadata storage (trait or registration)
- [ ] PositionComponent marked as OnChange
- [ ] OwnershipComponent marked as OnChange
- [ ] Pattern documented for future components

**Dependencies:**
- Blocked by: 0-025 (needs ECS framework)
- Blocks: None (Epic 1 will use this)

**Agent Notes:**
- Systems Architect: Prepares for Epic 1 delta sync. Most components are OnChange. EveryTick reserved for rapidly changing state.
- Game Designer: N/A (infrastructure for multiplayer)

---

### Ticket 0-032: Application Shutdown and Cleanup

**Type:** feature
**System:** Application
**Estimated Scope:** S

**Description:**
Graceful shutdown handling for both client and server modes. Save any pending state, clean up resources, notify connected clients (placeholder for Epic 1). Exit confirmation for clients with unsaved state.

**Acceptance Criteria:**
- [ ] Clean shutdown on window close or Escape->Quit
- [ ] Server shutdown warns connected players (placeholder message)
- [ ] Asset cache properly cleaned up
- [ ] SDL subsystems shutdown in reverse order
- [ ] No memory leaks on shutdown (verify with sanitizers)
- [ ] Exit confirmation dialog for clients (simple placeholder)

**Dependencies:**
- Blocked by: 0-002, 0-005 (needs SDL and state management)
- Blocks: None

**Agent Notes:**
- Systems Architect: Proper cleanup prevents resource leaks. Important for development iteration.
- Game Designer: REQ-16 (exit confirmation). Single confirmation, not multiple dialogs. Respect player's time.

---

### Ticket 0-033: Integration Smoke Test

**Type:** task
**System:** All Epic 0 Systems
**Estimated Scope:** M

**Description:**
Create a minimal integration test that verifies all Epic 0 systems work together correctly. This serves as a foundation health check before proceeding to Epic 1.

**Acceptance Criteria:**
- [ ] Window opens with correct title and dimensions
- [ ] Game loop runs at 20 ticks/second with stable frame timing
- [ ] Input system captures keyboard and mouse events
- [ ] Asset manager can load a test texture, 3D model (.glb), and audio file
- [ ] Asset paths resolve from canonical directories (assets/models/, assets/textures/, assets/audio/)
- [ ] State machine transitions between Menu, Playing, and Paused
- [ ] Server mode runs headless with --map-size flag without crashing
- [ ] Clean shutdown with no resource leaks

> **Revised 2026-01-28:** Updated asset loading test to include 3D model (.glb) and canonical directory paths. Added --map-size flag to server mode test. (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-006, 0-011, 0-017, 0-022 (all core systems)
- Blocks: Epic 1 (foundation must be solid)

**Agent Notes:**
- Systems Architect: Critical for validating the foundation before building on it
- Game Designer: Verifies the "feel" basics - timing and responsiveness

---

### Ticket 0-034: 3D Model Loading Support

**Type:** feature
**System:** AssetManager
**Estimated Scope:** M

**Description:**
Add support for loading 3D models in glTF format. This is required for Epic 2's 3D rendering system and the hybrid art pipeline (procedural + hand-modeled assets). Use a lightweight glTF loading library (cgltf recommended - single header, public domain). Models loaded from canonical directory: `assets/models/`.

**Acceptance Criteria:**
- [ ] cgltf (or tinygltf) added to project dependencies
- [ ] AssetManager can load .gltf and .glb files from assets/models/
- [ ] Loaded models expose: vertices, indices, normals, UVs
- [ ] Texture references in models are resolved via AssetManager (assets/textures/)
- [ ] Models are cached like other assets

> **Revised 2026-01-28:** Added canonical asset directory paths (assets/models/, assets/textures/) and hybrid art pipeline context per canon v0.5.0. (trigger: canon-update v0.5.0)

**Dependencies:**
- Blocked by: 0-008 (Asset Loading Interface)
- Blocks: Epic 2 (3D rendering requires models)

**Agent Notes:**
- Systems Architect: Required for 3D rendering. cgltf is lightweight and header-only.
- Decision record: /plans/decisions/3d-rendering-architecture.md

---

## Ticket Summary by Category

### Infrastructure/Setup (Priority 1)
- 0-001: Project Build Infrastructure
- 0-013: Logging Infrastructure
- 0-014: Configuration System
- 0-025: ECS Framework Integration
- 0-029: Unit Testing Framework

### Application System (Priority 2)
- 0-002: SDL3 Initialization and Shutdown
- 0-003: Window Management
- 0-004: Main Game Loop with Fixed Timestep
- 0-005: Application State Management
- 0-006: Server vs Client Mode
- 0-007: Interpolation Pattern
- 0-022: Server CLI Interface
- 0-030: Debug Overlay Foundation
- 0-032: Application Shutdown and Cleanup

### AssetManager System (Priority 3)
- 0-008: AssetManager Core Interface
- 0-009: Asset Reference Counting and Lifetime
- 0-010: Async Asset Loading
- 0-011: Asset Loading Progress Indication
- 0-012: SDL3 Asset Type Support
- 0-034: 3D Model Loading Support

### InputSystem (Priority 3)
- 0-015: SDL Event Polling
- 0-016: Keyboard State Tracking
- 0-017: Mouse State Tracking
- 0-018: Input Context Stack
- 0-019: Input Mapping System
- 0-020: Core Input Actions for Epic 0

### Core Types and Components (Priority 4)
- 0-021: Deterministic RNG Utility
- 0-026: Core Type Definitions
- 0-023: PositionComponent Definition
- 0-024: OwnershipComponent Definition
- 0-027: InputMessage Structure
- 0-028: Serialization Pattern
- 0-031: SyncPolicy Metadata Pattern

## Critical Path

The minimal path to a working foundation:

```
0-001 (Build)
  └─> 0-013 (Logging)
  └─> 0-025 (ECS Framework)
  └─> 0-002 (SDL Init)
        └─> 0-003 (Window)
              └─> 0-004 (Game Loop)
                    └─> 0-005 (State Mgmt)
        └─> 0-015 (Event Polling)
              └─> 0-016 (Keyboard)
              └─> 0-017 (Mouse)
        └─> 0-008 (AssetManager Core)
```

This critical path delivers: a window, a running game loop, basic input handling, and the ability to load assets. Everything else builds on this foundation.

---

## Ticket Classification (2026-01-28)

| Ticket | Status | Notes |
|--------|--------|-------|
| 0-001 | UNCHANGED | |
| 0-002 | UNCHANGED | |
| 0-003 | UNCHANGED | |
| 0-004 | UNCHANGED | |
| 0-005 | UNCHANGED | |
| 0-006 | UNCHANGED | |
| 0-007 | UNCHANGED | |
| 0-008 | MODIFIED | Added canonical asset directory structure (models/, textures/, audio/) |
| 0-009 | UNCHANGED | |
| 0-010 | UNCHANGED | |
| 0-011 | UNCHANGED | |
| 0-012 | MODIFIED | Added music streaming consideration and playlist/crossfade support |
| 0-013 | UNCHANGED | |
| 0-014 | MODIFIED | Added map_size server parameter and UI mode preference |
| 0-015 | UNCHANGED | |
| 0-016 | UNCHANGED | |
| 0-017 | UNCHANGED | |
| 0-018 | UNCHANGED | |
| 0-019 | UNCHANGED | |
| 0-020 | MODIFIED | Added UI_MODE_TOGGLE action for dual UI modes |
| 0-021 | UNCHANGED | |
| 0-022 | MODIFIED | Added --map-size startup flag and status display |
| 0-023 | UNCHANGED | |
| 0-024 | UNCHANGED | |
| 0-025 | UNCHANGED | |
| 0-026 | UNCHANGED | |
| 0-027 | UNCHANGED | |
| 0-028 | UNCHANGED | |
| 0-029 | UNCHANGED | |
| 0-030 | UNCHANGED | |
| 0-031 | UNCHANGED | |
| 0-032 | UNCHANGED | |
| 0-033 | MODIFIED | Updated to test 3D model loading and canonical paths |
| 0-034 | MODIFIED | Added canonical directory paths and hybrid pipeline context |
