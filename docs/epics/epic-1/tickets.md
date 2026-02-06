# Epic 1: Multiplayer Networking - Tickets

Generated: 2026-01-26
Canon Version: 0.13.0
Planning Agents: systems-architect, engine-developer, qa-engineer

## Revision History

| Date | Trigger | Tickets Modified | Tickets Removed | Tickets Added |
|------|---------|------------------|-----------------|---------------|
| 2026-01-28 | Canon update v0.3.0 → v0.5.0 | 1-006, 1-008, 1-014, 1-019, 1-021 | None | None |
| 2026-01-29 | canon-verification (v0.5.0 → v0.13.0) | — | — | — |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. No changes required. UI events (v0.13.0) are local client-side events that translate to InputMessages for network transmission - Epic 1 network protocol correctly handles this. ISimulationTime (v0.10.0) is consumed by simulation systems, not networking layer.

## Overview

Epic 1 establishes the multiplayer networking foundation that enables ZergCity's core vision: a shared alien colony builder where 2-4 players build together in real-time. This epic introduces two primary systems: **NetworkManager** (core system for socket connections, message serialization, and player connections) and **SyncSystem** (ECS system for delta updates and state synchronization). All subsequent gameplay systems will integrate with this networking layer.

## Key Decisions

The following technical decisions were resolved during planning:

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Networking library | ENet (reliable UDP) | Battle-tested, cross-platform, handles reliability/ordering, vcpkg available |
| Server executable | Single exe with --server flag | Consistent with Epic 0, simpler deployment |
| Entity IDs | Server-assigned, used directly in EnTT | No mapping overhead, simpler debugging |
| Delta detection | Dirty flags via EnTT signals | Efficient O(1) per change, automatic tracking |
| Serialization | Manual binary (NetworkBuffer class) | Consistent with Epic 0, full control, minimal overhead |
| Persistence | IPersistenceProvider interface, in-memory for MVP | Enables Epic 16 database swap without code changes |
| Heartbeat interval | 1 second | Balance between responsiveness and overhead |
| Soft timeout | 5 seconds | Allow for brief network hiccups |
| Hard disconnect | 10 seconds | Absolute disconnect declaration |
| Reconnection grace | 30 seconds | Player can alt-tab, brief ISP issues |
| Latency target | 250ms RTT | City builder tolerates more latency than action games |
| Client-side prediction | None (optimistic UI hints allowed) | Simplicity over responsiveness per canon |
| Lobby phase | Skip - join directly into live world | Endless sandbox feel, late-join first-class |
| Snapshot Strategy | Async with copy-on-write | Non-blocking snapshot generation ensures simulation continues during transfer |
| Lock-Free Queues | moodycamel::readerwriterqueue | Proven SPSC lock-free queue implementation via vcpkg |
| Compression | LZ4 required for snapshots and large messages | Fast compression for network efficiency |
| Testing | Parallel development (tests written with each ticket) | Tests developed alongside implementation, not deferred |

---

## Tickets

### Ticket 1-001: ENet Library Integration

**Type:** infrastructure
**System:** NetworkManager
**Estimated Scope:** S

**Description:**
Integrate ENet networking library into the project via vcpkg. Create a thin abstraction layer (INetworkTransport) to enable mocking for tests while wrapping ENet functionality. This is the foundation for all network communication.

**Acceptance Criteria:**
- [ ] ENet added to vcpkg.json dependencies
- [ ] CMakeLists.txt updated to link ENet
- [ ] INetworkTransport interface defined with send/receive/disconnect methods
- [ ] ENetTransport implementation wraps ENet host/peer operations
- [ ] MockTransport implementation stores messages in queues for testing
- [ ] Basic connection test passes (connect, send message, receive, disconnect)
- [ ] Windows build verified

**Dependencies:**
- Blocked by: None
- Blocks: 1-002, 1-003, 1-004

**Agent Notes:**
- Systems Architect: Abstract ENet behind ISocket interface for testability. Use vcpkg package `enet`.
- Engine Developer: ENet provides reliable UDP with ordered channels. Use 2 channels: reliable for actions, unreliable for optional future use.
- QA Engineer: MockTransport enables deterministic unit tests without real network.

---

### Ticket 1-002: NetworkBuffer Serialization Utilities

**Type:** infrastructure
**System:** NetworkManager
**Estimated Scope:** S

**Description:**
Create NetworkBuffer class with helpers for binary message serialization. This follows Epic 0's manual serialization pattern with added structure for network messages. All network messages will use this for consistent wire format.

**Acceptance Criteria:**
- [ ] NetworkBuffer class with write_u8, write_u16, write_u32, write_i32, write_f32, write_string methods
- [ ] Corresponding read methods with bounds checking
- [ ] Little-endian byte order enforced (per canon interfaces.yaml)
- [ ] String serialization uses length-prefix format
- [ ] Buffer overflow detection with clear error handling
- [ ] Unit tests for all data types including edge cases (INT32_MAX, negative values, empty strings)
- [ ] Round-trip serialization tests pass for all types

