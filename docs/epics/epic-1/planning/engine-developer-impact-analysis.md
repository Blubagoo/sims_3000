# Engine Developer Impact Analysis: Epic 1

**Epic:** Multiplayer Networking
**Analyst:** Engine Developer
**Date:** 2026-01-28
**Trigger:** Canon update v0.3.0 -> v0.5.0

---

## Trigger
Canon update v0.3.0 -> v0.5.0

## Overall Impact: LOW

## Impact Summary
The canon v0.5.0 changes are overwhelmingly visual, artistic, and UI-related. Epic 1 is the networking layer -- it serializes component data, synchronizes ECS state, and manages connections. The one area that requires attention is **configurable map sizes** (128x128 to 512x512), which directly affects snapshot size estimates and bandwidth calculations in the original analysis. All other changes (art direction, 3D models, shading, audio, UI modes, terminology) are rendering/client-side concerns that do not touch the networking wire protocol or synchronization architecture.

---

## Item-by-Item Assessment

### UNAFFECTED

- **NetworkManager Core (NET-01 through NET-07)** -- Reason: Socket connections, message serialization, ENet integration, threading model, and lobby management are protocol-level concerns. Art direction, model format, terrain visuals, audio, and UI modes have zero impact on the networking transport layer.

- **Message Serialization Framework (NET-04)** -- Reason: Binary message format (header with type/length/sequence) is independent of what is being rendered. The wire format serializes component data, not visual assets.

- **Message Types (NET-05, tickets 1-005, 1-006)** -- Reason: Client-to-server and server-to-client message types (InputMessage, StateUpdateMessage, FullStateMessage, etc.) are unchanged. The canon delta adds no new network message requirements.

- **Network Threading Model (NET-07, ticket 1-004)** -- Reason: Dedicated network I/O thread with lock-free SPSC queues is unaffected by visual or asset pipeline changes.

- **SyncSystem Change Detection (SYNC-01, ticket 1-012)** -- Reason: EnTT dirty flag tracking via signals is component-agnostic. New terrain types or building templates may add new components in future epics, but they will follow the same existing sync patterns.

- **SyncSystem Delta Generation/Application (SYNC-02/03, ticket 1-013)** -- Reason: Delta serialization operates on changed components regardless of their content. More terrain types or building templates do not change the delta mechanism.

- **Entity ID Synchronization (SYNC-06, ticket 1-015)** -- Reason: Monotonic uint64 counter is unaffected by visual changes.

- **Game Loop Integration (INT-01 through INT-04, ticket 1-017)** -- Reason: Poll/process/tick/send cycle is independent of rendering pipeline changes.

- **Server Executable Structure (SRV-01 through SRV-03, ticket 1-008)** -- Reason: Server is headless -- it never renders models, applies shaders, or plays audio. Art pipeline changes are irrelevant to server execution.

- **Connection Error Handling (ticket 1-018)** -- Reason: Malformed message handling, rate limiting, timeout detection are protocol-level. Unaffected.

- **Player Session Management (ticket 1-010)** -- Reason: PlayerID assignment, session tokens, reconnection grace periods are unaffected by visual changes.

- **Input Message Transmission (ticket 1-011)** -- Reason: Player actions (build, zone, demolish) are transmitted as abstract InputMessages. The visual representation of those actions is client-side only.

- **IPersistenceProvider Interface (ticket 1-016)** -- Reason: Persistence abstraction is independent of rendering or asset pipeline.

- **Testing Infrastructure (tickets 1-019 through 1-022)** -- Reason: MockSocket, TestHarness, serialization tests, multi-client scenarios, and network condition tests are all protocol-level. Unaffected by visual canon changes.

- **Risk 1: Network Library Complexity** -- Reason: ENet choice unaffected.

- **Risk 2: Threading Bugs** -- Reason: Threading model unaffected.

- **Risk 3: Serialization Performance** -- Reason: 80 serializations/sec estimate is based on tick rate and player count, not visual content. Component data sizes may grow slightly with new terrain/building types in later epics, but the serialization framework handles arbitrary component data.

- **Risk 5: Entity ID Lifecycle** -- Reason: Unaffected by visual changes.

- **Risk 6: Late Join Complexity** -- Reason: Late join mechanism is the same regardless of visual content.

- **Risk 7: Clock Synchronization** -- Reason: Unaffected.

- **Technical Recommendation 1 (ENet)** -- Reason: Library choice unaffected.

- **Technical Recommendation 2 (Message Serialization)** -- Reason: Binary format unaffected.

- **Technical Recommendation 3 (Threading Model)** -- Reason: Unaffected.

- **Technical Recommendation 4 (Game Loop Integration)** -- Reason: Unaffected.

- **Technical Recommendation 5 (Server Executable)** -- Reason: Unaffected.

### MODIFIED

