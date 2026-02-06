/**
 * @file test_network_buffer.cpp
 * @brief Unit tests for NetworkBuffer serialization utilities.
 *
 * Tests cover:
 * - All data types (u8, u16, u32, i32, f32, string)
 * - Edge cases (INT32_MAX, INT32_MIN, negative values, empty strings)
 * - Round-trip serialization for all types
 * - Little-endian byte order verification
 * - Buffer overflow detection
 * - Exact byte layout verification (for fuzz testing compatibility)
 */

#include "sims3000/net/NetworkBuffer.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <limits>
#include <cstdint>

using namespace sims3000;

// ============================================================================
// Test helpers
// ============================================================================

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("  FAIL: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, msg) \
    do { \
        float diff = std::fabs((a) - (b)); \
        if (diff > 0.0001f) { \
            printf("  FAIL: %s (expected %f, got %f)\n", msg, (b), (a)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_THROWS(expr, exception_type, msg) \
    do { \
        bool caught = false; \
        try { expr; } catch (const exception_type&) { caught = true; } \
        if (!caught) { \
            printf("  FAIL: %s (expected exception)\n", msg); \
            return false; \
        } \
    } while(0)

// ============================================================================
// U8 Tests
// ============================================================================

bool test_u8_basic() {
    printf("  test_u8_basic...\n");
    NetworkBuffer buf;

    buf.write_u8(0);
    buf.write_u8(127);
    buf.write_u8(255);

    TEST_ASSERT_EQ(buf.size(), 3, "buffer size after 3 u8 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u8(), 0, "read u8 value 0");
    TEST_ASSERT_EQ(buf.read_u8(), 127, "read u8 value 127");
    TEST_ASSERT_EQ(buf.read_u8(), 255, "read u8 value 255");

    printf("  PASS\n");
    return true;
}

bool test_u8_byte_layout() {
    printf("  test_u8_byte_layout...\n");
    NetworkBuffer buf;

    buf.write_u8(0xAB);
    TEST_ASSERT_EQ(buf.size(), 1, "u8 uses 1 byte");
    TEST_ASSERT_EQ(buf.data()[0], 0xAB, "u8 byte value");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// U16 Tests
// ============================================================================

bool test_u16_basic() {
    printf("  test_u16_basic...\n");
    NetworkBuffer buf;

    buf.write_u16(0);
    buf.write_u16(32767);
    buf.write_u16(65535);

    TEST_ASSERT_EQ(buf.size(), 6, "buffer size after 3 u16 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u16(), 0, "read u16 value 0");
    TEST_ASSERT_EQ(buf.read_u16(), 32767, "read u16 value 32767");
    TEST_ASSERT_EQ(buf.read_u16(), 65535, "read u16 value 65535");

    printf("  PASS\n");
    return true;
}

bool test_u16_little_endian() {
    printf("  test_u16_little_endian...\n");
    NetworkBuffer buf;

    // 0x1234 should be stored as [0x34, 0x12] in little-endian
    buf.write_u16(0x1234);
    TEST_ASSERT_EQ(buf.size(), 2, "u16 uses 2 bytes");
    TEST_ASSERT_EQ(buf.data()[0], 0x34, "u16 low byte");
    TEST_ASSERT_EQ(buf.data()[1], 0x12, "u16 high byte");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// U32 Tests
// ============================================================================

bool test_u32_basic() {
    printf("  test_u32_basic...\n");
    NetworkBuffer buf;

    buf.write_u32(0);
    buf.write_u32(2147483647);  // INT32_MAX
    buf.write_u32(4294967295);  // UINT32_MAX

    TEST_ASSERT_EQ(buf.size(), 12, "buffer size after 3 u32 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u32(), 0u, "read u32 value 0");
    TEST_ASSERT_EQ(buf.read_u32(), 2147483647u, "read u32 value INT32_MAX");
    TEST_ASSERT_EQ(buf.read_u32(), 4294967295u, "read u32 value UINT32_MAX");

    printf("  PASS\n");
    return true;
}

bool test_u32_little_endian() {
    printf("  test_u32_little_endian...\n");
    NetworkBuffer buf;

    // 0x12345678 should be stored as [0x78, 0x56, 0x34, 0x12] in little-endian
    buf.write_u32(0x12345678);
    TEST_ASSERT_EQ(buf.size(), 4, "u32 uses 4 bytes");
    TEST_ASSERT_EQ(buf.data()[0], 0x78, "u32 byte 0");
    TEST_ASSERT_EQ(buf.data()[1], 0x56, "u32 byte 1");
    TEST_ASSERT_EQ(buf.data()[2], 0x34, "u32 byte 2");
    TEST_ASSERT_EQ(buf.data()[3], 0x12, "u32 byte 3");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// I32 Tests
// ============================================================================

bool test_i32_basic() {
    printf("  test_i32_basic...\n");
    NetworkBuffer buf;

    buf.write_i32(0);
    buf.write_i32(100);
    buf.write_i32(-100);

    TEST_ASSERT_EQ(buf.size(), 12, "buffer size after 3 i32 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_i32(), 0, "read i32 value 0");
    TEST_ASSERT_EQ(buf.read_i32(), 100, "read i32 value 100");
    TEST_ASSERT_EQ(buf.read_i32(), -100, "read i32 value -100");

    printf("  PASS\n");
    return true;
}

bool test_i32_edge_cases() {
    printf("  test_i32_edge_cases...\n");
    NetworkBuffer buf;

    buf.write_i32(std::numeric_limits<std::int32_t>::max());  // INT32_MAX
    buf.write_i32(std::numeric_limits<std::int32_t>::min());  // INT32_MIN
    buf.write_i32(-1);

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_i32(), std::numeric_limits<std::int32_t>::max(), "read i32 INT32_MAX");
    TEST_ASSERT_EQ(buf.read_i32(), std::numeric_limits<std::int32_t>::min(), "read i32 INT32_MIN");
    TEST_ASSERT_EQ(buf.read_i32(), -1, "read i32 value -1");

    printf("  PASS\n");
    return true;
}

bool test_i32_negative_byte_layout() {
    printf("  test_i32_negative_byte_layout...\n");
    NetworkBuffer buf;

    // -1 in two's complement is 0xFFFFFFFF
    buf.write_i32(-1);
    TEST_ASSERT_EQ(buf.data()[0], 0xFF, "i32 -1 byte 0");
    TEST_ASSERT_EQ(buf.data()[1], 0xFF, "i32 -1 byte 1");
    TEST_ASSERT_EQ(buf.data()[2], 0xFF, "i32 -1 byte 2");
    TEST_ASSERT_EQ(buf.data()[3], 0xFF, "i32 -1 byte 3");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// F32 Tests
// ============================================================================

bool test_f32_basic() {
    printf("  test_f32_basic...\n");
    NetworkBuffer buf;

    buf.write_f32(0.0f);
    buf.write_f32(1.0f);
    buf.write_f32(-1.0f);
    buf.write_f32(3.14159f);

    TEST_ASSERT_EQ(buf.size(), 16, "buffer size after 4 f32 writes");

    buf.reset_read();
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 0.0f, "read f32 value 0.0");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 1.0f, "read f32 value 1.0");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), -1.0f, "read f32 value -1.0");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 3.14159f, "read f32 value pi");

    printf("  PASS\n");
    return true;
}

bool test_f32_edge_cases() {
    printf("  test_f32_edge_cases...\n");
    NetworkBuffer buf;

    buf.write_f32(std::numeric_limits<float>::max());
    buf.write_f32(std::numeric_limits<float>::min());
    buf.write_f32(std::numeric_limits<float>::lowest());
    buf.write_f32(std::numeric_limits<float>::epsilon());

    buf.reset_read();
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::max(), "read f32 FLT_MAX");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::min(), "read f32 FLT_MIN");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::lowest(), "read f32 FLT_LOWEST");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::epsilon(), "read f32 FLT_EPSILON");

    printf("  PASS\n");
    return true;
}

