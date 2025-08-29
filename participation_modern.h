// Copyright (c) 2025 MicroGuy / Goldcoin Developers  
// Proof of Participation - Modern C++23 Implementation
// Built on MicroGuy's Bitcoin refactoring foundation

#pragma once

#include "bitcoin.h"
#include "core.h"
#include "crypto_modern.h"
#include "util_modern.h"
#include <ranges>
#include <expected>
#include <chrono>
#include <random>
#include <unordered_map>
#include <vector>

namespace goldcoin::pop {

// Modern PoP constants using bitcoin namespace types
inline constexpr bitcoin::amount_t MINIMUM_STAKE = 1000 * bitcoin::COIN;
inline constexpr bitcoin::height_t STAKE_MATURITY_BLOCKS = 1440;  // 6 days
inline constexpr std::chrono::seconds BLOCK_TARGET_TIME{120};     // 2 minutes
inline constexpr bitcoin::height_t POP_ACTIVATION_HEIGHT = 3'500'000;

// Participation requirements (hardened security)
struct ParticipationRequirements {
    static constexpr int MIN_TRANSACTIONS = 10;
    static constexpr int MIN_UNIQUE_COUNTERPARTIES = 5; 
    static constexpr std::chrono::days MAX_INACTIVITY{90};
    static constexpr int MAX_NODES_PER_SUBNET = 2;
};

// Modern error handling
enum class ParticipationError {
    InsufficientStake,
    ImmatureCoins,
    InsufficientActivity,
    TooManySubnetNodes,
    InvalidVRF,
    NetworkError
};

// Participation score calculation
class ParticipationValidator {
public:
    struct WalletMetrics {
        bitcoin::amount_t balance;
        bitcoin::height_t coin_age_blocks;
        int transaction_count;
        int unique_counterparties;
        std::chrono::system_clock::time_point last_transaction;
        std::chrono::system_clock::time_point first_seen;
        double uptime_ratio;
        int transactions_relayed;
        bitcoin::core::NetAddr ip_address;
    };
    
    struct ParticipationScore {
        double base_score{0.0};
        double coin_age_bonus{0.0};
        double activity_bonus{0.0};
        double diversity_penalty{0.0};
        double final_weight{0.0};
        bool eligible{false};
        std::string reason;
    };

    [[nodiscard]] static std::expected<ParticipationScore, ParticipationError>
    CalculateScore(const WalletMetrics& metrics) noexcept;
    
    [[nodiscard]] static std::expected<bool, ParticipationError>
    ValidateParticipation(const WalletMetrics& metrics) noexcept;

private:
    static double CalculateCoinAgeBonus(bitcoin::height_t age_blocks) noexcept;
    static double CalculateActivityScore(int tx_count, int unique_partners) noexcept;
    static double CalculateSubnetPenalty(const bitcoin::core::NetAddr& addr) noexcept;
};

// VRF implementation for fair lottery selection
class VerifiableRandomFunction {
public:
    using Seed = std::array<std::uint8_t, 32>;
    using Proof = std::array<std::uint8_t, 64>;
    using Output = std::array<std::uint8_t, 32>;
    
    struct LotteryResult {
        Output hash;
        Proof proof;
        bool is_winner;
        double probability;
    };

    [[nodiscard]] static std::expected<LotteryResult, ParticipationError>
    ComputeLottery(const Seed& block_seed, 
                   const bitcoin::crypto::PublicKey& participant_key,
                   bitcoin::amount_t total_participating_stake) noexcept;
                   
    [[nodiscard]] static std::expected<bool, ParticipationError>
    VerifyLottery(const LotteryResult& result,
                  const Seed& block_seed,
                  const bitcoin::crypto::PublicKey& participant_key) noexcept;

private:
    static Output HashToTarget(const Seed& seed, const bitcoin::crypto::PublicKey& key) noexcept;
    static bool IsWinningHash(const Output& hash, double target_threshold) noexcept;
};

// Adaptive IP filtering system
class IPClusteringDetector {
public:
    enum class SubnetClass : int {
        ClassC = 24,   // /24 - default
        Block20 = 20,  // /20 - suspicious clustering
        Block16 = 16   // /16 - attack pattern
    };
    
