// Copyright (c) 2025 GoldcoinPoP Developers
// Distributed under the MIT/X11 software license
// Proof of Participation - The Evolution of Satoshi's Vision

#include "headers.h"
#include "participation.h"
#include <thread>
#include <chrono>
#include <format>

using namespace goldcoin;
using namespace std::chrono_literals;

// Replace energy-wasting mining with elegant participation
void ParticipationMiner()
{
    printf("ParticipationMiner started\n");
    printf("Minimum stake requirement: %s GLC\n", FormatMoney(MINIMUM_STAKE).c_str());
    
    // What would Satoshi do? Keep it simple.
    CKey key;
    key.MakeNewKey();
    CBigNum bnExtraNonce = 0;
    
    while (fGenerateBitcoins) // We keep the same flag for compatibility
    {
        std::this_thread::sleep_for(2s); // Check every 2 seconds, not 50ms - we're not grinding
        
        if (fShutdown)
            return;
            
        // Wait for network peers
        while (vNodes.empty())
        {
            std::this_thread::sleep_for(1s);
            if (fShutdown || !fGenerateBitcoins)
                return;
        }
        
        // Check if we have matured stake
        int64_t myStake = GetMyStakeAmount(); // Function to implement
        if (myStake < MINIMUM_STAKE)
        {
            if (GetTime() % 3600 == 0) // Log once per hour
                printf("Insufficient stake for participation. Current: %s, Required: %s\n", 
                       FormatMoney(myStake).c_str(), 
                       FormatMoney(MINIMUM_STAKE).c_str());
            continue;
        }
        
        CBlockIndex* pindexPrev = pindexBest;
        if (!pindexPrev)
            continue;
            
        // Check if we won the lottery for this block
        uint256 prevBlockHash = pindexPrev->GetBlockHash();
        uint160 myAddress = Hash160(key.GetPubKey());
        
        if (!g_participationValidator.checkParticipation(myAddress, prevBlockHash, pindexPrev->nHeight + 1))
        {
            // We didn't win this round - that's okay, equal chance next time
            continue;
        }
        
        // We won! Create the block
        printf("Won participation lottery for block %d!\n", pindexPrev->nHeight + 1);
        
        //
        // Create coinbase tx (same as Bitcoin, just earned differently)
        //
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig << ++bnExtraNonce;
        txNew.vin[0].scriptSig << std::vector<unsigned char>{0x50, 0x6F, 0x50}; // "PoP" marker
        txNew.vout.resize(1);
        txNew.vout[0].scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
        
        //
        // Create new block
        //
        auto pblock = std::make_unique<CBlock>();
        if (!pblock)
            return;
            
        // Set block header
        pblock->nVersion = 1;
        pblock->hashPrevBlock = prevBlockHash;
        pblock->nTime = GetAdjustedTime();
        pblock->nBits = GetNextWorkRequired(pindexPrev); // We'll modify this for PoP
        pblock->nNonce = 0; // No nonce needed in PoP!
        
        // Add coinbase transaction
        pblock->vtx.push_back(txNew);
        
        // Collect transactions (same as Bitcoin - why change what works?)
        int64_t nFees = 0;
        {
            CRITICAL_BLOCK(cs_main)
            CRITICAL_BLOCK(cs_mapTransactions)
            {
                CTxDB txdb("r");
                std::map<uint256, CTxIndex> mapTestPool;
                std::vector<bool> vfAlreadyAdded(mapTransactions.size());
                bool fFoundSomething = true;
                unsigned int nBlockSize = 1000; // Reserve 1KB for header
                
                while (fFoundSomething && nBlockSize < MAX_SIZE/2)
                {
                    fFoundSomething = false;
                    unsigned int n = 0;
                    
                    for (auto& [hash, tx] : mapTransactions)
                    {
                        if (vfAlreadyAdded[n++])
                            continue;
                            
                        if (tx.IsCoinBase() || !tx.IsFinal())
                            continue;
                        
                        // Check minimum fee
                        int64_t nMinFee = tx.GetMinFee(nBlockSize);
                        
                        // Verify transaction
                        auto mapTestPoolTmp = mapTestPool;
                        if (!tx.ConnectInputs(txdb, mapTestPoolTmp, CDiskTxPos(1,1,1), 0, nFees, false, true, nMinFee))
                            continue;
                            
                        mapTestPool = std::move(mapTestPoolTmp);
                        
                        // Add to block
                        pblock->vtx.push_back(tx);
                        nBlockSize += ::GetSerializeSize(tx, SER_NETWORK);
                        vfAlreadyAdded[n-1] = true;
                        fFoundSomething = true;
                    }
                }
            }
        }
        
        // Set coinbase value
        pblock->vtx[0].vout[0].nValue = GetBlockValue(pindexPrev->nHeight + 1, nFees);
        
        // Update merkle root
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();
        
        printf("ParticipationMiner: Creating block with %zu transactions\n", pblock->vtx.size());
        
        // Sign the block with our participation key
        std::vector<unsigned char> vchSig;
        if (!key.Sign(pblock->GetHash(), vchSig))
        {
            printf("Error: Failed to sign block\n");
            continue;
        }
        
        // Add signature to coinbase
        pblock->vtx[0].vin[0].scriptSig << vchSig;
        
        // Process the block
        printf("ParticipationMiner: Block created!\n");
        printf("  hash: %s\n", pblock->GetHash().GetHex().c_str());
        printf("  height: %d\n", pindexPrev->nHeight + 1);
        printf("  transactions: %zu\n", pblock->vtx.size());
        printf("  reward: %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue).c_str());
        
        // Check and process block
        if (!ProcessBlock(nullptr, pblock.release()))
        {
            printf("Error: ProcessBlock failed for our block\n");
        }
        else
        {
            printf("Successfully created block %d through Proof of Participation!\n", 
                   pindexPrev->nHeight + 1);
        }
        
        // Brief pause before checking for next block
        std::this_thread::sleep_for(500ms);
    }
}

// Helper function to get current stake amount
int64_t GetMyStakeAmount()
{
    int64_t nStake = 0;
    
    CRITICAL_BLOCK(cs_mapWallet)
    {
        for (const auto& [hash, wtx] : mapWallet)
        {
            // Check if this is a participation stake transaction
            if (wtx.IsParticipationStake() && wtx.IsConfirmed())
            {
                nStake += wtx.GetCredit();
            }
        }
    }
    
    return nStake;
}

// Get block reward (simplified version of Bitcoin's GetBlockValue)
int64_t GetBlockValue(int nHeight, int64_t nFees)
{
    int64_t nSubsidy = 50 * COIN;
    
    // Goldcoin specific: Different halving schedule
    // Halving every 840,000 blocks (~4 years at 2 min blocks)
    nSubsidy >>= (nHeight / 840000);
    
    // Never go below 1 GLC minimum reward
    if (nSubsidy < COIN)
        nSubsidy = COIN;
    
    return nSubsidy + nFees;
}

// Thread entry point (replacing ThreadBitcoinMiner)
void ThreadParticipationMiner(void* parg)
{
    try
    {
        vnThreadsRunning[3]++;
        printf("ThreadParticipationMiner started\n");
        ParticipationMiner();
        vnThreadsRunning[3]--;
    }
    catch (std::exception& e)
    {
        vnThreadsRunning[3]--;
        PrintException(&e, "ThreadParticipationMiner()");
    }
    catch (...)
    {
        vnThreadsRunning[3]--;
        PrintException(nullptr, "ThreadParticipationMiner()");
    }
    
    printf("ThreadParticipationMiner exiting, %d threads remaining\n", vnThreadsRunning[3]);
}

// Initialize participation system
void InitializeParticipation()
{
    printf("Initializing Proof of Participation system...\n");
    printf("Philosophy: One stake, one vote - as Satoshi intended\n");
    printf("Minimum participation: %s GLC\n", FormatMoney(MINIMUM_STAKE).c_str());
    printf("Stake maturity: %d blocks\n", STAKE_MATURITY);
    printf("Target block time: %d seconds\n", BLOCK_TIME_SECONDS);
    
    // Load existing participation entries from database
    CTxDB txdb("r");
    // TODO: Implement loading of participation stakes
    
    printf("Proof of Participation system initialized\n");
}

// Satoshi said: "The network is robust in its unstructured simplicity"
// We honor that vision.