bool test_f32_special_values() {
    printf("  test_f32_special_values...\n");
    NetworkBuffer buf;

    buf.write_f32(0.0f);
    buf.write_f32(-0.0f);

    buf.reset_read();
    float pos_zero = buf.read_f32();
    float neg_zero = buf.read_f32();

    TEST_ASSERT_FLOAT_EQ(pos_zero, 0.0f, "positive zero");
    TEST_ASSERT_FLOAT_EQ(neg_zero, 0.0f, "negative zero value");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// String Tests
// ============================================================================

bool test_string_basic() {
    printf("  test_string_basic...\n");
    NetworkBuffer buf;

    buf.write_string("hello");
    // 4 bytes length prefix + 5 bytes content
    TEST_ASSERT_EQ(buf.size(), 9, "string 'hello' uses 9 bytes");

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT(result == "hello", "read string matches");

    printf("  PASS\n");
    return true;
}

bool test_string_empty() {
    printf("  test_string_empty...\n");
    NetworkBuffer buf;

    buf.write_string("");
    // 4 bytes length prefix + 0 bytes content
    TEST_ASSERT_EQ(buf.size(), 4, "empty string uses 4 bytes");

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT(result.empty(), "empty string reads as empty");

    printf("  PASS\n");
    return true;
}

bool test_string_with_special_chars() {
    printf("  test_string_with_special_chars...\n");
    NetworkBuffer buf;

    std::string test_str = "hello\0world"; // Contains null byte
    test_str = std::string("hello\0world", 11);
    buf.write_string(test_str);

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT_EQ(result.size(), 11, "string with null preserves length");
    TEST_ASSERT(result == test_str, "string with null byte preserved");

    printf("  PASS\n");
    return true;
}

bool test_string_byte_layout() {
    printf("  test_string_byte_layout...\n");
    NetworkBuffer buf;

    buf.write_string("AB");
    // Length = 2 stored as little-endian u32: [0x02, 0x00, 0x00, 0x00]
    // Content: ['A', 'B']
    TEST_ASSERT_EQ(buf.size(), 6, "string 'AB' uses 6 bytes");
    TEST_ASSERT_EQ(buf.data()[0], 0x02, "length byte 0");
    TEST_ASSERT_EQ(buf.data()[1], 0x00, "length byte 1");
    TEST_ASSERT_EQ(buf.data()[2], 0x00, "length byte 2");
    TEST_ASSERT_EQ(buf.data()[3], 0x00, "length byte 3");
    TEST_ASSERT_EQ(buf.data()[4], 'A', "content byte 0");
    TEST_ASSERT_EQ(buf.data()[5], 'B', "content byte 1");

    printf("  PASS\n");
    return true;
}

bool test_string_long() {
    printf("  test_string_long...\n");
    NetworkBuffer buf;

    // Create a string longer than 256 bytes to test u32 length
    std::string long_str(1000, 'x');
    buf.write_string(long_str);

    TEST_ASSERT_EQ(buf.size(), 1004, "long string uses 1004 bytes");

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT_EQ(result.size(), 1000, "long string length preserved");
    TEST_ASSERT(result == long_str, "long string content matches");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Round-trip Tests
// ============================================================================

bool test_roundtrip_mixed_types() {
    printf("  test_roundtrip_mixed_types...\n");
    NetworkBuffer buf;

    // Write a mix of types
    buf.write_u8(42);
    buf.write_u16(1234);
    buf.write_u32(567890);
    buf.write_i32(-12345);
    buf.write_f32(3.14159f);
    buf.write_string("test message");
    buf.write_u8(255);

    // Calculate expected size
    // 1 + 2 + 4 + 4 + 4 + (4 + 12) + 1 = 32
    TEST_ASSERT_EQ(buf.size(), 32, "mixed types total size");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u8(), 42, "roundtrip u8");
    TEST_ASSERT_EQ(buf.read_u16(), 1234, "roundtrip u16");
    TEST_ASSERT_EQ(buf.read_u32(), 567890u, "roundtrip u32");
    TEST_ASSERT_EQ(buf.read_i32(), -12345, "roundtrip i32");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 3.14159f, "roundtrip f32");
    TEST_ASSERT(buf.read_string() == "test message", "roundtrip string");
    TEST_ASSERT_EQ(buf.read_u8(), 255, "roundtrip final u8");
    TEST_ASSERT(buf.at_end(), "buffer fully consumed");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Buffer Overflow Tests
// ============================================================================

bool test_overflow_u8() {
    printf("  test_overflow_u8...\n");
    NetworkBuffer buf;
    // Empty buffer - should throw on read
    TEST_ASSERT_THROWS(buf.read_u8(), BufferOverflowError, "read_u8 on empty buffer");
    printf("  PASS\n");
    return true;
}

bool test_overflow_u16() {
    printf("  test_overflow_u16...\n");
    NetworkBuffer buf;
    buf.write_u8(0xFF);  // Only 1 byte
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_u16(), BufferOverflowError, "read_u16 with insufficient data");
    printf("  PASS\n");
    return true;
}

