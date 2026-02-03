# QA Engineer Analysis: Epic 1 - Multiplayer Networking

## Overview

Epic 1 covers two systems critical to the multiplayer foundation:
- **NetworkManager** (core) - Socket connections, message serialization, lobby management
- **SyncSystem** (ECS) - Delta updates, state synchronization, snapshots

Network code is notoriously difficult to test due to its inherent non-determinism, timing dependencies, and the need to simulate multiple concurrent clients. This analysis outlines a comprehensive testing strategy.

---

## Key Test Scenarios

### NetworkManager Unit Tests

- [ ] **Socket initialization and teardown** - Verify clean socket lifecycle
- [ ] **Message serialization roundtrip** - All message types serialize/deserialize correctly
- [ ] **Message versioning** - Forward/backward compatibility of serialization format
- [ ] **Connection state machine** - Valid state transitions (disconnected -> connecting -> connected -> disconnected)
- [ ] **Player ID assignment** - Unique IDs assigned, respecting GAME_MASTER=0 reservation
- [ ] **Lobby creation and management** - Create, join, leave, destroy lobbies
- [ ] **Player list maintenance** - Accurate tracking of connected players
- [ ] **Message queuing** - Messages queued when network buffer full
- [ ] **Byte order handling** - Little-endian serialization across platforms
- [ ] **Fixed-size type serialization** - uint32_t, int64_t serialize correctly

### SyncSystem Unit Tests

- [ ] **Delta detection** - Correctly identifies changed components
- [ ] **Delta building** - Minimal delta packets (only changed data)
- [ ] **Delta application** - Remote updates apply correctly to local state
- [ ] **Snapshot creation** - Full state snapshot for reconnection
- [ ] **Snapshot restoration** - Game state correctly restored from snapshot
- [ ] **Entity creation sync** - New entities propagate to clients
- [ ] **Entity destruction sync** - Destroyed entities removed on clients
- [ ] **Component update sync** - Changed component values propagate
- [ ] **Tick synchronization** - Simulation time stays in sync across clients

### Integration Test Scenarios

- [ ] **Two-client connection** - Both clients connect, see each other
- [ ] **Four-client connection** - Maximum players connect and interact
- [ ] **Client sends input, server processes** - Build action propagates correctly
- [ ] **Server state update received by all clients** - Broadcast works
- [ ] **Late join full sync** - Player joining mid-game receives complete state
- [ ] **Player disconnect handling** - Other players notified, game continues
- [ ] **Player reconnection** - Player receives current state, resumes play
- [ ] **Concurrent actions** - Multiple players act simultaneously
- [ ] **20 ticks/sec sync rate** - Verify fixed timestep maintained under load

### Edge Cases - Disconnection

- [ ] **Graceful disconnect** - Player explicitly leaves
- [ ] **Abrupt disconnect** - Connection drops without goodbye
- [ ] **Server detects client timeout** - Heartbeat timeout triggers disconnect
- [ ] **Client detects server timeout** - Client handles unresponsive server
- [ ] **Disconnect during message send** - Partial message handling
- [ ] **Disconnect during state sync** - Incomplete snapshot handling
- [ ] **Multiple rapid disconnect/reconnect** - State remains consistent
- [ ] **Disconnect triggers ghost town process** - Ownership reverts correctly

### Edge Cases - Reconnection

- [ ] **Reconnect within timeout window** - Player resumes seamlessly
- [ ] **Reconnect after extended absence** - Player gets fresh snapshot
- [ ] **Reconnect to moved game state** - World changed while away
- [ ] **Reconnect with pending actions** - Server processed actions while disconnected
- [ ] **Reconnect with stale client state** - Client state overwritten by server
- [ ] **Multiple reconnection attempts** - Rapid connect/disconnect cycles
- [ ] **Reconnect during server tick** - Thread-safe state restoration

### Edge Cases - Late Join

- [ ] **Join empty server** - First player gets initial state
- [ ] **Join mid-simulation** - New player receives current snapshot
- [ ] **Join during active play** - Other players mid-action
- [ ] **Join at max capacity** - Rejected gracefully with message
- [ ] **Join with slow connection** - Large snapshot transfers reliably
- [ ] **Join during disaster event** - Active game events included in snapshot
- [ ] **Join and immediately disconnect** - Clean state, no corruption

### Edge Cases - Message Handling

- [ ] **Malformed message rejection** - Invalid data doesn't crash server
- [ ] **Message too large** - Graceful rejection
- [ ] **Message flood protection** - Rate limiting prevents abuse
- [ ] **Out-of-order messages** - Sequence numbers or tolerance
- [ ] **Duplicate message handling** - Idempotent processing
- [ ] **Unknown message type** - Graceful skip, no crash

---

## Testing Infrastructure Needed

### Mock Network Layer

- **MockSocket** - Simulates network without real sockets
  - Configurable latency injection
  - Configurable packet loss percentage
  - Configurable bandwidth limits
  - Message interception for verification

