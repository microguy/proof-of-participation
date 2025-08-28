// Copyright (c) 2025 MicroGuy / Goldcoin Developers  
// Hybrid Fee System - Following Goldcoin's actual implementation
// 5% of block space reserved for free high-priority transactions
// Remaining space uses optional fees during congestion

#include "headers.h"
#include "goldcoin_specs.h"
#include <format>

using namespace goldcoin;

// Goldcoin Fee Economy - Satoshi's priority system preserved
class HybridFeeSystem {
private:
    // Block size tracking  
    static constexpr unsigned int DEFAULT_BLOCK_MAX_SIZE = 32000000;  // 32MB 
    static constexpr unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = DEFAULT_BLOCK_MAX_SIZE * 5 / 100; // 5% free
    static constexpr int64_t MIN_TX_FEE = 100000;  // 0.001 GLC per KB when paying
    static constexpr int64_t MIN_RELAY_TX_FEE = 100000;  // Minimum to relay
    static constexpr double FREE_TX_PRIORITY = 57600000; // Satoshi's threshold
    
    unsigned int nCurrentBlockSize = 0;
    unsigned int nCurrentBlockTx = 0;
    int64_t nCurrentBlockFees = 0;
    
public:
    // Initialize hybrid fee system
    void Initialize() {
        printf("================================================================================\n");
        printf("                  GOLDCOIN HYBRID FEE SYSTEM ACTIVATED                         \n");
        printf("================================================================================\n");
        printf("Block Space Allocation:\n");
        printf("  First 5%%: FREE for high-priority transactions\n");
        printf("  Remaining 95%%: Optional fees during congestion\n");
        printf("Free Transaction Threshold: Priority > 57,600,000 (Satoshi's formula)\n");
        printf("Minimum Fee (when required): 0.001 GLC per KB\n");
        printf("Philosophy: Everyone gets a chance at free transactions\n");
        printf("================================================================================\n");
    }
    
    // Calculate transaction priority (Satoshi's formula)
    double GetPriority(const CTransaction& tx, int nHeight) const {
        if (tx.IsCoinBase())
            return 0.0;
            
        double dPriority = 0;
        for (const CTxIn& txin : tx.vin) {
            // In production: lookup actual input values and ages
            // For now: assume 100 GLC inputs with 144 confirmations (1 day)
            int64_t nInputValue = 100 * COIN;
            int nConf = 144;
            
            dPriority += (double)nInputValue * nConf;
        }
        
        // Divide by transaction size in bytes
        unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK);
        dPriority /= nTxSize;
        
