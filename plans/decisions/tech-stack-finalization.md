# Decision: Technology Stack Finalization

**Date:** 2026-02-02
**Status:** accepted
**Epic:** 0, 1
**Affects:** All subsequent epics

## Context

The project confidence assessment identified several gaps requiring pre-POC decisions:
1. Server-side database for persistent state
2. Exact library versions for all dependencies
3. Build system configuration requirements

This document finalizes these decisions to enable implementation to proceed.

---

## 1. Database Selection

### Decision: **SQLite 3.45+**

### Rationale

| Factor | SQLite | LevelDB | PostgreSQL |
|--------|--------|---------|------------|
| **Embedded** | Yes | Yes | No (requires separate server) |
| **ACID transactions** | Yes | No (eventual consistency) | Yes |
| **Single-file storage** | Yes | Directory of files | Multiple files |
| **SQL queries** | Yes | Key-value only | Yes |
| **Save file portability** | Excellent (single .db file) | Poor (directory) | N/A |
| **Concurrent access** | WAL mode supports it | Limited | Excellent |
| **Complexity** | Low | Low | High |
| **Multiplayer implications** | Server-only access (safe) | Server-only (safe) | Separate deployment |

### Why SQLite

1. **Portability:** Single `.zcsave` file per world - easy to backup, share, move
2. **ACID guarantees:** Crash recovery without manual journaling
3. **Schema flexibility:** SQL allows flexible queries for debugging, admin tools
4. **WAL mode:** Write-ahead logging enables concurrent reads during writes
5. **Proven at scale:** Used in browsers, mobile apps, embedded systems worldwide
6. **No deployment complexity:** No separate database server to run

### Implementation Notes

```cpp
// Connection setup
sqlite3* db;
sqlite3_open_v2("world.zcsave", &db,
    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_WAL,
    nullptr);

// Enable WAL mode for better concurrency
sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

// Checkpoint periodically (every N ticks or on demand)
sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
```

### Schema Strategy

- **Entities table:** `entity_id, archetype_id, created_tick`
- **Components tables:** One table per component type, entity_id as foreign key
- **Dense grids:** Stored as BLOBs (LZ4 compressed)
- **Metadata table:** World settings, tick count, RNG state

### Alternatives Rejected

- **LevelDB:** No ACID, no SQL for debugging, directory-based storage less portable
- **PostgreSQL:** Overkill for 2-4 player game, requires separate server deployment
- **Raw files:** No ACID, manual crash recovery, format migrations harder

### Risks

- SQLite write performance may bottleneck at very high entity counts (>100K)
- Mitigation: Batch writes, checkpoint tuning, component delta tracking

---

## 2. Library Versions

All versions are pinned to specific releases for reproducibility.

### Core Graphics/Audio

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **SDL3** | 3.2.0 | `sdl3` | Latest stable release (Jan 2026). SDL_GPU support mature. |
| **SDL3_image** | 3.0.0 | `sdl3-image` | Texture loading (PNG, JPEG). Matches SDL3 version series. |

**Note:** SDL3 3.2.0 is the target. If vcpkg provides a newer patch version (3.2.x), that is acceptable.

### ECS Framework

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **EnTT** | 3.13.2 | `entt` | Latest stable. Sparse-set storage, header-only, well-documented. |

**Note:** EnTT is header-only. Version pinned for API stability.

### Networking

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **ENet** | 1.3.18 | `enet` | Reliable UDP with sequencing. Simple API. Widely used in games. |

**Alternatives Considered:**
- **GameNetworkingSockets (Valve):** More features but more complex, larger dependency
- **Raw UDP + custom reliability:** Too much custom code

### Compression

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **LZ4** | 1.9.4 | `lz4` | Extremely fast (4+ GB/s decompress). Good ratios for game state. Per epic-1-lz4-compression.md decision. |

### Audio

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **libvorbis** | 1.3.7 | `libvorbis` | OGG Vorbis decoding. Canon recommends OGG for SFX and music. |
| **libogg** | 1.3.5 | `libogg` | OGG container format. Required by libvorbis. |

**Note:** SDL3 can use SDL_mixer, but direct libvorbis gives more control for streaming/positional audio.

