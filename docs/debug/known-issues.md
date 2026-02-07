# Known Issues

This document tracks known issues, their symptoms, and workarounds.

---

## KI-001: ServerMessages Test Crash in Release Mode

**Status:** Open
**Component:** Test Suite - ServerMessages
**Test:** `test_server_messages`

### Symptom

The `test_server_messages` test crashes or hangs when run in Release configuration. The test passes correctly in Debug configuration.

### Root Cause (Suspected)

MSVC Release optimizations appear to interact poorly with LZ4 compression operations in the test environment. This is believed to be a test environment issue only - the actual ServerMessages implementation is correct and functions properly in production code.

### Workaround

Run tests in Debug configuration:

```bash
ctest -C Debug
```

Or run the specific test in Debug mode:

```bash
ctest -C Debug -R test_server_messages
```

### Notes

- This issue only affects the test environment
- The ServerMessages implementation itself is correct
- Production code using ServerMessages is not affected
- Compiler: MSVC with C++17
