# Decision: Configuration Format

**Date:** 2026-01-25
**Status:** accepted
**Epic:** 0
**Affects Tickets:** 0-003

## Context

Epic 0 needs a configuration system for game settings (window size, audio levels, key bindings, etc.). Need to choose a format and parsing library.

## Options Considered

### 1. JSON with nlohmann/json
- Human-readable
- Header-only C++ library
- Widely used, well-documented
- Matches our YAML canon files in spirit

### 2. TOML
- Very readable for config files
- Less common in C++ ecosystem
- Would need toml++ or similar library

### 3. INI
- Simple and easy to parse
- Limited structure (no nested objects, arrays)
- May not scale for complex config

### 4. Binary format
- Fast to parse
- Compact
- Not human-editable

## Decision

**JSON with nlohmann/json** - Use JSON format with the nlohmann/json library.

## Rationale

1. **Readability:** Human-editable config files
2. **Structure:** Supports nested objects and arrays for complex config
3. **Library:** nlohmann/json is header-only, modern C++, well-maintained
4. **Consistency:** Similar to our YAML canon files
5. **Familiarity:** Most developers know JSON
6. **Tooling:** Wide editor support, syntax highlighting

## Consequences

- Add nlohmann/json to vcpkg.json dependencies
- Config files use .json extension
- Config locations:
  - Windows: %APPDATA%/ZergCity/config.json
  - Linux: ~/.config/zergcity/config.json
  - macOS: ~/Library/Application Support/ZergCity/config.json
- Default config embedded in executable, user config overrides

## File Structure

```json
{
  "video": {
    "width": 1920,
    "height": 1080,
    "fullscreen": false,
    "vsync": true
  },
  "audio": {
    "master_volume": 0.8,
    "music_volume": 0.6,
    "sfx_volume": 1.0
  },
  "input": {
    "bindings": {
      "camera_pan_up": ["W", "Up"],
      "camera_pan_down": ["S", "Down"]
    }
  }
}
```

## References

- nlohmann/json: https://github.com/nlohmann/json
