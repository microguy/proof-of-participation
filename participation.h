// Copyright (c) 2025 GoldcoinPoP Developers
// Distributed under the MIT/X11 software license
// Proof of Participation - Following Satoshi's Vision

#ifndef GOLDCOIN_PARTICIPATION_H
#define GOLDCOIN_PARTICIPATION_H

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <concepts>
#include <ranges>
#include "uint256.h"
#include "bignum.h"

// C++23 features for clean, modern code
namespace goldcoin {

// Satoshi would appreciate the elegance: one stake, one vote
inline constexpr int64_t MINIMUM_STAKE = 100 * COIN; // 100 GLC
inline constexpr int STAKE_MATURITY = 1440; // blocks before stake is active
inline constexpr int BLOCK_TIME_SECONDS = 120;

// Participation entry - simple and transparent
struct ParticipationEntry {
    uint256 txid;          // Transaction that locked the stake
    int64_t amount;        // Amount staked (must be >= MINIMUM_STAKE)
    uint160 address;       // Participant's address  
    int height;            // Block height when staked
    
    [[nodiscard]] auto isMatured(int currentHeight) const noexcept -> bool {
        return (currentHeight - height) >= STAKE_MATURITY;
    }
    
    // C++23 spaceship operator for easy comparison
    auto operator<=>(const ParticipationEntry&) const = default;
};

// Verifiable Random Function - deterministic but unpredictable
class VRF {
private:
    uint256 seed;
    
public:
    explicit VRF(const uint256& previousBlockHash) : seed(previousBlockHash) {}
    
    // Select winner from participants - pure, no side effects
    [[nodiscard]] auto selectWinner(std::ranges::range auto const& participants, 
                                   int64_t blockHeight) const -> std::optional<ParticipationEntry> {
        if (participants.empty()) return std::nullopt;
        
        // Combine previous block hash with height for randomness
        uint256 selection = Hash(seed.begin(), seed.end(), 
                                BEGIN(blockHeight), END(blockHeight));
        
        // Simple modulo selection - every participant has equal chance
        size_t winnerIndex = selection.Get64() % participants.size();
        return participants[winnerIndex];
    }
};

// The new "miner" - actually a participation validator
class ParticipationValidator {
private:
    std::vector<ParticipationEntry> activeParticipants;
    mutable std::mutex participantMutex;
    
public:
    // Add a new participant (when they lock stake)
    void addParticipant(ParticipationEntry entry) {
        std::lock_guard lock(participantMutex);
        activeParticipants.push_back(std::move(entry));
    }
    
    // Remove participant (when they unlock stake)  
    void removeParticipant(const uint256& txid) {
        std::lock_guard lock(participantMutex);
        std::erase_if(activeParticipants, 
                     [&txid](const auto& p) { return p.txid == txid; });
    }
    
    // Get all matured participants
    [[nodiscard]] auto getActiveParticipants(int currentHeight) const 
        -> std::vector<ParticipationEntry> {
        std::lock_guard lock(participantMutex);
        
        auto matured = activeParticipants 
            | std::views::filter([currentHeight](const auto& p) {
                return p.isMatured(currentHeight);
              })
            | std::ranges::to<std::vector>();
            
        return matured;
    }
    
    // Check if we won the right to create next block
    [[nodiscard]] auto checkParticipation(const uint256& myAddress,
                                         const uint256& previousBlockHash,
                                         int currentHeight) const -> bool {
        auto participants = getActiveParticipants(currentHeight);
        if (participants.empty()) return false;
        
        VRF vrf(previousBlockHash);
        auto winner = vrf.selectWinner(participants, currentHeight);
        
        return winner.has_value() && 
               winner->address == uint160(myAddress);
    }
};

// Global participation validator instance
inline ParticipationValidator g_participationValidator;

} // namespace goldcoin

#endif // GOLDCOIN_PARTICIPATION_H