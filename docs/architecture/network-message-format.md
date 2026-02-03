# Network Message Format Specification

**Version:** 1.0
**Date:** 2026-02-02
**Status:** Canonical
**Epic:** 1 (Multiplayer Networking), POC-2

---

## 1. Overview

This document specifies the binary network message format for ZergCity multiplayer communication. All messages follow a common header structure with type-specific payloads.

### Design Principles

1. **Little-endian byte order** (x86/x64 native)
2. **Fixed-size headers** for efficient parsing
3. **LZ4 compression** for payloads > 1KB
4. **Version tagging** for protocol evolution
5. **Tight packing** (no padding unless specified)

---

## 2. Message Header

All messages begin with a 16-byte header.

```
Offset  Size  Field             Description
------  ----  -----             -----------
0       4     magic             Magic bytes: 0x5A43 0x4E54 ("ZCNT" - ZergCity NeT)
4       1     protocol_version  Protocol version (starts at 1)
5       1     message_type      Message type enum (see section 3)
6       1     flags             Bit flags (see below)
7       1     reserved          Reserved for future use (must be 0)
8       4     payload_length    Payload length in bytes (after header)
12      4     sequence          Sequence number (monotonic per connection)
------  ----
        16    TOTAL HEADER SIZE
```

### Flags Byte

```
Bit  Name         Description
---  ----         -----------
0    COMPRESSED   Payload is LZ4 compressed
1    FRAGMENTED   Message is part of a fragment sequence (reserved)
2    ACK_REQUEST  Sender requests acknowledgment
3    RELIABLE     Message requires reliable delivery (ENet channel 0)
4-7  reserved     Reserved for future use
```

### Magic Bytes

The magic bytes `0x5A 0x43 0x4E 0x54` ("ZCNT") identify valid ZergCity network packets. Any packet not starting with these bytes should be discarded.

### Protocol Version

Version 1 is the initial protocol. Version mismatches result in connection rejection with an error message.

---

## 3. Message Types

```cpp
enum class MessageType : uint8_t {
    // Connection (0x00-0x0F)
    ClientHello         = 0x00,
    ServerHello         = 0x01,
    Disconnect          = 0x02,
    Ping                = 0x03,
    Pong                = 0x04,

    // State Sync (0x10-0x1F)
    FullSnapshot        = 0x10,
    DeltaSnapshot       = 0x11,
    SnapshotAck         = 0x12,

    // Input (0x20-0x2F)
    InputMessage        = 0x20,
    InputAck            = 0x21,

    // Game Actions (0x30-0x3F)
    PurchaseTile        = 0x30,
    PlaceBuilding       = 0x31,
    DesignateZone       = 0x32,
    Demolish            = 0x33,
    SetTributeRate      = 0x34,

    // Chat (0x40-0x4F)
    ChatMessage         = 0x40,
    ChatBroadcast       = 0x41,

    // Admin (0xF0-0xFF)
    ServerCommand       = 0xF0,
    ErrorMessage        = 0xFE,
    Reserved            = 0xFF
};
```

---

## 4. Connection Messages

### 4.1 ClientHello (0x00)

Sent by client to initiate connection.

```
Offset  Size  Field              Description
------  ----  -----              -----------
0       16    header             Standard header (message_type = 0x00)
16      4     client_version     Client build version
20      32    player_name        UTF-8 player name (null-padded)
52      16    session_token      Reconnection token (zeros if new connection)
68      4     requested_slot     Preferred player slot (0-3, or 0xFF for any)
------  ----
        72    TOTAL SIZE
```

### 4.2 ServerHello (0x01)

Server response to ClientHello.

```
Offset  Size  Field              Description
------  ----  -----              -----------
0       16    header             Standard header (message_type = 0x01)
16      4     server_version     Server build version
20      1     result             0 = accepted, 1+ = error code
21      1     assigned_slot      Player slot (0-3)
22      2     padding            Alignment padding
24      16    session_token      Token for reconnection
40      4     tick_rate          Server tick rate (ticks per second, normally 20)
44      8     current_tick       Current simulation tick
52      4     map_width          Map width in tiles
56      4     map_height         Map height in tiles
60      4     num_players        Current player count
------  ----
        64    TOTAL SIZE
```

**Result Codes:**
- 0: Accepted
- 1: Server full
- 2: Version mismatch
- 3: Invalid session token
- 4: Banned
- 5: Game not accepting new players

