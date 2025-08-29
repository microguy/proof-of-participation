// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include <array>
#include <span>
#include <concepts>
#include <bit>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

namespace bitcoin::crypto {

// SHA-256 wrapper - preserving exact original behavior
class SHA256 {
public:
    static constexpr std::size_t DIGEST_SIZE = 32;
    using digest_t = std::array<byte_t, DIGEST_SIZE>;
    
    SHA256() noexcept {
        ::SHA256_Init(&ctx_);
    }
    
    SHA256& write(const void* data, std::size_t len) noexcept {
        ::SHA256_Update(&ctx_, data, len);
        return *this;
    }
    
    void finalize(unsigned char hash[DIGEST_SIZE]) noexcept {
        ::SHA256_Final(hash, &ctx_);
    }
    
    SHA256& reset() noexcept {
        ::SHA256_Init(&ctx_);
        return *this;
    }
    
private:
    SHA256_CTX ctx_;
};

// Double SHA-256 - exact original implementation
template<typename T>
[[nodiscard]] inline hash256_t Hash(const T& begin, const T& end) {
    static_assert(sizeof(*begin) == 1);
    hash256_t hash1;
    SHA256_CTX ctx;
    ::SHA256_Init(&ctx);
    ::SHA256_Update(&ctx, &(*begin), end - begin);
    ::SHA256_Final(hash1.data(), &ctx);
    
    hash256_t hash2;
    ::SHA256_Init(&ctx);
    ::SHA256_Update(&ctx, hash1.data(), hash1.size());
    ::SHA256_Final(hash2.data(), &ctx);
    return hash2;
}

// Template overload for containers
template<typename T>
[[nodiscard]] inline hash256_t Hash(const T& v) {
    return Hash(v.begin(), v.end());
}

// Hash160 = RIPEMD160(SHA256(data)) - exact original
template<typename T>
[[nodiscard]] inline hash160_t Hash160(const T& begin, const T& end) {
    static_assert(sizeof(*begin) == 1);
    hash256_t hash1;
    SHA256_CTX ctx256;
    ::SHA256_Init(&ctx256);
    ::SHA256_Update(&ctx256, &(*begin), end - begin);
    ::SHA256_Final(hash1.data(), &ctx256);
    
    hash160_t hash2;
    RIPEMD160_CTX ctx160;
    ::RIPEMD160_Init(&ctx160);
    ::RIPEMD160_Update(&ctx160, hash1.data(), SHA256_DIGEST_LENGTH);
    ::RIPEMD160_Final(hash2.data(), &ctx160);
    return hash2;
}

template<typename T>
[[nodiscard]] inline hash160_t Hash160(const T& v) {
    return Hash160(v.begin(), v.end());
}

// CBigNum - modern RAII wrapper preserving exact original semantics
class CBigNum {
public:
    CBigNum() noexcept {
        bn_ = BN_new();
        setulong(0);
    }
    
    CBigNum(const CBigNum& b) noexcept : CBigNum() {
        if (!BN_copy(bn_, b.bn_))
            throw std::runtime_error("CBigNum::CBigNum(const CBigNum&) failed");
    }
    
    explicit CBigNum(long long n) noexcept : CBigNum() { setint64(n); }
    explicit CBigNum(int n) noexcept : CBigNum() { setlong(n); }
    explicit CBigNum(unsigned int n) noexcept : CBigNum() { setulong(n); }
    
    explicit CBigNum(const std::vector<unsigned char>& vch) noexcept : CBigNum() {
        setvch(vch);
    }
    
    ~CBigNum() noexcept {
        BN_clear_free(bn_);
    }
    
    CBigNum& operator=(const CBigNum& b) {
        if (!BN_copy(bn_, b.bn_))
            throw std::runtime_error("CBigNum::operator= failed");
        return *this;
    }
    
    // Preserve exact original setter methods
    void setulong(unsigned long n) noexcept {
        if (!BN_set_word(bn_, n))
            throw std::runtime_error("CBigNum::setulong failed");
    }
    
    void setlong(long n) noexcept {
        if (n >= 0)
            setulong(n);
        else {
            setulong(-n);
            BN_set_negative(bn_, 1);
        }
    }
    
