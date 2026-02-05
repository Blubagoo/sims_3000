#include "network_buffer.h"

void NetworkBuffer::check_read(size_t bytes) const {
    if (read_pos_ + bytes > data_.size()) {
        throw std::runtime_error("NetworkBuffer: read past end");
    }
}

void NetworkBuffer::write_u8(uint8_t v) {
    data_.push_back(v);
}

void NetworkBuffer::write_u16(uint16_t v) {
    data_.push_back(static_cast<uint8_t>(v & 0xFF));
    data_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void NetworkBuffer::write_u32(uint32_t v) {
    uint8_t bytes[4];
    std::memcpy(bytes, &v, 4);
    data_.insert(data_.end(), bytes, bytes + 4);
}

void NetworkBuffer::write_u64(uint64_t v) {
    uint8_t bytes[8];
    std::memcpy(bytes, &v, 8);
    data_.insert(data_.end(), bytes, bytes + 8);
}

void NetworkBuffer::write_float(float v) {
    uint8_t bytes[4];
    std::memcpy(bytes, &v, 4);
    data_.insert(data_.end(), bytes, bytes + 4);
}

void NetworkBuffer::write_bytes(const void* data, size_t size) {
    auto ptr = static_cast<const uint8_t*>(data);
    data_.insert(data_.end(), ptr, ptr + size);
}

uint8_t NetworkBuffer::read_u8() {
    check_read(1);
    return data_[read_pos_++];
}

uint16_t NetworkBuffer::read_u16() {
    check_read(2);
    uint16_t v = static_cast<uint16_t>(data_[read_pos_])
               | (static_cast<uint16_t>(data_[read_pos_ + 1]) << 8);
    read_pos_ += 2;
    return v;
}

uint32_t NetworkBuffer::read_u32() {
    check_read(4);
    uint32_t v;
    std::memcpy(&v, &data_[read_pos_], 4);
    read_pos_ += 4;
    return v;
}

uint64_t NetworkBuffer::read_u64() {
    check_read(8);
    uint64_t v;
    std::memcpy(&v, &data_[read_pos_], 8);
    read_pos_ += 8;
    return v;
}

float NetworkBuffer::read_float() {
    check_read(4);
    float v;
    std::memcpy(&v, &data_[read_pos_], 4);
    read_pos_ += 4;
    return v;
}

void NetworkBuffer::read_bytes(void* out, size_t size) {
    check_read(size);
    std::memcpy(out, &data_[read_pos_], size);
    read_pos_ += size;
}