**Dependencies:**
- Blocked by: None
- Blocks: 1-005, 1-006, 1-007

**Agent Notes:**
- Systems Architect: Continue Epic 0's manual serialization pattern. No external schema libraries.
- Engine Developer: Zero-copy potential for optimization later. Keep wire format simple.
- QA Engineer: Test exact byte layout in tests. Fuzz testing target for deserialize methods.

---

### Ticket 1-003: Message Protocol Framework

**Type:** infrastructure
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Define the message envelope format and message type registry. Each message has a header (type, length, sequence) followed by payload. Implement base NetworkMessage class and message routing infrastructure.

**Acceptance Criteria:**
- [ ] Message envelope format: [1 byte protocol version][2 bytes type][2 bytes length][N bytes payload]
- [ ] MessageType enum defined with extensible numbering scheme
- [ ] NetworkMessage base class with serialize/deserialize using NetworkBuffer
- [ ] Message factory for creating messages from type ID
- [ ] Sequence number tracking for ordering (optional for reliable channel)
- [ ] Protocol version validation (reject incompatible versions)
- [ ] Unknown message type handling (log warning, skip bytes, no crash)

**Dependencies:**
- Blocked by: 1-002 (NetworkBuffer)
- Blocks: 1-005, 1-006, 1-007

**Agent Notes:**
- Systems Architect: Include version byte for protocol evolution. Start at version 1.
- Engine Developer: Message types partitioned: 0-99 system, 100-199 gameplay, 200+ reserved.
- QA Engineer: Test malformed messages don't crash. Test unknown type handling.

---

### Ticket 1-004: Network Thread and Message Queues

**Type:** infrastructure
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Implement dedicated network I/O thread with lock-free message queues for communication with main thread. Network thread handles ENet polling and serialization. Main thread processes messages during game loop. Use moodycamel::readerwriterqueue for lock-free SPSC queues.

**Acceptance Criteria:**
- [ ] Network thread polls ENet continuously (1ms timeout)
- [ ] Lock-free SPSC queue for incoming messages (Network -> Main)
- [ ] Lock-free SPSC queue for outgoing messages (Main -> Network)
- [ ] moodycamel::readerwriterqueue added to vcpkg.json
- [ ] Thread-safe startup and shutdown sequence
- [ ] No shared mutable state beyond queues
- [ ] Main thread never blocks on network operations
- [ ] Clean thread join on application shutdown
- [ ] Memory leak test: connect/disconnect 1000 times, verify stable memory

**Dependencies:**
- Blocked by: 1-001 (ENet)
- Blocks: 1-008, 1-009

**Agent Notes:**
- Systems Architect: Network thread NEVER touches ECS registry. Clear thread ownership.
- Engine Developer: Use moodycamel::ReaderWriterQueue or simple ring buffer. SPSC for optimal performance.
- QA Engineer: Stress test high message volume. Verify no dropped messages. Test clean shutdown.
- Decision record: /plans/decisions/epic-1-lockfree-queue.md

---

### Ticket 1-005: Core Message Types - Client to Server

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Implement client-to-server message types. These messages represent player intent sent to the authoritative server for validation and processing.

**Acceptance Criteria:**
- [ ] JoinMessage: player name, requested session token (for reconnection)
- [ ] InputMessage: player action (reuse Epic 0 definition, add network serialization)
- [ ] ChatMessage: text content with sender ID
- [ ] HeartbeatMessage: client keepalive with timestamp for RTT measurement
- [ ] ReconnectMessage: session token for recovering existing session
- [ ] All messages implement serialize/deserialize with NetworkBuffer
- [ ] Round-trip serialization tests for each message type
- [ ] Size validation to prevent oversized messages

**Dependencies:**
- Blocked by: 1-002 (NetworkBuffer), 1-003 (Protocol)
- Blocks: 1-008, 1-011

**Agent Notes:**
- Systems Architect: JoinMessage skips lobby - joins directly into live world per decision.
- Engine Developer: InputMessage extends Epic 0's format with network header.
- QA Engineer: Test each message type's serialization independently.

---

### Ticket 1-006: Core Message Types - Server to Client

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Implement server-to-client message types. These messages represent authoritative state updates and server responses. Large messages (>1KB) use LZ4 compression.

**Acceptance Criteria:**
- [ ] StateUpdateMessage: tick number, list of entity deltas (create/update/destroy)
- [ ] FullStateMessage: complete world snapshot for reconnection (chunked for large states)
- [ ] PlayerListMessage: current players with IDs, names, connection status
- [ ] RejectionMessage: action rejected with reason code and human-readable message
- [ ] EventMessage: game events (placeholder for disasters, milestones)
- [ ] HeartbeatResponseMessage: server response with RTT measurement
- [ ] ServerStatusMessage: server state (LOADING, READY, RUNNING)
- [ ] All messages implement serialize/deserialize
- [ ] Snapshot chunking: 64KB chunks with sequence numbers
- [ ] Message includes compression flag indicating whether payload is LZ4 compressed
- [ ] ServerStatusMessage includes map_size_tier field (small/medium/large) and map dimensions so clients know world size on connect

