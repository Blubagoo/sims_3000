# QA Engineer Impact Analysis: Epic 1

## Trigger
Canon update v0.3.0 → v0.5.0

## Overall Impact: LOW

## Impact Summary
The v0.5.0 canon update is overwhelmingly visual/design-oriented (art direction, model pipeline, UI modes, audio, terrain art). Epic 1 is networking infrastructure — socket connections, message serialization, state synchronization, and test harnesses. The only delta items with any bearing on Epic 1 are **configurable map sizes** (which affect snapshot sizing and performance test parameters) and **expanded terrain types** (which add more component data flowing through the sync pipeline). All other changes are rendering, audio, or UI concerns that do not touch the networking layer.

---

## Item-by-Item Assessment

### UNAFFECTED

- **NetworkManager Unit Tests (all items)** — Reason: Socket lifecycle, message serialization, connection state machine, player ID assignment, lobby management, message queuing, byte order, and fixed-size type serialization are transport-layer concerns. Art direction, models, audio, and UI modes do not affect the wire protocol.

- **SyncSystem Unit Tests (all items)** — Reason: Delta detection, delta building, delta application, snapshot creation/restoration, entity sync, component sync, and tick synchronization operate on abstract component data. The sync layer serializes whatever components exist; the addition of terrain types or building templates does not change how sync works — it only changes what data flows through it. Component serialization (ticket 1-007) already requires each syncable component to implement serialize/deserialize, which is enforced per-component, not per-canon version.

- **Integration Test Scenarios (all items)** — Reason: Two-client connection, four-client connection, client-server input processing, broadcast, late join, disconnect handling, reconnection, concurrent actions, and tick rate verification are protocol and state-machine tests. They are agnostic to what game content is being synchronized.

- **Edge Cases — Disconnection (all items)** — Reason: Graceful disconnect, abrupt disconnect, heartbeat timeout, connection drops, partial messages, incomplete snapshots, rapid disconnect/reconnect, and ghost town triggers are all network-layer behaviors unaffected by visual or content changes.

- **Edge Cases — Reconnection (all items)** — Reason: Reconnect within timeout, after extended absence, to changed state, with pending actions, with stale state, multiple attempts, and during server tick are all connection-management scenarios. No art/UI delta affects these.

- **Edge Cases — Late Join (all items)** — Reason: Join empty server, mid-simulation, during active play, at max capacity, with slow connection, during disaster, and immediate disconnect are all connection and snapshot delivery scenarios. Unaffected by visual changes.

- **Edge Cases — Message Handling (all items)** — Reason: Malformed message rejection, oversized message handling, flood protection, out-of-order messages, duplicate handling, and unknown message types are defensive network code. No canon visual change touches these.

- **Testing Infrastructure — Mock Network Layer** — Reason: MockSocket, MockConnection, latency injection, packet loss simulation, and bandwidth limits are transport abstractions. Unchanged.

- **Testing Infrastructure — Test Harness for Multi-Client** — Reason: TestServer, TestClient, scripted scenarios, and state assertions are test scaffolding. Unchanged.

- **Testing Infrastructure — Network Simulation Framework** — Reason: LatencySimulator, PacketLossSimulator, and ConnectionQualityProfiles are network condition simulators. Unchanged.

- **Testing Infrastructure — State Verification Utilities** — Reason: StateDiffer, SnapshotValidator, and ComponentChecker compare abstract ECS state. They work on whatever components exist. Unchanged.

- **Recommended Test Approaches (all sections)** — Reason: Google Test / Google Mock framework choice, unit testing strategy, integration testing approach, property-based testing, fuzz testing targets, CI/CD pipeline design, and test organization are all methodology decisions independent of game content.

- **Risks & Concerns — High Risk items 1-5** — Reason: Non-deterministic flakiness, state desync, memory leaks, deadlocks, and security vulnerabilities are network-layer risks. No visual/content canon change affects their likelihood or mitigation.

- **Risks & Concerns — Medium Risk items 7-9** — Reason: Tick rate sustainability (already tests 20 ticks/sec), platform differences, and late join edge cases are protocol concerns. Unchanged.

- **Testing Priorities (all P0, P1, P2 items)** — Reason: Message serialization, connection lifecycle, two-client integration, delta sync, mock infrastructure, four-client tests, late join scenarios, reconnection tests, network condition simulation, snapshot correctness, fuzz testing, performance baselines, stress testing, and cross-platform verification are all networking test categories. Unchanged.

- **Open Questions (all items)** — Reason: Tick rate testing without wall-clock dependencies, test client process model, test mode flag, and reconnection acceptance criteria are all test-infrastructure questions. Unchanged.

### MODIFIED