### 4.3 Disconnect (0x02)

Graceful disconnection.

```
Offset  Size  Field              Description
------  ----  -----              -----------
0       16    header             Standard header (message_type = 0x02)
16      1     reason             Disconnect reason code
17      127   message            UTF-8 human-readable message (null-terminated)
------  ----
        144   TOTAL SIZE
```

**Reason Codes:**
- 0: Normal disconnect
- 1: Kicked by host
- 2: Server shutdown
- 3: Timeout
- 4: Protocol error

### 4.4 Ping (0x03) / Pong (0x04)

Latency measurement (sent over unreliable channel).

```
Offset  Size  Field              Description
------  ----  -----              -----------
0       16    header             Standard header
16      8     timestamp          Sender's timestamp (microseconds since epoch)
24      8     server_tick        Current server tick (only in Pong)
------  ----
        32    TOTAL SIZE
```

---

## 5. State Synchronization

### 5.1 FullSnapshot (0x10)

Complete world state for late-join or reconnection. Always LZ4 compressed.

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       16        header             Standard header (COMPRESSED flag set)
16      4         uncompressed_size  Original size before compression
20      8         snapshot_tick      Tick this snapshot represents
28      N         compressed_data    LZ4-compressed snapshot payload
------  ----
        28+N      TOTAL SIZE
```

**Compressed Payload Structure:**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         section_count      Number of sections
4       ...       sections[]         Array of section headers
...     ...       section_data       Concatenated section data
```

**Section Header (12 bytes each):**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         section_type       Section type enum
4       4         offset             Offset from start of section_data
8       4         length             Section data length in bytes
```

**Section Types:**

```cpp
enum class SnapshotSection : uint32_t {
    Metadata        = 0x0001,  // Game settings, tick, RNG state
    Players         = 0x0002,  // Player info, ownership
    Terrain         = 0x0003,  // Dense terrain grid
    Entities        = 0x0004,  // Entity registry
    Components      = 0x0005,  // Component data (multiple)
    DenseGrids      = 0x0006,  // Coverage/simulation grids
};
```

### 5.2 DeltaSnapshot (0x11)

Incremental state update, sent at 20Hz.

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       16        header             Standard header
16      8         base_tick          Previous tick this delta is relative to
24      8         target_tick        Tick this delta brings state to
32      2         entity_count       Number of entity updates
34      2         component_count    Number of component updates
36      N         payload            Delta payload (see below)
------  ----
        36+N      TOTAL SIZE
```

**Delta Payload Structure:**

```
// Entity Updates
for each entity_count:
    4 bytes: entity_id
    1 byte:  operation (0=created, 1=destroyed, 2=modified)
    if operation == 0 (created):
        4 bytes: archetype_id

// Component Updates
for each component_count:
    4 bytes: entity_id
    2 bytes: component_type_id
    1 byte:  operation (0=added, 1=removed, 2=modified)
    if operation != 1 (not removed):
        2 bytes: data_length
        N bytes: component_data
```

**Compression:**
- DeltaSnapshots are LZ4 compressed if payload > 1024 bytes
- Header COMPRESSED flag indicates compression state

### 5.3 SnapshotAck (0x12)

Client acknowledgment of received state.

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       16        header             Standard header
16      8         acked_tick         Highest tick client has processed
24      4         last_input_seq     Last input sequence server should have received
------  ----
        28        TOTAL SIZE
```

---

## 6. Input Messages

### 6.1 InputMessage (0x20)

Client input to server. Sent immediately on user action.

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       16        header             Standard header (RELIABLE flag set)
16      4         input_sequence     Monotonic input sequence number
20      8         client_tick        Client's estimated server tick
28      1         input_type         Input type enum
29      N         input_data         Type-specific input data
------  ----
        29+N      TOTAL SIZE
```

**Input Types:**

```cpp
enum class InputType : uint8_t {
    None            = 0x00,
    PlaceBuilding   = 0x01,
    DesignateZone   = 0x02,
    Demolish        = 0x03,
    PurchaseTile    = 0x04,
    SetTributeRate  = 0x05,
    PlaceRoad       = 0x06,
    PlaceConduit    = 0x07,
    PlacePipe       = 0x08,
    CancelAction    = 0x09,
};
```

**PlaceBuilding Data (InputType 0x01):**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         building_type_id   Building type to place
4       4         grid_x             Grid X coordinate
8       4         grid_y             Grid Y coordinate
12      1         rotation           Rotation (0, 90, 180, 270 degrees / 90)
------  ----
        13        DATA SIZE
