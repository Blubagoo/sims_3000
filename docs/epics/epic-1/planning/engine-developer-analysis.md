# Engine Developer Analysis: Epic 1

**Epic:** Multiplayer Networking
**Analyst:** Engine Developer
**Date:** 2026-01-26
**Canon Version Referenced:** 0.3.0

---

## Overview

Epic 1 introduces the networking layer that enables ZergCity's multiplayer architecture. This epic covers two systems:

1. **NetworkManager** (core) - Socket connections, message serialization/deserialization, client/server state, lobby management, player connections
2. **SyncSystem** (ECS) - Delta updates, applying remote state, snapshots for reconnection

Key architectural constraints from canon:
- Dedicated server architecture (separate .exe, not player-hosted)
- Server-authoritative for ALL simulation
- 20 ticks/sec sync rate (50ms per tick)
- No client-side prediction (simplicity over responsiveness)
- Database-backed persistence (deferred to Epic 16, but architecture must support it)

---

## Key Work Items

### NetworkManager Core

- [ ] **NET-01: Networking Library Integration**
  - Integrate chosen networking library (see Technical Recommendations)
  - Add to vcpkg.json dependencies
  - Basic connection/disconnection handling
  - Error handling with alien terminology ("Link Disruption Detected")

- [ ] **NET-02: Server Socket Management**
  - Create listening socket for incoming connections
  - Accept new client connections (up to 4 players per canon)
  - Track connection state per client
  - Handle graceful and ungraceful disconnections

- [ ] **NET-03: Client Connection Management**
  - Connect to server by IP/hostname and port
  - Handle connection timeout
  - Reconnection logic with exponential backoff
  - Connection state machine (Disconnected -> Connecting -> Connected -> Authenticated)

- [ ] **NET-04: Message Serialization Framework**
  - Binary message format with header (type, length, sequence)
  - Little-endian byte order per canon interfaces.yaml
  - Version field for protocol evolution
  - Integrate with Epic 0's ISerializable interface

- [ ] **NET-05: Message Types Definition**
  - Client-to-server: InputMessage, JoinMessage, ChatMessage, PurchaseTileMessage, TradeMessage
  - Server-to-client: StateUpdateMessage, EventMessage, FullStateMessage, TradeNotification
  - Message routing to appropriate handlers

- [ ] **NET-06: Lobby Management**
  - Player join/leave handling
  - Player list management
  - Player authentication (simple password or token initially)
  - Late join support per canon

- [ ] **NET-07: Network Threading Model**
  - Dedicated network I/O thread (see Technical Recommendations)
  - Thread-safe message queues (send/receive)
  - Main thread processes received messages during game loop
  - No blocking on main thread

### SyncSystem (ECS)

- [ ] **SYNC-01: Change Detection System**
  - Track dirty components since last sync
  - Use EnTT signals/observers for change notification
  - Efficient dirty flag management per entity/component

- [ ] **SYNC-02: Delta Update Generation (Server)**
  - Build delta update containing only changed components
  - Compress redundant data (entity IDs, unchanged fields)
  - Respect SyncPolicy metadata from Epic 0 (EveryTick, OnChange, Never)

- [ ] **SYNC-03: Delta Update Application (Client)**
  - Receive and apply delta updates to local ECS registry
  - Handle entity creation/destruction messages
  - Update interpolation buffers (previous = current, current = new)

- [ ] **SYNC-04: Full Snapshot Generation (Server)**
  - Serialize complete game state for reconnection
  - Include all entities and components with SyncPolicy != Never
  - Compress for efficient transmission (consider zlib/lz4)

- [ ] **SYNC-05: Snapshot Application (Client)**
  - Clear local state and apply full snapshot
  - Resume normal delta sync after snapshot
  - Loading progress indication during snapshot transfer

- [ ] **SYNC-06: Entity ID Synchronization**
  - Server assigns EntityIDs (authoritative)
  - Client maps server EntityIDs to local EnTT handles
  - Handle EntityID reuse after entity destruction

### Integration with Epic 0

- [ ] **INT-01: Game Loop Integration**
  - NetworkManager::poll() called each frame
  - Received messages processed before simulation tick
  - Outbound messages (deltas) sent after simulation tick
  - Maintain 50ms tick interval on server

- [ ] **INT-02: Application State Integration**
  - Connecting state uses NetworkManager
  - Playing state processes network messages
  - Handle disconnect -> return to Menu state

- [ ] **INT-03: Server Mode Integration**
  - Server mode (--server flag) runs NetworkManager in server role
  - No window/rendering, only simulation and network
  - Server CLI commands from Epic 0 now functional (players, kick, etc.)