- **Performance Testing — Snapshot size thresholds**
  - Previous: Risk item 6 stated "Snapshot size growth" concern with mitigation of "compression testing, chunked transfer, progress indicators." Ticket 1-006 references snapshot size testing at "100KB, 500KB, 2MB, 5MB." Ticket 1-014 mentions "1-5MB for mature worlds."
  - Revised: Configurable map sizes (128x128 = 16K tiles, 256x256 = 65K tiles, 512x512 = 262K tiles) mean snapshot size varies by up to 16x between small and large maps. Performance test thresholds should be parameterized by map size tier. Snapshot size tests in ticket 1-006 and 1-014 should include a test matrix: small map baseline, medium map (current assumption), and large map (worst case). The 512x512 map with 262K tiles, each potentially having multiple components, could push mature-world snapshots well above the 5MB upper bound currently tested.
  - Reason: The 16x variation in tile count across map sizes directly affects snapshot payload sizes, chunked transfer counts, and reconnection time. The existing test range assumed a single fixed map size (64x64 in v0.3.0 = 4K tiles). Now the largest map is 65x larger than the old fixed size.

- **Performance Testing — Tick budget under load**
  - Previous: Performance thresholds suggest "Tick completion: < 50ms" and "Memory growth: < 10KB per connected player." Testing referenced generic "increasing client counts: 1, 2, 4, 8."
  - Revised: Performance tests should be parameterized by map size tier. A 512x512 map with 262K entities will stress the delta generation and dirty-set scanning differently than a 128x128 map with 16K entities. Tick budget tests should cover all three size tiers to verify 20 ticks/sec is sustainable on large maps. Memory growth per player will also scale with map size due to client-side ECS state.
  - Reason: Configurable map sizes mean "load" is no longer a single dimension (player count) — it is now player count x map size. The largest map is 16x the medium and 256x the old fixed size.

### INVALIDATED

None. All items in the original analysis remain valid. No networking concept, test scenario, or infrastructure recommendation has been rendered incorrect by the v0.5.0 changes.

### NEW CONCERNS

- **NEW: Snapshot size explosion on large maps** — With 512x512 maps (262K tiles), each tile potentially having TerrainComponent, OwnershipComponent, and additional terrain-effect components (10 terrain types now have gameplay effects), a full snapshot for a mature large-map world could significantly exceed the previously estimated 5MB ceiling. If each tile averages 50-100 bytes of component data, a large map alone is 13-26MB before buildings. This needs explicit load testing and may require snapshot streaming or progressive loading strategies beyond simple 64KB chunking. The bounded delta queue (max 100 ticks = 5 seconds during snapshot transfer) may be insufficient if snapshot transfer takes longer on large maps.

- **NEW: Map size must be included in initial handshake or snapshot metadata** — Clients need to know the map size to allocate ECS storage and prepare rendering. This is a minor protocol consideration: the server status or snapshot-start message should include the map size tier. This is not a QA concern per se, but tests should verify that clients correctly receive and apply the map size configuration during connection.

---

## Questions for Other Agents

- **@systems-architect:** With configurable map sizes up to 512x512 (262K tiles), has the snapshot size ceiling been re-estimated? The original 1-5MB estimate was based on a fixed 64x64 map. A mature 512x512 world could be 10-50MB+. Does this change the chunking strategy, compression requirements, or snapshot transfer approach?

- **@engine-developer:** How will the map size configuration be communicated to connecting clients? Should it be part of the ServerStatusMessage, the SnapshotStart message, or a new MapConfigMessage? Tests need to know what to assert on.

---

## Affected Tickets

- **1-006 (Server-to-Client Messages):** MODIFY — QA agent note should be updated from "Test snapshot sizes: 100KB, 500KB, 2MB, 5MB" to "Test snapshot sizes parameterized by map size: small map (~500KB), medium map (~2MB), large map (~20MB+), with boundary testing at 64KB chunk boundaries for each tier."

- **1-014 (Full State Snapshot):** MODIFY — Acceptance criteria note "1-5MB for mature worlds" should be revised to account for 512x512 maps. Add acceptance criterion: "Snapshot generation and transfer tested at all three map size tiers." The bounded delta queue (max 100 ticks) should be validated as sufficient for large-map snapshot transfer times. Consider adding: "Snapshot transfer time under 10 seconds for large maps on LAN."

- **1-019 (Testing Infrastructure):** MODIFY — Connection quality profiles and test scenarios should include a map-size dimension. TestServer should support configurable map size for parameterized tests.

- **1-001 through 1-005:** UNCHANGED — Transport abstraction, serialization, protocol, threading, and client messages are unaffected.
- **1-007:** UNCHANGED — Component serialization pattern is generic; new terrain types add components but don't change the serialization framework.
- **1-008:** UNCHANGED — Server network loop is content-agnostic.
- **1-009:** UNCHANGED — Client network loop is content-agnostic.
- **1-010:** UNCHANGED — Player session management is content-agnostic.
- **1-011:** UNCHANGED — Input message transmission is content-agnostic.
- **1-012:** UNCHANGED — Change detection via dirty flags is content-agnostic.
- **1-013:** UNCHANGED — Delta generation/application is content-agnostic.
- **1-015:** UNCHANGED — Entity ID synchronization is content-agnostic.
- **1-016:** UNCHANGED — Persistence interface is content-agnostic.
- **1-017:** UNCHANGED — Game loop integration is content-agnostic.
- **1-018:** UNCHANGED — Error handling is content-agnostic.
- **1-020:** UNCHANGED — Serialization unit tests are content-agnostic.
- **1-021:** UNCHANGED — Multi-client integration tests are content-agnostic.
- **1-022:** UNCHANGED — Network condition tests are content-agnostic.