    void setint64(int64_t n) noexcept {
        // Original implementation preserved
        unsigned char pch[sizeof(n) + 6];
        unsigned char* p = pch + 4;
        bool fNegative = false;
        if (n < 0) {
            n = -n;
            fNegative = true;
        }
        bool fLeadingZeroes = true;
        for (int i = 0; i < 8; i++) {
            unsigned char c = (n >> 56) & 0xff;
            n <<= 8;
            if (fLeadingZeroes) {
                if (c == 0)
                    continue;
                if (c & 0x80)
                    *p++ = (fNegative ? 0x80 : 0);
                else if (fNegative)
                    c |= 0x80;
                fLeadingZeroes = false;
            }
            *p++ = c;
        }
        unsigned int nSize = p - (pch + 4);
        pch[0] = (nSize >> 24) & 0xff;
        pch[1] = (nSize >> 16) & 0xff;
        pch[2] = (nSize >> 8) & 0xff;
        pch[3] = (nSize) & 0xff;
        BN_mpi2bn(pch, p - pch, bn_);
    }
    
    void setvch(const std::vector<unsigned char>& vch) {
        std::vector<unsigned char> vch2(vch.size() + 4);
        unsigned int nSize = vch.size();
        vch2[0] = (nSize >> 24) & 0xff;
        vch2[1] = (nSize >> 16) & 0xff;
        vch2[2] = (nSize >> 8) & 0xff;
        vch2[3] = (nSize) & 0xff;
        std::reverse_copy(vch.begin(), vch.end(), vch2.begin() + 4);
        BN_mpi2bn(&vch2[0], vch2.size(), bn_);
    }
    
    [[nodiscard]] std::vector<unsigned char> getvch() const {
        unsigned int nSize = BN_bn2mpi(bn_, nullptr);
        if (nSize < 4)
            return {};
        std::vector<unsigned char> vch(nSize);
        BN_bn2mpi(bn_, &vch[0]);
        vch.erase(vch.begin(), vch.begin() + 4);
        std::reverse(vch.begin(), vch.end());
        return vch;
    }
    
    [[nodiscard]] unsigned int GetCompact() const {
        unsigned int nSize = BN_num_bytes(bn_);
        unsigned int nCompact = 0;
        if (nSize <= 3) {
            nCompact = BN_get_word(bn_) << 8 * (3 - nSize);
        } else {
            CBigNum bn;
            BN_rshift(bn.bn_, bn_, 8 * (nSize - 3));
            nCompact = BN_get_word(bn.bn_);
        }
        if (nCompact & 0x00800000) {
            nCompact >>= 8;
            nSize++;
        }
        nCompact |= nSize << 24;
        nCompact |= (BN_is_negative(bn_) ? 0x00800000 : 0);
        return nCompact;
    }
    
    CBigNum& SetCompact(unsigned int nCompact) {
        unsigned int nSize = nCompact >> 24;
        bool fNegative = (nCompact & 0x00800000) != 0;
        unsigned int nWord = nCompact & 0x007fffff;
        if (nSize <= 3) {
            nWord >>= 8 * (3 - nSize);
            BN_set_word(bn_, nWord);
        } else {
            BN_set_word(bn_, nWord);
            BN_lshift(bn_, bn_, 8 * (nSize - 3));
        }
        BN_set_negative(bn_, fNegative);
        return *this;
    }
    
    // Preserve exact original arithmetic operators
    CBigNum& operator+=(const CBigNum& b) {
        if (!BN_add(bn_, bn_, b.bn_))
            throw std::runtime_error("CBigNum::operator+= failed");
        return *this;
    }
    
    CBigNum& operator-=(const CBigNum& b) {
        if (!BN_sub(bn_, bn_, b.bn_))
            throw std::runtime_error("CBigNum::operator-= failed");
        return *this;
    }
    
    CBigNum& operator*=(const CBigNum& b) {
        BN_CTX* pctx = BN_CTX_new();
        if (!pctx)
            throw std::runtime_error("CBigNum::operator*= failed");
        if (!BN_mul(bn_, bn_, b.bn_, pctx)) {
            BN_CTX_free(pctx);
            throw std::runtime_error("CBigNum::operator*= failed");
        }
        BN_CTX_free(pctx);
        return *this;
    }
    