- [ ] **INT-04: InputMessage Transmission**
  - Epic 0 defines InputMessage format
  - NetworkManager serializes and transmits to server
  - Server validates and applies to simulation

### Server Executable Structure

- [ ] **SRV-01: Server Configuration**
  - Port number (default 7777)
  - Max players (default 4, configurable)
  - Database connection string (placeholder for Epic 16)
  - Tick rate (fixed 20/sec)

- [ ] **SRV-02: Server Main Loop**
  - Accept connections
  - Process incoming messages
  - Run simulation tick
  - Send delta updates
  - Handle admin commands from CLI

- [ ] **SRV-03: Player Session Management**
  - Track connected players with PlayerID
  - Assign PlayerIDs on join (1-4 per canon)
  - Handle reconnection (same player, same PlayerID)
  - Session timeout for disconnected players

---

## Questions for Other Agents

### @systems-architect:

- **Q1:** For the server executable, should we use the same single-executable approach from Epic 0 (--server flag) or create a dedicated server project in CMake? The single executable is simpler but the server will eventually need different dependencies (database driver, no SDL video).

- **Q2:** What's the expected message frequency for StateUpdateMessage? One per tick (20/sec) or batched? Canon says "sync happens after each simulation tick" which implies one per tick.

- **Q3:** For entity ID synchronization, should clients maintain a bidirectional map (server ID <-> local EnTT handle) or can we require EnTT to use server-assigned IDs directly?

- **Q4:** How should we handle the scenario where a client is receiving a large snapshot during reconnection but other players are still playing? Queue up delta updates during snapshot transfer?

- **Q5:** The canon mentions database-backed persistence. Should NetworkManager have an interface for persistence operations (saveState, loadState) even if the actual database is Epic 16?

### @qa-engineer:

- **Q6:** What network conditions should we test? Suggestions: packet loss, high latency (200ms+), jitter, bandwidth throttling, disconnect/reconnect cycles?

- **Q7:** How do we test multiplayer without spinning up multiple machines? Should we build a localhost multi-instance test framework?

- **Q8:** What are the acceptance criteria for "smooth" synchronization? Max allowed latency between server state change and client visual update?

---

## Risks & Concerns

### Risk 1: Network Library Complexity
**Severity:** High
**Description:** Raw sockets require handling fragmentation, reliability, ordering manually. Higher-level libraries (ENet, GameNetworkingSockets) provide this but add dependencies and learning curve.
**Mitigation:** Use ENet - mature, lightweight, handles reliability/ordering, minimal dependencies. See Technical Recommendations.

### Risk 2: Threading Bugs
**Severity:** High
**Description:** Network I/O on separate thread introduces race conditions, deadlocks. EnTT registry is not thread-safe by default.
**Mitigation:**
- Network thread only touches message queues (not ECS)
- Use lock-free queues (SPSC) where possible
- Main thread exclusively modifies ECS
- Document thread ownership clearly

### Risk 3: Serialization Performance
**Severity:** Medium
**Description:** 20 messages/sec to potentially 4 clients = 80 serializations/sec for delta updates. Plus component data. Could be CPU-bound.
**Mitigation:**
- Profile early with representative data
- Consider memory pooling for message buffers
- Only serialize changed components (delta tracking is critical)
- Binary format, not text (JSON would be too slow)

### Risk 4: Snapshot Size for Large Cities
**Severity:** Medium
**Description:** Late game cities may have thousands of entities. Full snapshot for reconnection could be megabytes.
**Mitigation:**
- Compress snapshots (lz4 for speed, zlib for ratio)
- Stream snapshot in chunks with progress indication
- Consider incremental snapshots if full proves too large

### Risk 5: Entity ID Lifecycle
**Severity:** Medium
**Description:** Server creates entities, assigns IDs. Client must track mapping. When server destroys entity, client must clean up. ID reuse can cause stale references.
**Mitigation:**
- Generation counter in EntityID (high bits = generation, low bits = index)
- Or use monotonically increasing IDs (simpler, but grows forever)
- Clear guidance in documentation

### Risk 6: Late Join Complexity
**Severity:** Medium
**Description:** Canon requires late join support. New player joins mid-game, needs full state snapshot while other players continue playing.
**Mitigation:**
- Server generates snapshot for new player
- New player in "loading" state, receives snapshot
- Once loaded, switches to delta sync
- Other players continue normally (their deltas queued for new player)