**Dependencies:**
- Blocked by: 1-002 (NetworkBuffer), 1-003 (Protocol)
- Blocks: 1-009, 1-012, 1-013

**Agent Notes:**
- Systems Architect: FullStateMessage may be 1-30MB depending on map size tier. Chunk at 64KB. Include map_size_tier in ServerStatusMessage header for client resource allocation.
- Engine Developer: Include tick number in StateUpdateMessage for ordering. Map dimensions communicated via ServerStatusMessage, not a separate message type.
- QA Engineer: Test snapshot sizes parameterized by map size: small map (~500KB), medium map (~2MB), large map (~20MB+). Boundary test at 64KB chunk boundaries for each tier.
- Decision record: /plans/decisions/epic-1-lz4-compression.md

> **Revised 2026-01-28:** Added map_size_tier to ServerStatusMessage; updated snapshot size estimates from "1-5MB" to "1-30MB depending on map size tier"; updated QA test matrix to parameterize by map size (trigger: canon-update v0.5.0)

---

### Ticket 1-007: Component Serialization for Sync

**Type:** feature
**System:** SyncSystem
**Estimated Scope:** M

**Description:**
Add network serialization methods to all syncable components. Each component gets serialize/deserialize using NetworkBuffer, plus a component type ID for the wire format.

**Acceptance Criteria:**
- [ ] Component type registry with unique IDs per component type
- [ ] PositionComponent serialization (grid_x, grid_y, elevation)
- [ ] OwnershipComponent serialization (owner PlayerID, ownership state)
- [ ] Component version byte for future backward compatibility
- [ ] get_serialized_size() method for pre-allocation optimization
- [ ] Round-trip tests for each component type
- [ ] Default values for missing fields when deserializing older versions

**Dependencies:**
- Blocked by: 1-002 (NetworkBuffer)
- Blocks: 1-012, 1-013

**Agent Notes:**
- Systems Architect: Follow ISerializable pattern from Epic 0. Version byte per component.
- Engine Developer: Enforce modification through registry.patch() for signal triggering.
- QA Engineer: Test all component edge cases: max values, negative values, empty strings.

---

### Ticket 1-008: Server Network Loop

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** L

**Description:**
Implement server-side network loop that accepts connections, processes incoming messages, and broadcasts state updates. Server runs in headless mode (no window) when --server flag is passed.

**Acceptance Criteria:**
- [ ] Server creates listening ENet host on configurable port (default 7777)
- [ ] Server accepts map_size configuration parameter (small/medium/large, default medium) at startup
- [ ] Accept incoming client connections (max 4 per canon)
- [ ] Reject 5th+ connection with "Server full" message
- [ ] Per-client state tracking (connection status, PlayerID, message queue)
- [ ] Route incoming messages to appropriate handlers via INetworkHandler
- [ ] Broadcast StateUpdateMessage to all connected clients after each tick
- [ ] Targeted message sending (to specific client)
- [ ] Heartbeat sending every 1 second per client
- [ ] Connection timeout detection (5 consecutive missed heartbeats = 5s)
- [ ] Hard disconnect after 10 seconds of silence
- [ ] Server CLI commands functional: `players`, `kick <name>`, `say <message>`

**Dependencies:**
- Blocked by: 1-001 (ENet), 1-004 (Threading), 1-005 (C2S Messages)
- Blocks: 1-010, 1-011, 1-014

**Agent Notes:**
- Systems Architect: Server state machine: INITIALIZING -> LOADING -> READY -> RUNNING. Map size is a server-level parameter set at game creation, communicated to clients via ServerStatusMessage.
- Engine Developer: Skip SDL_INIT_VIDEO in server mode. Use SDL_INIT_TIMER | SDL_INIT_EVENTS only. Map size config stored alongside port, max_players, tick_rate.
- QA Engineer: Test max player rejection. Test graceful/abrupt disconnect handling.

> **Revised 2026-01-28:** Added map_size server configuration parameter (small/medium/large) per canon v0.5.0 configurable map sizes (trigger: canon-update v0.5.0)

---

### Ticket 1-009: Client Network Loop

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** L

**Description:**
Implement client-side network loop that connects to server, sends player input, and receives state updates. Client handles connection state machine and reconnection logic.