bool test_overflow_u32() {
    printf("  test_overflow_u32...\n");
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_u32(), BufferOverflowError, "read_u32 with insufficient data");
    printf("  PASS\n");
    return true;
}

bool test_overflow_i32() {
    printf("  test_overflow_i32...\n");
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_i32(), BufferOverflowError, "read_i32 with insufficient data");
    printf("  PASS\n");
    return true;
}

bool test_overflow_f32() {
    printf("  test_overflow_f32...\n");
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_f32(), BufferOverflowError, "read_f32 with insufficient data");
    printf("  PASS\n");
    return true;
}

bool test_overflow_string_length() {
    printf("  test_overflow_string_length...\n");
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes, need 4 for length
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_string(), BufferOverflowError, "read_string length with insufficient data");
    printf("  PASS\n");
    return true;
}

bool test_overflow_string_content() {
    printf("  test_overflow_string_content...\n");
    NetworkBuffer buf;
    buf.write_u32(100);  // Claims 100 bytes of content
    buf.write_u8('x');   // But only 1 byte of content
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_string(), BufferOverflowError, "read_string content with insufficient data");
    printf("  PASS\n");
    return true;
}

bool test_overflow_read_bytes() {
    printf("  test_overflow_read_bytes...\n");
    NetworkBuffer buf;
    buf.write_u32(0x12345678);
    buf.reset_read();
    char out[10];
    TEST_ASSERT_THROWS(buf.read_bytes(out, 10), BufferOverflowError, "read_bytes with insufficient data");
    printf("  PASS\n");
    return true;
}

