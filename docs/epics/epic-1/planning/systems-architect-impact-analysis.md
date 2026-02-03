# Systems Architect Impact Analysis: Epic 1

**Epic:** Multiplayer Networking
**Analyst:** Systems Architect
**Date:** 2026-01-28
**Previous Analysis Canon Version:** 0.3.0
**Current Canon Version:** 0.5.0

---

## Trigger
Canon update v0.3.0 -> v0.5.0

## Overall Impact: LOW

## Impact Summary
The v0.5.0 canon changes are overwhelmingly visual, artistic, and UI-focused (bioluminescent palette, toon shading, hybrid 3D pipeline, dual UI modes, audio direction, building templates, terrain visuals). Epic 1 is networking infrastructure -- sockets, serialization, sync, sessions -- which is agnostic to what the data looks like when rendered. The one change with material relevance is **configurable map sizes (128/256/512)**, which has a modest effect on snapshot sizing estimates and state update payloads, but does not invalidate any architectural decisions or ticket structures.

---

## Item-by-Item Assessment

### UNAFFECTED

- **NET-01: Socket Abstraction Layer** -- Reason: Art direction, terrain visuals, UI modes, audio, and building templates have zero impact on socket operations. Map size does not affect socket-level abstraction.

- **NET-02: Message Protocol Design** -- Reason: Binary message format, headers, versioning, and compression options are payload-agnostic. The wire protocol carries serialized component data regardless of what those components represent visually.

- **NET-03: Server Network Loop** -- Reason: Server connection handling, broadcasting, heartbeat, and timeout mechanics are unaffected by visual/art changes. Max player count (2-4) unchanged.

- **NET-04: Client Network Loop** -- Reason: Client connection state machine, reconnection logic, and backoff timers are transport concerns. No art/visual dependency.

- **NET-05: Lobby Management (skip lobby, join live world)** -- Reason: Lobby decision was architectural. New UI modes (Classic vs Holographic) are presentation-layer concerns in Epic 12, not networking.

- **NET-06: Authentication & Session Management** -- Reason: Session tokens, PlayerID assignment, reconnection tokens are unaffected by any visual or content changes.

- **NET-07: Database Integration for Persistence (IPersistenceProvider)** -- Reason: Persistence interface is schema-agnostic at this level. What gets persisted is determined by components, not by networking infrastructure.

- **NET-08: INetworkHandler Interface Implementation** -- Reason: Message routing infrastructure is type-agnostic. Adding new component types or terrain types does not change the handler pattern.

- **SYNC-01: Change Detection** -- Reason: Dirty flags and EnTT signals are component-type-agnostic. New terrain types or building templates produce the same kind of component mutations.

- **SYNC-02: Delta Update Generation** -- Reason: Delta generation serializes whatever components are dirty. More terrain types or building variety does not change the delta mechanism.

- **SYNC-03: Delta Update Application (Client)** -- Reason: Client-side application of deltas is symmetric to generation. Agnostic to content.

- **SYNC-05: Snapshot Application (Client)** -- Reason: Clearing and replacing ECS state from a snapshot is structurally unchanged.

- **SYNC-06: Conflict Resolution** -- Reason: Server-authoritative model, validation, and rollback are unaffected by content changes.

- **MSG-01: Client-to-Server Messages** -- Reason: JoinMessage, InputMessage, ChatMessage, HeartbeatMessage, ReconnectMessage are all unchanged. Player actions (build, zone, demolish) still send the same abstract messages regardless of building templates or terrain types.

- **MSG-02: Server-to-Client Messages** -- Reason: StateUpdateMessage, FullStateMessage, PlayerListMessage, RejectionMessage, EventMessage structures are payload-agnostic.

- **ERR-01: Connection Error Handling** -- Reason: Error handling is transport-level. Unaffected by content.

- **ERR-02: Desync Detection** -- Reason: Checksum-based desync detection is content-agnostic.

- **ERR-03: Disconnect Handling** -- Reason: Ghost town process and disconnect behavior unchanged.

- **System Interaction Map** -- Reason: The server/client architecture diagram and data flow per tick remain structurally identical.

- **Multiplayer Architecture Notes (server authority, sync rate, connection states, late join flow, ghost town integration)** -- Reason: All architectural decisions remain valid. No new authority model or sync pattern required.

- **Risk 1: Network Library Choice** -- Reason: ENet decision unaffected by content changes.

- **Risk 2: Database Write Throughput** -- Reason: Unchanged at this level. Map size variation is already within the scope considered (discussed below under MODIFIED).

- **Risk 4: No Client-Side Prediction** -- Reason: Design constraint unchanged by art direction.