**Acceptance Criteria:**
- [ ] Connect to server by IP/hostname and port
- [ ] Connection state machine: Disconnected -> Connecting -> Connected -> Playing
- [ ] Reconnection state with exponential backoff (retry every 2s, max 30s)
- [ ] Send JoinMessage on successful connection
- [ ] Queue and send InputMessages generated by InputSystem
- [ ] Receive and queue StateUpdateMessages for SyncSystem
- [ ] Heartbeat response handling for RTT measurement
- [ ] RTT tracking and display (optional UI indicator)
- [ ] Graceful disconnect handling (return to menu)
- [ ] Handle ServerStatusMessage (show "Server loading..." during LOADING state)
- [ ] Connection timeout detection (2s = indicator, 5s = banner, 15s = full UI)

**Dependencies:**
- Blocked by: 1-001 (ENet), 1-004 (Threading), 1-006 (S2C Messages)
- Blocks: 1-010, 1-011, 1-014

**Agent Notes:**
- Systems Architect: Accept connections during LOADING phase but hold in pending state.
- Engine Developer: Client shows "Establishing neural link..." while connecting.
- QA Engineer: Test all connection states. Test reconnection with exponential backoff.

---

### Ticket 1-010: Player Session Management

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Implement player session management including PlayerID assignment, session tokens for reconnection, and player list tracking. Server assigns PlayerIDs 1-4 (0 is reserved for GAME_MASTER per canon).

**Acceptance Criteria:**
- [ ] Server assigns unique PlayerID (1-4) on join
- [ ] Session token generation (128-bit random) for reconnection
- [ ] Session token validation on reconnect (same player = same PlayerID)
- [ ] Player list maintained with name, PlayerID, connection status, last activity
- [ ] Broadcast PlayerListMessage when player joins/leaves
- [ ] 30-second grace period for reconnection (player keeps their session)
- [ ] Session cleanup after grace period expires
- [ ] Duplicate connection handling (second connection with same token kicks first)
- [ ] Activity tracking for ghost town timer (per Q012: real-world time, not ticks)

**Dependencies:**
- Blocked by: 1-008 (Server Loop), 1-009 (Client Loop)
- Blocks: 1-014, 1-016

**Agent Notes:**
- Systems Architect: Session tokens not encrypted for MVP but validated server-side.
- Engine Developer: Store last_activity timestamp per player for inactivity detection.
- QA Engineer: Test rapid connect/disconnect cycles. Test session recovery within grace period.

---

### Ticket 1-011: Input Message Transmission

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** S

**Description:**
Connect Epic 0's InputSystem to NetworkManager so player actions are transmitted to server. Server validates and applies actions, clients wait for authoritative state update.

**Acceptance Criteria:**
- [ ] InputSystem produces InputMessage on player action
- [ ] InputMessage queued for network transmission (not applied locally)
- [ ] Server receives InputMessage, validates against game rules
- [ ] Valid actions applied to server ECS
- [ ] Invalid actions generate RejectionMessage with reason
- [ ] Client receives RejectionMessage, shows feedback (per Q015 design)
- [ ] Pending action visual state (ghost building while awaiting confirmation)
- [ ] Mid-action disconnect: server rolls back pending actions (per Q010)

**Dependencies:**
- Blocked by: 1-005 (C2S Messages), 1-008 (Server Loop), 1-009 (Client Loop)
- Blocks: 1-017

**Agent Notes:**
- Systems Architect: Server-authoritative: server state always wins.
- Engine Developer: Optimistic UI hints allowed but no client-side prediction.
- QA Engineer: Test rejection feedback. Test mid-action disconnect rollback.

---

### Ticket 1-012: SyncSystem - Change Detection

**Type:** feature
**System:** SyncSystem
**Estimated Scope:** M

**Description:**
Implement change detection using EnTT signals to track dirty entities. SyncSystem subscribes to component modification signals and maintains a dirty set that is flushed each tick.

**Acceptance Criteria:**
- [ ] Subscribe to on_construct, on_update, on_destroy signals for all syncable components
- [ ] Maintain dirty_entities set (entities with changes since last sync)
- [ ] Track change type per entity: created, updated, destroyed
- [ ] Entity added to dirty set on any component modification
- [ ] Dirty set cleared after delta generation
- [ ] SyncPolicy metadata respected: Periodic, OnChange, None (per Epic 0)
- [ ] Components with SyncPolicy::Never excluded from sync
- [ ] Enforcement: all modifications must use registry.patch() for signal triggering

**Dependencies:**
- Blocked by: 1-007 (Component Serialization)
- Blocks: 1-013

**Agent Notes:**
- Systems Architect: Dirty flag pattern, not full state diffing. O(1) per change.
- Engine Developer: on_update signal only fires with registry.patch() or registry.replace(), NOT direct member access.
- QA Engineer: Verify no "lost" modifications. Integration test: modify entity, verify in delta.

---

### Ticket 1-013: SyncSystem - Delta Generation and Application

**Type:** feature
**System:** SyncSystem
**Estimated Scope:** L

**Description:**
Implement delta generation on server (build StateUpdateMessage from dirty entities) and delta application on client (update local ECS from received StateUpdateMessage).

