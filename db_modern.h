// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)  
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "serialize_modern.h"
#include "core.h"
#include <db_cxx.h>
#include <memory>
#include <filesystem>
#include <expected>
#include <ranges>
#include <mutex>
#include <shared_mutex>

namespace bitcoin::db {

// Forward declarations
class CTxDB;
class CWalletDB;
class CAddrDB;

// Database error codes
enum class DBError {
    ok = 0,
    not_found,
    corrupted,
    io_error,
    version_mismatch,
    locked,
    insufficient_space
};

// Global database environment - preserving original singleton pattern
class CDBEnv {
public:
    static CDBEnv& instance() {
        static CDBEnv env;
        return env;
    }
    
    CDBEnv(const CDBEnv&) = delete;
    CDBEnv& operator=(const CDBEnv&) = delete;
    
    bool Open(const std::filesystem::path& pathDataDir);
    void Close();
    void Flush(bool fShutdown);
    void CheckpointLSN(std::string_view strFile);
    void CloseDb(const std::string& strFile);
    
    DbEnv* get() noexcept { return dbenv.get(); }
    
private:
    CDBEnv() = default;
    ~CDBEnv() { Close(); }
    
    std::unique_ptr<DbEnv> dbenv;
    std::filesystem::path pathDataDir;
    std::map<std::string, Db*, std::less<>> mapDb;
    std::map<std::string, int, std::less<>> mapFileUseCount;
    mutable std::shared_mutex cs_db;
};

// Modern database wrapper preserving exact original behavior
class CDB {
protected:
    Db* pdb;
    std::string strFile;
    std::vector<DbTxn*> vTxn;
    bool fReadOnly;
    
    explicit CDB(const char* pszFile, const char* pszMode = "r+");
    ~CDB();
    
public:
    CDB(const CDB&) = delete;
    CDB& operator=(const CDB&) = delete;
    