    struct ClusterAnalysis {
        SubnetClass recommended_mask;
        int node_count_in_subnet;
        std::vector<bitcoin::core::NetAddr> clustered_nodes;
        bool suspicious_pattern;
        std::string analysis;
    };
    
    [[nodiscard]] ClusterAnalysis AnalyzeIPClustering(
        const bitcoin::core::NetAddr& new_node,
        const std::vector<bitcoin::core::NetAddr>& existing_nodes) const noexcept;
        
    [[nodiscard]] bool ShouldAllowNode(
        const bitcoin::core::NetAddr& addr,
        const WalletMetrics& metrics,
        const ClusterAnalysis& analysis) const noexcept;

private:
    std::uint32_t GetSubnetMask(const bitcoin::core::NetAddr& addr, SubnetClass mask_bits) const noexcept;
    int CountNodesInSubnet(const bitcoin::core::NetAddr& addr, 
                          SubnetClass mask_bits,
                          const std::vector<bitcoin::core::NetAddr>& nodes) const noexcept;
};

// Main PoP consensus engine
class ProofOfParticipation {
public:
    struct BlockCandidate {
        std::vector<bitcoin::core::Transaction> transactions;
        bitcoin::timestamp_t timestamp;
        bitcoin::core::NetAddr producer_addr;
        bitcoin::crypto::PublicKey producer_key;
        VerifiableRandomFunction::LotteryResult lottery_proof;
        bitcoin::amount_t total_fees;
    };
    
    struct NetworkState {
        std::vector<WalletMetrics> participating_nodes;
        bitcoin::amount_t total_participating_stake;
        bitcoin::height_t current_height;
        std::chrono::system_clock::time_point last_block_time;
        IPClusteringDetector::ClusterAnalysis cluster_analysis;
    };

    explicit ProofOfParticipation(NetworkState initial_state) noexcept;
    
    // Core PoP operations
    [[nodiscard]] std::expected<BlockCandidate, ParticipationError>
    TryGenerateBlock(const bitcoin::crypto::KeyPair& local_keys) noexcept;
    
    [[nodiscard]] std::expected<bool, ParticipationError>
    ValidateBlock(const BlockCandidate& candidate) noexcept;
    
    [[nodiscard]] std::expected<std::vector<WalletMetrics>, ParticipationError>
    GetEligibleParticipants() noexcept;
    
    // Network state management
    void UpdateNetworkState(const NetworkState& new_state) noexcept;
    [[nodiscard]] const NetworkState& GetNetworkState() const noexcept { return state_; }
    
    // Statistics and monitoring
    struct Stats {
        int total_participants;
        int eligible_participants; 
        bitcoin::amount_t average_stake;
        double network_decentralization_index;
        std::chrono::milliseconds average_block_time;
        int suspicious_ip_clusters;
    };
    
    [[nodiscard]] Stats GetNetworkStats() const noexcept;

private:
    NetworkState state_;
    ParticipationValidator validator_;
    VerifiableRandomFunction vrf_;
    IPClusteringDetector ip_detector_;
    
    // Internal helpers
    [[nodiscard]] VerifiableRandomFunction::Seed GenerateBlockSeed() const noexcept;
    [[nodiscard]] double CalculateWinProbability(bitcoin::amount_t stake, bitcoin::amount_t total) const noexcept;
    [[nodiscard]] bool IsBlockTimeValid(std::chrono::system_clock::time_point block_time) const noexcept;
};

// Integration with existing Bitcoin infrastructure
class PopConsensusAdapter {
public:
    // Interface with main.cpp
    static void InitializePoP() noexcept;
    static bool ProcessPopBlock(const bitcoin::core::Block& block) noexcept;
    static bool GeneratePopBlock(bitcoin::core::Block& block) noexcept;
    
    // Interface with net.cpp  
    static void OnNewPeer(const bitcoin::core::NetAddr& addr) noexcept;
    static void OnPeerDisconnect(const bitcoin::core::NetAddr& addr) noexcept;
    
    // Interface with rpc.cpp
    static bitcoin::util::JsonValue GetParticipationInfo() noexcept;
    static bitcoin::util::JsonValue GetNetworkHealth() noexcept;
    
private:
    static inline std::unique_ptr<ProofOfParticipation> pop_engine_;
    static inline std::mutex pop_mutex_;
};

} // namespace goldcoin::pop