**Acceptance Criteria:**
- [ ] Server: generate StateUpdateMessage containing only changed entities/components
- [ ] Include entity create events (new entities with all components)
- [ ] Include entity update events (changed components only)
- [ ] Include entity destroy events (entity ID to remove)
- [ ] Batch all changes into single message per tick (not per change)
- [ ] Client: deserialize StateUpdateMessage
- [ ] Client: create new entities with server-assigned IDs
- [ ] Client: update existing entity components
- [ ] Client: destroy entities as directed
- [ ] Handle out-of-order messages gracefully (tick number comparison)
- [ ] Handle duplicate messages (idempotent application)

**Dependencies:**
- Blocked by: 1-006 (S2C Messages), 1-007 (Component Serialization), 1-012 (Change Detection)
- Blocks: 1-014, 1-017

**Agent Notes:**
- Systems Architect: One StateUpdateMessage per tick (20/sec). Delta compression is key.
- Engine Developer: Entity IDs used directly in EnTT via registry.create(entt::entity{id}).
- QA Engineer: Test state consistency after 1000 ticks. Test duplicate/out-of-order handling.

---

### Ticket 1-014: Full State Snapshot Generation and Application

**Type:** feature
**System:** SyncSystem
**Estimated Scope:** L

**Description:**
Implement full state snapshot for reconnection and late join. Server generates complete world state asynchronously using a copy-on-write pattern to ensure consistency during generation, sends in chunks, client replaces local state with snapshot.

**Acceptance Criteria:**
- [ ] Server: generate complete snapshot of all entities with SyncPolicy != Never
- [ ] Snapshot generation is asynchronous (non-blocking)
- [ ] Copy-on-write pattern ensures consistency during generation
- [ ] LZ4 compression applied to all snapshots
- [ ] lz4 added to vcpkg.json
- [ ] Chunk snapshot at 64KB boundaries for transmission
- [ ] SnapshotStart message with tick number and total chunks
- [ ] SnapshotChunk messages with index and data
- [ ] SnapshotEnd message marking completion
- [ ] Client: buffer incoming chunks during snapshot transfer
- [ ] Client: buffer delta updates received during snapshot transfer
- [ ] Client: clear local ECS state when applying snapshot
- [ ] Client: apply snapshot, then apply buffered deltas in tick order
- [ ] Progress indication during snapshot transfer
- [ ] Bounded delta queue during snapshot (max 100 ticks = 5 seconds)

**Dependencies:**
- Blocked by: 1-006 (S2C Messages), 1-008 (Server Loop), 1-009 (Client Loop), 1-013 (Delta)
- Blocks: 1-015

**Agent Notes:**
- Systems Architect: Snapshot size varies by map tier — small maps: 1-5MB, medium maps: 5-15MB, large maps: 10-30MB for mature worlds. Queue deltas during transfer. Bounded delta queue (100 ticks) may need tuning for large-map snapshot transfers.
- Engine Developer: Generate snapshot without blocking simulation (interleave or separate thread). Large maps (512x512) will have proportionally more entities; LZ4 compression and 64KB chunking handle scaling.
- QA Engineer: Test late join during active play. Test reconnection with large state changes. Test snapshot generation and transfer at all three map size tiers (128x128, 256x256, 512x512). Target: snapshot transfer under 10 seconds for large maps on LAN.
- Decision record: /plans/decisions/epic-1-snapshot-strategy.md
- Decision record: /plans/decisions/epic-1-lz4-compression.md

> **Revised 2026-01-28:** Updated snapshot size estimates from "1-5MB" to per-map-tier ranges (1-30MB); added large-map snapshot test scenario; added note on bounded delta queue tuning for large maps (trigger: canon-update v0.5.0)

---

### Ticket 1-015: Entity ID Synchronization

**Type:** feature
**System:** SyncSystem
**Estimated Scope:** M

**Description:**
Implement server-side entity ID generation and client-side handling. Server uses monotonic counter, never reuses IDs during session. Clients use server-assigned IDs directly.

**Acceptance Criteria:**
- [ ] Server: EntityIdGenerator with monotonic uint64_t counter starting at 1
- [ ] ID 0 reserved for null/invalid
- [ ] IDs never reused during session (no recycling)
- [ ] next_id persisted for server restart recovery (via IPersistenceProvider)
- [ ] Client: create entities with server-provided ID via registry.create(entt::entity{id})
- [ ] Client: reject messages referencing unknown entity IDs (log warning)
- [ ] Reconnection: player receives snapshot with all current IDs
- [ ] No bidirectional mapping needed - direct ID usage

**Dependencies:**
- Blocked by: 1-013 (Delta), 1-014 (Snapshot)
- Blocks: 1-016

**Agent Notes:**
- Systems Architect: Direct IDs simplify debugging - entity 42 is 42 everywhere.
- Engine Developer: 64-bit counter lasts ~584 million years at 1000/sec.
- QA Engineer: Test IDs unique across create/destroy cycles. Test reconnection ID consistency.

