# Systems Architect Analysis: Epic 1

**Epic:** Multiplayer Networking
**Analyst:** Systems Architect
**Date:** 2026-01-26
**Canon Version Referenced:** 0.3.0

---

## Overview

Epic 1 establishes the multiplayer networking foundation that enables ZergCity's core vision: a shared alien colony builder where 2-4 players build together in real-time. This epic introduces two primary systems:

1. **NetworkManager** (core) - Socket connections, message serialization/deserialization, client/server state, lobby management, player connections
2. **SyncSystem** (ECS) - Delta updates, applying remote state, snapshots for reconnection

This is arguably the most architecturally critical epic after the foundation. Every subsequent gameplay system must consider how its state synchronizes across the network. The decisions made here ripple through all 15+ remaining epics.

---

## Key Work Items

### NetworkManager (Core System)

- [ ] **NET-01: Socket Abstraction Layer**
  - Abstract socket operations (TCP for reliable, UDP for optional future optimization)
  - Cross-platform socket handling (Windows/Linux/macOS differences)
  - Connection establishment, maintenance, and teardown
  - Non-blocking I/O with select/poll or async patterns

- [ ] **NET-02: Message Protocol Design**
  - Binary message format with header (type, size, sequence number)
  - Message type registry for routing
  - Versioning support for protocol evolution
  - Compression option for large state updates

- [ ] **NET-03: Server Network Loop**
  - Accept incoming client connections
  - Per-client state management (connection state, player info, message queues)
  - Broadcast vs targeted message sending
  - Connection timeout and keepalive handling
  - Maximum player limit enforcement (2-4 players per canon)

- [ ] **NET-04: Client Network Loop**
  - Connect to server by IP/port
  - Reconnection logic with exponential backoff
  - Handle server unavailable gracefully
  - Connection state machine (Disconnected, Connecting, Connected, Reconnecting)

- [ ] **NET-05: Lobby Management**
  - Player join/leave handling
  - Player name assignment and validation
  - Ready state for game start coordination
  - Late join support (player joins active game)
  - Spectator mode consideration (not in MVP but design for extensibility)

- [ ] **NET-06: Authentication & Session Management**
  - Simple session tokens (not full auth - no accounts yet)
  - Reconnection token for client recovery
  - Server-assigned PlayerID (1-4, not 0 which is GAME_MASTER)
  - Session persistence across server restarts (database-backed)

- [ ] **NET-07: Database Integration for Persistence**
  - Database connection management (SQLite for MVP, PostgreSQL later)
  - Continuous state persistence (every N ticks or on significant changes)
  - Schema design for game state tables
  - Recovery on server restart: load state from DB, allow reconnections

- [ ] **NET-08: INetworkHandler Interface Implementation**
  - Per-canon interface: `handle_message(message, sender)` and `get_handled_types()`
  - Message routing from NetworkManager to registered handlers
  - Handler registration/unregistration

### SyncSystem (ECS System)

- [ ] **SYNC-01: Change Detection**
  - Track which components changed since last sync tick
  - Dirty flag pattern or generation counter per component
  - Efficient iteration over changed entities only
  - Support for SyncPolicy metadata (EveryTick, OnChange, Never)

- [ ] **SYNC-02: Delta Update Generation**
  - Build minimal delta message containing only changed component data
  - Entity create/destroy events
  - Component add/remove events
  - Batch changes efficiently (one message per tick, not per change)

- [ ] **SYNC-03: Delta Update Application (Client)**
  - Deserialize delta message
  - Create/destroy entities as directed by server
  - Update component values
  - Handle out-of-order or duplicate messages gracefully

- [ ] **SYNC-04: Full State Snapshot**
  - Generate complete world state for reconnection
  - Compress snapshot (can be large for mature cities)
  - Incremental snapshot streaming for large states
  - Client requests snapshot on connect or after detecting desync

- [ ] **SYNC-05: Snapshot Application (Client)**
  - Replace entire ECS state with snapshot
  - Clear local state before applying
  - Resume normal delta updates after snapshot

- [ ] **SYNC-06: Conflict Resolution**
  - Server-authoritative model: server state always wins
  - Client input validation before sending (optimistic UI)
  - Server validation and rejection with feedback
  - Rollback pattern if client made invalid assumption

### Message Types (Per Canon)

- [ ] **MSG-01: Client-to-Server Messages**
  - InputMessage: Player action (build, zone, demolish, etc.) - defined in Epic 0
  - JoinMessage: Player joining game
  - ChatMessage: Text chat (alien-themed: "broadcast to fellow overseers")
  - PurchaseTileMessage: Buy a tile from Game Master
  - TradeMessage: Resource trade offer/accept (future: Epic 11 Financial)
  - ReconnectMessage: Client recovering session

