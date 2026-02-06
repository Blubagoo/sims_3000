/**
 * @file NetworkBuffer.h
 * @brief Binary message serialization buffer for network communication.
 *
 * NetworkBuffer provides helpers for serializing and deserializing network
 * messages in a consistent binary format. All multi-byte values use little-endian
 * byte order as per canon interfaces.yaml.
 *
 * Key features:
 * - Write methods for u8, u16, u32, i32, f32, and strings
 * - Corresponding read methods with bounds checking
 * - Little-endian byte order enforced
 * - String serialization uses length-prefix format
 * - Buffer overflow detection with clear error handling
 *
 * Usage:
 *   // Writing
 *   NetworkBuffer buf;
 *   buf.write_u32(42);
 *   buf.write_string("hello");
 *
 *   // Reading
 *   buf.reset_read();
 *   uint32_t val = buf.read_u32();
 *   std::string str = buf.read_string();
 */

#ifndef SIMS3000_NET_NETWORKBUFFER_H
#define SIMS3000_NET_NETWORKBUFFER_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace sims3000 {

/**
 * @brief Exception thrown when a read operation would exceed buffer bounds.
 */
class BufferOverflowError : public std::runtime_error {
public:
    explicit BufferOverflowError(const std::string& msg)
        : std::runtime_error("NetworkBuffer overflow: " + msg) {}
};

/**
 * @class NetworkBuffer
 * @brief Binary serialization buffer for network messages.
 *
 * Provides read/write operations for primitive types with automatic
 * little-endian byte order and bounds checking on read operations.
 */
class NetworkBuffer {
public:
    /// Default constructor creates an empty buffer.
    NetworkBuffer() = default;

    /// Construct with reserved capacity for better performance.
    explicit NetworkBuffer(std::size_t reserve_size) {
        m_data.reserve(reserve_size);
    }

    /// Construct from existing data (for reading).
    NetworkBuffer(const std::uint8_t* data, std::size_t size)
        : m_data(data, data + size) {}

    // =========================================================================
    // Write operations (always succeed, buffer grows as needed)
    // =========================================================================

    /// Write an unsigned 8-bit integer.
    void write_u8(std::uint8_t value);

    /// Write an unsigned 16-bit integer (little-endian).
    void write_u16(std::uint16_t value);

    /// Write an unsigned 32-bit integer (little-endian).
    void write_u32(std::uint32_t value);

    /// Write a signed 32-bit integer (little-endian).
    void write_i32(std::int32_t value);

    /// Write a 32-bit floating point value (little-endian).
    void write_f32(float value);

    /// Write a length-prefixed string. Length is stored as u32.
    void write_string(const std::string& value);

    /// Write raw bytes to the buffer.
    void write_bytes(const void* data, std::size_t size);

    // =========================================================================
    // Read operations (throw BufferOverflowError if insufficient data)
    // =========================================================================

    /// Read an unsigned 8-bit integer.
    /// @throws BufferOverflowError if insufficient data remains.
    std::uint8_t read_u8();

    /// Read an unsigned 16-bit integer (little-endian).
    /// @throws BufferOverflowError if insufficient data remains.
    std::uint16_t read_u16();

    /// Read an unsigned 32-bit integer (little-endian).
    /// @throws BufferOverflowError if insufficient data remains.
    std::uint32_t read_u32();

    /// Read a signed 32-bit integer (little-endian).
    /// @throws BufferOverflowError if insufficient data remains.
    std::int32_t read_i32();

    /// Read a 32-bit floating point value (little-endian).
    /// @throws BufferOverflowError if insufficient data remains.
    float read_f32();

    /// Read a length-prefixed string.
    /// @throws BufferOverflowError if insufficient data for length prefix or string content.
    std::string read_string();

    /// Read raw bytes from the buffer.
    /// @throws BufferOverflowError if insufficient data remains.
    void read_bytes(void* out, std::size_t size);

    // =========================================================================
    // Buffer state and manipulation
    // =========================================================================

    /// Get pointer to underlying data.
    const std::uint8_t* data() const { return m_data.data(); }

    /// Get mutable pointer to underlying data.
    std::uint8_t* data() { return m_data.data(); }

    /// Get total size of data in buffer.
    std::size_t size() const { return m_data.size(); }

    /// Get current read position.
    std::size_t read_position() const { return m_read_pos; }

    /// Get number of bytes remaining to be read.
    std::size_t remaining() const { return m_data.size() - m_read_pos; }

    /// Check if read position is at end of buffer.
    bool at_end() const { return m_read_pos >= m_data.size(); }

    /// Check if buffer is empty.
    bool empty() const { return m_data.empty(); }

    /// Reset read position to beginning of buffer.
    void reset_read() { m_read_pos = 0; }

    /// Clear all data and reset read position.
    void clear() {
        m_data.clear();
        m_read_pos = 0;
    }

    /// Reserve capacity for better write performance.
    void reserve(std::size_t capacity) { m_data.reserve(capacity); }

    /// Get underlying vector for direct access (use with caution).
    std::vector<std::uint8_t>& raw() { return m_data; }
    const std::vector<std::uint8_t>& raw() const { return m_data; }

private:
    /// Check that sufficient bytes remain for reading.
    /// @throws BufferOverflowError if insufficient bytes.
    void check_read(std::size_t bytes, const char* operation) const;

    std::vector<std::uint8_t> m_data;
    std::size_t m_read_pos = 0;
};

// Static assertions to ensure expected type sizes for serialization
static_assert(sizeof(float) == 4, "float must be 4 bytes for NetworkBuffer serialization");
static_assert(sizeof(std::uint8_t) == 1, "uint8_t must be 1 byte");
static_assert(sizeof(std::uint16_t) == 2, "uint16_t must be 2 bytes");
static_assert(sizeof(std::uint32_t) == 4, "uint32_t must be 4 bytes");
static_assert(sizeof(std::int32_t) == 4, "int32_t must be 4 bytes");

} // namespace sims3000

#endif // SIMS3000_NET_NETWORKBUFFER_H