---

### Ticket 1-016: IPersistenceProvider Interface

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Define persistence interface for game state and implement in-memory version for MVP. This enables Epic 16 to swap in SQLite without changing NetworkManager code.

**Acceptance Criteria:**
- [ ] IPersistenceProvider interface with saveCheckpoint, loadLatestCheckpoint, logAction, getActionsSince
- [ ] InMemoryPersistenceProvider implementation stores checkpoints in std::map
- [ ] Store action log in std::vector for replay
- [ ] Save next_entity_id for server restart
- [ ] Save current tick number
- [ ] Save player session data (for reconnection after restart)
- [ ] Data lost on shutdown (acceptable for MVP)
- [ ] Clear documentation that Epic 16 will add SQLitePersistenceProvider

**Dependencies:**
- Blocked by: 1-010 (Sessions), 1-015 (Entity IDs)
- Blocks: None (enables Epic 16)

**Agent Notes:**
- Systems Architect: Dependency inversion - NetworkManager depends on abstraction.
- Engine Developer: Separate DB thread architecture documented for Epic 16.
- QA Engineer: Test checkpoint/restore cycle. Test action replay from log.

---

### Ticket 1-017: Game Loop Integration

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Integrate NetworkManager and SyncSystem into the Epic 0 game loop. Server processes messages, runs simulation, sends deltas. Client receives state, applies updates, renders.

**Acceptance Criteria:**
- [ ] NetworkManager::poll() called each frame
- [ ] Received messages processed before simulation tick
- [ ] Simulation tick runs at fixed 50ms intervals (20 ticks/sec)
- [ ] Server: SyncSystem generates and sends deltas after each tick
- [ ] Client: SyncSystem applies pending updates before render
- [ ] Client: interpolation alpha calculated for smooth rendering
- [ ] Accumulator pattern for fixed timestep (per Epic 0)
- [ ] Tick number synchronized between server and all clients
- [ ] Application state integration: Connecting, Loading, Playing states

**Dependencies:**
- Blocked by: 1-011 (Input Transmission), 1-013 (Delta)
- Blocks: 1-018

**Agent Notes:**
- Systems Architect: Data flow: receive -> process -> tick -> generate -> send.
- Engine Developer: Client interpolates between last two states for smooth 60fps.
- QA Engineer: Verify tick rate maintained under load. Verify state consistency.

---

### Ticket 1-018: Connection Error Handling

**Type:** feature
**System:** NetworkManager
**Estimated Scope:** M

**Description:**
Implement robust error handling for network edge cases including malformed messages, connection timeouts, and rate limiting.

**Acceptance Criteria:**
- [ ] Malformed message handling: log warning, drop message, connection survives
- [ ] Message too large: reject before parsing, log warning
- [ ] Unknown message type: ignore, log warning, no crash
- [ ] Connection timeout: fire disconnect event, clean up resources
- [ ] Rate limiting: per-player action buckets (10/sec building, 20/sec zoning, etc.)
- [ ] Rate limit exceeded: silently drop excess actions (per Q039)
- [ ] Egregious abuse logging (100+ actions/sec)
- [ ] Invalid PlayerID in message: reject, log security warning
- [ ] Buffer overflow protection in all deserialize paths

**Dependencies:**
- Blocked by: 1-008 (Server Loop), 1-009 (Client Loop), 1-017 (Game Loop)
- Blocks: 1-019

**Agent Notes:**
- Systems Architect: Server must never crash from malformed client data.
- Engine Developer: Token bucket algorithm for rate limiting.
- QA Engineer: Fuzz testing priority target. Test all failure modes from Q008.

---

### Ticket 1-019: Testing Infrastructure - MockSocket and TestHarness

**Type:** infrastructure
**System:** Testing
**Estimated Scope:** L

**Description:**
Build testing infrastructure for network testing including MockSocket for unit tests and TestHarness for integration tests with multiple clients. Tests should be written in parallel with implementation, not deferred to the end.

**Acceptance Criteria:**
- [ ] MockSocket: in-memory message passing, no real network
- [ ] Configurable latency injection
- [ ] Configurable packet loss percentage
- [ ] Configurable bandwidth limits
- [ ] Message interception for verification
- [ ] TestServer: lightweight server wrapper with state inspection
- [ ] TestClient: scriptable client with assertion helpers
- [ ] TestHarness: orchestrator for multi-client scenarios
- [ ] wait_for_sync(): blocks until all clients synced
- [ ] assert_state_match(): compares server and all client states
- [ ] StateDiffer: returns list of differences between ECS states
- [ ] Connection quality profiles: PERFECT, LAN, GOOD_WIFI, POOR_WIFI, MOBILE_3G, HOSTILE
- [ ] Headless mode support (--headless flag, no SDL window)
- [ ] TestServer supports configurable map size parameter for parameterized test scenarios