- [ ] **MSG-02: Server-to-Client Messages**
  - StateUpdateMessage: Delta component changes
  - EventMessage: Game events (disaster, milestone)
  - FullStateMessage: Complete snapshot (reconnect)
  - TradeNotification: Trade offers and completions
  - PlayerListMessage: Current players in game
  - RejectionMessage: Server rejected client action (with reason)

### Error Handling & Edge Cases

- [ ] **ERR-01: Connection Error Handling**
  - Network timeout handling
  - Malformed message handling (don't crash server)
  - Rate limiting (prevent DoS from misbehaving client)
  - Graceful degradation (partial connectivity)

- [ ] **ERR-02: Desync Detection**
  - Periodic checksum of critical state
  - Client can request resync if checksum mismatch
  - Logging of desync events for debugging

- [ ] **ERR-03: Disconnect Handling**
  - Client disconnect: Player colony continues (per canon - no pause, no AI takeover)
  - Server disconnect: Clients wait for reconnection opportunity
  - Ghost town process trigger on abandonment (prolonged disconnect or explicit restart)

---

## Questions for Other Agents

### @engine-developer

- **Q1:** What socket library should we use? Options: raw BSD sockets, SDL_net (if it exists for SDL3), ENet, boost::asio, or custom abstraction over platform APIs?

- **Q2:** For database persistence, SQLite is simple but single-writer. Should we use a separate DB thread with message passing, or serialize all DB writes to the server's main thread?

- **Q3:** What's the expected latency budget? 100ms round-trip is typical LAN, 200-500ms for internet. How should we tune keepalive intervals and timeout thresholds?

- **Q4:** For message serialization, should we use a library (FlatBuffers, protobuf, Cap'n Proto) or hand-roll binary serialization? Epic 0 established manual serialization - continue that pattern?

- **Q5:** How should we handle the two-phase server startup? Phase 1: Load DB state. Phase 2: Accept connections. Should there be a "loading" period where connections are queued but not processed?

### @qa-engineer

- **Q6:** What network conditions should we test? Packet loss, high latency, jitter, reconnection scenarios? Should we build a network simulation layer for testing?

- **Q7:** How do we test multiplayer without multiple machines? Multiple client instances on same machine? Headless test harness?

- **Q8:** What are the failure modes we need to validate? Server crash recovery, client crash recovery, mid-action disconnect, malformed messages?

- **Q9:** Should we have automated integration tests for the sync protocol? E.g., spin up server + clients, perform actions, verify state consistency?

### @game-designer

- **Q10:** When a player disconnects mid-action (e.g., placing a building), what happens? Cancel the action, or complete it? Server-authoritative means server decides, but what's the UX intent?

- **Q11:** For late join, how much history should a new player see? Just current state, or a brief "catch-up" log of recent significant events (buildings constructed, disasters, etc.)?

- **Q12:** The ghost town process is triggered by "prolonged inactivity" - what's the threshold? 24 real-world hours? 100 simulation cycles? Should it be configurable by server operator?

- **Q13:** Chat messages - should they be persistent (stored in DB) or ephemeral (lost on disconnect)? How many messages to show to late-joining players?

- **Q14:** For the lobby phase before game start, what does a player see/do? Empty map preview? Player list? Ready checkbox? Alien-themed "Awaiting fellow overseers..." messaging?

- **Q15:** When server rejects a player action (e.g., can't afford building), how should the rejection be communicated? Immediate UI feedback? Toast notification? Sound effect?

---

## Risks & Concerns

### Risk 1: Network Library Choice (High Impact)

**Severity:** High
**Description:** The socket/networking library choice affects all networking code. Wrong choice could require significant rewrite.
**Mitigation:** Prototype with 2-3 options. ENet is battle-tested for games (UDP with reliability). SDL_net may not exist for SDL3. Raw sockets work but require more code.
**Recommendation:** ENet for its game-focused design, or asio for modern C++ patterns.

### Risk 2: Database Write Throughput (Medium Impact)

**Severity:** Medium
**Description:** Continuous persistence at 20 ticks/second could overwhelm SQLite. Each tick might have dozens of component changes.
**Mitigation:** Batch writes. Write every N ticks (e.g., every 20 ticks = 1 second). Use transactions for atomicity. Defer non-critical writes. Consider write-ahead log pattern.
**Concern:** Data loss window between writes if server crashes.

### Risk 3: State Snapshot Size (Medium Impact)

**Severity:** Medium
**Description:** Full state snapshots for reconnection could be very large for mature cities (thousands of entities, components). Serializing and sending could take seconds.
**Mitigation:** Incremental snapshot streaming. Compress snapshots. Client shows "Loading colony state..." during sync. Consider delta-from-previous-snapshot for returning players.

### Risk 4: No Client-Side Prediction (Design Constraint)

**Severity:** Low (accepted trade-off)
**Description:** Canon explicitly forbids client-side prediction for simplicity. This means all player actions have at least one round-trip latency before visual feedback.
**Mitigation:** Optimistic UI (show "pending" state while waiting for server confirmation). Clear visual feedback for pending vs confirmed actions. Accept that high-latency connections (500ms+) will feel sluggish.
**Note:** This is a design decision, not a risk to mitigate. Document the trade-off.

### Risk 5: Message Ordering and Reliability (Medium Impact)

**Severity:** Medium
**Description:** TCP guarantees order and delivery but can have head-of-line blocking. UDP is faster but requires manual reliability layer.
**Mitigation:** Use TCP for MVP (simpler). If performance is an issue, consider UDP + reliability layer (ENet provides this). State updates are ordered by server tick, so out-of-order client messages are resolvable.

### Risk 6: Late Join Complexity (Medium Impact)

**Severity:** Medium
**Description:** Late-joining player needs full state snapshot while other players continue playing. Server must handle snapshot generation without blocking simulation.
**Mitigation:** Snapshot generation on separate thread or interleaved with simulation. Queue incoming actions during snapshot send. Client buffers updates received during snapshot apply.

### Risk 7: Ghost Town Handoff (Low but Nuanced)

**Severity:** Low
**Description:** When player abandons (triggers ghost town), their tiles transition to GAME_MASTER ownership over time. Other players might try to interact with decaying structures. Need clear rules.
**Mitigation:** Define interaction rules: Can other players demolish ghost town buildings? Can they purchase ghost town tiles immediately or must they wait for "cleared" state? These are design questions requiring @game-designer input.

### Risk 8: Security Concerns (Low for MVP)

**Severity:** Low (for private games)
**Description:** No encryption, no authentication. Clients could spoof messages, view others' state, or DoS the server.
**Mitigation:** MVP is for trusted friend groups on LAN/private servers. Document security limitations. Future epic could add TLS and authentication.

---

## Dependencies

### Epic 1 Depends On

- **Epic 0 (Foundation)** - Hard dependency
  - Application: Game loop, server/client mode detection
  - InputSystem: Produces InputMessages to send
  - Core Types: EntityID, PlayerID, GridPosition, SimulationTime
  - Serialization: ISerializable interface, byte buffer operations
  - ECS: EnTT integration for SyncSystem

### Epic 1 Blocks

- **Epic 2 (Rendering)** - Soft dependency
  - RenderingSystem can work locally, but multiplayer rendering needs synced state

- **Epic 3 (Terrain)** - Soft dependency
  - Terrain needs to sync across players

- **Epic 4+ (All Gameplay)** - Hard dependency
  - Every gameplay system must integrate with SyncSystem
  - All components must implement ISerializable
  - All player actions must go through NetworkManager

### Internal Dependencies (Within Epic 1)

```
NET-01 (Sockets) ──► NET-03 (Server Loop) ──► NET-05 (Lobby)
        │                    │                      │
        ▼                    ▼                      ▼
    NET-02 (Protocol)   NET-04 (Client Loop)  NET-06 (Sessions)
        │                    │                      │
        ▼                    ▼                      ▼
    MSG-01, MSG-02      SYNC-03 (Apply)       NET-07 (Database)
        │                    │
        ▼                    ▼
    NET-08 (Handlers)   SYNC-04, SYNC-05 (Snapshots)

SYNC-01 (Change Detection) ──► SYNC-02 (Delta Gen) ──► MSG-02 (StateUpdate)
                                      │
                                      ▼
                              SYNC-06 (Conflict Res)
```

Critical path:
1. NET-01 (Sockets) - fundamental
2. NET-02 (Protocol) - message format
3. NET-03 (Server Loop) - server can accept connections
4. NET-04 (Client Loop) - client can connect
5. MSG-01/MSG-02 (Messages) - meaningful communication
6. SYNC-01, SYNC-02 (Delta) - state synchronization
7. NET-07 (Database) - persistence

---

## System Interaction Map

```
┌─────────────────────────────────────────────────────────────────────┐
│                           SERVER PROCESS                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐    ┌──────────────┐    ┌───────────────────┐      │
│  │ Application │───►│  Simulation  │───►│     SyncSystem    │      │
│  │ (Epic 0)    │    │  Tick Loop   │    │  (Delta + Snap)   │      │
│  └─────────────┘    └──────────────┘    └─────────┬─────────┘      │
│         │                  │                      │                 │
│         ▼                  ▼                      ▼                 │
│  ┌─────────────┐    ┌──────────────┐    ┌───────────────────┐      │
│  │   ECS       │◄──►│  All Game    │    │  NetworkManager   │      │
│  │  Registry   │    │   Systems    │    │  (Core)           │      │
│  └─────────────┘    └──────────────┘    └────────┬──────────┘      │
│                                                   │                 │
│                                                   ▼                 │
│                                          ┌───────────────────┐      │
│                                          │     Database      │      │
│                                          │   (Persistence)   │      │
│                                          └───────────────────┘      │
│                                                   │                 │
└───────────────────────────────────────────────────┼─────────────────┘
                                                    │ TCP/UDP
                           ┌────────────────────────┼────────────────────────┐
                           │                        │                        │
                           ▼                        ▼                        ▼
┌──────────────────────────────┐  ┌──────────────────────────────┐  ┌──────────────────┐
│       CLIENT 1               │  │       CLIENT 2               │  │   CLIENT 3...    │
├──────────────────────────────┤  ├──────────────────────────────┤  ├──────────────────┤
│                              │  │                              │  │                  │
│  ┌────────────┐              │  │  ┌────────────┐              │  │   (Same as       │
│  │ InputSys   │──InputMsg──► │  │  │ InputSys   │──InputMsg──► │  │    Client 1)     │
│  └────────────┘              │  │  └────────────┘              │  │                  │
│        │                     │  │        │                     │  │                  │
│        ▼                     │  │        ▼                     │  │                  │
│  ┌────────────┐              │  │  ┌────────────┐              │  │                  │
│  │ NetworkMgr │◄─StateUpdate─│  │  │ NetworkMgr │◄─StateUpdate─│  │                  │
│  └────────────┘              │  │  └────────────┘              │  │                  │
│        │                     │  │        │                     │  │                  │
│        ▼                     │  │        ▼                     │  │                  │
│  ┌────────────┐              │  │  ┌────────────┐              │  │                  │
│  │ SyncSystem │              │  │  │ SyncSystem │              │  │                  │
│  │ (Apply)    │              │  │  │ (Apply)    │              │  │                  │
│  └────────────┘              │  │  └────────────┘              │  │                  │
│        │                     │  │        │                     │  │                  │
│        ▼                     │  │        ▼                     │  │                  │
│  ┌────────────┐              │  │  ┌────────────┐              │  │                  │
│  │   ECS      │              │  │  │   ECS      │              │  │                  │
│  │  Registry  │              │  │  │  Registry  │              │  │                  │
│  └────────────┘              │  │  └────────────┘              │  │                  │
│        │                     │  │        │                     │  │                  │
│        ▼                     │  │        ▼                     │  │                  │
│  ┌────────────┐              │  │  ┌────────────┐              │  │                  │
│  │ RenderSys  │              │  │  │ RenderSys  │              │  │                  │
│  │ (Epic 2)   │              │  │  │ (Epic 2)   │              │  │                  │
│  └────────────┘              │  │  └────────────┘              │  │                  │
│                              │  │                              │  │                  │
└──────────────────────────────┘  └──────────────────────────────┘  └──────────────────┘
```

### Data Flow Per Tick

**Server (Every 50ms):**
1. Receive all pending InputMessages from clients
2. Validate each input (player owns target tile, has resources, etc.)
3. Apply valid inputs to ECS
4. Run simulation tick (all ISimulatable systems)
5. SyncSystem detects changed components
6. SyncSystem builds StateUpdateMessage (delta)
7. NetworkManager broadcasts StateUpdateMessage to all clients
8. Periodically: Persist state to database

**Client (Every Frame):**
1. InputSystem captures player input
2. InputSystem produces InputMessage
3. NetworkManager queues InputMessage for sending
4. NetworkManager sends queued messages
5. NetworkManager receives StateUpdateMessage
6. SyncSystem applies delta to local ECS
7. RenderSystem renders local ECS state (with interpolation)

---

## Multiplayer Architecture Notes

### Server Authority Model

Per canon, the server is fully authoritative:

| Aspect | Authority |
|--------|-----------|
| Entity creation/destruction | Server |
| Component values | Server |
| Random number generation | Server |
| Time progression | Server |
| Input validation | Server (client can pre-validate for UX) |
| Conflict resolution | Server state wins |

Clients are essentially "dumb terminals" that:
1. Send input
2. Receive state
3. Render state

### Sync Rate and Interpolation

- **Simulation:** 20 ticks/second (50ms per tick)
- **Sync:** StateUpdateMessage sent after each tick
- **Rendering:** 60fps target, interpolate between last two received states
- **Latency hiding:** None (no prediction). Accepted trade-off for simplicity.

### Connection States

```
Client State Machine:
┌──────────────┐
│ Disconnected │◄──────────────────────────────────────┐
└──────┬───────┘                                       │
       │ connect()                                     │ timeout/error
       ▼                                               │
┌──────────────┐                                       │
│  Connecting  │───────────────────────────────────────┤
└──────┬───────┘                                       │
       │ JoinMessage accepted                          │
       ▼                                               │
┌──────────────┐                                       │
│   InLobby    │                                       │
└──────┬───────┘                                       │
       │ Game starts / late join                       │
       ▼                                               │
┌──────────────┐    FullStateMessage                   │
│   Loading    │◄──────────────────────────────────────┤
└──────┬───────┘                                       │
       │ Snapshot applied                              │
       ▼                                               │
┌──────────────┐    connection lost                    │
│   Playing    │───────────────────────────────────────┤
└──────┬───────┘                                       │
       │ explicit disconnect                           │
       ▼                                               │
┌──────────────┐                                       │
│ Disconnected │◄──────────────────────────────────────┘
└──────────────┘
```

### Database Persistence Strategy

**Why database-backed persistence (per canon):**
- Game state survives server restart
- Server crash doesn't lose hours of play
- Enables "pause server, resume later" pattern
- Foundation for future server migration/scaling

**Implementation approach:**
1. Write-ahead log for critical actions (building placed, tile purchased)
2. Periodic full checkpoint (every N ticks or on significant state change)
3. On server start: Load last checkpoint, replay WAL, resume accepting connections

**Tables (conceptual):**
- `entities` - EntityID, component bitmask, creation tick
- `components_position` - EntityID, grid_x, grid_y, elevation
- `components_ownership` - EntityID, owner, state, state_changed_at
- `players` - PlayerID, name, session_token, last_seen
- `game_meta` - current_tick, rng_seed, etc.

### Late Join Flow

1. Client connects, sends JoinMessage
2. Server assigns PlayerID (or reconnects existing if session valid)
3. Server sends PlayerListMessage (current players)
4. Server begins snapshot generation (potentially async)
5. During snapshot generation, server continues simulation, queues deltas
6. Server sends FullStateMessage (snapshot)
7. Server sends queued deltas (if any)
8. Client applies snapshot, then deltas
9. Client now receives normal delta stream

### Ghost Town Integration

When player abandons (per canon patterns.yaml):
1. AbandonmentSystem (part of SimulationCore, Epic 10) detects trigger
2. OwnershipComponent.state transitions: Owned -> Abandoned
3. SyncSystem broadcasts change
4. Decay timer begins (configurable duration)
5. After decay: Abandoned -> GhostTown -> Cleared
6. Cleared tiles have owner = GAME_MASTER, can be purchased

**Network implication:** Ownership changes must sync. All clients see ghost town decay.

---

## Interface Implementation Details

### ISerializable (From Epic 0)

Every synced component must implement:
```cpp
void serialize(WriteBuffer& buffer) const;
void deserialize(ReadBuffer& buffer);
size_t get_serialized_size() const;
```

### INetworkHandler (New in Epic 1)

Systems that handle specific message types:
```cpp
void handle_message(const NetworkMessage& message, PlayerID sender);
std::vector<MessageType> get_handled_types() const;
```

Example handlers:
- `InputHandler`: Handles InputMessage, applies to simulation
- `ChatHandler`: Handles ChatMessage, broadcasts to all
- `TradeHandler`: Handles TradeMessage (future)

### NetworkMessage Base

```cpp
struct NetworkMessage {
    MessageType type;
    uint32_t sequence;
    uint64_t timestamp;
    // Payload follows, type-specific
};
```

---

## Summary

Epic 1 is the multiplayer backbone. Key principles:

1. **Server is the single source of truth** - No client-side prediction, no distributed state
2. **Database persistence is continuous** - Crash recovery is built-in, not an afterthought
3. **Delta sync for efficiency** - Only changed components travel the wire each tick
4. **Full snapshots for reconnection** - Late join and recovery are first-class features
5. **Ghost town process is network-aware** - Ownership transitions sync to all clients

The work items above establish the networking layer. Once complete, all future gameplay systems simply:
1. Implement ISerializable on their components
2. Register with SyncSystem for sync
3. Handle relevant network messages if needed

The complexity is front-loaded in Epic 1 so subsequent epics can focus on gameplay.