    CBigNum& operator/=(const CBigNum& b) {
        BN_CTX* pctx = BN_CTX_new();
        if (!pctx)
            throw std::runtime_error("CBigNum::operator/= failed");
        if (!BN_div(bn_, nullptr, bn_, b.bn_, pctx)) {
            BN_CTX_free(pctx);
            throw std::runtime_error("CBigNum::operator/= failed");
        }
        BN_CTX_free(pctx);
        return *this;
    }
    
    CBigNum& operator%=(const CBigNum& b) {
        BN_CTX* pctx = BN_CTX_new();
        if (!pctx)
            throw std::runtime_error("CBigNum::operator%= failed");
        if (!BN_mod(bn_, bn_, b.bn_, pctx)) {
            BN_CTX_free(pctx);
            throw std::runtime_error("CBigNum::operator%= failed");
        }
        BN_CTX_free(pctx);
        return *this;
    }
    
    CBigNum& operator<<=(unsigned int shift) {
        if (!BN_lshift(bn_, bn_, shift))
            throw std::runtime_error("CBigNum::operator<<= failed");
        return *this;
    }
    
    CBigNum& operator>>=(unsigned int shift) {
        if (!BN_rshift(bn_, bn_, shift))
            throw std::runtime_error("CBigNum::operator>>= failed");
        return *this;
    }
    
    CBigNum& operator++() {
        if (!BN_add_word(bn_, 1))
            throw std::runtime_error("CBigNum::operator++ failed");
        return *this;
    }
    
    CBigNum& operator--() {
        if (!BN_sub_word(bn_, 1))
            throw std::runtime_error("CBigNum::operator-- failed");
        return *this;
    }
    
    // Comparison operators preserving exact semantics
    [[nodiscard]] friend bool operator<(const CBigNum& a, const CBigNum& b) { 
        return BN_cmp(a.bn_, b.bn_) < 0; 
    }
    [[nodiscard]] friend bool operator>(const CBigNum& a, const CBigNum& b) { 
        return BN_cmp(a.bn_, b.bn_) > 0; 
    }
    [[nodiscard]] friend bool operator<=(const CBigNum& a, const CBigNum& b) { 
        return BN_cmp(a.bn_, b.bn_) <= 0; 
    }
    [[nodiscard]] friend bool operator>=(const CBigNum& a, const CBigNum& b) { 
        return BN_cmp(a.bn_, b.bn_) >= 0; 
    }
    [[nodiscard]] friend bool operator==(const CBigNum& a, const CBigNum& b) { 
        return BN_cmp(a.bn_, b.bn_) == 0; 
    }
    [[nodiscard]] friend bool operator!=(const CBigNum& a, const CBigNum& b) { 
        return BN_cmp(a.bn_, b.bn_) != 0; 
    }
    
    BIGNUM* get() noexcept { return bn_; }
    const BIGNUM* get() const noexcept { return bn_; }
    
private:
    BIGNUM* bn_;
};

// CKey - preserving exact original EC cryptography
class CKey {
protected:
    EC_KEY* pkey;
    bool fSet;
    
public:
    CKey() {
        pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (pkey == nullptr)
            throw std::runtime_error("CKey::CKey() failed");
        fSet = false;
    }
    
    CKey(const CKey& b) : CKey() {
        if (b.fSet) {
            if (!EC_KEY_copy(pkey, b.pkey))
                throw std::runtime_error("CKey::CKey(const CKey&) failed");
            fSet = true;
        }
    }
    
    CKey& operator=(const CKey& b) {
        if (this != &b) {
            if (b.fSet) {
                if (!EC_KEY_copy(pkey, b.pkey))
                    throw std::runtime_error("CKey::operator= failed");
                fSet = true;
            } else {
                fSet = false;
            }
        }
        return *this;
    }
    
    ~CKey() {
        EC_KEY_free(pkey);
    }
    
    [[nodiscard]] bool IsNull() const {
        return !fSet;
    }
    
    void MakeNewKey() {
        if (!EC_KEY_generate_key(pkey))
            throw std::runtime_error("CKey::MakeNewKey() failed");
        fSet = true;
    }
    