**Dependencies:**
- Blocked by: 1-001 (ENet)
- Blocks: None (parallel development with 1-008, 1-009)

**Agent Notes:**
- Systems Architect: MockSocket enables deterministic unit tests.
- Engine Developer: Use port 0 for automatic port assignment in tests. TestServer should accept map_size_tier for creating test worlds at different scales.
- QA Engineer: Three-tier strategy: unit (mock), integration (localhost), E2E (separate processes). All performance and snapshot tests should be parameterized by map size tier (small/medium/large).
- NOTE: Tests should be written in parallel with implementation, not deferred. Begin test infrastructure early to enable TDD approach.

> **Revised 2026-01-28:** Added configurable map size support to TestServer; parameterized test infrastructure by map size tier (trigger: canon-update v0.5.0)

---

### Ticket 1-020: Unit Tests - Serialization and State Machine

**Type:** task
**System:** Testing
**Estimated Scope:** M

**Description:**
Implement unit tests for message serialization and connection state machines using MockSocket for determinism. Tests should be written in parallel with the corresponding implementation tickets.

**Acceptance Criteria:**
- [ ] Message serialization roundtrip tests for all message types
- [ ] Byte-level layout verification for serialized messages
- [ ] Connection state machine tests: all valid transitions
- [ ] Connection state machine tests: invalid transitions rejected
- [ ] Player ID assignment tests: unique, respects GAME_MASTER=0
- [ ] Session token generation and validation tests
- [ ] Rate limiting tests: verify token bucket behavior
- [ ] NetworkBuffer edge cases: empty, max size, overflow
- [ ] Component serialization tests for all component types
- [ ] Tests run without real network (MockSocket only)

**Dependencies:**
- Blocked by: None (parallel development with 1-002, 1-003, 1-005, 1-006)
- Blocks: None

**Agent Notes:**
- Systems Architect: Foundation for confidence in wire format.
- Engine Developer: Exact byte layout tests catch endianness issues.
- QA Engineer: P0 priority - must pass before any integration testing.
- NOTE: Write serialization tests as each message type is implemented (parallel development). Do not wait for all messages to be complete.

---

### Ticket 1-021: Integration Tests - Multi-Client Scenarios

**Type:** task
**System:** Testing
**Estimated Scope:** L

**Description:**
Implement integration tests for multi-client scenarios including connection, synchronization, late join, and reconnection. Tests should be written incrementally as features are implemented.

**Acceptance Criteria:**
- [ ] Smoke tests (run on every commit, under 30 seconds):
  - Server starts and accepts connection
  - Client connects and receives initial state
  - Two clients connect and see each other
  - Client sends action, server processes, both clients see result
  - Client disconnects gracefully
- [ ] Scenario tests (run on PR merge, under 5 minutes):
  - Full 4-player session lifecycle
  - Late join: player joins mid-game, receives correct snapshot
  - Reconnection: player disconnects and rejoins within grace period
  - Reconnection: player reconnects after grace period (new session)
  - Concurrent actions: all 4 players act simultaneously
  - Entity lifecycle: create, modify, destroy - all clients sync
- [ ] State consistency verification after each scenario
- [ ] Deterministic RNG seeding for reproducibility
- [ ] Large-map integration test: 512x512 map with substantial entity count, verify snapshot transfer completes and state syncs correctly

**Dependencies:**
- Blocked by: 1-008 (Server Loop), 1-009 (Client Loop)
- Blocks: None (parallel development where possible)

**Agent Notes:**
- Systems Architect: These tests prove the system works end-to-end. Include a large-map scenario to validate snapshot transfer at scale.
- Engine Developer: Use faster tick rate in tests for speed. Large-map test may need extended timeout for snapshot transfer.
- QA Engineer: Smoke tests are CI gate. Scenario tests run on PR merge. Large-map scenario verifies performance at the upper bound of configurable map sizes.
- NOTE: Write basic connection smoke tests as soon as server/client loops are functional. Add scenario tests incrementally as snapshot and session features are completed.

> **Revised 2026-01-28:** Added large-map integration test scenario (512x512) to verify snapshot transfer at scale (trigger: canon-update v0.5.0)

---

### Ticket 1-022: Integration Tests - Network Conditions and Edge Cases

**Type:** task
**System:** Testing
**Estimated Scope:** L

**Description:**
Implement integration tests for adverse network conditions and failure modes using NetworkSimulator. Tests should be written as error handling features are implemented.

**Acceptance Criteria:**
- [ ] Network condition tests:
  - LAN profile (1-5ms latency, 0% loss): must pass
  - GOOD_WIFI profile (20-50ms latency, 0.1% loss): must pass
  - POOR_WIFI profile (50-200ms latency, 2% loss): should pass with degradation
  - MOBILE_3G profile (100-500ms latency, 5% loss): informational
