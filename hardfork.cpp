// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Goldcoin Hard Fork: Transitioning to Proof of Participation

#include "headers.h"
#include "goldcoin.h"
#include "participation.h"
#include <format>
#include <iostream>

using namespace goldcoin;

// Hard fork activation logic
class HardForkManager {
private:
    bool fActivated = false;
    int nActivationHeight = 0;
    uint256 activationBlockHash;
    
public:
    // Check if we should activate the hard fork
    bool CheckActivation(int nHeight, const uint256& blockHash) {
        if (fActivated) {
            return true;
        }
        
        if (nHeight >= HARD_FORK_HEIGHT) {
            ActivateHardFork(nHeight, blockHash);
            return true;
        }
        
        return false;
    }
    
    // Activate the Proof of Participation hard fork
    void ActivateHardFork(int nHeight, const uint256& blockHash) {
        if (fActivated) return;
        
        printf("\n");
        printf("================================================================================\n");
        printf("                     GOLDCOIN HARD FORK ACTIVATION                             \n");
        printf("================================================================================\n");
        printf("Block Height: %d\n", nHeight);
        printf("Block Hash: %s\n", blockHash.GetHex().c_str());
        printf("Timestamp: %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
        printf("\n");
        printf("Goldcoin is now transitioning from Proof of Work to Proof of Participation!\n");
        printf("This is a historic moment for cryptocurrency.\n");
        printf("\n");
        printf("Creator: MicroGuy\n");
        printf("Vision: The Greatest Form of Money in the Galaxy\n");
        printf("Energy Usage: Reduced by 99.99%%\n");
        printf("Decentralization: Enhanced - Everyone can participate\n");
        printf("\n");
        printf("Minimum Stake: %s GLC\n", FormatMoney(MINIMUM_STAKE).c_str());
        printf("Block Time: %d seconds\n", BLOCK_TIME_SECONDS);
        printf("Stake Maturity: %d blocks\n", STAKE_MATURITY);
        printf("\n");
        printf("What would Satoshi do? He would evolve. And so we have.\n");
        printf("================================================================================\n");
        printf("\n");
        
        fActivated = true;
        nActivationHeight = nHeight;
        activationBlockHash = blockHash;
        
        // Initialize Proof of Participation system
        InitializeParticipation();
        
        // Write activation to database
        CWalletDB walletdb;
        walletdb.WriteHardForkActivation(nHeight, blockHash);
        
        // Notify all connected nodes
        BroadcastHardForkActivation();
    }
    
    // Broadcast hard fork activation to network
    void BroadcastHardForkActivation() {
        CRITICAL_BLOCK(cs_vNodes) {
            for (CNode* pnode : vNodes) {
                pnode->PushMessage("hardfork", HARD_FORK_HEIGHT, HARD_FORK_VERSION);
            }
        }
    }
    
    // Validate blocks based on current consensus rules
    bool ValidateBlock(const CBlock& block, int nHeight) {
        if (IsProofOfParticipationActive(nHeight)) {
            return ValidatePoPBlock(block, nHeight);
        } else {
            return ValidatePoWBlock(block, nHeight);
        }
    }
    
    // Validate Proof of Work block (pre-fork)
    bool ValidatePoWBlock(const CBlock& block, int nHeight) {
        // Original Goldcoin PoW validation
        if (block.GetHash() > block.GetWorkRequired()) {
            return error("ValidatePoWBlock: proof of work failed");
        }
        return true;
    }
    
    // Validate Proof of Participation block (post-fork)
    bool ValidatePoPBlock(const CBlock& block, int nHeight) {
        // Check if block producer had valid stake
        CTransaction stakeTx = block.vtx[0]; // Coinbase contains stake proof
        
        // Verify minimum stake
        int64_t stakeAmount = GetStakeAmount(stakeTx);
        if (stakeAmount < MINIMUM_STAKE) {
            return error("ValidatePoPBlock: insufficient stake %s < %s", 
                        FormatMoney(stakeAmount).c_str(),
                        FormatMoney(MINIMUM_STAKE).c_str());
        }
        
        // Verify stake maturity
        int stakeAge = GetStakeAge(stakeTx, nHeight);
        if (stakeAge < STAKE_MATURITY) {
            return error("ValidatePoPBlock: stake not mature %d < %d blocks",
                        stakeAge, STAKE_MATURITY);
        }
        
        // Verify VRF lottery win
        if (!VerifyLotteryWin(block, nHeight)) {
            return error("ValidatePoPBlock: block producer did not win lottery");
        }
        
        return true;
    }
    
    // Get stake amount from transaction
    int64_t GetStakeAmount(const CTransaction& tx) {
        // Implementation to extract stake amount
        // This would look up the staking transaction
        return 100 * COIN; // Placeholder
    }
    
    // Get stake age in blocks
    int GetStakeAge(const CTransaction& tx, int nCurrentHeight) {
        // Implementation to calculate stake age
        return STAKE_MATURITY + 1; // Placeholder
    }
    
    // Verify that block producer won the VRF lottery
    bool VerifyLotteryWin(const CBlock& block, int nHeight) {
        // Extract VRF proof from block
        // Verify against previous block hash
        // This is simplified - real implementation needs full VRF verification
        return true; // Placeholder
    }
    
    bool IsActivated() const { return fActivated; }
    int GetActivationHeight() const { return nActivationHeight; }
};

// Global hard fork manager
HardForkManager g_hardForkManager;

// Called when processing each new block
bool ProcessHardFork(const CBlock& block, int nHeight) {
    // Check if we should activate
    if (!g_hardForkManager.IsActivated() && 
        nHeight >= HARD_FORK_HEIGHT) {
        
        g_hardForkManager.ActivateHardFork(nHeight, block.GetHash());
        
        // Special message for MicroGuy's vision
        printf("\n");
        printf("MicroGuy's Vision Realized:\n");
        printf("\"We're not just creating a cryptocurrency.\n");
        printf(" We're creating the future of money.\n");
        printf(" Energy-efficient, accessible to all,\n");
        printf(" following Satoshi's principles.\n");
        printf(" This is Goldcoin's destiny.\"\n");
        printf("\n");
    }
    
    return g_hardForkManager.ValidateBlock(block, nHeight);
}

// Get current consensus mechanism name
std::string GetConsensusMechanism(int nHeight) {
    if (IsProofOfParticipationActive(nHeight)) {
        return "Proof of Participation (PoP)";
    } else {
        return "Proof of Work (PoW)";
    }
}

// Estimate time until hard fork
std::string GetTimeUntilHardFork() {
    int currentHeight = nBestHeight;
    if (currentHeight >= HARD_FORK_HEIGHT) {
        return "Hard fork activated!";
    }
    
    int blocksRemaining = HARD_FORK_HEIGHT - currentHeight;
    int secondsRemaining = blocksRemaining * BLOCK_TIME_SECONDS;
    
    int days = secondsRemaining / 86400;
    int hours = (secondsRemaining % 86400) / 3600;
    int minutes = (secondsRemaining % 3600) / 60;
    
    return std::format("{} days, {} hours, {} minutes", days, hours, minutes);
}

// RPC command to check hard fork status
Value gethardforkinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "gethardforkinfo\n"
            "Returns information about the Proof of Participation hard fork.");
    }
    
    Object obj;
    obj.push_back(Pair("current_height", nBestHeight));
    obj.push_back(Pair("hardfork_height", HARD_FORK_HEIGHT));
    obj.push_back(Pair("activated", g_hardForkManager.IsActivated()));
    obj.push_back(Pair("consensus", GetConsensusMechanism(nBestHeight)));
    obj.push_back(Pair("time_until_fork", GetTimeUntilHardFork()));
    obj.push_back(Pair("minimum_stake", FormatMoney(MINIMUM_STAKE)));
    obj.push_back(Pair("version", HARD_FORK_VERSION));
    
    return obj;
}