### Configuration & Serialization

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **nlohmann_json** | 3.11.3 | `nlohmann-json` | JSON parsing for config files and client settings. Header-only, widely used. |
| **yaml-cpp** | 0.8.0 | `yaml-cpp` | YAML parsing for canon files. Clean API, well-maintained. |

**Usage:**
- JSON: Client settings (`settings.json`), network message debug output
- YAML: Canon files read at development time (not shipped in release)

### Database

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **SQLite** | 3.45.0+ | `sqlite3` | Server-side persistence. Single-file, ACID, WAL mode. |

### 3D Model Loading

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **cgltf** | 1.14 | `cgltf` | glTF 2.0 loading. Header-only, minimal dependencies. Canon specifies glTF for models. |

**Alternatives Considered:**
- **tinygltf:** More features but heavier
- **Assimp:** Too general, large dependency tree

### Math

| Library | Version | vcpkg Port | Rationale |
|---------|---------|------------|-----------|
| **glm** | 1.0.1 | `glm` | OpenGL Mathematics. Header-only. Industry standard for 3D math. |

---

## 3. Complete vcpkg.json

```json
{
    "name": "sims-3000",
    "version": "0.1.0",
    "dependencies": [
        "sdl3",
        "sdl3-image",
        "entt",
        "enet",
        "lz4",
        "libvorbis",
        "libogg",
        "nlohmann-json",
        "yaml-cpp",
        "sqlite3",
        "cgltf",
        "glm"
    ],
    "overrides": [
        { "name": "sdl3", "version": "3.2.0" },
        { "name": "entt", "version": "3.13.2" },
        { "name": "enet", "version": "1.3.18" },
        { "name": "lz4", "version": "1.9.4" },
        { "name": "sqlite3", "version": "3.45.0" }
    ]
}
```

---

## 4. Build System Requirements

### CMake Minimum Version

**CMake 3.25+** required.

Rationale:
- SDL3 CMake support requires 3.16+
- vcpkg manifest mode works best with 3.21+
- Presets (CMakePresets.json) require 3.21+
- 3.25 provides improved diagnostics and target property support

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.25)
project(ZergCity VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# vcpkg toolchain (set via preset or command line)
# -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Find packages
find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(enet CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# cgltf is header-only, just need include path
```

### Build Targets

| Target | Type | Description |
|--------|------|-------------|
| `zergcity_core` | STATIC | Shared code: ECS, components, systems, serialization |
| `zergcity_server` | EXECUTABLE | Dedicated server (no rendering, no audio) |
| `zergcity_client` | EXECUTABLE | Client (rendering, audio, UI) |
| `zergcity_tests` | EXECUTABLE | Test runner |

### Compile Definitions

| Define | Scope | Purpose |
|--------|-------|---------|
| `ZERGCITY_SERVER` | Server only | Exclude rendering code |
| `ZERGCITY_CLIENT` | Client only | Include rendering code |
| `ZERGCITY_DEBUG` | Debug builds | Enable asserts, verbose logging |
| `ZERGCITY_RELEASE` | Release builds | Disable asserts, minimal logging |

### Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Windows 10/11 | Primary | Development platform |
| Linux (Ubuntu 22.04+) | Secondary | CI/CD, server deployment |
| macOS 13+ | Tertiary | Community builds |

---

## 5. Consequences

### Positive

- All dependencies pinned for reproducible builds
- vcpkg handles cross-platform complexity
- SQLite provides crash recovery without external database
- Save files are portable single-file format
- Build system supports server/client separation from start

### Negative

- vcpkg overrides may conflict with newer library features
- SQLite schema changes require migration code
- Library updates require coordinated version bumps

### Migration Path

When upgrading libraries:
1. Test in feature branch
2. Update version in vcpkg.json overrides
3. Run full test suite
4. Update this document

---

## 6. References

- SDL3 documentation: https://wiki.libsdl.org/SDL3
- EnTT wiki: https://github.com/skypjack/entt/wiki
- ENet documentation: http://enet.bespin.org/
- LZ4 format: https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md
- SQLite documentation: https://sqlite.org/docs.html
- vcpkg documentation: https://vcpkg.io/en/docs/README.html
- glTF 2.0 spec: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
