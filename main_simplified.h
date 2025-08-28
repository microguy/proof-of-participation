// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Simplified main.h for Proof of Participation - Channeling Satoshi

#ifndef GOLDCOIN_MAIN_H
#define GOLDCOIN_MAIN_H

#include "bignum.h"
#include "uint256.h"
#include "script.h"
#include <vector>
#include <map>

// Core Goldcoin parameters - simple and immutable
static const int64_t COIN = 100000000;
static const int64_t CENT = 1000000;
static const int64_t MAX_MONEY = 1172245700LL * COIN; // Total Goldcoin supply

// Block parameters
static const unsigned int MAX_BLOCK_SIZE = 32 * 1024 * 1024; // 32MB blocks
static const int COINBASE_MATURITY = 100;
static const int BLOCK_TIME_SECONDS = 120; // 2 minute blocks

// PoP Hard Fork Height
static const int POP_ACTIVATION_HEIGHT = 3500000;

// Network magic bytes
static const unsigned char pchMessageStart[4] = {0x47, 0x4C, 0x44, 0x21}; // "GLD!"

// NO PROOF OF WORK LIMIT - We don't need it!
// NO DIFFICULTY ADJUSTMENT - Not required!
// NO CHECKPOINTS - Security through economics!

class COutPoint;
class CInPoint;
class CTxIn;
class CTxOut;
class CTransaction;
class CBlock;
class CBlockIndex;
class CWalletTx;
class CKeyItem;

// Simple transaction output
class CTxOut
{
public:
    int64_t nValue;
    CScript scriptPubKey;

    CTxOut() { SetNull(); }
    CTxOut(int64_t nValueIn, CScript scriptPubKeyIn);
    
    void SetNull() {
        nValue = -1;
        scriptPubKey.clear();
    }
    
    bool IsNull() const {
        return (nValue == -1);
    }
    
    uint256 GetHash() const;
    
    friend bool operator==(const CTxOut& a, const CTxOut& b) {
        return (a.nValue == b.nValue && a.scriptPubKey == b.scriptPubKey);
    }
};

// Simple transaction - unchanged from Satoshi's design
class CTransaction
{
public:
    int nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    unsigned int nLockTime;

    CTransaction() { SetNull(); }
    
    void SetNull() {
        nVersion = 1;
        vin.clear();
        vout.clear();
        nLockTime = 0;
    }
    
    bool IsNull() const {
        return (vin.empty() && vout.empty());
    }
    
    uint256 GetHash() const;
    
    bool IsCoinBase() const {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }
    
    bool IsFinal(int nBlockHeight = 0, int64_t nBlockTime = 0) const;
    
    int64_t GetValueOut() const {
        int64_t nValueOut = 0;
        for (const auto& out : vout) {
            nValueOut += out.nValue;
            if (!MoneyRange(out.nValue) || !MoneyRange(nValueOut))
                throw std::runtime_error("CTransaction::GetValueOut() : value out of range");
        }
        return nValueOut;
    }
    
    // Zero fees in Goldcoin!
    int64_t GetMinFee(unsigned int nBlockSize = 1) const {
        return 0; // ZERO FEES FOREVER
    }
    
    bool CheckTransaction() const;
    bool AcceptToMemoryPool(bool fCheckInputs = true);
};

// Simplified block - no nonce, no proof of work!
class CBlock
{
public:
    // Header
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;  // Kept for compatibility, not used in PoP
    unsigned int nNonce; // Kept for compatibility, always 0 in PoP
    
    // Network and disk
    std::vector<CTransaction> vtx;
    
    // Memory only
    mutable std::vector<uint256> vMerkleTree;
    
    CBlock() { SetNull(); }
    
    void SetNull() {
        nVersion = 1;
        hashPrevBlock = 0;
        hashMerkleRoot = 0;
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        vtx.clear();
        vMerkleTree.clear();
    }
    
    uint256 GetHash() const;
    uint256 BuildMerkleTree() const;
    
    int64_t GetBlockTime() const {
        return (int64_t)nTime;
    }
    
    // Proof of Participation - no work required!
    bool CheckBlock() const {
        // Check block sanity
        if (vtx.empty() || vtx.size() > MAX_BLOCK_SIZE)
            return false;
            
        // Check coinbase
        if (!vtx[0].IsCoinBase())
            return false;
            
        // Check transactions
        for (const auto& tx : vtx) {
            if (!tx.CheckTransaction())
                return false;
        }
        
        // No proof of work validation needed!
        // No difficulty check needed!
        // Just pure transaction validation
        
        return true;
    }
    
    bool AcceptBlock();
};

// Simplified block index
class CBlockIndex
{
public:
    const uint256* phashBlock;
    CBlockIndex* pprev;
    CBlockIndex* pnext;
    int nHeight;
    
    // Block header
    int nVersion;
    uint256 hashMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;  // Not used in PoP
    unsigned int nNonce; // Not used in PoP
    
    CBlockIndex() {
        phashBlock = nullptr;
        pprev = nullptr;
        pnext = nullptr;
        nHeight = 0;
        nVersion = 0;
        hashMerkleRoot = 0;
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }
    
    CBlockIndex(CBlock& block) {
        pprev = nullptr;
        pnext = nullptr;
        nVersion = block.nVersion;
        hashMerkleRoot = block.hashMerkleRoot;
        nTime = block.nTime;
        nBits = block.nBits;
        nNonce = block.nNonce;
    }
    
    uint256 GetBlockHash() const {
        return *phashBlock;
    }
    
    int64_t GetBlockTime() const {
        return (int64_t)nTime;
    }
    
    bool IsInMainChain() const {
        return (pnext || this == pindexBest);
    }
};

// Global state - simplified
extern CCriticalSection cs_main;
extern std::map<uint256, CTransaction> mapTransactions;
extern std::map<uint256, CBlockIndex*> mapBlockIndex;
extern CBlockIndex* pindexBest;
extern int nBestHeight;
extern uint256 hashBestChain;
extern CBlockIndex* pindexGenesisBlock;
extern std::map<uint256, CBlock*> mapOrphanBlocks;

// Core functions - simplified
bool ProcessBlock(CNode* pfrom, CBlock* pblock);
bool CheckTransaction(const CTransaction& tx);
bool AcceptToMemoryPool(CTransaction& tx, bool fCheckInputs = true);
bool MoneyRange(int64_t nValue);
int64_t GetBalance();
bool CreateTransaction(CScript scriptPubKey, int64_t nValue, CTransaction& wtxNew, int64_t& nFeeRet);
bool CommitTransaction(CTransaction& wtxNew);

// Proof of Participation functions
bool IsProofOfParticipationActive(int nHeight);
bool ValidateParticipation(const CBlock& block, int nHeight);
void InitializeParticipation();

// Simplified validation - no proof of work!
inline bool CheckProofOfWork(uint256 hash, unsigned int nBits) {
    // NOT NEEDED FOR PROOF OF PARTICIPATION!
    return true;
}

// Money supply
inline bool MoneyRange(int64_t nValue) { 
    return (nValue >= 0 && nValue <= MAX_MONEY); 
}

// What would Satoshi do? Keep it simple.
// "The design supports a tremendous variety of possible transaction types 
//  that I designed years ago. Escrow transactions, bonded contracts, 
//  third party arbitration, multi-party signature, etc. If Bitcoin 
//  catches on in a big way, these are things we'll want to explore 
//  in the future, but they all had to be designed at the beginning 
//  to make sure they would be possible later." - Satoshi
//
// We honor this by maintaining transaction compatibility while 
// evolving the consensus mechanism.

#endif // GOLDCOIN_MAIN_H