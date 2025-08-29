// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "serialize_modern.h"
#include <vector>
#include <stack>
#include <variant>
#include <expected>
#include <ranges>
#include <algorithm>

namespace bitcoin {

// Script opcodes - preserving exact original values
enum class opcodetype : std::uint8_t {
    // push value
    OP_0 = 0x00,
    OP_FALSE = OP_0,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2 = 0x4d,
    OP_PUSHDATA4 = 0x4e,
    OP_1NEGATE = 0x4f,
    OP_RESERVED = 0x50,
    OP_1 = 0x51,
    OP_TRUE = OP_1,
    OP_2 = 0x52,
    OP_3 = 0x53,
    OP_4 = 0x54,
    OP_5 = 0x55,
    OP_6 = 0x56,
    OP_7 = 0x57,
    OP_8 = 0x58,
    OP_9 = 0x59,
    OP_10 = 0x5a,
    OP_11 = 0x5b,
    OP_12 = 0x5c,
    OP_13 = 0x5d,
    OP_14 = 0x5e,
    OP_15 = 0x5f,
    OP_16 = 0x60,

    // control
    OP_NOP = 0x61,
    OP_VER = 0x62,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_VERIF = 0x65,
    OP_VERNOTIF = 0x66,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
    OP_RETURN = 0x6a,

    // stack ops
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_2DROP = 0x6d,
    OP_2DUP = 0x6e,
    OP_3DUP = 0x6f,
    OP_2OVER = 0x70,
    OP_2ROT = 0x71,
    OP_2SWAP = 0x72,
    OP_IFDUP = 0x73,
    OP_DEPTH = 0x74,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_NIP = 0x77,
    OP_OVER = 0x78,
    OP_PICK = 0x79,
    OP_ROLL = 0x7a,
    OP_ROT = 0x7b,
    OP_SWAP = 0x7c,
    OP_TUCK = 0x7d,

    // splice ops
    OP_CAT = 0x7e,
    OP_SUBSTR = 0x7f,
    OP_LEFT = 0x80,
    OP_RIGHT = 0x81,
    OP_SIZE = 0x82,

    // bit logic
    OP_INVERT = 0x83,
    OP_AND = 0x84,
    OP_OR = 0x85,
    OP_XOR = 0x86,
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,
    OP_RESERVED1 = 0x89,
    OP_RESERVED2 = 0x8a,

    // numeric
    OP_1ADD = 0x8b,
    OP_1SUB = 0x8c,
    OP_2MUL = 0x8d,
    OP_2DIV = 0x8e,
    OP_NEGATE = 0x8f,
    OP_ABS = 0x90,
    OP_NOT = 0x91,
    OP_0NOTEQUAL = 0x92,

    OP_ADD = 0x93,
    OP_SUB = 0x94,
    OP_MUL = 0x95,
    OP_DIV = 0x96,
    OP_MOD = 0x97,
    OP_LSHIFT = 0x98,
    OP_RSHIFT = 0x99,

    OP_BOOLAND = 0x9a,
    OP_BOOLOR = 0x9b,
    OP_NUMEQUAL = 0x9c,
    OP_NUMEQUALVERIFY = 0x9d,
    OP_NUMNOTEQUAL = 0x9e,
    OP_LESSTHAN = 0x9f,
    OP_GREATERTHAN = 0xa0,
    OP_LESSTHANOREQUAL = 0xa1,
    OP_GREATERTHANOREQUAL = 0xa2,
    OP_MIN = 0xa3,
    OP_MAX = 0xa4,

    OP_WITHIN = 0xa5,

    // crypto
    OP_RIPEMD160 = 0xa6,
    OP_SHA1 = 0xa7,
    OP_SHA256 = 0xa8,
    OP_HASH160 = 0xa9,
    OP_HASH256 = 0xaa,
    OP_CODESEPARATOR = 0xab,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,
    OP_CHECKMULTISIG = 0xae,
    OP_CHECKMULTISIGVERIFY = 0xaf,