```

**DesignateZone Data (InputType 0x02):**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       1         zone_type          Zone type (0=none, 1=habitation, 2=exchange, 3=fabrication)
1       1         density            Density (0=low, 1=high)
2       2         rect_count         Number of rectangles in selection
4       N*16      rectangles         Array of GridRect (x, y, width, height - each 4 bytes)
------  ----
        4+N*16    DATA SIZE
```

**Demolish Data (InputType 0x03):**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       2         tile_count         Number of tiles to demolish
2       N*8       tiles              Array of (grid_x, grid_y) pairs (each 4+4 bytes)
------  ----
        2+N*8     DATA SIZE
```

### 6.2 InputAck (0x21)

Server acknowledgment of processed input.

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       16        header             Standard header
16      4         input_sequence     Acknowledged input sequence
20      1         result             0=success, 1+=error code
21      8         applied_tick       Tick input was applied (or 0 if rejected)
------  ----
        29        TOTAL SIZE
```

**Result Codes:**
- 0: Success
- 1: Invalid position
- 2: Insufficient funds
- 3: Not owner
- 4: Blocked by terrain
- 5: Requirements not met
- 6: Rate limited

---

## 7. Entity State Format

Entities in snapshots and deltas use this serialization format.

### 7.1 Entity Header

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         entity_id          EnTT entity ID
4       4         archetype_id       Archetype identifier (component combination)
8       1         component_mask_len Byte length of component mask
9       N         component_mask     Bitmask of present components
------  ----
        9+N       HEADER SIZE
```

### 7.2 Component Serialization

Each component type has a registered type ID and serialization format.

```cpp
// Component type registry
enum class ComponentTypeID : uint16_t {
    Position        = 0x0001,
    Ownership       = 0x0002,
    Building        = 0x0003,
    Zone            = 0x0004,
    Energy          = 0x0005,
    Fluid           = 0x0006,
    Transport       = 0x0007,
    Population      = 0x0008,
    // ... etc
};
```

**Component Data Format:**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       2         type_id            ComponentTypeID
2       2         version            Component format version
4       2         data_length        Serialized data length
6       N         data               Component-specific data
------  ----
        6+N       COMPONENT SIZE
```

### 7.3 Standard Component Layouts

**PositionComponent:**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         grid_x             int32_t X coordinate
4       4         grid_y             int32_t Y coordinate
8       4         elevation          int32_t elevation (0-31)
------  ----
        12        SIZE
```

**OwnershipComponent:**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       1         owner_id           Player ID (0 = Game Master)
1       1         state              OwnershipState enum
2       2         padding            Alignment
4       8         state_changed_at   Tick when state changed
------  ----
        12        SIZE
```

**BuildingComponent:**

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         building_type      BuildingType ID
4       1         state              Building state enum
5       1         level              Development level (1-3)
6       2         padding            Alignment
8       4         construction_tick  Tick construction started
12      4         population         Current population (if applicable)
------  ----
        16        SIZE
```

---

## 8. Compression Strategy

### 8.1 When to Compress

| Message Type | Compression |
|--------------|-------------|
| FullSnapshot | Always (typically 100KB-5MB uncompressed) |
| DeltaSnapshot | If payload > 1024 bytes |
| Other messages | Never (already small) |

### 8.2 LZ4 Format

Using LZ4 block format (not frame format) for minimal overhead.

```cpp
// Compression
int compressed_size = LZ4_compress_default(
    input_data, output_buffer,
    input_size, LZ4_compressBound(input_size)
);

// Decompression
int decompressed_size = LZ4_decompress_safe(
    compressed_data, output_buffer,
    compressed_size, expected_size
);
```

### 8.3 Compression Header

When COMPRESSED flag is set, payload begins with:

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       4         uncompressed_size  Original size (for buffer allocation)
4       N         compressed_data    LZ4 compressed payload
------  ----
        4+N       COMPRESSED PAYLOAD
```

---

## 9. Versioning Strategy

### 9.1 Protocol Version

The `protocol_version` field in the header is incremented when:
- Message header format changes
- New required message types added
- Existing message formats change incompatibly

Version 1 is the initial protocol. Servers reject clients with mismatched protocol versions.

### 9.2 Component Versioning

Each component includes a 16-bit version number in its serialization header. This allows:
- Older clients to skip unknown component fields
- Migration code to upgrade component data
- Forward compatibility within a protocol version