- **MockConnection** - In-memory client-server connection
  - Synchronous mode for deterministic unit tests
  - Async mode for integration tests
  - Error injection (disconnect, timeout, corruption)

### Test Harness for Multi-Client

- **TestServer** - Lightweight server for testing
  - Can be started/stopped programmatically
  - Exposes internal state for assertions
  - Configurable tick rate for faster tests

- **TestClient** - Scriptable client
  - Connect/disconnect commands
  - Send specific messages
  - Assert on received state
  - Multiple instances in same process

### Network Simulation Framework

- **LatencySimulator** - Adds artificial delay
  - Configurable min/max latency
  - Jitter simulation
  - Per-connection settings

- **PacketLossSimulator** - Drops packets
  - Random loss percentage
  - Burst loss patterns
  - One-way loss (send vs receive)

- **ConnectionQualityProfiles** - Preset network conditions
  - `PERFECT` - No latency, no loss
  - `LAN` - 1-5ms latency, 0% loss
  - `GOOD_WIFI` - 20-50ms latency, 0.1% loss
  - `POOR_WIFI` - 50-200ms latency, 2% loss
  - `MOBILE_3G` - 100-500ms latency, 5% loss
  - `HOSTILE` - 500-2000ms latency, 20% loss

### State Verification Utilities

- **StateDiffer** - Compares ECS state between server and client
- **SnapshotValidator** - Verifies snapshot completeness and correctness
- **ComponentChecker** - Validates component values match expected

### Performance Testing Tools

- **LoadGenerator** - Simulates many concurrent connections
- **ThroughputMeasurer** - Messages per second capacity
- **LatencyProfiler** - End-to-end action latency
- **MemoryTracker** - Detect connection-related leaks

---

## Questions for Other Agents

### @systems-architect:
- What networking library should we use (raw sockets, asio, ENet, etc.)? This affects our mock strategy.
- Should we use TCP, UDP, or a reliable UDP protocol? Reliability guarantees affect test scenarios.
- What is the expected maximum message size for full state snapshots? Need this for buffer sizing tests.
- How do we handle clock drift between server and clients? Do we need NTP or a simpler approach?
- What is the heartbeat/timeout interval for detecting disconnections?
- Should messages be encrypted? If so, what crypto library, and do we need key exchange tests?

### @engine-developer:
- How will the ECS registry support efficient delta detection? Does SyncSystem need special hooks?
- What serialization format for components - binary, protobuf, flatbuffers? Affects test complexity.
- How do we handle component versioning if component structure changes after release?
- Will there be a "dirty flag" on components or does SyncSystem need to diff previous state?
- What threading model - single-threaded tick, or network on separate thread?
- How are entity IDs generated to ensure uniqueness across reconnections?

### @game-designer:
- What is the maximum acceptable latency before player actions feel sluggish?
- Should there be any client-side prediction for responsiveness, or pure server-authoritative?
- What visual feedback should clients show during reconnection (loading screen, desync indicator)?
- How should rate limiting work for player actions (prevent spam-clicking)?

---

## Risks & Concerns

### High Risk

1. **Non-deterministic test flakiness** - Network timing is inherently variable
   - Mitigation: Use MockSocket for unit tests, separate integration test suite with retry tolerance

2. **State desync bugs** - Server and client disagree on game state
   - Mitigation: Frequent state checksums, automatic resync detection, comprehensive diff tools

3. **Memory leaks on disconnect** - Connections hold resources that may not be freed
   - Mitigation: RAII patterns, explicit leak detection in CI, stress testing connect/disconnect cycles

4. **Deadlocks in concurrent scenarios** - Network thread + game thread interactions
   - Mitigation: Thread sanitizer in CI, lock-free queues where possible, clear threading model

5. **Security vulnerabilities** - Malformed packets crash server or allow cheating
   - Mitigation: Fuzz testing, input validation, server-authoritative design already helps

### Medium Risk

6. **Snapshot size growth** - Large worlds create huge reconnection payloads
   - Mitigation: Compression testing, chunked transfer, progress indicators

7. **Tick rate sustainability** - 20 ticks/sec under load with multiple clients
   - Mitigation: Performance testing early, profiling, budget per tick

8. **Platform differences** - Windows/Linux socket behavior differences
   - Mitigation: CI on multiple platforms, cross-platform testing

9. **Late join edge cases** - Complex state at join time (mid-disaster, etc.)
   - Mitigation: Explicit late-join test scenarios for each major game state

### Low Risk (but still test)

10. **Endianness issues** - Unlikely on modern x86/ARM but possible
    - Mitigation: Explicit little-endian serialization, cross-platform roundtrip tests

---

## Recommended Test Approaches

### Unit Testing Strategy

**Framework:** Google Test (gtest) with Google Mock (gmock)
- Familiar, well-documented, good mocking support
- Native C++ integration
- Strong assertion library

**Approach:**
- Mock the socket layer entirely for unit tests
- Use synchronous message passing for determinism
- Test serialization roundtrips exhaustively
- Test state machine transitions in isolation

