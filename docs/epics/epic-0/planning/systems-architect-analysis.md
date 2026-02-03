# Systems Architect Analysis: Epic 0

**Epic:** Project Foundation
**Analyst:** Systems Architect
**Date:** 2026-01-25
**Canon Version Referenced:** 0.3.0

---

## Overview

Epic 0 establishes the three core non-ECS systems that all future epics depend on:
- **Application** - SDL3 lifecycle, window, game loop, frame timing, state management
- **AssetManager** - Asset loading, caching, async queue, lifetime management
- **InputSystem** - SDL event polling, keyboard/mouse state, input mapping

Additionally, two cross-cutting components are introduced:
- **PositionComponent** - Grid-based positioning for all spatial entities
- **OwnershipComponent** - Player ownership tracking with Game Master model

---

## Key Work Items

### Application System

- [ ] **APP-01: SDL3 Initialization/Shutdown**
  - Initialize SDL3 with video, audio, and events subsystems
  - Proper cleanup on shutdown (reverse order of init)
  - Error handling for SDL failures

- [ ] **APP-02: Window Management**
  - Create SDL3 window with configurable resolution
  - Handle resize events
  - Fullscreen/windowed toggle support
  - Expose window handle for RenderingSystem (Epic 2)

- [ ] **APP-03: Main Game Loop**
  - Fixed timestep simulation loop (50ms = 20 ticks/sec per canon)
  - Variable timestep rendering (uncapped or vsync)
  - Accumulator-based tick scheduling
  - Frame timing and delta time calculation

- [ ] **APP-04: State Management**
  - Application states: Menu, Playing, Paused
  - State transitions with proper enter/exit hooks
  - State-dependent system activation (pause should stop simulation tick)

- [ ] **APP-05: Server vs Client Mode Detection**
  - Application must know if it's running as server or client
  - Server: No window, no rendering, headless operation
  - Client: Full window, rendering, input handling

### AssetManager System

- [ ] **ASSET-01: Asset Loading Interface**
  - Generic `load<T>(path)` interface returning AssetHandle<T>
  - Support for textures, sprites, audio files
  - Async loading capability with completion callbacks

- [ ] **ASSET-02: Asset Caching**
  - Cache loaded assets by path
  - Reference counting for lifetime management
  - Configurable cache size limits

- [ ] **ASSET-03: Async Loading Queue**
  - Background thread for asset loading
  - Priority queue (some assets more urgent)
  - Thread-safe completion notification to main thread

- [ ] **ASSET-04: Asset Lifetime Management**
  - RAII-based handles that auto-release
  - Explicit unload capability for memory pressure
  - Asset hot-reloading support (development feature)

- [ ] **ASSET-05: SDL3 Asset Type Support**
  - SDL_Texture loading via SDL_image
  - SDL_Audio loading via SDL_mixer (or SDL3 native audio)
  - Potential future: Font loading

### InputSystem

- [ ] **INPUT-01: SDL Event Polling**
  - Poll all SDL events each frame
  - Route events to appropriate handlers (input, window, quit)
  - Handle window focus gain/loss (affects input state)

- [ ] **INPUT-02: Keyboard State Tracking**
  - Current frame key states (pressed, held, released)
  - Previous frame comparison for edge detection
  - Support for key combinations (Ctrl+Z, etc.)

- [ ] **INPUT-03: Mouse State Tracking**
  - Mouse position (screen and world coordinates - latter needs RenderingSystem)
  - Button states with edge detection
  - Mouse wheel support
  - Cursor confinement to window (optional)

- [ ] **INPUT-04: Input Mapping System**
  - Abstract actions (PAN_LEFT, ZOOM_IN, BUILD_ROAD)
  - Configurable key/mouse bindings
  - Rebindable at runtime (for settings menu)
  - Default bindings

- [ ] **INPUT-05: Input for Client-Server**
  - InputSystem is CLIENT-ONLY (server has no input)
  - Input generates InputMessages for network transmission
  - Local input buffering before server confirmation

### Cross-Cutting Components

- [ ] **COMP-01: PositionComponent Definition**
  - Per canon: `grid_x`, `grid_y`, `elevation` (int32_t)
  - Trivially copyable (no pointers)
  - Serialization support for network sync

- [ ] **COMP-02: OwnershipComponent Definition**
  - Per canon: `owner` (PlayerID), `state` (OwnershipState), `state_changed_at` (tick)
  - OwnershipState enum: Available, Owned, Abandoned, GhostTown
  - GAME_MASTER = 0 as initial owner of all tiles
  - Serialization support for network sync

### ECS Integration Foundation

- [ ] **ECS-01: Component Registry Design**
  - How will components register themselves?
  - Component ID assignment (compile-time or runtime?)
  - Component storage strategy (SoA vs AoS, sparse vs dense)

- [ ] **ECS-02: Entity Management Design**
  - Entity ID allocation and recycling
  - Entity creation/destruction API
  - Entity-Component relationship storage

---

## Questions for Other Agents

### @game-designer