    void Close();
    
protected:
    template<typename K>
    bool Read(const K& key, std::string& value) {
        if (!pdb)
            return false;
        
        // Serialize key
        CDataStream ssKey(SER_DISK, serialize::VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        
        // Read from database
        Dbt datKey(ssKey.data(), ssKey.size());
        Dbt datValue;
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pdb->get(GetTxn(), &datKey, &datValue, 0);
        
        if (ret == 0) {
            value.assign(static_cast<char*>(datValue.get_data()), 
                        datValue.get_size());
            free(datValue.get_data());
            return true;
        }
        return false;
    }
    
    template<typename K, typename T>
    bool Read(const K& key, T& value) {
        std::string strValue;
        if (!Read(key, strValue))
            return false;
        
        // Deserialize value
        try {
            CDataStream ssValue(strValue, SER_DISK, serialize::VERSION);
            ssValue >> value;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    template<typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true) {
        if (!pdb)
            return false;
        if (fReadOnly) {
            assert(!"Write called on database in read-only mode");
            return false;
        }
        
        // Serialize key
        CDataStream ssKey(SER_DISK, serialize::VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        
        // Serialize value  
        CDataStream ssValue(SER_DISK, serialize::VERSION);
        ssValue.reserve(10000);
        ssValue << value;
        
        // Write to database
        Dbt datKey(ssKey.data(), ssKey.size());
        Dbt datValue(ssValue.data(), ssValue.size());
        
        int ret = pdb->put(GetTxn(), &datKey, &datValue, 
                          fOverwrite ? 0 : DB_NOOVERWRITE);
        return (ret == 0);
    }
    
    template<typename K>
    bool Erase(const K& key) {
        if (!pdb)
            return false;
        if (fReadOnly) {
            assert(!"Erase called on database in read-only mode");
            return false;
        }
        
        // Serialize key
        CDataStream ssKey(SER_DISK, serialize::VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        
        // Erase from database
        Dbt datKey(ssKey.data(), ssKey.size());
        int ret = pdb->del(GetTxn(), &datKey, 0);
        return (ret == 0 || ret == DB_NOTFOUND);
    }
    
    template<typename K>
    bool Exists(const K& key) {
        if (!pdb)
            return false;
        
        // Serialize key
        CDataStream ssKey(SER_DISK, serialize::VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        
        // Check existence
        Dbt datKey(ssKey.data(), ssKey.size());
        int ret = pdb->exists(GetTxn(), &datKey, 0);
        return (ret == 0);
    }
    
    bool TxnBegin() {
        if (!pdb)
            return false;
        DbTxn* ptxn = nullptr;
        int ret = CDBEnv::instance().get()->txn_begin(GetTxn(), &ptxn, 0);
        if (ret != 0)
            return false;
        vTxn.push_back(ptxn);
        return true;
    }
    
    bool TxnCommit() {
        if (!pdb)
            return false;
        if (vTxn.empty())
            return false;
        int ret = vTxn.back()->commit(0);
        vTxn.pop_back();
        return (ret == 0);
    }
    
    bool TxnAbort() {
        if (!pdb)
            return false;
        if (vTxn.empty())
            return false;
        int ret = vTxn.back()->abort();
        vTxn.pop_back();
        return (ret == 0);
    }
    
    DbTxn* GetTxn() {
        return vTxn.empty() ? nullptr : vTxn.back();
    }
};

// Transaction database - preserving exact original schema
class CTxDB : public CDB {
public:
    CTxDB(const char* pszMode = "r+") : CDB("blkindex.dat", pszMode) {}
    
    // Core transaction operations
    bool ReadTxIndex(const hash256_t& hash, core::Transaction& tx) {
        return Read(std::make_pair(std::string("tx"), hash), tx);
    }
    
    bool WriteTxIndex(const core::Transaction& tx) {
        return Write(std::make_pair(std::string("tx"), tx.hash()), tx);
    }
    
    bool EraseTxIndex(const hash256_t& hash) {
        return Erase(std::make_pair(std::string("tx"), hash));
    }
    
    // Block index operations
    bool ReadBlockIndex(const hash256_t& hash, core::BlockHeader& block) {
        return Read(std::make_pair(std::string("blockindex"), hash), block);
    }
    
    bool WriteBlockIndex(const core::BlockHeader& block) {
        return Write(std::make_pair(std::string("blockindex"), block.hash()), block);
    }
    
    bool EraseBlockIndex(const hash256_t& hash) {
        return Erase(std::make_pair(std::string("blockindex"), hash));
    }
    
    // Best chain tracking
    bool ReadHashBestChain(hash256_t& hashBestChain) {
        return Read(std::string("hashBestChain"), hashBestChain);
    }
    
    bool WriteHashBestChain(const hash256_t& hashBestChain) {
        return Write(std::string("hashBestChain"), hashBestChain);
    }
    
    // Version checking
    bool ReadVersion(int& nVersion) {
        return Read(std::string("version"), nVersion);
    }
    
    bool WriteVersion(int nVersion) {
        return Write(std::string("version"), nVersion);
    }
};

// Wallet database structure
struct CWalletKey {
    std::vector<unsigned char> vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;
    
    CWalletKey(int64_t nExpires = 0) {
        nTimeCreated = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        nTimeExpires = nExpires;
    }
};

// Wallet database - preserving exact original schema  
class CWalletDB : public CDB {
public:
    CWalletDB(const char* pszMode = "r+") : CDB("wallet.dat", pszMode) {}
    
    // Key operations
    bool WriteKey(const std::vector<unsigned char>& vchPubKey, 
                  const CWalletKey& wkey) {
        return Write(std::make_pair(std::string("key"), vchPubKey), wkey);
    }
    
    bool ReadKey(const std::vector<unsigned char>& vchPubKey,
                 CWalletKey& wkey) {
        return Read(std::make_pair(std::string("key"), vchPubKey), wkey);
    }
    
    bool EraseKey(const std::vector<unsigned char>& vchPubKey) {
        return Erase(std::make_pair(std::string("key"), vchPubKey));
    }
    
    // Default key
    bool WriteDefaultKey(const std::vector<unsigned char>& vchPubKey) {
        return Write(std::string("defaultkey"), vchPubKey);
    }
    
    bool ReadDefaultKey(std::vector<unsigned char>& vchPubKey) {
        return Read(std::string("defaultkey"), vchPubKey);
    }
    
    // Wallet transactions
    bool WriteTx(const hash256_t& hash, const core::Transaction& tx) {
        return Write(std::make_pair(std::string("tx"), hash), tx);
    }
    
    bool ReadTx(const hash256_t& hash, core::Transaction& tx) {
        return Read(std::make_pair(std::string("tx"), hash), tx);
    }
    
    bool EraseTx(const hash256_t& hash) {
        return Erase(std::make_pair(std::string("tx"), hash));
    }
    
    // Name operations (for compatibility)
    bool WriteName(const std::string& strAddress, const std::string& strName) {
        return Write(std::make_pair(std::string("name"), strAddress), strName);
    }
    
    bool ReadName(const std::string& strAddress, std::string& strName) {
        return Read(std::make_pair(std::string("name"), strAddress), strName);
    }
    
    bool EraseName(const std::string& strAddress) {
        return Erase(std::make_pair(std::string("name"), strAddress));
    }
    
    // Settings
    template<typename T>
    bool WriteSetting(const std::string& strKey, const T& value) {
        return Write(std::make_pair(std::string("setting"), strKey), value);
    }
    
    template<typename T>
    bool ReadSetting(const std::string& strKey, T& value) {
        return Read(std::make_pair(std::string("setting"), strKey), value);
    }
    
    bool EraseSetting(const std::string& strKey) {
        return Erase(std::make_pair(std::string("setting"), strKey));
    }
};

// Address database - preserving exact original schema
class CAddrDB : public CDB {
public:
    CAddrDB(const char* pszMode = "r+") : CDB("addr.dat", pszMode) {}
    
    bool WriteAddress(const net::Address& addr) {
        return Write(std::make_pair(std::string("addr"), addr.GetKey()), addr);
    }
    
    bool ReadAddress(const std::string& key, net::Address& addr) {
        return Read(std::make_pair(std::string("addr"), key), addr);
    }
    
    bool EraseAddress(const std::string& key) {
        return Erase(std::make_pair(std::string("addr"), key));
    }
};

// Legacy CDataStream for database compatibility
class CDataStream {
protected:
    typedef std::vector<char, std::pmr::polymorphic_allocator<char>> vector_type;
    vector_type vch;
    unsigned int nReadPos;
    int nType;
    int nVersion;
    
public:
    CDataStream(int nTypeIn, int nVersionIn) 
        : vch(&g_memory_pool), nReadPos(0), nType(nTypeIn), nVersion(nVersionIn) {}
    
    CDataStream(const std::string& str, int nTypeIn, int nVersionIn)
        : vch(str.begin(), str.end(), &g_memory_pool)
        , nReadPos(0), nType(nTypeIn), nVersion(nVersionIn) {}
    
    CDataStream& operator<<(const auto& obj) {
        ::Serialize(*this, obj, nType, nVersion);
        return *this;
    }
    
    CDataStream& operator>>(auto& obj) {
        ::Unserialize(*this, obj, nType, nVersion);
        return *this;
    }
    
    void reserve(size_t n) { vch.reserve(n); }
    const char* data() const { return vch.data() + nReadPos; }
    size_t size() const { return vch.size() - nReadPos; }
    
    void write(const char* pch, size_t nSize) {
        vch.insert(vch.end(), pch, pch + nSize);
    }
    
    void read(char* pch, size_t nSize) {
        assert(nSize <= size());
        std::memcpy(pch, data(), nSize);
        nReadPos += nSize;
    }
};

// Serialize/Unserialize compatibility layer
template<typename Stream, typename T>
void Serialize(Stream& s, const T& obj, int nType, int nVersion) {
    serialize::Buffer buffer{std::span<byte_t>{
        reinterpret_cast<byte_t*>(const_cast<char*>(s.data())), s.size()}};
    serialize::Serializer<T>::serialize(buffer, obj);
}

template<typename Stream, typename T>
void Unserialize(Stream& s, T& obj, int nType, int nVersion) {
    std::vector<byte_t> bytes(s.size());
    s.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    serialize::Buffer buffer{bytes};
    obj = *serialize::Serializer<T>::deserialize(buffer);
}

constexpr int SER_DISK = 0x01;

} // namespace bitcoin::db