    // expansion
    OP_NOP1 = 0xb0,
    OP_NOP2 = 0xb1,
    OP_NOP3 = 0xb2,
    OP_NOP4 = 0xb3,
    OP_NOP5 = 0xb4,
    OP_NOP6 = 0xb5,
    OP_NOP7 = 0xb6,
    OP_NOP8 = 0xb7,
    OP_NOP9 = 0xb8,
    OP_NOP10 = 0xb9,

    // template matching params
    OP_PUBKEYHASH = 0xfd,
    OP_PUBKEY = 0xfe,

    OP_INVALIDOPCODE = 0xff,
};

// Modern Script class using ranges and concepts
class Script {
public:
    using container = std::vector<byte_t>;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    
    Script() noexcept = default;
    
    explicit Script(std::span<const byte_t> data) 
        : data_(data.begin(), data.end()) {}
    
    explicit Script(container data) noexcept 
        : data_(std::move(data)) {}
    
    // Preserve exact original push operations
    Script& operator<<(std::int64_t n) {
        if (n == -1 || (n >= 1 && n <= 16)) {
            push_opcode(n == -1 || n == 0 ? opcodetype(n + static_cast<int>(opcodetype::OP_1) - 1)
                                           : opcodetype(static_cast<int>(opcodetype::OP_1) + n - 1));
        } else {
            *this << to_bignum(n);
        }
        return *this;
    }
    
    Script& operator<<(opcodetype opcode) {
        push_opcode(opcode);
        return *this;
    }
    
    Script& operator<<(std::span<const byte_t> data) {
        if (data.size() <= static_cast<std::size_t>(opcodetype::OP_PUSHDATA1)) {
            data_.push_back(static_cast<byte_t>(data.size()));
        } else if (data.size() <= 0xff) {
            data_.push_back(static_cast<byte_t>(opcodetype::OP_PUSHDATA1));
            data_.push_back(static_cast<byte_t>(data.size()));
        } else if (data.size() <= 0xffff) {
            data_.push_back(static_cast<byte_t>(opcodetype::OP_PUSHDATA2));
            auto size = static_cast<std::uint16_t>(data.size());
            data_.push_back(size & 0xff);
            data_.push_back((size >> 8) & 0xff);
        } else {
            data_.push_back(static_cast<byte_t>(opcodetype::OP_PUSHDATA4));
            auto size = static_cast<std::uint32_t>(data.size());
            data_.push_back(size & 0xff);
            data_.push_back((size >> 8) & 0xff);
            data_.push_back((size >> 16) & 0xff);
            data_.push_back((size >> 24) & 0xff);
        }
        data_.insert(data_.end(), data.begin(), data.end());
        return *this;
    }
    
    [[nodiscard]] bool get_op(const_iterator& pc, opcodetype& opcodeRet, 
                              std::vector<byte_t>& vchRet) const {
        opcodeRet = opcodetype::OP_INVALIDOPCODE;
        vchRet.clear();
        if (pc >= data_.end())
            return false;

        // Read instruction
        if (pc >= data_.end())
            return false;
        auto opcode = static_cast<opcodetype>(*pc++);

        // Immediate operand
        if (opcode <= opcodetype::OP_PUSHDATA4) {
            std::size_t nSize = 0;
            if (opcode < opcodetype::OP_PUSHDATA1) {
                nSize = static_cast<std::size_t>(opcode);
            } else if (opcode == opcodetype::OP_PUSHDATA1) {
                if (pc >= data_.end())
                    return false;
                nSize = *pc++;
            } else if (opcode == opcodetype::OP_PUSHDATA2) {
                if (std::distance(pc, data_.end()) < 2)
                    return false;
                nSize = *pc++;
                nSize |= static_cast<std::size_t>(*pc++) << 8;
            } else if (opcode == opcodetype::OP_PUSHDATA4) {
                if (std::distance(pc, data_.end()) < 4)
                    return false;
                nSize = *pc++;
                nSize |= static_cast<std::size_t>(*pc++) << 8;
                nSize |= static_cast<std::size_t>(*pc++) << 16;
                nSize |= static_cast<std::size_t>(*pc++) << 24;
            }
            if (std::distance(pc, data_.end()) < static_cast<std::ptrdiff_t>(nSize))
                return false;
            vchRet.assign(pc, pc + nSize);
            pc += nSize;
        }

        opcodeRet = opcode;
        return true;
    }
    