- **Q1:** What application states beyond Menu/Playing/Paused do we need? Canon mentions "immortalize" for freezing cities - is that a separate state?

- **Q2:** For input mapping, what are the core actions we need to support in Epic 0? The minimal set needed before UI (Epic 12) exists?

- **Q3:** Should the escape key always pause/unpause, or should that be a game design decision that varies by context?

- **Q4:** Are there any specific keyboard shortcuts from SimCity 2000 you want to preserve for nostalgia/familiarity?

- **Q5:** When a player is in Paused state in multiplayer, does that pause only their view (simulation continues) or request a global pause (requires all players to agree)?

- **Q6:** For the server-only mode, should there be any console/CLI interface for server operators, or is it purely headless until we add admin tools later?

---

## Risks & Concerns

### Risk 1: Fixed Timestep + Variable Render Interpolation
**Severity:** Medium
**Description:** The 50ms fixed tick (20 Hz) is quite coarse for smooth rendering. We need interpolation between simulation states for smooth visuals. This adds complexity to all systems that render.
**Mitigation:** Define the interpolation pattern early in Epic 0. All renderable components need previous+current state for lerping.

### Risk 2: Async Asset Loading Thread Safety
**Severity:** Medium
**Description:** Async loading introduces threading complexity. Main thread may try to use assets that are still loading. Reference counting across threads is error-prone.
**Mitigation:** Clear ownership rules - loading thread never touches game state, only asset cache. Use std::atomic or mutex for cache access. AssetHandle pattern hides complexity.

### Risk 3: Input System Coupling to Network
**Severity:** Low (but design impact)
**Description:** InputSystem must produce InputMessages for the network layer (Epic 1). But InputSystem is Epic 0, NetworkManager is Epic 1. We need to define the message format now without the network layer.
**Mitigation:** Define InputMessage as a data structure in Epic 0. NetworkManager (Epic 1) will serialize and transmit it. Avoids circular dependency.

### Risk 4: Headless Server Mode
**Severity:** Medium
**Description:** Server must run without SDL window/renderer. SDL3 may still require initialization for audio if server plays sounds (unlikely but possible). Need conditional compilation or runtime branching.
**Mitigation:** Application detects mode at startup. Server mode skips window creation, RenderingSystem, InputSystem. May need SDL_INIT_NOPARACHUTE or similar flags.

### Risk 5: ECS Library vs Custom
**Severity:** High (blocking decision)
**Description:** Canon says "custom engine" but doesn't specify ECS implementation. Options: EnTT, flecs, custom. This decision affects ALL subsequent epics.
**Mitigation:** Need explicit decision in `/plans/decisions/` before starting implementation. Recommend EnTT for maturity and performance, but this is a technical decision.

### Risk 6: Component Serialization Strategy
**Severity:** Medium
**Description:** PositionComponent and OwnershipComponent need serialization for network sync (Epic 1). The serialization approach (reflection, manual, code-gen) affects every component.
**Mitigation:** Define serialization pattern in Epic 0 even though network uses it in Epic 1. Components should implement ISerializable interface from canon.

---

## Dependencies

### Epic 0 Depends On
- **Nothing** - This is the foundation epic with no dependencies

### Epic 0 Blocks
- **Epic 1 (Multiplayer)** - NetworkManager needs Application running, InputMessages defined
- **Epic 2 (Rendering)** - RenderingSystem needs window handle from Application, AssetManager for textures
- **Epic 3 (Terrain)** - TerrainSystem uses PositionComponent defined here
- **All subsequent epics** - ECS foundation (components, entity management) affects everything

### Internal Dependencies (within Epic 0)
```
APP-01 (SDL Init) --> APP-02 (Window) --> APP-03 (Game Loop)
                                      --> APP-04 (State Mgmt)
                                      --> APP-05 (Server Mode)

APP-01 (SDL Init) --> ASSET-01 (Loading) --> ASSET-02 (Caching)
                                         --> ASSET-03 (Async)
                                         --> ASSET-05 (SDL Types)

APP-01 (SDL Init) --> INPUT-01 (Event Polling) --> INPUT-02 (Keyboard)
       |                                       --> INPUT-03 (Mouse)
       |                                       --> INPUT-04 (Mapping)
       v
APP-02 (Window) --> INPUT-03 (Mouse screen coords need window)

COMP-01 (Position) --> No dependencies
COMP-02 (Ownership) --> No dependencies, but uses PlayerID from interfaces.yaml

ECS-01, ECS-02 --> Required before any component can be instantiated
```

---

## System Interaction Map

```
Epic 0 Systems Internal:
+------------------+
|   Application    |
|  (SDL3 Lifecycle)|
+--------+---------+
         |
         | provides window handle
         v
+------------------+     +------------------+
|   InputSystem    |     |   AssetManager   |
| (SDL Events)     |     | (Load/Cache)     |
+--------+---------+     +--------+---------+
         |                        |
         | InputMessages          | AssetHandle<T>
         v                        v
    [To Epic 1]              [To Epic 2]
    NetworkManager           RenderingSystem

Epic 0 Components:
+-------------------+     +--------------------+
| PositionComponent |     | OwnershipComponent |
| (grid_x, y, elev) |     | (owner, state)     |
+-------------------+     +--------------------+
         |                         |
         | Used by                 | Used by
         v                         v
    [All Spatial              [All Owned
     Systems]                  Entities]
```

