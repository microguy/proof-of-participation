// Copyright (c) 2013-2025 Goldcoin Developers
// Copyright (c) 2025 MicroGuy / Goldcoin PoP Developers
// Goldcoin Consensus Rules - PoP Implementation

#pragma once

#include "bitcoin.h"
#include <chrono>

namespace goldcoin::consensus {

// Goldcoin Supply Constants
inline constexpr bitcoin::amount_t MAX_MONEY = 1'172'245'700LL * bitcoin::COIN;
inline constexpr bitcoin::amount_t TREASURY_RESERVE = 1'100'000'000LL * bitcoin::COIN;
inline constexpr bitcoin::amount_t CIRCULATING_SUPPLY = MAX_MONEY - TREASURY_RESERVE;

// Goldcoin Block Parameters (current network)
inline constexpr std::size_t MAX_BLOCK_SIZE = 32'000'000;  // 32MB (not Bitcoin's 1MB!)
inline constexpr std::size_t MAX_BLOCK_SIGOPS = 40'000 * 16;  // As per consensus.h
inline constexpr std::chrono::seconds TARGET_SPACING{120};  // 2 minutes (not Bitcoin's 10)

// Network Performance Targets
inline constexpr int TARGET_TPS = 1120;  // Transactions per second
inline constexpr bool ZERO_FEES_DEFAULT = true;  // Goldcoin's key feature

// PoP Activation Parameters
inline constexpr bitcoin::height_t POP_ACTIVATION_HEIGHT = 3'500'000;  // Future fork height
inline constexpr bitcoin::height_t DIFFICULTY_ADJUSTMENT_INTERVAL = 60;  // Every 60 blocks (not 2016)

// PoP Consensus Parameters 
inline constexpr bitcoin::amount_t MINIMUM_PARTICIPATION_STAKE = 1000 * bitcoin::COIN;
inline constexpr bitcoin::height_t STAKE_MATURITY_PERIOD = 1440;  // 6 days at 2min blocks = 2 days

// Network Identity
inline constexpr int DEFAULT_PORT = 8121;  // Goldcoin main network port
inline constexpr int TESTNET_PORT = 18121;
inline constexpr int REGTEST_PORT = 18130;

// Historical Fork Heights (preserve Goldcoin history)
struct ForkHeights {
    static constexpr bitcoin::height_t JULY_FORK = 45'000;
    static constexpr bitcoin::height_t OCTOBER_FORK = 100'000;
    static constexpr bitcoin::height_t NOVEMBER_FORK_1 = 103'000;
    static constexpr bitcoin::height_t NOVEMBER_FORK_2 = 118'800;
    static constexpr bitcoin::height_t MAY_FORK = 248'000;
    static constexpr bitcoin::height_t JULY_FORK_2 = 251'230;
    static constexpr bitcoin::height_t FEBRUARY_FORK = 372'000;
    static constexpr bitcoin::height_t POP_FORK = POP_ACTIVATION_HEIGHT;  // Our fork!
};

// Genesis Block (Goldcoin launch: May 14, 2013)
inline constexpr std::uint32_t GENESIS_TIMESTAMP = 1368576000;  // May 14, 2013
inline constexpr std::string_view GENESIS_MESSAGE = "May 15 2013 bitcointalk.org Goldcoin launch";

// Hybrid Fee System (Goldcoin's 5% free space model)
inline constexpr std::size_t FREE_TRANSACTION_ZONE = MAX_BLOCK_SIZE * 5 / 100;  // 5% = 1.6MB
inline constexpr bitcoin::amount_t MIN_FEE_WHEN_REQUIRED = 100'000;  // 0.001 GLC per KB
inline constexpr double FREE_TX_PRIORITY_THRESHOLD = 57'600'000.0;  // Satoshi's formula

// AI Autonomy Timeline (Goldcoin's roadmap)
struct AIAutonomyPhases {
    static constexpr int PHASE_1_YEAR = 2025;
    static constexpr int PHASE_1_AI_PERCENTAGE = 60;
    
    static constexpr int PHASE_2_YEAR = 2026; 
    static constexpr int PHASE_2_AI_PERCENTAGE = 90;
    
    static constexpr int PHASE_3_YEAR = 2027;
    static constexpr int PHASE_3_AI_PERCENTAGE = 100;  // Fully autonomous
};

// Goldcoin vs Bitcoin Key Differences
struct NetworkComparison {
    // Goldcoin advantages
    static constexpr bool ASIC_RESISTANT = true;      // Originally Scrypt, now PoP
    static constexpr bool ZERO_FEES = true;           // vs Bitcoin's high fees
    static constexpr bool FAST_BLOCKS = true;         // 2min vs Bitcoin's 10min
    static constexpr bool LARGE_BLOCKS = true;        // 32MB vs Bitcoin's 1MB
    static constexpr bool AI_AUTONOMOUS = true;       // Unique to Goldcoin
    static constexpr double ENERGY_REDUCTION = 0.9999; // 99.99% less than Bitcoin
};

// Proof of Participation Specific Rules
namespace pop {
    // Equal lottery system - key principle
    inline constexpr bool EQUAL_CHANCES_REGARDLESS_OF_STAKE = true;
    
    // Participation requirements (hardened security)
    inline constexpr int MIN_TRANSACTION_HISTORY = 10;
    inline constexpr int MIN_UNIQUE_COUNTERPARTIES = 5;
    inline constexpr std::chrono::days MAX_INACTIVITY{90};
    
    // IP filtering (adaptive)
    inline constexpr int MAX_NODES_PER_CLASS_C = 2;
    inline constexpr int EXPAND_TO_CLASS_B_THRESHOLD = 10;
    inline constexpr int BLOCK_CLASS_B_THRESHOLD = 50;
    
    // Economic security
    inline constexpr double ATTACK_COST_MULTIPLIER = 1000.0;  // 1000x harder than simple stake
}

} // namespace goldcoin::consensus