    [[nodiscard]] bool is_pay_to_script_hash() const noexcept {
        // Exact pattern: OP_HASH160 <20 bytes> OP_EQUAL
        return data_.size() == 23 &&
               data_[0] == static_cast<byte_t>(opcodetype::OP_HASH160) &&
               data_[1] == 0x14 &&
               data_[22] == static_cast<byte_t>(opcodetype::OP_EQUAL);
    }
    
    [[nodiscard]] bool is_push_only() const noexcept {
        auto pc = data_.begin();
        while (pc < data_.end()) {
            opcodetype opcode;
            std::vector<byte_t> data;
            if (!get_op(pc, opcode, data))
                return false;
            if (opcode > opcodetype::OP_16)
                return false;
        }
        return true;
    }
    
    [[nodiscard]] std::size_tragic size() const noexcept { return data_.size(); }
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
    [[nodiscard]] const byte_t* data() const noexcept { return data_.data(); }
    [[nodiscard]] auto begin() const noexcept { return data_.begin(); }
    [[nodiscard]] auto end() const noexcept { return data_.end(); }
    
    void clear() noexcept { data_.clear(); }
    
    [[nodiscard]] std::expected<void, serialize::Error>
    serialize(serialize::Buffer& buffer) const noexcept {
        return serialize::Serializer<std::vector<byte_t>>::serialize(buffer, data_);
    }
    
    [[nodiscard]] static std::expected<Script, serialize::Error>
    deserialize(serialize::Buffer& buffer) noexcept {
        auto data = serialize::Serializer<std::vector<byte_t>>::deserialize(buffer);
        if (!data) return std::unexpected(data.error());
        return Script{std::move(*data)};
    }
    
    auto operator<=>(const Script&) const = default;
    
private:
    void push_opcode(opcodetype opcode) {
        data_.push_back(static_cast<byte_t>(opcode));
    }
    
    [[nodiscard]] static std::vector<byte_t> to_bignum(std::int64_t n) {
        std::vector<byte_t> result;
        if (n == 0) return result;
        
        const bool neg = n < 0;
        std::uint64_t absvalue = neg ? -n : n;
        
        while (absvalue) {
            result.push_back(absvalue & 0xff);
            absvalue >>= 8;
        }
        
        if (result.back() & 0x80) {
            result.push_back(neg ? 0x80 : 0);
        } else if (neg) {
            result.back() |= 0x80;
        }
        
        return result;
    }
    
    container data_;
};

// Script verification flags - preserving original behavior
enum class ScriptFlags : std::uint32_t {
    NONE = 0,
    VERIFY_P2SH = (1U << 0),
    VERIFY_STRICTENC = (1U << 1),
    VERIFY_DERSIG = (1U << 2),
    VERIFY_LOW_S = (1U << 3),
    VERIFY_NULLDUMMY = (1U << 4),
    VERIFY_SIGPUSHONLY = (1U << 5),
    VERIFY_MINIMALDATA = (1U << 6),
    VERIFY_DISCOURAGE_UPGRADABLE_NOPS = (1U << 7),
    VERIFY_CLEANSTACK = (1U << 8),
};

// Modern signature hash types
enum class SigHashType : std::uint32_t {
    ALL = 1,
    NONE = 2,
    SINGLE = 3,
    ANYONECANPAY = 0x80,
};

[[nodiscard]] constexpr SigHashType operator|(SigHashType a, SigHashType b) noexcept {
    return static_cast<SigHashType>(
        static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b)
    );
}

[[nodiscard]] constexpr bool has_flag(SigHashType value, SigHashType flag) noexcept {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}

} // namespace bitcoin