// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include <bit>
#include <concepts>
#include <expected>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <source_location>

namespace bitcoin::serialize {

// Version for backward compatibility
inline constexpr std::uint32_t VERSION = 31100;
inline constexpr std::size_t MAX_SIZE = 0x02000000;

// Modern error handling
enum class Error {
    buffer_overflow,
    invalid_format,
    size_too_large,
    unexpected_end,
    invalid_variant_index
};

[[nodiscard]] constexpr std::string_view error_message(Error e) noexcept {
    switch (e) {
        case Error::buffer_overflow: return "Buffer overflow";
        case Error::invalid_format: return "Invalid format";
        case Error::size_too_large: return "Size too large";
        case Error::unexpected_end: return "Unexpected end of data";
        case Error::invalid_variant_index: return "Invalid variant index";
    }
    return "Unknown error";
}

// Concept for primitive serializable types
template<typename T>
concept Primitive = std::integral<T> || std::floating_point<T>;

// Concept for container types
template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::input_or_output_iterator;
};

// Modern buffer management with span
class Buffer {
public:
    using value_type = byte_t;
    using iterator = byte_t*;
    using const_iterator = const byte_t*;
    
    explicit Buffer(std::span<byte_t> data) noexcept 
        : data_(data), pos_(0) {}
    
    [[nodiscard]] constexpr std::size_t remaining() const noexcept {
        return data_.size() - pos_;
    }
    
    [[nodiscard]] constexpr bool has_space(std::size_t n) const noexcept {
        return remaining() >= n;
    }
    
    [[nodiscard]] std::expected<std::span<byte_t>, Error> 
    write_bytes(std::span<const byte_t> bytes) noexcept {
        if (!has_space(bytes.size())) {
            return std::unexpected(Error::buffer_overflow);
        }
        std::ranges::copy(bytes, data_.begin() + pos_);
        pos_ += bytes.size();
        return data_.subspan(pos_ - bytes.size(), bytes.size());
    }
    
    [[nodiscard]] std::expected<std::span<const byte_t>, Error>
    read_bytes(std::size_t n) noexcept {
        if (!has_space(n)) {
            return std::unexpected(Error::unexpected_end);
        }
        auto result = data_.subspan(pos_, n);
        pos_ += n;
        return result;
    }
    
    template<Primitive T>
    [[nodiscard]] std::expected<void, Error> write(T value) noexcept {
        if constexpr (std::endian::native != std::endian::little) {
            value = std::byteswap(value);
        }
        return write_bytes(std::as_bytes(std::span{&value, 1}))
            .transform([](auto) { return std::monostate{}; });
    }
    
    template<Primitive T>
    [[nodiscard]] std::expected<T, Error> read() noexcept {
        T value{};
        auto bytes = read_bytes(sizeof(T));
        if (!bytes) return std::unexpected(bytes.error());
        std::memcpy(&value, bytes->data(), sizeof(T));
        if constexpr (std::endian::native != std::endian::little) {
            value = std::byteswap(value);
        }
        return value;
    }
    
    void reset() noexcept { pos_ = 0; }
    [[nodiscard]] std::size_t position() const noexcept { return pos_; }
    
private:
    std::span<byte_t> data_;
    std::size_t pos_;
};

// CompactSize encoding - Satoshi's variable length integer
class CompactSize {
public:
    explicit constexpr CompactSize(std::uint64_t n) noexcept : value_(n) {}
    
    [[nodiscard]] constexpr std::uint64_t value() const noexcept { return value_; }
    
    [[nodiscard]] constexpr std::size_t encoded_size() const noexcept {
        if (value_ < 253) return 1;
        if (value_ <= 0xFFFF) return 3;
        if (value_ <= 0xFFFFFFFF) return 5;
        return 9;
    }
    
    [[nodiscard]] std::expected<void, Error> 
    serialize(Buffer& buffer) const noexcept {
        if (value_ < 253) {
            return buffer.write(static_cast<std::uint8_t>(value_));
        }
        if (value_ <= 0xFFFF) {
            if (auto r = buffer.write(std::uint8_t{253}); !r) return r;
            return buffer.write(static_cast<std::uint16_t>(value_));
        }
        if (value_ <= 0xFFFFFFFF) {
            if (auto r = buffer.write(std::uint8_t{254}); !r) return r;
            return buffer.write(static_cast<std::uint32_t>(value_));
        }
        if (auto r = buffer.write(std::uint8_t{255}); !r) return r;
        return buffer.write(value_);
    }
    