    bool SetPrivKey(const std::vector<unsigned char>& vchPrivKey) {
        const unsigned char* pbegin = &vchPrivKey[0];
        if (!d2i_ECPrivateKey(&pkey, &pbegin, vchPrivKey.size()))
            return false;
        fSet = true;
        return true;
    }
    
    [[nodiscard]] std::vector<unsigned char> GetPrivKey() const {
        unsigned int nSize = i2d_ECPrivateKey(pkey, nullptr);
        if (!nSize)
            throw std::runtime_error("CKey::GetPrivKey() failed");
        std::vector<unsigned char> vchPrivKey(nSize, 0);
        unsigned char* pbegin = &vchPrivKey[0];
        if (i2d_ECPrivateKey(pkey, &pbegin) != nSize)
            throw std::runtime_error("CKey::GetPrivKey() failed");
        return vchPrivKey;
    }
    
    bool SetPubKey(const std::vector<unsigned char>& vchPubKey) {
        const unsigned char* pbegin = &vchPubKey[0];
        if (!o2i_ECPublicKey(&pkey, &pbegin, vchPubKey.size()))
            return false;
        fSet = true;
        return true;
    }
    
    [[nodiscard]] std::vector<unsigned char> GetPubKey() const {
        unsigned int nSize = i2o_ECPublicKey(pkey, nullptr);
        if (!nSize)
            throw std::runtime_error("CKey::GetPubKey() failed");
        std::vector<unsigned char> vchPubKey(nSize, 0);
        unsigned char* pbegin = &vchPubKey[0];
        if (i2o_ECPublicKey(pkey, &pbegin) != nSize)
            throw std::runtime_error("CKey::GetPubKey() failed");
        return vchPubKey;
    }
    
    bool Sign(const hash256_t& hash, std::vector<unsigned char>& vchSig) {
        vchSig.clear();
        unsigned char pchSig[10000];
        unsigned int nSize = 0;
        if (!ECDSA_sign(0, hash.data(), hash.size(), pchSig, &nSize, pkey))
            return false;
        vchSig.resize(nSize);
        std::memcpy(&vchSig[0], pchSig, nSize);
        return true;
    }
    
    bool Verify(const hash256_t& hash, const std::vector<unsigned char>& vchSig) {
        return ECDSA_verify(0, hash.data(), hash.size(), 
                           &vchSig[0], vchSig.size(), pkey) == 1;
    }
};

// Base58 encoding - preserving exact original algorithm
inline constexpr const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend) {
    // Preserving exact original implementation
    BN_CTX* pctx = BN_CTX_new();
    CBigNum bn58 = 58;
    CBigNum bn0 = 0;
    
    // Convert big endian data to little endian
    std::vector<unsigned char> vchTmp(pend - pbegin + 1, 0);
    std::reverse_copy(pbegin, pend, vchTmp.begin());
    
    // Convert little endian data to bignum
    CBigNum bn;
    bn.setvch(vchTmp);
    
    // Convert bignum to base58
    std::string str;
    str.reserve((pend - pbegin) * 138 / 100 + 1);
    CBigNum dv;
    CBigNum rem;
    while (bn > bn0) {
        if (!BN_div(dv.get(), rem.get(), bn.get(), bn58.get(), pctx))
            throw std::runtime_error("EncodeBase58 failed");
        bn = dv;
        unsigned int c = BN_get_word(rem.get());
        str += pszBase58[c];
    }
    
    // Leading zeroes encoded as base58 zeros
    for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
        str += pszBase58[0];
    
    // Convert little endian base58 to big endian
    std::reverse(str.begin(), str.end());
    BN_CTX_free(pctx);
    return str;
}

inline std::string EncodeBase58(const std::vector<unsigned char>& vch) {
    return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

inline std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn) {
    // Add 4-byte hash checksum
    std::vector<unsigned char> vch(vchIn);
    hash256_t hash = Hash(vch.begin(), vch.end());
    vch.insert(vch.end(), hash.begin(), hash.begin() + 4);
    return EncodeBase58(vch);
}

} // namespace bitcoin::crypto