/**
 * @file NetworkBuffer.cpp
 * @brief Implementation of NetworkBuffer binary serialization.
 */

#include "sims3000/net/NetworkBuffer.h"
#include <sstream>

namespace sims3000 {

// ============================================================================
// Write Operations
// ============================================================================

void NetworkBuffer::write_u8(std::uint8_t value) {
    m_data.push_back(value);
}

void NetworkBuffer::write_u16(std::uint16_t value) {
    // Little-endian: low byte first
    m_data.push_back(static_cast<std::uint8_t>(value & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void NetworkBuffer::write_u32(std::uint32_t value) {
    // Little-endian: low byte first
    m_data.push_back(static_cast<std::uint8_t>(value & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

void NetworkBuffer::write_i32(std::int32_t value) {
    // Reinterpret as unsigned and write in little-endian
    std::uint32_t uvalue;
    std::memcpy(&uvalue, &value, sizeof(value));
    write_u32(uvalue);
}

void NetworkBuffer::write_f32(float value) {
    // Copy float bytes directly (IEEE 754 representation)
    std::uint8_t bytes[4];
    std::memcpy(bytes, &value, sizeof(value));
    m_data.insert(m_data.end(), bytes, bytes + 4);
}

void NetworkBuffer::write_string(const std::string& value) {
    // Length prefix as u32, then raw string bytes (no null terminator)
    write_u32(static_cast<std::uint32_t>(value.size()));
    if (!value.empty()) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(value.data());
        m_data.insert(m_data.end(), bytes, bytes + value.size());
    }
}

void NetworkBuffer::write_bytes(const void* data, std::size_t size) {
    if (size > 0 && data != nullptr) {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        m_data.insert(m_data.end(), bytes, bytes + size);
    }
}

// ============================================================================
// Read Operations
// ============================================================================

void NetworkBuffer::check_read(std::size_t bytes, const char* operation) const {
    if (m_read_pos + bytes > m_data.size()) {
        std::ostringstream oss;
        oss << operation << " requires " << bytes << " bytes, but only "
            << (m_data.size() - m_read_pos) << " bytes remain (pos="
            << m_read_pos << ", size=" << m_data.size() << ")";
        throw BufferOverflowError(oss.str());
    }
}

std::uint8_t NetworkBuffer::read_u8() {
    check_read(1, "read_u8");
    return m_data[m_read_pos++];
}

std::uint16_t NetworkBuffer::read_u16() {
    check_read(2, "read_u16");
    // Little-endian: low byte first
    std::uint16_t value = static_cast<std::uint16_t>(m_data[m_read_pos])
                        | (static_cast<std::uint16_t>(m_data[m_read_pos + 1]) << 8);
    m_read_pos += 2;
    return value;
}

std::uint32_t NetworkBuffer::read_u32() {
    check_read(4, "read_u32");
    // Little-endian: low byte first
    std::uint32_t value = static_cast<std::uint32_t>(m_data[m_read_pos])
                        | (static_cast<std::uint32_t>(m_data[m_read_pos + 1]) << 8)
                        | (static_cast<std::uint32_t>(m_data[m_read_pos + 2]) << 16)
                        | (static_cast<std::uint32_t>(m_data[m_read_pos + 3]) << 24);
    m_read_pos += 4;
    return value;
}

std::int32_t NetworkBuffer::read_i32() {
    std::uint32_t uvalue = read_u32();
    std::int32_t value;
    std::memcpy(&value, &uvalue, sizeof(value));
    return value;
}

float NetworkBuffer::read_f32() {
    check_read(4, "read_f32");
    float value;
    std::memcpy(&value, &m_data[m_read_pos], sizeof(value));
    m_read_pos += 4;
    return value;
}

std::string NetworkBuffer::read_string() {
    std::uint32_t length = read_u32();

    if (length == 0) {
        return std::string();
    }

    check_read(length, "read_string (content)");
    std::string value(reinterpret_cast<const char*>(&m_data[m_read_pos]), length);
    m_read_pos += length;
    return value;
}

void NetworkBuffer::read_bytes(void* out, std::size_t size) {
    if (size == 0) {
        return;
    }
    check_read(size, "read_bytes");
    std::memcpy(out, &m_data[m_read_pos], size);
    m_read_pos += size;
}

} // namespace sims3000