// ============================================================================
// Buffer State Tests
// ============================================================================

bool test_buffer_state() {
    printf("  test_buffer_state...\n");
    NetworkBuffer buf;

    TEST_ASSERT(buf.empty(), "new buffer is empty");
    TEST_ASSERT_EQ(buf.size(), 0, "new buffer size is 0");
    TEST_ASSERT(buf.at_end(), "new buffer is at end");

    buf.write_u32(42);
    TEST_ASSERT(!buf.empty(), "buffer not empty after write");
    TEST_ASSERT_EQ(buf.size(), 4, "buffer size after u32 write");
    TEST_ASSERT_EQ(buf.read_position(), 0, "read position before read");
    TEST_ASSERT_EQ(buf.remaining(), 4, "remaining bytes before read");
    TEST_ASSERT(!buf.at_end(), "buffer not at end before read");

    buf.read_u32();
    TEST_ASSERT_EQ(buf.read_position(), 4, "read position after read");
    TEST_ASSERT_EQ(buf.remaining(), 0, "remaining bytes after read");
    TEST_ASSERT(buf.at_end(), "buffer at end after read");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_position(), 0, "read position after reset");
    TEST_ASSERT_EQ(buf.remaining(), 4, "remaining bytes after reset");

    buf.clear();
    TEST_ASSERT(buf.empty(), "buffer empty after clear");
    TEST_ASSERT_EQ(buf.size(), 0, "buffer size 0 after clear");
    TEST_ASSERT_EQ(buf.read_position(), 0, "read position 0 after clear");

    printf("  PASS\n");
    return true;
}

