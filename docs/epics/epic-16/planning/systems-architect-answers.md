# Systems Architect Answers: Epic 16 Planning Discussion

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**In Response To:** Game Designer questions from epic-16-planning.discussion.yaml

---

## GD-1: Database Technology and Write Throughput

**Question:** What database technology supports 20 writes/second with transactional integrity? How are component changes batched for efficient DB writes? Is there a write-ahead log for crash recovery?

**Answer:**

Epic 1 established SQLite as the database for MVP (with PostgreSQL as a future option). SQLite easily handles 20 writes/second when properly configured - the key is batching. Rather than writing every component change individually (which could mean hundreds of writes per tick), Epic 1's design batches all changes within a tick into a single transaction. This gives us atomicity (all changes commit together or none do) and dramatically improves throughput.

The batching strategy works as follows: during each simulation tick, the SyncSystem collects all dirty components into a delta. At the end of the tick (or every N ticks, configurable), this delta is persisted to SQLite within a single transaction. With WAL (Write-Ahead Log) mode enabled - which is standard for SQLite in production - we get crash recovery: if the server crashes mid-transaction, the WAL ensures we recover to the last committed state.

For Epic 16 save files, this same batched approach applies. The key distinction is that Epic 1's database persistence is *continuous* (operational recovery), while Epic 16's save files are *intentional* snapshots. When a save is triggered, we capture the current database state (or ECS state directly) and serialize it to a portable file. The database remains the source of truth for crash recovery; save files are for user-facing backup/restore/sharing.

---

## GD-2: World Snapshot Size and Storage

**Question:** How large is a full world snapshot for each map size? What compression is used for checkpoint files? Where are checkpoints stored (local, cloud, both)?

**Answer:**

Based on my analysis in the Epic 16 planning document, estimated save file sizes with LZ4 compression are:

| Map Size | Dense Grids (uncompressed) | Dense Grids (compressed) | Entities + Components | Total Estimated |
|----------|---------------------------|-------------------------|----------------------|-----------------|
| 128x128  | ~400 KB | ~40 KB | ~100 KB | ~150 KB |
| 256x256  | ~1.5 MB | ~150 KB | ~250 KB | ~400 KB |
| 512x512  | ~6 MB | ~500 KB | ~500 KB | ~1 MB |

Dense grids (terrain, coverage, contamination, etc.) compress extremely well due to spatial coherence - typical ratios of 10:1 to 15:1. Entity and component data compresses less aggressively (2-4x) but starts smaller. LZ4 was chosen in Epic 1's decision record (`epic-1-lz4-compression.md`) for its speed (4+ GB/s decompression) while still achieving good compression ratios.

For storage locations: save files are stored locally on the server machine in a configurable directory (default: `saves/` relative to the server executable). Cloud storage is not in MVP scope - save files are portable, so players can manually copy them to cloud storage or share via Discord. A future epic could add cloud sync integration, but the current design keeps files local with export/import capability for sharing.

Checkpoints (Epic 1's continuous persistence) are stored in the SQLite database file, also local to the server. This is separate from Epic 16's user-facing save files.

---

## GD-3: Immortalized Worlds Storage

**Question:** Are immortalized worlds stored in the same DB or separate? How is the Monument Gallery indexed and searched? What's the storage cost per immortalized colony?

**Answer:**

Immortalized worlds (achieved via Transcendence Monument) are conceptually "completed" saves that deserve special treatment. My recommendation is to store them as separate save files with special metadata - not in the active game database. This keeps the Monument Gallery decoupled from active gameplay and makes sharing easier.

For the Monument Gallery, I propose the following structure:

```
saves/
  gallery/
    gallery_index.json    # Searchable metadata for all immortalized worlds
    immortalized_001.zcsv # The actual save files
    immortalized_002.zcsv
    thumbnails/
      immortalized_001.png
```

The `gallery_index.json` contains searchable fields: world name, player names, population at immortalization, date achieved, play time, and notable achievements. This is a simple local index - full-text search via SQLite FTS5 could be added if the gallery grows large, but JSON with in-memory filtering should suffice for the expected scale (tens of immortalized worlds, not thousands).

Storage cost per immortalized colony is the same as a regular save file (~150 KB to ~1 MB depending on map size), plus ~50 KB for the thumbnail. The Monument Gallery is essentially a "hall of fame" save browser with special UI treatment, not a fundamentally different storage mechanism.

---

## GD-4: Multiplayer Recovery Coordination

**Question:** When server restores from backup, how are all clients synchronized? Is there a "state hash" for quick corruption detection? How is recovery coordinated across connected clients?

**Answer:**

Server recovery from backup follows this coordinated flow:

1. **Server announces reload:** Before loading, server broadcasts a `WorldReloadingMessage` to all connected clients. Clients show a loading screen.

2. **Server loads state:** Server pauses the simulation, clears the ECS registry, deserializes the save file (or database checkpoint), and rebuilds all system state. The database is updated to match the loaded state if loading from a save file.

3. **Full state sync:** Server generates a full state snapshot (same mechanism used for late-joining players) and sends it to all clients. Clients clear their local state and apply the snapshot.

4. **Resume:** Server resumes simulation. Clients receive normal delta updates going forward.

For corruption detection, I recommend a **state checksum** at two levels:

- **Save file checksum:** CRC32 of the save file data (stored in header). Detects file corruption before attempting load.
- **Runtime state hash:** A lightweight hash of critical state (entity count, total population, treasury values) computed periodically (every 100 ticks or so). Clients can compare their hash to the server's; mismatch triggers a resync request.

The runtime hash is NOT cryptographic - it's a quick sanity check. True verification happens by the server-authoritative model: server state is always correct by definition. If a client detects a hash mismatch, they request a full snapshot rather than trying to reconcile differences. This keeps the recovery path simple and reliable.

For disconnected players during recovery: they'll receive the standard reconnection flow when they return (full state snapshot). Their session token remains valid if recovery happened within the session timeout window.

---

**End of Systems Architect Answers**
