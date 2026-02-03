# Game Designer Analysis: Epic 0

**Epic:** Project Foundation (Application, AssetManager, InputSystem)
**Agent:** Game Designer
**Date:** 2026-01-25

---

## Experience Requirements

Foundation work is invisible when done right - and painfully obvious when done wrong. These requirements ensure the foundation *feels* good before any gameplay even begins.

### Game Loop Feel

- [ ] **REQ-01: Buttery 60 FPS target** - Simulation and rendering must maintain consistent frame timing. Frame drops feel "cheap" and undermine player trust in the simulation.
- [ ] **REQ-02: Consistent tick rate feel** - The 20 tick/second simulation rate (per canon) should feel smooth. Beings and structures should never "teleport" or stutter between states.
- [ ] **REQ-03: Responsive input acknowledgment** - Under 100ms from keypress to visible response. Building placement, camera movement, and UI interaction must feel instant.
- [ ] **REQ-04: No "dead frames"** - Even during heavy processing, something should move/animate. A frozen screen, even for 500ms, feels like a crash.

### Input Experience

- [ ] **REQ-05: Camera movement feels "gliding"** - Camera pan should have subtle acceleration/deceleration, not hard start/stop. Makes exploring the colony feel pleasant.
- [ ] **REQ-06: Zoom feels natural** - Mouse wheel zoom should center on cursor position, not screen center. Zoom speed should feel proportional to current zoom level.
- [ ] **REQ-07: Input mapping flexibility** - Support for remapping keys early. Accessibility matters from day one.
- [ ] **REQ-08: Multi-input support** - Keyboard+mouse as primary, but gamepad support consideration for couch co-op scenarios (2-4 player local possibility?).

### Loading Experience

- [ ] **REQ-09: No hard loading screens** - Async asset loading must allow gameplay to continue. Loading a new area shouldn't freeze the game.
- [ ] **REQ-10: Progressive detail loading** - Load essential assets first (terrain, nearby structures), detail assets later. Players see something immediately.
- [ ] **REQ-11: Loading progress indication** - When loading is unavoidable (game start, reconnection), show meaningful progress. "Loading..." with no context feels broken.
- [ ] **REQ-12: Graceful missing assets** - If an asset fails to load, show a placeholder (bright pink cube of shame?) rather than crash. Players shouldn't lose progress to a missing texture.

### State Transitions

- [ ] **REQ-13: Menu-to-game feels like "arrival"** - Starting a game should feel like entering your colony. Consider a brief animated transition rather than hard cut.
- [ ] **REQ-14: Pause maintains atmosphere** - Pause screen should dim but not hide the colony. Players want to admire their creation even while paused.
- [ ] **REQ-15: Reconnection feels seamless** - When reconnecting to server, player should feel like they're "back" not "starting over." Show their colony during reconnect if possible.
- [ ] **REQ-16: Exit confirmation** - Unsaved state warning, but not annoying. Single confirmation, not multiple dialogs.

### Performance Feel

- [ ] **REQ-17: No "warming up" period** - Game should feel responsive immediately after launch, not sluggish for the first 30 seconds.
- [ ] **REQ-18: Predictable performance** - Same actions should take the same time. Variable frame times feel unreliable.

---

## Questions for Other Agents

### For Systems Architect

- @systems-architect: For REQ-02 (smooth tick interpolation), how should we handle the visual interpolation between 20hz simulation ticks and 60fps rendering? Should entities store previous/current state for lerping, or should this be a rendering-layer concern?

- @systems-architect: For REQ-04 (no dead frames), should we implement a "minimum render" guarantee where basic animation/camera continues even if simulation is lagging? What's the architecture for decoupling render from simulation?

- @systems-architect: Regarding REQ-09 (async loading), how does the AssetManager integrate with SDL3's threading model? Are there SDL3-specific considerations for loading textures on background threads?

- @systems-architect: For state management, is there a canonical pattern for "game states" (menu, playing, paused) that allows partial simulation (e.g., visual-only updates during pause)?

- @systems-architect: How should input mapping persistence work? Should it be player-local config, or synced via server for consistent experience across machines?

- @systems-architect: For REQ-15 (reconnection), what does the client receive during reconnection? Can we get enough state to render the colony before full sync completes?

---

## Risks & Concerns

### Technical Risks Affecting Experience

1. **Frame timing variance** - If frame timing is inconsistent early, it becomes hard to fix later. Everything built on top inherits the jitter. This must be rock-solid from Epic 0.

2. **Input latency hidden by complexity** - Early prototypes feel responsive because there's nothing happening. As systems pile on, input latency can creep up. Need measurement infrastructure from the start.

3. **Async loading complexity** - Async loading is easy to get wrong. Race conditions, main-thread stalls waiting for assets, memory spikes from loading too much at once. This system needs careful design.

4. **State machine complexity** - Game state management (menu/play/pause) can become spaghetti fast. Need clean architecture that supports future states (multiplayer lobby, settings screens, etc.).

### Player Experience Risks