### Data Flow

1. **Frame Start**
   - Application updates frame timing
   - InputSystem polls SDL events
   - AssetManager processes async load completions

2. **Simulation Tick** (every 50ms)
   - Application triggers tick for all ISimulatable systems
   - Systems read/write components
   - (Future: SyncSystem packages deltas for network)

3. **Render Frame** (variable rate)
   - (Future: RenderingSystem interpolates between ticks)
   - (Future: UISystem renders overlays)

4. **Frame End**
   - InputSystem clears edge-detection flags
   - Application handles quit events

---

## Multiplayer Considerations

Even though Epic 0 predates Epic 1 (Multiplayer), these design decisions affect multiplayer:

### 1. Client vs Server Executable
**Decision needed:** Single executable with mode flag, or separate client/server executables?

**Recommendation:** Single executable with `--server` flag. Simplifies build, deployment, and development. Application detects mode and conditionally initializes subsystems.

### 2. Simulation Tick Authority
**Canon states:** Server runs simulation at 20 ticks/sec. Clients receive state updates.

**Epic 0 implication:** The game loop must support both:
- **Server mode:** Run simulation ticks, no rendering
- **Client mode:** Receive simulation state, interpolate for rendering

The Application's tick function signature should anticipate this. Simulation tick is separate from render frame.

### 3. Input Message Format
**Canon states:** InputMessages go from client to server.

**Epic 0 must define:** The InputMessage structure, even without network layer:
```cpp
struct InputMessage {
    PlayerID player;
    uint64_t tick;           // When client sent this
    ActionType action;       // Abstract action (BUILD, ZONE, etc.)
    GridPosition target;     // Where action applies
    // ... action-specific data
};
```

### 4. Component Sync Flags
**Canon states:** Components must be serializable, some sync every tick, some on change.

**Epic 0 should define:** Component metadata pattern for sync behavior:
- `SyncPolicy::EveryTick`
- `SyncPolicy::OnChange`
- `SyncPolicy::Never` (client-only state)

PositionComponent and OwnershipComponent should both be `SyncPolicy::OnChange`.

### 5. Deterministic Simulation
**Canon states:** All random values from server (seeded RNG).

**Epic 0 implication:** No `rand()` calls. Introduce a seeded RNG utility that will be server-controlled. Even if we don't use it in Epic 0, establish the pattern.

### 6. Time Representation
**Canon states:** Simulation time progresses in ticks.

**Epic 0 must define:**
- `SimulationTime` type (probably uint64_t tick count)
- `GameTime` for in-game calendar (cycles/phases in alien terminology)
- Conversion between real time and simulation time

---

## Architectural Recommendations

### 1. Use EnTT for ECS
EnTT is a mature, header-only, high-performance ECS library for C++. It provides:
- Component storage (sparse set based)
- Views/queries for component iteration
- Signals/events
- Extensive documentation

Unless there's a strong reason for custom ECS, EnTT reduces risk significantly.

### 2. Define Core Interfaces Early
Even though Epic 1+ implements them, define these in Epic 0:
- `ISerializable` - For component network sync
- `ISimulatable` - For tick-based systems (used in game loop)
- Data types: `GridPosition`, `PlayerID`, `EntityID` (from canon)

### 3. Establish Logging Infrastructure
Before any system can fail gracefully, we need logging. Recommend:
- Log levels per canon (ERROR, WARN, INFO, DEBUG)
- Log to console and file
- Include timestamps and source location

### 4. Configuration System
Application, AssetManager, and InputSystem all have configurable values:
- Window resolution
- Asset cache size
- Key bindings

Establish a simple config system (INI, JSON, or YAML) in Epic 0.

---

## Open Questions Requiring Decisions

1. **ECS Library:** EnTT, flecs, or custom? Needs `/plans/decisions/` record.
2. **Build System:** CMake confirmed, but what about package management (vcpkg already in repo)?
3. **SDL3 Version:** SDL3 is still evolving. Pin a specific version?
4. **Asset Formats:** PNG/JPG for textures? WAV/OGG for audio? Establish conventions.
5. **Unit Testing Framework:** Catch2, GoogleTest, doctest? Testing is crucial for foundation code.

---

## Summary

Epic 0 is deceptively complex. While it's "just" SDL init, asset loading, and input handling, the decisions made here echo through all 17 epics. Key points:

1. **Fixed timestep simulation with variable render requires interpolation pattern**
2. **Headless server mode must be considered from the start**
3. **Component serialization pattern affects all future components**
4. **ECS library choice is a blocking decision**
5. **Input produces messages that Epic 1 will transmit - define format now**

The work items above are sequenced by dependency. Start with SDL init, then branch into Application, AssetManager, and InputSystem in parallel. Components can be defined once ECS foundation exists.