- **Risk 5: Message Ordering and Reliability** -- Reason: TCP/UDP decision unaffected.

- **Risk 6: Late Join Complexity** -- Reason: Unchanged by content -- snapshot generation mechanics are the same.

- **Risk 7: Ghost Town Handoff** -- Reason: Ownership state transitions unchanged.

- **Risk 8: Security Concerns** -- Reason: No impact from visual/content changes.

- **Dependencies (Epic 0 -> Epic 1 -> Epic 2+)** -- Reason: Dependency graph unchanged. Epic 1 still blocks all gameplay epics. The soft dependency on Epic 2 (rendering) remains soft.

- **Interface Implementation Details (ISerializable, INetworkHandler, NetworkMessage)** -- Reason: These contracts are unchanged. New component types (terrain types, building templates) will implement ISerializable when defined in their respective epics, not in Epic 1.

### MODIFIED

- **Item: Risk 3 -- State Snapshot Size estimates**
  - Previous: "Full state snapshots could be very large for mature cities (thousands of entities, components). Serializing and sending could take seconds." Assumed a fixed 64x64 map (4,096 tiles per canon v0.3.0 implied tile size).
  - Revised: Map size is now configurable: small (128x128 = 16,384 tiles), medium (256x256 = 65,536 tiles), large (512x512 = 262,144 tiles). The upper bound for snapshot size is significantly larger than previously estimated. A large map with a mature world could have 250K+ terrain entities plus buildings, infrastructure, and population entities. Snapshot sizes could reach 10-20MB for large maps, not the 1-5MB estimated in the original analysis.
  - Reason: Canon v0.5.0 introduces configurable map sizes up to 512x512, which is a 64x increase over the smallest tier and approximately 4x over the old assumed size. This amplifies the importance of LZ4 compression (already planned in ticket 1-014) and chunked snapshot transfer (already planned at 64KB chunks). The existing mitigation is adequate, but the upper bound estimates in ticket 1-006 and 1-014 acceptance criteria should be updated to reflect larger possible snapshot sizes.

- **Item: SYNC-04 -- Full State Snapshot (sizing/testing parameters)**
  - Previous: "Snapshot may be 1-5MB for mature worlds" in agent notes for ticket 1-014.
  - Revised: Snapshot size range should be updated to "1-5MB (small map), 5-15MB (medium map), 10-30MB (large map, mature world)" to account for configurable map sizes. Test scenarios in ticket 1-014 should include a large map stress test.
  - Reason: 512x512 maps have 16x more tiles than 128x128, proportionally increasing entity counts and snapshot sizes.

### INVALIDATED

None. All architectural decisions, system boundaries, interfaces, and patterns from the original analysis remain valid under canon v0.5.0. The networking layer is properly decoupled from visual/content concerns.

### NEW CONCERNS

- **NEW CONCERN: Map size as a network-relevant game creation parameter**
  The configurable map size (small/medium/large) is a per-game setting chosen at game creation. This parameter must be communicated to clients during the join flow (currently part of JoinMessage response or ServerStatusMessage) so clients can allocate appropriate memory and display loading expectations. This is a minor detail that fits naturally into existing message types (e.g., include map_size_tier in the ServerStatusMessage or FullStateMessage header). No new ticket is needed -- this is a detail to capture in ticket 1-006 (Server-to-Client Messages) acceptance criteria when implementing ServerStatusMessage or FullStateMessage.

---

## Questions for Other Agents

None. The canon v0.5.0 changes do not raise new architectural questions for Epic 1. The one relevant change (configurable map sizes) is well within the existing design's capacity and only requires parameter adjustments, not architectural rethinking.

---

## Affected Tickets

- **1-006**: MODIFY -- Update snapshot size estimates in agent notes from "1-5MB for mature worlds" to "1-30MB depending on map size tier." Add map_size_tier field to ServerStatusMessage or FullStateMessage header so clients know the map dimensions on join.
- **1-014**: MODIFY -- Update snapshot size test scenarios to cover all three map tiers. Large map stress test (512x512, 200K+ entities) should be an explicit acceptance criterion. Adjust the "bounded delta queue during snapshot" to account for potentially longer snapshot transfer times on large maps.
- **1-021**: MODIFY -- Add a large-map integration test scenario to the scenario tests. Verify snapshot transfer completes successfully for a 512x512 map with substantial entity counts.
- **1-001 through 1-005, 1-007 through 1-013, 1-015 through 1-020, 1-022**: UNCHANGED -- No modifications needed.
- NEW: Not needed. The map size parameter concern is small enough to fold into existing tickets (1-006 and 1-014).