- **Risk 4: Snapshot Size for Large Cities**
  - Previous: Estimated "thousands of entities" for late-game cities. Bandwidth estimate assumed a single map size (64x64 tiles = 4096 tiles from v0.3.0 canon). Full snapshot estimated at ~50KB uncompressed for 1000 entities, ~15-20KB compressed with LZ4.
  - Revised: With configurable map sizes up to 512x512 (262,144 tiles), the worst-case entity count is dramatically higher. A fully developed large map could have tens of thousands of entities (terrain tiles with components, buildings, infrastructure). Full snapshot for a large map could reach 10-15MB uncompressed. LZ4 compression would reduce this to 3-5MB, which is still manageable but requires the chunked transfer approach to be robust. The existing mitigation (LZ4 compression + 64KB chunking + progress indication) remains valid but the upper bound estimates need adjustment.
  - Reason: Canon v0.5.0 introduces configurable map sizes. The 512x512 large map is 64x the tile count of the previous 64x64 assumption. This does not require architectural changes (the chunked snapshot design in ticket 1-014 already handles arbitrarily large snapshots), but the bandwidth estimates in the analysis should reflect the new upper bound.

- **Bandwidth Estimates Section**
  - Previous: Assumed ~1000 entities mid-game. Delta update ~188 bytes/tick, ~3.7 KB/sec per client. Full snapshot ~50KB uncompressed, ~15-20KB compressed.
  - Revised: Estimates should include per-map-size projections:
    - Small (128x128): ~500-2000 entities, snapshot ~25-100KB uncompressed, ~8-30KB compressed. Delta ~150-350 bytes/tick.
    - Medium (256x256): ~2000-8000 entities, snapshot ~100-400KB uncompressed, ~30-120KB compressed. Delta ~200-600 bytes/tick.
    - Large (512x512): ~5000-25000 entities, snapshot ~250KB-1.25MB uncompressed, ~75-375KB compressed. Delta ~300-1200 bytes/tick.
    - All figures remain well within typical network capacity. Even worst-case large map: ~24KB/sec per client for deltas, ~375KB snapshot compressed. No architectural concern.
  - Reason: Configurable map sizes mean bandwidth estimates should cover the range, not assume a single map size. The existing architecture handles all tiers without modification.

- **Component Serialization (ticket 1-007)**
  - Previous: PositionComponent (grid_x, grid_y, elevation), OwnershipComponent (owner, state).
  - Revised: No change to these specific components. However, the expanded terrain type set (10 types with gameplay effects) implies that a TerrainComponent will carry more variant data when it is defined in later epics. The component serialization framework (version byte per component, extensible format) already accommodates this. The only revision is to note that terrain component serialization should anticipate 10+ terrain types with associated gameplay effect flags, not just the original 4.
  - Reason: Expanded terrain types from 4 to 10 means the eventual TerrainComponent will have more possible values. This does not change ticket 1-007's implementation (it serializes whatever components exist) but the planning note should acknowledge the wider data range.

### INVALIDATED
None. All original analysis items remain valid. The networking architecture is correctly decoupled from visual/rendering concerns.

### NEW CONCERNS

- **NEW CONCERN: Map Size in Server Configuration (SRV-01)**
  - The canon now specifies configurable map sizes (small/medium/large) set at game creation. The server configuration (ticket 1-008, SRV-01 in original analysis) should include map_size as a server startup parameter alongside port, max_players, and tick_rate. This parameter must be communicated to clients during join (likely as part of ServerStatusMessage or a new GameInfoMessage) so the client can allocate appropriate resources. This is a minor addition to server config -- not a new ticket, but a note for existing tickets 1-008 and 1-006.

- **NEW CONCERN: Snapshot Size Scaling with Map Size**
  - Large maps (512x512) could produce snapshots significantly larger than the original estimates. The existing snapshot chunking design (64KB chunks) handles this correctly, but reconnection time on large maps will be noticeably longer. Consider adding a map-size-aware progress indicator that sets client expectations during late join. This is a UX concern more than an engineering one, and the existing ticket 1-014 (snapshot transfer with progress indication) already covers the mechanism. No new ticket needed, but the acceptance criteria for 1-014 should note testing with large map snapshots.

---

## Questions for Other Agents

- **@systems-architect:** Should the map_size parameter be included in the ServerStatusMessage (ticket 1-006) or a separate GameInfoMessage sent on join? The client needs to know the map dimensions for resource allocation before receiving the full snapshot. This is a minor wire protocol question introduced by the configurable map size feature.

---

## Affected Tickets

- **1-006 (Server-to-Client Messages):** MODIFY -- ServerStatusMessage or a new GameInfoMessage should include map_size_tier (or map_dimensions) so the client knows the world size on connect. This is a small field addition, not a structural change.
- **1-007 (Component Serialization):** UNCHANGED -- Framework is extensible by design. Note that terrain data range is wider (10 types vs 4) for future reference.
- **1-008 (Server Network Loop):** MODIFY -- Server configuration (SRV-01 scope) should include map_size as a startup parameter. Minor addition to the config struct.
- **1-014 (Full State Snapshot):** MODIFY -- Acceptance criteria should include a test case for large map (512x512) snapshot generation and transfer to validate performance at scale. The architecture is unchanged; only testing scope expands.
- **All other tickets (1-001 through 1-005, 1-009 through 1-013, 1-015 through 1-022):** UNCHANGED -- No impact from canon v0.5.0 delta.