**Migration Pattern:**

```cpp
// On deserialization
if (component_version < CURRENT_VERSION) {
    migrate_component(data, component_version, CURRENT_VERSION);
}
```

### 9.3 Build Version

The `client_version` and `server_version` fields contain build identifiers (semantic versioning or build number). These are logged but do not affect connection acceptance unless protocol version mismatches.

---

## 10. ENet Channel Mapping

ENet supports multiple channels with different reliability guarantees.

| Channel | Reliability | Usage |
|---------|-------------|-------|
| 0 | Reliable ordered | Connection handshake, InputMessage, game actions |
| 1 | Reliable unordered | FullSnapshot, large data transfers |
| 2 | Unreliable | DeltaSnapshot, Ping/Pong |

### 10.1 Channel Selection

```cpp
// Reliable, ordered (inputs must arrive in order)
enet_peer_send(peer, 0, reliable_packet);

// Reliable, unordered (snapshots can arrive out of order)
enet_peer_send(peer, 1, snapshot_packet);

// Unreliable (delta updates, latency measurement)
enet_peer_send(peer, 2, unreliable_packet);
```

---

## 11. Example Message Flows

### 11.1 Client Connection

```
Client                              Server
  |                                   |
  |--- ClientHello (CH0) ------------>|
  |                                   |
  |<-- ServerHello (CH0) -------------|
  |                                   |
  |<-- FullSnapshot (CH1) ------------|
  |                                   |
  |--- SnapshotAck (CH0) ------------>|
  |                                   |
  |<-- DeltaSnapshot (CH2) @ 20Hz ----|
  |                                   |
```

### 11.2 Player Action

```
Client                              Server
  |                                   |
  |--- InputMessage (CH0) ----------->|
  |    (PlaceBuilding, seq=42)        |
  |                                   | (validates, applies at tick N)
  |<-- InputAck (CH0) ----------------|
  |    (seq=42, result=0, tick=N)     |
  |                                   |
  |<-- DeltaSnapshot (CH2) -----------|
  |    (includes new building entity) |
  |                                   |
```

### 11.3 Reconnection

```
Client                              Server
  |                                   |
  |--- ClientHello (CH0) ------------>|
  |    (with session_token)           |
  |                                   | (validates token)
  |<-- ServerHello (CH0) -------------|
  |    (same assigned_slot)           |
  |                                   |
  |<-- FullSnapshot (CH1) ------------|
  |    (current world state)          |
  |                                   |
  |--- SnapshotAck (CH0) ------------>|
  |                                   |
```

---

## 12. Error Handling

### 12.1 ErrorMessage (0xFE)

Generic error notification.

```
Offset  Size      Field              Description
------  ----      -----              -----------
0       16        header             Standard header
16      4         error_code         Error code
20      4         context            Context-specific value
24      200       message            UTF-8 error message (null-terminated)
------  ----
        224       TOTAL SIZE
```

### 12.2 Error Codes

```cpp
enum class ErrorCode : uint32_t {
    None                = 0,
    ProtocolMismatch    = 1,
    InvalidMessage      = 2,
    InvalidSequence     = 3,
    RateLimited         = 4,
    ServerFull          = 5,
    NotAuthorized       = 6,
    InternalError       = 7,
    Timeout             = 8,
};
```

---

## 13. Security Considerations

### 13.1 Validation

All received messages must be validated:
- Magic bytes must match
- Protocol version must match
- Payload length must not exceed maximum (16MB)
- Message type must be known
- Compressed data must decompress to stated size

### 13.2 Rate Limiting

- Clients limited to 60 InputMessages per second
- FullSnapshot requests limited to 1 per 30 seconds
- Invalid message count tracked; disconnect after 10 invalid messages

### 13.3 Session Tokens

- 128-bit random tokens generated on connection
- Stored server-side with player slot and last activity
- Expire after 1 hour of inactivity
- Invalid tokens result in new slot assignment (if available)

---

## 14. References

- Canon: `/docs/canon/patterns.yaml` (multiplayer section)
- Canon: `/docs/canon/systems.yaml` (NetworkManager, SyncSystem)
- Decision: `/plans/decisions/epic-1-lz4-compression.md`
- Decision: `/plans/decisions/epic-1-snapshot-strategy.md`
- ENet documentation: http://enet.bespin.org/
- LZ4 documentation: https://github.com/lz4/lz4
