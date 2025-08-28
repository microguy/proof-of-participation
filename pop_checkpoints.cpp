// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Proof of Participation Checkpoints - Simplified for PoP
// No hashrate thresholds needed - just finality markers

#include "headers.h"
#include "goldcoin_specs.h"
#include <format>

using namespace goldcoin;

// Simplified checkpointing for PoP
class PoPCheckpoints {
private:
    // Finality checkpoints - blocks that cannot be reorganized
    struct Checkpoint {
        int height;
        uint256 hash;
        int64_t timestamp;
        std::string description;
    };
    
    std::vector<Checkpoint> checkpoints;
    int lastCheckpointHeight = 0;
    
    // PoP doesn't need dynamic thresholds - use simple finality
    static constexpr int FINALITY_DEPTH = 30; // 30 blocks = 1 hour at 2 min/block
    static constexpr int CHECKPOINT_INTERVAL = 10000; // Checkpoint every 10k blocks
    
public:
    // Initialize PoP checkpoints
    void Initialize() {
        printf("================================================================================\n");
        printf("                   PROOF OF PARTICIPATION CHECKPOINTS                          \n");
        printf("================================================================================\n");
        printf("Purpose: Historical finality markers (not security)\n");
        printf("Finality: %d blocks (%.1f hours)\n", FINALITY_DEPTH, FINALITY_DEPTH * 2.0 / 60);
        printf("Interval: Every %d blocks\n", CHECKPOINT_INTERVAL);
        printf("\n");
        printf("Note: PoP doesn't need checkpoints for security like PoW did.\n");
        printf("      These are just historical markers and optimization hints.\n");
        printf("================================================================================\n");
        
        LoadHistoricalCheckpoints();
    }
    
    // Load important historical checkpoints
    void LoadHistoricalCheckpoints() {
        // Key milestones in Goldcoin history
        checkpoints.push_back({0, uint256("0x..."), 1368576000, "Genesis - May 15, 2013"});
        checkpoints.push_back({100000, uint256("0x..."), 0, "First 100k blocks"});
        checkpoints.push_back({1000000, uint256("0x..."), 0, "1 Million blocks"});
        checkpoints.push_back({3000000, uint256("0x..."), 0, "3 Million blocks"});
        checkpoints.push_back({3500000, uint256("0x..."), 0, "PoP ACTIVATION - The Evolution"});
        
        if (!checkpoints.empty()) {
            lastCheckpointHeight = checkpoints.back().height;
        }
        
        printf("Loaded %zu historical checkpoints\n", checkpoints.size());
    }
    
    // Check if a block is finalized (cannot be reorganized)
    bool IsBlockFinalized(int nHeight) {
        int currentHeight = nBestHeight;
        
        // Blocks older than FINALITY_DEPTH are finalized
        if (currentHeight - nHeight >= FINALITY_DEPTH) {
            return true;
        }
        
        // Historical checkpoints are always finalized
        for (const auto& cp : checkpoints) {
            if (cp.height == nHeight) {
                return true;
            }
        }
        
        return false;
    }
    
    // Add automatic checkpoint (every CHECKPOINT_INTERVAL blocks)
    void MaybeAddCheckpoint(int nHeight, const uint256& hash) {
        // Only add checkpoints at intervals
        if (nHeight % CHECKPOINT_INTERVAL != 0) {
            return;
        }
        
        // Don't checkpoint recent blocks
        if (nBestHeight - nHeight < FINALITY_DEPTH) {
            return;
        }
        
        Checkpoint cp;
        cp.height = nHeight;
        cp.hash = hash;
        cp.timestamp = GetTime();
        cp.description = std::format("Automatic checkpoint at height {}", nHeight);
        
        checkpoints.push_back(cp);
        lastCheckpointHeight = nHeight;
        
        printf("Added checkpoint at height %d\n", nHeight);
        SaveCheckpoint(cp);
    }
    
    // Verify block against checkpoints
    bool VerifyCheckpoint(int nHeight, const uint256& hash) {
        // Find checkpoint for this height
        for (const auto& cp : checkpoints) {
            if (cp.height == nHeight) {
                if (cp.hash != hash) {
                    return error("Block at height %d does not match checkpoint", nHeight);
                }
                return true;
            }
        }
        
        // No checkpoint for this height - that's fine
        return true;
    }
    
    // Get checkpoint by height
    std::optional<Checkpoint> GetCheckpoint(int nHeight) {
        for (const auto& cp : checkpoints) {
            if (cp.height == nHeight) {
                return cp;
            }
        }
        return std::nullopt;
    }
    
    // Save checkpoint to database
    void SaveCheckpoint(const Checkpoint& cp) {
        // Write to database
        CWalletDB walletdb;
        // walletdb.WriteCheckpoint(cp.height, cp.hash);
    }
    
    // Get status string
    std::string GetStatus() {
        return std::format(
            "Checkpoints: {} | Last: {} | Finality: {} blocks",
            checkpoints.size(),
            lastCheckpointHeight,
            FINALITY_DEPTH
        );
    }
    
    // Export checkpoints for new nodes (fast sync)
    std::vector<std::pair<int, uint256>> ExportCheckpoints() {
        std::vector<std::pair<int, uint256>> exports;
        for (const auto& cp : checkpoints) {
            exports.push_back({cp.height, cp.hash});
        }
        return exports;
    }
};

// Global PoP checkpoints instance
PoPCheckpoints g_popCheckpoints;

// Initialize PoP checkpoints
void InitializePoPCheckpoints() {
    g_popCheckpoints.Initialize();
}

// Check if block is finalized
bool IsBlockFinalized(int nHeight) {
    return g_popCheckpoints.IsBlockFinalized(nHeight);
}

// Maybe add automatic checkpoint
void MaybeAddCheckpoint(int nHeight, const uint256& hash) {
    g_popCheckpoints.MaybeAddCheckpoint(nHeight, hash);
}

// Verify checkpoint
bool VerifyCheckpoint(int nHeight, const uint256& hash) {
    return g_popCheckpoints.VerifyCheckpoint(nHeight, hash);
}

// RPC command to get checkpoint info
Value getcheckpointinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error(
            "getcheckpointinfo [height]\n"
            "Returns checkpoint information.");
    }
    
    Object obj;
    obj.push_back(Pair("consensus", "Proof of Participation"));
    obj.push_back(Pair("security_model", "Economic (not checkpoint-based)"));
    obj.push_back(Pair("status", g_popCheckpoints.GetStatus()));
    obj.push_back(Pair("finality_depth", FINALITY_DEPTH));
    obj.push_back(Pair("note", "PoP doesn't need checkpoints for security"));
    
    if (params.size() == 1) {
        int height = params[0].get_int();
        auto cp = g_popCheckpoints.GetCheckpoint(height);
        if (cp.has_value()) {
            obj.push_back(Pair("checkpoint_found", true));
            obj.push_back(Pair("height", cp->height));
            obj.push_back(Pair("hash", cp->hash.GetHex()));
            obj.push_back(Pair("description", cp->description));
        } else {
            obj.push_back(Pair("checkpoint_found", false));
        }
    }
    
    return obj;
}

// The beauty of PoP: We don't need complex checkpoint systems!
// Security comes from economics, not from centralized checkpoints.