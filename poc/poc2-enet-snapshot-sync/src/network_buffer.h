#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <stdexcept>

class NetworkBuffer {
public:
    NetworkBuffer() = default;
    explicit NetworkBuffer(size_t reserve_size) { data_.reserve(reserve_size); }
    NetworkBuffer(const uint8_t* data, size_t size) : data_(data, data + size) {}

    // Write operations
    void write_u8(uint8_t v);
    void write_u16(uint16_t v);
    void write_u32(uint32_t v);
    void write_u64(uint64_t v);
    void write_float(float v);
    void write_bytes(const void* data, size_t size);

    // Read operations (advance read position)
    uint8_t read_u8();
    uint16_t read_u16();
    uint32_t read_u32();
    uint64_t read_u64();
    float read_float();
    void read_bytes(void* out, size_t size);

    // Accessors
    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }
    size_t size() const { return data_.size(); }
    size_t read_pos() const { return read_pos_; }
    size_t remaining() const { return data_.size() - read_pos_; }
    bool at_end() const { return read_pos_ >= data_.size(); }

    void reset_read() { read_pos_ = 0; }
    void clear() { data_.clear(); read_pos_ = 0; }
    void resize(size_t new_size) { data_.resize(new_size); }

    // Direct access to underlying vector for compression output
    std::vector<uint8_t>& raw() { return data_; }
    const std::vector<uint8_t>& raw() const { return data_; }

private:
    void check_read(size_t bytes) const;

    std::vector<uint8_t> data_;
    size_t read_pos_ = 0;
};