5. **"Unfinished" feel from placeholder systems** - Even Epic 0 will be seen by players (dev builds, playtests). Rough edges in foundation create negative first impressions that persist. Consider polish budget even for foundation.

6. **Multiplayer desync perception** - With 20hz ticks and no client prediction (per canon), there will be a 50-100ms+ delay on all actions. This is fine for city builder, but we need to ensure visual feedback is instant even if simulation update isn't.

7. **Loading state uncertainty** - Players don't know if loading is taking long or if the game hung. Progress indicators aren't optional.

---

## Feel Guidelines

### How the Foundation Should FEEL

**The Invisible Foundation**
When playing ZergCity, nobody should think about Epic 0. The game loop, asset loading, and input handling should be completely invisible - like air. You don't notice air until it's gone.

**Responsive Like a Well-Oiled Machine**
Every input should have immediate visual acknowledgment. Click a button? It depresses instantly. Move the camera? It moves with your hand. Place a structure? The ghost preview tracks your cursor perfectly. Even if the *simulation* has latency (multiplayer), the *feedback* must be instant.

**Smooth Like Water**
Camera movement, scrolling, zooming - all should feel fluid. No stutters, no hitches, no sudden jumps. City builders are about long sessions of gradual construction. Jerky movement causes eye strain and breaks immersion.

**Confident Like a Professional Tool**
Loading should feel purposeful, not anxious. Progress indicators should advance steadily. Transitions should feel intentional. The game should feel like a robust, professional application - not a prototype that might crash.

**Welcoming, Not Intimidating**
State transitions (menu to game, game to pause) should feel like moving between comfortable rooms, not crossing technical boundaries. The player is an Overseer surveying their domain - the tool should get out of the way.

---

## Early Theme Considerations

Even at the foundation level, we can plant seeds for the alien theme.

### Terminology in System Messages

- Use "Initializing Systems..." not "Loading..."
- Use "Connecting to Colony Network" not "Connecting to Server"
- Use "Session Paused" or "Simulation Suspended" not just "Paused"
- Error messages should feel alien: "Synchronization Anomaly Detected" vs "Connection Error"

### Visual Identity Seeds

- **Loading indicators**: Consider alien-themed loading animations from the start. A pulsing hexagonal pattern? Growing crystalline structure? Don't just use a spinning circle.
- **Color palette groundwork**: Foundation should establish the primary/secondary color variables that will carry the alien theme. Even placeholder UI should use theme-appropriate colors.
- **Cursor design consideration**: The mouse cursor is always visible. An alien-themed cursor (even simple) sets tone immediately.

### Audio Foundation

- The audio system should be ready for ambient alien soundscapes
- Menu music should be considered early - it's the first thing players hear
- UI sounds (click, hover) should feel "alien" not generic Windows/Mac sounds

### State Transition Atmosphere

- Menu-to-game transition could show "descent to colony" animation later
- Pause could have subtle ambient effects (faint pulse, distant sounds) rather than dead silence
- These hooks should be in place even if not implemented in Epic 0

---

## Multiplayer Experience Notes

Even though Epic 0 is foundational, multiplayer considerations affect these systems directly.

### Connection Experience

- **First connection** should feel like entering a shared world. Other players should be visible/acknowledged immediately, not a surprise discovery.
- **Server browser / lobby** (if applicable) should feel social. Show player colonies, activity indicators.
- **Connection failure** should be graceful. "The colony network is currently unreachable" not a crash or frozen screen.

### Input Fairness

- All players should have equivalent input latency to the server. Local host shouldn't have an advantage.
- Input mapping should support diverse setups - one player on laptop, one on gaming PC with controller.

### State Synchronization Feel

- When another player builds something, it should appear smoothly - not pop in suddenly
- Player join/leave should be communicated clearly but not disruptively
- Server state should feel like "ground truth" that all players share, not separate worlds stitched together

### Social Presence Hooks

- Foundation should support player presence indicators (who's connected, who's active)
- Chat system needs input infrastructure (text entry, display)
- Consider "ping" or "look here" feature - needs input binding support

### Reconnection as Social Feature

- When reconnecting, ideally show "Returning to Colony..." with player's own territory visible
- Other players might see "[Overseer Name] has returned" - reconnection is a social event
- Reconnection should restore camera position and UI state if possible

---

## Summary

Epic 0 is about building a foundation that *disappears*. Players should never notice the game loop, asset loading, or input systems - they should just feel the colony. The foundation must be rock-solid, responsive, and smooth from day one, because everything else builds on top.

Key experience priorities:
1. **Frame timing consistency** - This affects everything
2. **Input responsiveness** - Players must feel in control
3. **Async loading without hitches** - Modern players expect it
4. **Graceful state transitions** - Polish even at foundation level

The alien theme can begin subtly in Epic 0 through terminology, color choices, and loading animations. These small touches accumulate into a cohesive identity.

For multiplayer, the foundation must treat network play as primary, not secondary. All timing, loading, and state management should assume multiple connected clients from the start.
