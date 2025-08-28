// Copyright (c) 2013-2025 MicroGuy / Goldcoin Developers
// The Greatest Form of Money in the Galaxy
// Goldcoin: Proof of Participation Hard Fork

#ifndef GOLDCOIN_H
#define GOLDCOIN_H

#include "uint256.h"
#include <string>
#include <chrono>

namespace goldcoin {

// Goldcoin Network Parameters
inline constexpr const char* COIN_NAME = "Goldcoin";
inline constexpr const char* COIN_TICKER = "GLC";
inline constexpr int64_t COIN = 100000000; // 1 GLC = 100,000,000 units (same as BTC)
inline constexpr int64_t CENT = 1000000;   // 0.01 GLC

// Original Goldcoin launch: May 15, 2013 on bitcointalk.org
inline constexpr auto GOLDCOIN_GENESIS_TIME = std::chrono::seconds(1368576000); // May 15, 2013

// Total supply: 1,172,245,700 GLC (actual Goldcoin supply)
inline constexpr int64_t MAX_MONEY = 1172245700 * COIN;
inline constexpr int64_t TREASURY_RESERVE = 1100000000 * COIN; // 100-Year Treasury

// Proof of Participation Parameters (Hard Fork)
inline constexpr int HARD_FORK_HEIGHT = 3500000; // Block height for PoP activation
inline constexpr const char* HARD_FORK_VERSION = "2.0.0-pop";

// Network Magic Bytes (different from Bitcoin to prevent cross-contamination)
inline constexpr unsigned char GOLDCOIN_MAGIC[4] = {0x47, 0x4C, 0x44, 0x21}; // "GLD!"

// Block Parameters
inline constexpr int BLOCK_TIME_SECONDS = 120; // 2 minutes (faster than Bitcoin's 10)
inline constexpr int BLOCKS_PER_DAY = 720;     // 24 * 60 / 2
inline constexpr int BLOCKS_PER_YEAR = 262800; // 365 * 720

// Proof of Participation specific
inline constexpr int64_t MINIMUM_STAKE = 100 * COIN;  // 100 GLC to participate
inline constexpr int STAKE_MATURITY = 1440;           // 2 days of confirmations
inline constexpr int STAKE_MAX_AGE = 90 * BLOCKS_PER_DAY; // 90 days max age

// Goldcoin Block Rewards (different from Bitcoin)
inline int64_t GetBlockValue(int nHeight, int64_t nFees = 0) {
    int64_t nSubsidy = 50 * COIN;
    
    // Goldcoin specific halving schedule
    // More gradual than Bitcoin to ensure long-term mining incentive
    if (nHeight < 840000) {
        nSubsidy = 50 * COIN;
    } else if (nHeight < 1680000) {
        nSubsidy = 25 * COIN;
    } else if (nHeight < 2520000) {
        nSubsidy = 10 * COIN;
    } else if (nHeight < 3360000) {
        nSubsidy = 5 * COIN;
    } else {
        nSubsidy = 2 * COIN; // Minimum 2 GLC forever
    }
    
    return nSubsidy + nFees;
}

// Check if we've reached the hard fork
inline bool IsProofOfParticipationActive(int nHeight) {
    return nHeight >= HARD_FORK_HEIGHT;
}

// Network ports
inline constexpr int GOLDCOIN_PORT = 8121;        // Main network
inline constexpr int GOLDCOIN_TESTNET_PORT = 18121; // Test network
inline constexpr int GOLDCOIN_RPC_PORT = 8122;    // RPC port

// Genesis Block Message - MicroGuy's vision
inline constexpr const char* GENESIS_BLOCK_MESSAGE = 
    "Goldcoin 2025: The Evolution - From Proof of Work to Proof of Participation. "
    "Energy efficient, truly decentralized, the greatest form of money in the galaxy. "
    "What would Satoshi do? This. -MicroGuy";

// Goldcoin Address Prefixes
inline constexpr unsigned char PUBKEY_ADDRESS = 32;      // 'G' addresses
inline constexpr unsigned char SCRIPT_ADDRESS = 5;       // '3' for multisig
inline constexpr unsigned char PUBKEY_ADDRESS_TEST = 111; // 'm' or 'n' for testnet

// DNS Seeds for Goldcoin network
inline const char* DNS_SEEDS[] = {
    "seed.goldcoin.org",
    "seed2.goldcoin.org", 
    "dnsseed.goldcoin.io",
    nullptr
};

// Checkpoint system (will be updated with actual Goldcoin checkpoints)
struct Checkpoint {
    int height;
    uint256 hash;
};

// Key checkpoints in Goldcoin history
inline const Checkpoint CHECKPOINTS[] = {
    {0, uint256("0x...")},        // Genesis
    {100000, uint256("0x...")},   // First 100k blocks
    {500000, uint256("0x...")},   // Half million
    {1000000, uint256("0x...")},  // 1 million
    {2000000, uint256("0x...")},  // 2 million
    {3000000, uint256("0x...")},  // 3 million
    {HARD_FORK_HEIGHT, uint256("0x...")}, // PoP activation
};

// Version information
inline std::string GetVersionString() {
    return std::string(COIN_NAME) + " " + HARD_FORK_VERSION + " (Proof of Participation)";
}

// Chain ID for replay protection
inline constexpr int GOLDCOIN_CHAIN_ID = 8121; // Using port number as chain ID

// MicroGuy's vision statement
inline constexpr const char* VISION = R"(
Goldcoin represents the evolution of money. Born in 2013 from the 
Bitcointalk forums, it has grown from a simple cryptocurrency to 
the most advanced form of digital gold. With Proof of Participation,
we honor Satoshi's vision while eliminating the waste. Every holder
can participate. No massive mining farms. No wasted energy. Just
pure, democratic consensus. This is what money should be.
- MicroGuy, Creator of Goldcoin
)";

} // namespace goldcoin

#endif // GOLDCOIN_H