        return dPriority;
    }
    
    // Validate transaction with hybrid fee rules
    bool ValidateTransaction(const CTransaction& tx, int nHeight) {
        unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK);
        
        // In the first 5% of block - free for high priority
        if (nCurrentBlockSize < DEFAULT_BLOCK_PRIORITY_SIZE) {
            double dPriority = GetPriority(tx, nHeight);
            
            if (dPriority > FREE_TX_PRIORITY) {
                // High priority = free transaction
                printf("Free high-priority tx accepted (priority: %.0f)\n", dPriority);
                return true;
            }
            // Low priority in free zone - still allow but warn
            printf("Low priority tx in free zone (priority: %.0f)\n", dPriority);
        }
        
        // After 5% or low priority - check for optional fee
        int64_t nMinFee = GetMinimumFee(nTxSize);
        int64_t nTxFee = GetTransactionFee(tx);
        
        // Fee is optional unless block is congested
        if (nCurrentBlockSize > DEFAULT_BLOCK_PRIORITY_SIZE && nTxFee < nMinFee) {
            // Only enforce fees when block is filling up
            if (nCurrentBlockSize > DEFAULT_BLOCK_MAX_SIZE * 90 / 100) {
                return error("Block 90%% full - fee required: %s GLC", 
                            FormatMoney(nMinFee).c_str());
            }
        }
        
        return true;
    }
    
    // Get transaction fee (actual fee paid)
    int64_t GetTransactionFee(const CTransaction& tx) const {
        if (tx.IsCoinBase())
            return 0;
            
        // In production: calculate from inputs - outputs
        // For now: check if fee field exists
        return 0;  // Placeholder
    }
    
    // Get minimum fee when required
    int64_t GetMinimumFee(unsigned int nBytes) const {
        // First 5% is always free for high-priority
        if (nCurrentBlockSize < DEFAULT_BLOCK_PRIORITY_SIZE) {
            return 0;  // Can be free
        }
        
        // Goldcoin fee: 0.001 GLC per KB
        int64_t nMinFee = MIN_TX_FEE * (1 + nBytes / 1000);
        
        return nMinFee;
    }
    
    // Add transaction to current block
    bool AddToBlock(const CTransaction& tx) {
        unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK);
        
        // Check block size limit
        if (nCurrentBlockSize + nTxSize > DEFAULT_BLOCK_MAX_SIZE) {
            return false;  // Block full
        }
        
        nCurrentBlockSize += nTxSize;
        nCurrentBlockTx++;
        nCurrentBlockFees += GetTransactionFee(tx);
        
        return true;
    }
    
    // Reset for new block
    void OnNewBlock() {
        nCurrentBlockSize = 0;
        nCurrentBlockTx = 0;
        nCurrentBlockFees = 0;
        
        // Log statistics
        if (nCurrentBlockTx > 0) {
            printf("Block completed: %u tx, %u bytes, %s GLC fees collected\n",
                   nCurrentBlockTx, nCurrentBlockSize, 
                   FormatMoney(nCurrentBlockFees).c_str());
        }
    }
    
    // Build priority queue for free transactions
    void AddPriorityTransactions(std::vector<CTransaction>& vtx, int nHeight) {
        // Reserve first 5% for high-priority free transactions
        std::vector<std::pair<double, CTransaction>> vecPriority;
        
        // Collect all pending transactions
        for (const auto& tx : mempool) {
            double dPriority = GetPriority(tx, nHeight);
            if (dPriority > FREE_TX_PRIORITY) {
                vecPriority.push_back({dPriority, tx});
            }
        }
        
        // Sort by priority (highest first)
        std::sort(vecPriority.begin(), vecPriority.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Add high-priority transactions to free zone
        for (const auto& [priority, tx] : vecPriority) {
            if (nCurrentBlockSize >= DEFAULT_BLOCK_PRIORITY_SIZE) {
                break;  // Free zone full
            }
            
            if (AddToBlock(tx)) {
                vtx.push_back(tx);
                printf("Added free tx with priority %.0f\n", priority);
            }
        }
    }
    
    // Build remaining block with fee-paying transactions
    void AddFeeTransactions(std::vector<CTransaction>& vtx) {
        // After 5%, add transactions sorted by fee rate
        std::vector<std::pair<int64_t, CTransaction>> vecFees;
        
        for (const auto& tx : mempool) {
            int64_t nFee = GetTransactionFee(tx);
            if (nFee > 0) {
                vecFees.push_back({nFee, tx});
            }
        }
        
        // Sort by fee (highest first)
        std::sort(vecFees.begin(), vecFees.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Fill remaining block space
        for (const auto& [fee, tx] : vecFees) {
            if (nCurrentBlockSize >= DEFAULT_BLOCK_MAX_SIZE * 95 / 100) {
                break;  // Leave 5% headroom
            }
            
            if (AddToBlock(tx)) {
                vtx.push_back(tx);
            }
        }
    }
};

// Global hybrid fee system
HybridFeeSystem g_hybridFeeSystem;

// Initialize hybrid fee economy
void InitializeHybridFees() {
    g_hybridFeeSystem.Initialize();
}

// Build block with priority transactions
void CreateNewBlock(CBlock* pblock, CReserveKey& reservekey) {
    // Get current height
    int nHeight = pindexBest->nHeight + 1;
    
    // Reset for new block
    g_hybridFeeSystem.OnNewBlock();
    
    // Create coinbase tx
    CTransaction txCoinbase;
    txCoinbase.vin.resize(1);
    txCoinbase.vin[0].prevout.SetNull();
    txCoinbase.vout.resize(1);
    txCoinbase.vout[0].scriptPubKey << reservekey.GetReservedKey() << OP_CHECKSIG;
    
    // Start with coinbase
    pblock->vtx.clear();
    pblock->vtx.push_back(txCoinbase);
    
    // Add priority transactions (first 5% free)
    g_hybridFeeSystem.AddPriorityTransactions(pblock->vtx, nHeight);
    
    // Add fee-paying transactions (remaining 95%)
    g_hybridFeeSystem.AddFeeTransactions(pblock->vtx);
    
    // Update coinbase value with fees
    pblock->vtx[0].vout[0].nValue = GetBlockValue(nHeight, g_hybridFeeSystem.nCurrentBlockFees);
    
    // Build merkle tree
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

// Modified mempool acceptance for hybrid fees
bool AcceptToMemoryPool(CTxDB& txdb, CTransaction& tx, bool fCheckInputs) {
    // Check transaction basics
    if (!tx.CheckTransaction())
        return error("AcceptToMemoryPool: CheckTransaction failed");
    
    // Check for conflicts
    uint256 hash = tx.GetHash();
    if (mapTransactions.count(hash))
        return false; // Already in pool
    
    // Get current height
    int nHeight = pindexBest ? pindexBest->nHeight + 1 : 0;
    
    // Validate with hybrid fee system
    if (!g_hybridFeeSystem.ValidateTransaction(tx, nHeight))
        return false;
    
    if (fCheckInputs) {
        // Check inputs
        map<uint256, CTxIndex> mapUnused;
        int64_t nValueIn = 0;
        if (!tx.ConnectInputs(txdb, mapUnused, CDiskTxPos(1,1,1), 0, nValueIn, false, false))
            return error("AcceptToMemoryPool: ConnectInputs failed");
    }
    
    // Add to memory pool
    {
        CRITICAL_BLOCK(cs_mapTransactions)
        {
            mapTransactions[hash] = tx;
            for (int i = 0; i < tx.vin.size(); i++)
                mapNextTx[tx.vin[i].prevout] = CInPoint(&mapTransactions[hash], i);
        }
    }
    
    double dPriority = g_hybridFeeSystem.GetPriority(tx, nHeight);
    int64_t nFee = g_hybridFeeSystem.GetTransactionFee(tx);
    
    printf("AcceptToMemoryPool: tx %s accepted (priority: %.0f, fee: %s)\n", 
           hash.ToString().substr(0,10).c_str(), dPriority, FormatMoney(nFee).c_str());
    
    return true;
}

// RPC command to get fee info
Value getfeeinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "getfeeinfo\n"
            "Returns information about the hybrid fee system.");
    }
    
    Object obj;
    obj.push_back(Pair("system", "Hybrid Fee Economy"));
    obj.push_back(Pair("free_space", "5% of block"));
    obj.push_back(Pair("free_threshold", 57600000));
    obj.push_back(Pair("minimum_fee", FormatMoney(MIN_TX_FEE) + " per KB"));
    obj.push_back(Pair("block_size", "32 MB"));
    obj.push_back(Pair("philosophy", "Everyone deserves free transactions"));
    
    // Current block stats
    obj.push_back(Pair("current_block_size", g_hybridFeeSystem.nCurrentBlockSize));
    obj.push_back(Pair("current_block_tx", g_hybridFeeSystem.nCurrentBlockTx));
    obj.push_back(Pair("current_block_fees", FormatMoney(g_hybridFeeSystem.nCurrentBlockFees)));
    
    return obj;
}

// Satoshi: "We define an electronic coin as a chain of digital signatures"
// MicroGuy: "We define freedom as the ability to transact without permission or fees"