    [[nodiscard]] static std::expected<CompactSize, Error>
    deserialize(Buffer& buffer) noexcept {
        auto first = buffer.read<std::uint8_t>();
        if (!first) return std::unexpected(first.error());
        
        if (*first < 253) {
            return CompactSize{*first};
        }
        
        std::uint64_t value;
        if (*first == 253) {
            auto v = buffer.read<std::uint16_t>();
            if (!v) return std::unexpected(v.error());
            value = *v;
        } else if (*first == 254) {
            auto v = buffer.read<std::uint32_t>();
            if (!v) return std::unexpected(v.error());
            value = *v;
        } else {
            auto v = buffer.read<std::uint64_t>();
            if (!v) return std::unexpected(v.error());
            value = *v;
        }
        
        if (value > MAX_SIZE) {
            return std::unexpected(Error::size_too_large);
        }
        
        return CompactSize{value};
    }
    
private:
    std::uint64_t value_;
};

// Serialization traits for compile-time dispatch
template<typename T>
struct Serializer {
    [[nodiscard]] static std::expected<void, Error> 
    serialize(Buffer& buffer, const T& value) noexcept = delete;
    
    [[nodiscard]] static std::expected<T, Error>
    deserialize(Buffer& buffer) noexcept = delete;
};

// Specialization for primitives
template<Primitive T>
struct Serializer<T> {
    [[nodiscard]] static std::expected<void, Error> 
    serialize(Buffer& buffer, const T& value) noexcept {
        return buffer.write(value);
    }
    
    [[nodiscard]] static std::expected<T, Error>
    deserialize(Buffer& buffer) noexcept {
        return buffer.read<T>();
    }
};

// Specialization for strings
template<>
struct Serializer<std::string> {
    [[nodiscard]] static std::expected<void, Error>
    serialize(Buffer& buffer, const std::string& str) noexcept {
        CompactSize size{str.size()};
        if (auto r = size.serialize(buffer); !r) return r;
        return buffer.write_bytes(std::as_bytes(std::span{str}))
            .transform([](auto) { return std::monostate{}; });
    }
    
    [[nodiscard]] static std::expected<std::string, Error>
    deserialize(Buffer& buffer) noexcept {
        auto size = CompactSize::deserialize(buffer);
        if (!size) return std::unexpected(size.error());
        
        auto bytes = buffer.read_bytes(size->value());
        if (!bytes) return std::unexpected(bytes.error());
        
        return std::string{
            reinterpret_cast<const char*>(bytes->data()), 
            bytes->size()
        };
    }
};

// Specialization for vectors
template<typename T>
struct Serializer<std::vector<T>> {
    [[nodiscard]] static std::expected<void, Error>
    serialize(Buffer& buffer, const std::vector<T>& vec) noexcept {
        CompactSize size{vec.size()};
        if (auto r = size.serialize(buffer); !r) return r;
        
        for (const auto& item : vec) {
            if (auto r = Serializer<T>::serialize(buffer, item); !r) return r;
        }
        return {};
    }
    
    [[nodiscard]] static std::expected<std::vector<T>, Error>
    deserialize(Buffer& buffer) noexcept {
        auto size = CompactSize::deserialize(buffer);
        if (!size) return std::unexpected(size.error());
        
        std::vector<T> vec;
        vec.reserve(size->value());
        
        for (std::size_t i = 0; i < size->value(); ++i) {
            auto item = Serializer<T>::deserialize(buffer);
            if (!item) return std::unexpected(item.error());
            vec.push_back(std::move(*item));
        }
        
        return vec;
    }
};

// Serialization for hash types
template<std::size_t N>
struct Serializer<std::array<byte_t, N>> {
    [[nodiscard]] static std::expected<void, Error>
    serialize(Buffer& buffer, const std::array<byte_t, N>& arr) noexcept {
        return buffer.write_bytes(arr)
            .transform([](auto) { return std::monostate{}; });
    }
    
    [[nodiscard]] static std::expected<std::array<byte_t, N>, Error>
    deserialize(Buffer& buffer) noexcept {
        auto bytes = buffer.read_bytes(N);
        if (!bytes) return std::unexpected(bytes.error());
        
        std::array<byte_t, N> arr;
        std::ranges::copy(*bytes, arr.begin());
        return arr;
    }
};

// Helper functions for cleaner API
template<typename T>
[[nodiscard]] std::expected<std::vector<byte_t>, Error> 
to_bytes(const T& value) noexcept {
    std::vector<byte_t> bytes(1024); // Start with reasonable size
    Buffer buffer{bytes};
    
    if (auto r = Serializer<T>::serialize(buffer, value); !r) {
        return std::unexpected(r.error());
    }
    
    bytes.resize(buffer.position());
    return bytes;
}

template<typename T>
[[nodiscard]] std::expected<T, Error>
from_bytes(std::span<const byte_t> bytes) noexcept {
    std::vector<byte_t> mutable_bytes{bytes.begin(), bytes.end()};
    Buffer buffer{mutable_bytes};
    return Serializer<T>::deserialize(buffer);
}

} // namespace bitcoin::serialize