- [ ] Failure mode tests (P0 from Q008):
  - Server crash mid-game: clients detect, reconnect, resume
  - Client crash mid-game: server continues, client rejoins
  - Mid-action disconnect: action cancelled, no partial state
  - Malformed message: server survives, logs warning
  - Message too large: rejected, connection survives
  - Invalid message type: ignored, connection survives
- [ ] Failure mode tests (P1 from Q008):
  - Disconnect during snapshot receive
  - Server full rejection
  - Client sends action for wrong player
  - Rapid connect/disconnect cycles (10 times in 5 seconds)
- [ ] Chaos tests (weekly, informational):
  - Random network conditions
  - Random disconnects during test
  - Goal: no crashes, no hangs, eventual consistency

**Dependencies:**
- Blocked by: 1-018 (Connection Error Handling)
- Blocks: None

**Agent Notes:**
- Systems Architect: These tests build confidence for real-world conditions.
- Engine Developer: Use deterministic seed for reproducible chaos tests.
- QA Engineer: P0 failure modes block release. P1 are important edge cases. Chaos tests track stability trends.
- NOTE: Write error handling tests alongside 1-018 implementation. Network condition tests can begin as soon as MockSocket latency injection is available.

---

## Implementation Order

Recommended implementation sequence:

**Phase 1: Infrastructure (Week 1)**
1. 1-001: ENet Library Integration
2. 1-002: NetworkBuffer Serialization
3. 1-003: Message Protocol Framework
4. 1-004: Network Thread and Queues

**Phase 2: Messages (Week 1-2)**
5. 1-005: Client-to-Server Messages
6. 1-006: Server-to-Client Messages
7. 1-007: Component Serialization

**Phase 3: Network Core (Week 2-3)**
8. 1-008: Server Network Loop
9. 1-009: Client Network Loop
10. 1-010: Player Session Management
11. 1-011: Input Message Transmission

**Phase 4: Synchronization (Week 3-4)**
12. 1-012: Change Detection
13. 1-013: Delta Generation/Application
14. 1-014: Full State Snapshot
15. 1-015: Entity ID Synchronization

**Phase 5: Integration (Week 4)**
16. 1-016: IPersistenceProvider Interface
17. 1-017: Game Loop Integration
18. 1-018: Connection Error Handling

**Phase 6: Testing (Week 4-5)**
19. 1-019: Testing Infrastructure
20. 1-020: Unit Tests
21. 1-021: Integration Tests - Multi-Client
22. 1-022: Integration Tests - Network Conditions

---

## Appendix: Resolved Discussion Questions

Key questions resolved in `/docs/discussions/epic-1-planning.discussion.yaml`:

| ID | Question | Resolution |
|----|----------|------------|
| Q001 | Socket library | ENet (reliable UDP) |
| Q003 | Latency budget | 250ms RTT target |
| Q004 | Serialization format | Manual binary with NetworkBuffer |
| Q010 | Mid-action disconnect | Cancel action (server rolls back) |
| Q014 | Lobby phase | Skip entirely - join directly into live world |
| Q016 | Server executable | Single exe with --server flag |
| Q018 | Entity ID mapping | Direct use in EnTT (no bidirectional map) |
| Q020 | Persistence interface | IPersistenceProvider with in-memory MVP |
| Q028 | Heartbeat/timeout | 1s heartbeat, 5s soft, 10s hard, 30s grace |
| Q030 | Delta detection | Dirty flags via EnTT signals |
| Q037 | Client prediction | None, but optimistic UI hints allowed |

---

## Ticket Classification (2026-01-28)

| Ticket | Status | Notes |
|--------|--------|-------|
| 1-001 | UNCHANGED | |
| 1-002 | UNCHANGED | |
| 1-003 | UNCHANGED | |
| 1-004 | UNCHANGED | |
| 1-005 | UNCHANGED | |
| 1-006 | MODIFIED | Added map_size_tier to ServerStatusMessage; updated snapshot size estimates and test matrix for configurable map sizes |
| 1-007 | UNCHANGED | |
| 1-008 | MODIFIED | Added map_size server configuration parameter (small/medium/large) |
| 1-009 | UNCHANGED | |
| 1-010 | UNCHANGED | |
| 1-011 | UNCHANGED | |
| 1-012 | UNCHANGED | |
| 1-013 | UNCHANGED | |
| 1-014 | MODIFIED | Updated snapshot size estimates to per-map-tier ranges; added large-map test scenario; bounded delta queue tuning note |
| 1-015 | UNCHANGED | |
| 1-016 | UNCHANGED | |
| 1-017 | UNCHANGED | |
| 1-018 | UNCHANGED | |
| 1-019 | MODIFIED | TestServer supports configurable map size parameter for parameterized tests |
| 1-020 | UNCHANGED | |
| 1-021 | MODIFIED | Added large-map integration test scenario (512x512) |
| 1-022 | UNCHANGED | |