**Example Test Pattern:**
```cpp
TEST(NetworkManagerTest, MessageSerializationRoundtrip) {
    InputMessage original{.action = Action::BUILD, .position = {10, 20}};
    WriteBuffer buffer;
    original.serialize(buffer);

    ReadBuffer reader(buffer.data());
    InputMessage deserialized;
    deserialized.deserialize(reader);

    EXPECT_EQ(original.action, deserialized.action);
    EXPECT_EQ(original.position.x, deserialized.position.x);
    EXPECT_EQ(original.position.y, deserialized.position.y);
}
```

### Integration Testing Strategy

**Approach:**
- Use real networking but localhost only
- Multiple client instances in separate threads or processes
- Scripted scenarios with expected outcomes
- Timeout-based failure detection (but generous timeouts)

**Test Categories:**
1. **Smoke tests** - Basic connect, send, receive, disconnect
2. **Scenario tests** - Multi-step workflows (join, build, leave)
3. **Stress tests** - Many connections, rapid messages
4. **Chaos tests** - Inject failures during operations

### Simulation/Property-Based Testing

**Tool:** Consider using a property-based testing library (e.g., RapidCheck for C++)

**Properties to verify:**
- "After any sequence of valid inputs, server state is consistent"
- "If client receives update, applying it results in server's state"
- "Serialization is bijective: deserialize(serialize(x)) == x"

### Fuzz Testing

**Tool:** AFL++, libFuzzer, or similar

**Targets:**
- Message deserialization (find crash bugs)
- Connection handshake (find protocol bugs)
- Snapshot parsing (find state corruption)

### Performance Testing

**Approach:**
- Establish baseline metrics early
- Run performance tests in CI (not just locally)
- Test with increasing client counts: 1, 2, 4, 8 (simulated)
- Measure:
  - Messages processed per second
  - Tick completion time under load
  - Memory usage over time
  - Reconnection snapshot generation time

**Thresholds (suggested, to be refined):**
- Tick completion: < 50ms (for 20 ticks/sec)
- State update latency: < 100ms end-to-end
- Snapshot generation: < 2 seconds for large world
- Memory growth: < 10KB per connected player

### Test Organization

```
tests/
  unit/
    network/
      NetworkManager_test.cpp
      MessageSerialization_test.cpp
      ConnectionState_test.cpp
    ecs/
      SyncSystem_test.cpp
      DeltaDetection_test.cpp
      SnapshotTest.cpp
  integration/
    MultiClient_test.cpp
    LateJoin_test.cpp
    Reconnection_test.cpp
    NetworkConditions_test.cpp
  fixtures/
    MockSocket.h
    MockSocket.cpp
    TestServer.h
    TestClient.h
    NetworkSimulator.h
    StateVerifier.h
  performance/
    Throughput_test.cpp
    Latency_test.cpp
    ScalabilityTest.cpp
  fuzz/
    MessageFuzzer.cpp
    SnapshotFuzzer.cpp
```

### CI/CD Considerations

- Run unit tests on every commit (fast)
- Run integration tests on PR merge (medium speed)
- Run performance tests nightly (slow, resource intensive)
- Run fuzz tests continuously in background
- Platform matrix: Windows, Linux (matching server deployment targets)

---

## Testing Priorities for Epic 1

### Must Have (P0)
1. Message serialization unit tests - Foundation for everything
2. Connection lifecycle unit tests - Connect, disconnect, timeout
3. Two-client integration test - Prove basic multiplayer works
4. Delta sync unit tests - State propagation works
5. Mock network infrastructure - Enable deterministic testing

### Should Have (P1)
1. Four-client integration tests - Full player count
2. Late join scenario tests - Critical for UX
3. Reconnection tests - Ghost town trigger path
4. Network condition simulation - Latency/loss handling
5. Snapshot correctness tests - Full state capture

### Nice to Have (P2)
1. Fuzz testing setup - Security hardening
2. Performance baseline - Throughput/latency metrics
3. Stress testing - Connection limits
4. Cross-platform verification - Windows + Linux parity

---

## Open Questions (Self-Notes)

- How do we test the 20 ticks/sec requirement without wall-clock timing dependencies?
- Should test clients run in same process (faster, shared memory) or separate processes (more realistic)?
- Do we need a "test mode" flag in NetworkManager to bypass real sockets?
- What is the acceptance criteria for "reconnection works"? Time limit? State accuracy?

---

## Summary

Epic 1 networking is foundational and high-risk. Testing must be thorough but also practical given the inherent complexity of distributed systems. The recommended approach prioritizes:

1. **Deterministic unit tests** with mocked networking for reliability
2. **Integration tests** with real networking for confidence
3. **Infrastructure investment** in mock sockets and test harnesses
4. **Early performance baselines** to catch regressions
5. **Security testing** via fuzzing given exposure to network input

The testing infrastructure built for Epic 1 will be reused throughout the project as more systems need network synchronization.