bool test_buffer_construction() {
    printf("  test_buffer_construction...\n");

    // Test construction with reserved capacity
    NetworkBuffer buf1(1024);
    TEST_ASSERT(buf1.empty(), "reserved buffer is empty");

    // Test construction from existing data
    std::uint8_t data[] = {0x78, 0x56, 0x34, 0x12};
    NetworkBuffer buf2(data, 4);
    TEST_ASSERT_EQ(buf2.size(), 4, "buffer from data has correct size");
    TEST_ASSERT_EQ(buf2.read_u32(), 0x12345678u, "buffer from data reads correctly");

    printf("  PASS\n");
    return true;
}

bool test_write_bytes_and_read_bytes() {
    printf("  test_write_bytes_and_read_bytes...\n");
    NetworkBuffer buf;

    std::uint8_t write_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    buf.write_bytes(write_data, 4);

    TEST_ASSERT_EQ(buf.size(), 4, "write_bytes size");

    buf.reset_read();
    std::uint8_t read_data[4] = {0};
    buf.read_bytes(read_data, 4);

    TEST_ASSERT_EQ(read_data[0], 0xDE, "read_bytes byte 0");
    TEST_ASSERT_EQ(read_data[1], 0xAD, "read_bytes byte 1");
    TEST_ASSERT_EQ(read_data[2], 0xBE, "read_bytes byte 2");
    TEST_ASSERT_EQ(read_data[3], 0xEF, "read_bytes byte 3");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== NetworkBuffer Unit Tests ===\n\n");

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(test) \
        do { \
            printf("Running %s\n", #test); \
            if (test()) { passed++; } else { failed++; } \
        } while(0)

    printf("--- U8 Tests ---\n");
    RUN_TEST(test_u8_basic);
    RUN_TEST(test_u8_byte_layout);

    printf("\n--- U16 Tests ---\n");
    RUN_TEST(test_u16_basic);
    RUN_TEST(test_u16_little_endian);

    printf("\n--- U32 Tests ---\n");
    RUN_TEST(test_u32_basic);
    RUN_TEST(test_u32_little_endian);

    printf("\n--- I32 Tests ---\n");
    RUN_TEST(test_i32_basic);
    RUN_TEST(test_i32_edge_cases);
    RUN_TEST(test_i32_negative_byte_layout);

    printf("\n--- F32 Tests ---\n");
    RUN_TEST(test_f32_basic);
    RUN_TEST(test_f32_edge_cases);
    RUN_TEST(test_f32_special_values);

    printf("\n--- String Tests ---\n");
    RUN_TEST(test_string_basic);
    RUN_TEST(test_string_empty);
    RUN_TEST(test_string_with_special_chars);
    RUN_TEST(test_string_byte_layout);
    RUN_TEST(test_string_long);

    printf("\n--- Round-trip Tests ---\n");
    RUN_TEST(test_roundtrip_mixed_types);

    printf("\n--- Buffer Overflow Tests ---\n");
    RUN_TEST(test_overflow_u8);
    RUN_TEST(test_overflow_u16);
    RUN_TEST(test_overflow_u32);
    RUN_TEST(test_overflow_i32);
    RUN_TEST(test_overflow_f32);
    RUN_TEST(test_overflow_string_length);
    RUN_TEST(test_overflow_string_content);
    RUN_TEST(test_overflow_read_bytes);

    printf("\n--- Buffer State Tests ---\n");
    RUN_TEST(test_buffer_state);
    RUN_TEST(test_buffer_construction);
    RUN_TEST(test_write_bytes_and_read_bytes);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);

    if (failed > 0) {
        printf("\n=== TESTS FAILED ===\n");
        return 1;
    }

    printf("\n=== All tests passed! ===\n");
    return 0;
}