### Risk 7: Clock Synchronization
**Severity:** Low
**Description:** Server tick rate is 20/sec. Clients need to know server's current tick for interpolation. Clock drift can cause desync.
**Mitigation:**
- Include server tick number in every StateUpdateMessage
- Client interpolates between two most recent states
- No need for tight clock sync - we're not doing prediction

---

## Technical Recommendations

### 1. Networking Library: ENet

**Recommendation:** Use ENet (http://enet.besez.net/)

**Rationale:**
- Lightweight C library, easy C++ wrapper
- Reliable UDP with sequencing and channels
- Handles packet fragmentation/reassembly
- Connection management built-in
- Proven in games (Cube 2, several indie titles)
- vcpkg package available: `enet`
- Cross-platform (Windows, Linux, macOS)

**Alternatives Considered:**
- **SDL_net:** Too low-level, no reliability layer, would need to implement ordering ourselves
- **GameNetworkingSockets (Valve):** More complex, better for Steam integration (which we don't need)
- **RakNet:** Discontinued, licensing unclear
- **Raw sockets:** Maximum control but huge implementation effort, bug-prone

**Integration:**
```cpp
// Server initialization
ENetHost* server = enet_host_create(&address, 4, 2, 0, 0);
// 4 = max clients, 2 = channels (reliable + unreliable)

// Client initialization
ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
```

### 2. Message Serialization

**Recommendation:** Custom binary serialization with manual packing

**Rationale:**
- Full control over wire format
- Minimal overhead (no schema metadata)
- Aligns with Epic 0's ISerializable pattern
- Simple for fixed-layout messages

**Format:**
```cpp
struct MessageHeader {
    uint16_t type;        // MessageType enum
    uint16_t length;      // Payload length in bytes
    uint32_t sequence;    // For ordering/ack
};
// Total header: 8 bytes

// Example StateUpdateMessage payload:
struct StateUpdatePayload {
    uint64_t tick;           // Server tick number
    uint16_t entity_count;   // Number of entity updates
    // Variable: EntityUpdate[] entities
};

struct EntityUpdate {
    uint32_t entity_id;
    uint8_t component_mask;  // Bitfield of which components changed
    // Variable: component data based on mask
};
```

**Alternatives Considered:**
- **FlatBuffers/Cap'n Proto:** Zero-copy but complex, overkill for our message sizes
- **Protocol Buffers:** Schema overhead, reflection costs
- **JSON:** Way too slow for 20/sec sync, larger wire size

### 3. Threading Model

**Recommendation:** Dedicated network I/O thread with lock-free message queues

**Architecture:**
```
+-------------------+                    +-------------------+
|   Main Thread     |                    |  Network Thread   |
|-------------------|                    |-------------------|
| - Game loop       |   Send Queue       | - ENet polling    |
| - ECS updates     | ----------------> | - Socket send/recv|
| - Message process |   (lock-free)      | - Serialization   |
|                   | <---------------- |                   |
|                   |   Receive Queue    |                   |
+-------------------+   (lock-free)      +-------------------+
```

**Rules:**
- Network thread NEVER touches ECS registry
- Network thread only reads from send queue, writes to receive queue
- Main thread processes receive queue during update phase
- Use SPSC (single-producer single-consumer) queues for optimal performance

**Implementation:**
- Use `moodycamel::ReaderWriterQueue` (header-only, high performance)
- Or roll simple ring buffer if we want minimal dependencies

### 4. Integration with Epic 0 Game Loop

**Current loop (from main.cpp):**
```cpp
while (running) {
    SDL_PollEvent(...);
    // Render
}
```

**Epic 1 loop structure:**
```cpp
while (running) {
    // 1. Poll input events (Epic 0)
    inputSystem.pollEvents();

    // 2. Poll network messages (Epic 1)
    networkManager.poll();

    // 3. Process received messages
    networkManager.processReceivedMessages();

    // 4. Simulation tick (if accumulator >= TICK_DURATION)
    while (accumulator >= TICK_DURATION) {
        if (isServer) {
            simulationCore.tick();
            syncSystem.generateAndSendDeltas();
        } else {
            // Client: apply received state updates
            syncSystem.applyPendingUpdates();
        }
        accumulator -= TICK_DURATION;
    }

    // 5. Render (client only, with interpolation)
    if (!isServer) {
        float alpha = accumulator / TICK_DURATION;
        renderingSystem.render(alpha);
    }
}
```

### 5. Server Executable Structure

**Recommendation:** Single executable with --server flag (as established in Epic 0)

**Server-specific behavior:**
- Skip SDL_INIT_VIDEO (audio TBD, probably skip too)
- Skip window creation
- Skip rendering system
- Skip input system (use CLI instead)
- Run simulation at fixed 20 ticks/sec
- Process network messages
- Send delta updates

**CLI integration with Epic 0 tickets (0-022):**
- `status` - now shows connected players, tick rate
- `players` - now shows actual connected players
- `kick <player>` - now functional, disconnects player
- `say <message>` - broadcasts to all connected clients

**Server main function:**
```cpp
int main(int argc, char* argv[]) {
    bool serverMode = hasFlag(argv, "--server");

    Application app;
    app.init(serverMode);  // Conditionally inits SDL

    if (serverMode) {
        ServerCLI cli;
        NetworkManager network(NetworkRole::Server);
        // ... server loop
    } else {
        NetworkManager network(NetworkRole::Client);
        // ... client loop with rendering
    }
}
```

---

## Integration with Epic 0

### Dependencies on Epic 0

| Epic 0 Ticket | Used By Epic 1 | How |
|---------------|----------------|-----|
| 0-002 (SDL Init) | NetworkManager | Conditional init for server mode |
| 0-004 (Game Loop) | NetworkManager, SyncSystem | Poll and process network in loop |
| 0-005 (State Management) | NetworkManager | Connecting state, disconnect handling |
| 0-006 (Server Mode) | NetworkManager | --server flag detection |
| 0-022 (Server CLI) | NetworkManager | CLI commands now functional |
| 0-026 (Core Types) | All messages | PlayerID, EntityID, GridPosition |
| 0-027 (InputMessage) | NetworkManager | Transmit input to server |
| 0-028 (Serialization) | All messages | ISerializable interface |
| 0-031 (SyncPolicy) | SyncSystem | Determines what syncs how often |

### Modifications to Epic 0 Systems

**Application:**
- Add NetworkManager to initialization sequence
- Add network poll to game loop
- Handle connection state transitions

**InputSystem:**
- InputMessages now transmitted via NetworkManager (not just queued locally)

**ECS Components (PositionComponent, OwnershipComponent):**
- Now actually synchronized over network using SyncSystem

---

## Bandwidth Estimates

Rough calculations for design validation:

**Per-tick delta update (typical):**
- Header: 8 bytes
- Tick number: 8 bytes
- Entity count: 2 bytes
- Per entity (10 changed per tick average):
  - Entity ID: 4 bytes
  - Component mask: 1 byte
  - PositionComponent: 12 bytes
  - Total per entity: ~17 bytes
- Total per tick: 8 + 8 + 2 + (10 * 17) = ~188 bytes

**Per second (20 ticks):**
- 188 * 20 = 3,760 bytes/sec = ~3.7 KB/sec per client
- 4 clients = ~15 KB/sec outbound from server

**Full snapshot (mid-game, 1000 entities):**
- Entity ID + components: ~50 bytes average per entity
- 1000 * 50 = 50 KB uncompressed
- With lz4: ~15-20 KB compressed
- Acceptable for reconnection

These numbers are well within typical network capacity and suggest our design is feasible.

---

## Implementation Order

Suggested implementation sequence based on dependencies:

1. **Phase 1: Library Integration**
   - NET-01: Integrate ENet
   - NET-04: Message serialization framework

2. **Phase 2: Basic Connectivity**
   - NET-02: Server socket management
   - NET-03: Client connection management
   - NET-07: Threading model

3. **Phase 3: Message Flow**
   - NET-05: Message types
   - INT-01: Game loop integration
   - INT-04: InputMessage transmission

4. **Phase 4: State Synchronization**
   - SYNC-01: Change detection
   - SYNC-02: Delta generation (server)
   - SYNC-03: Delta application (client)
   - SYNC-06: Entity ID synchronization

5. **Phase 5: Reconnection Support**
   - SYNC-04: Full snapshot generation
   - SYNC-05: Snapshot application

6. **Phase 6: Player Management**
   - NET-06: Lobby management
   - SRV-03: Player session management
   - INT-02, INT-03: Application integration

---

## Summary

Epic 1 is the first truly complex epic - it introduces threading, network I/O, state synchronization, and the client-server split. The technical recommendations above (ENet, custom binary serialization, dedicated network thread) balance simplicity with performance needs.

Key decisions:
- **ENet** for networking (reliable UDP with minimal complexity)
- **Binary serialization** for messages (performance critical)
- **Dedicated network thread** with lock-free queues (no blocking on main thread)
- **Single executable** with --server flag (consistency with Epic 0)

The integration with Epic 0 is well-defined - we extend the existing game loop and leverage the serialization patterns already established. The main risk areas are threading bugs and entity ID synchronization, both of which have clear mitigation strategies.
