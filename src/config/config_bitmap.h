#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace config {
    // Most typed values are integers. Do not construct an MSVC checked-vector
    // iterator proxy for their unused bitmap payloads.
    class s_config_bitmap {
      public:
        s_config_bitmap() = default;
        s_config_bitmap(const s_config_bitmap& other) {
            if (other.bytes_) bytes_ = std::make_unique<std::vector<uint8_t>>(*other.bytes_);
        }
        s_config_bitmap(s_config_bitmap&&) noexcept = default;
        s_config_bitmap& operator=(s_config_bitmap&&) noexcept = default;
        s_config_bitmap& operator=(const s_config_bitmap& other) {
            if (this != &other) *this = s_config_bitmap(other);
            return *this;
        }
        size_t size() const {
            return bytes_ ? bytes_->size() : 0;
        }
        bool empty() const {
            return size() == 0;
        }
        const uint8_t* data() const {
            return bytes_ ? bytes_->data() : nullptr;
        }
        uint8_t* data() {
            return bytes_ ? bytes_->data() : nullptr;
        }
        uint8_t& operator[](size_t index) {
            return (*bytes_)[index];
        }
        uint8_t operator[](size_t index) const {
            return (*bytes_)[index];
        }
        uint8_t back() const {
            return bytes_->back();
        }
        const uint8_t* begin() const {
            return data();
        }
        const uint8_t* end() const {
            return bytes_ ? bytes_->data() + bytes_->size() : nullptr;
        }
        void resize(size_t size) {
            Storage_().resize(size);
        }
        void assign(size_t size, uint8_t value) {
            Storage_().assign(size, value);
        }
        void assign(const uint8_t* first, const uint8_t* last) {
            Storage_().assign(first, last);
        }
        bool operator==(const s_config_bitmap& other) const {
            return empty() ? other.empty() : !other.empty() && *bytes_ == *other.bytes_;
        }
        bool operator==(const std::vector<uint8_t>& other) const {
            return bytes_ ? *bytes_ == other : other.empty();
        }

      private:
        std::vector<uint8_t>& Storage_() {
            if (!bytes_) bytes_ = std::make_unique<std::vector<uint8_t>>();
            return *bytes_;
        }
        std::unique_ptr<std::vector<uint8_t>> bytes_;
    };
} // namespace config