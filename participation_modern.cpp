// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Proof of Participation - Modern C++23 Implementation  
// Following Satoshi's principles with modern C++

#include "participation_modern.h"
#include "main_modern.h"
#include "net_modern.h"
#include "crypto_modern.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace goldcoin::pop {

// ParticipationValidator Implementation
std::expected<ParticipationValidator::ParticipationScore, ParticipationError>
ParticipationValidator::CalculateScore(const WalletMetrics& metrics) noexcept {
    ParticipationScore score;
    
    try {
        // Basic eligibility checks
        auto validation = ValidateParticipation(metrics);
        if (!validation) {
            score.eligible = false;
            score.reason = "Failed basic validation";
            return score;
        }
        
        score.eligible = true;
        
        // Calculate base score from transaction activity
        score.base_score = CalculateActivityScore(metrics.transaction_count, 
                                                metrics.unique_counterparties);
        
        // Coin age bonus (rewards long-term holders)
        score.coin_age_bonus = CalculateCoinAgeBonus(metrics.coin_age_blocks);
        
        // Network diversity penalty (for IP clustering)
        score.diversity_penalty = CalculateSubnetPenalty(metrics.ip_address);
        
        // Activity bonus for recent network participation
        auto now = std::chrono::system_clock::now();
        auto days_since_tx = std::chrono::duration_cast<std::chrono::days>(
            now - metrics.last_transaction).count();
        
        if (days_since_tx < 30) {
            score.activity_bonus = 2.0;
        } else if (days_since_tx < 60) {
            score.activity_bonus = 1.0;
        }
        
        // Penalize perfect uptime (likely bots)
        double uptime_factor = metrics.uptime_ratio > 0.98 ? 0.9 : 1.0;
        
        // Calculate final weighted score
        score.final_weight = (score.base_score + score.coin_age_bonus + score.activity_bonus)
                           * (1.0 - score.diversity_penalty) * uptime_factor;
        
        // Ensure minimum participation chance (permissionless principle)
        score.final_weight = std::max(score.final_weight, 0.01);
        
        return score;
        
    } catch (...) {
        return std::unexpected(ParticipationError::NetworkError);
    }
}

std::expected<bool, ParticipationError>
ParticipationValidator::ValidateParticipation(const WalletMetrics& metrics) noexcept {
    // Must have minimum stake
    if (metrics.balance < MINIMUM_STAKE) {
        return std::unexpected(ParticipationError::InsufficientStake);
    }
    
    // Coins must be mature (prevents rapid reshuffling attacks)
    if (metrics.coin_age_blocks < STAKE_MATURITY_BLOCKS) {
        return std::unexpected(ParticipationError::ImmatureCoins);
    }
    
    // Must have transaction history (proof of genuine participation)
    if (metrics.transaction_count < ParticipationRequirements::MIN_TRANSACTIONS) {
        return std::unexpected(ParticipationError::InsufficientActivity);
    }
    
    // Must have interacted with multiple unique addresses
    if (metrics.unique_counterparties < ParticipationRequirements::MIN_UNIQUE_COUNTERPARTIES) {
        return std::unexpected(ParticipationError::InsufficientActivity);
    }
    
    // Must have recent activity (prevents dormant wallets)
    auto now = std::chrono::system_clock::now();
    if (now - metrics.last_transaction > ParticipationRequirements::MAX_INACTIVITY) {
        return std::unexpected(ParticipationError::InsufficientActivity);
    }
    
    return true;
}

double ParticipationValidator::CalculateCoinAgeBonus(bitcoin::height_t age_blocks) noexcept {
    // Logarithmic bonus for coin age (max 10 points for ~1 year)  
    return std::min(10.0, std::log10(age_blocks / 1440.0 + 1) * 5.0);
}

double ParticipationValidator::CalculateActivityScore(int tx_count, int unique_partners) noexcept {
    // Score based on transaction diversity (harder for attackers to fake)
    double tx_score = std::min(5.0, tx_count / 20.0);
    double diversity_score = std::min(5.0, unique_partners / 10.0);
    return tx_score + diversity_score;
}

double ParticipationValidator::CalculateSubnetPenalty(const bitcoin::core::NetAddr& addr) noexcept {
    // This would integrate with the IP clustering detector
    // For now, return no penalty - will be calculated by IPClusteringDetector
    return 0.0;
}

// VRF Implementation
std::expected<VerifiableRandomFunction::LotteryResult, ParticipationError>
VerifiableRandomFunction::ComputeLottery(
    const Seed& block_seed,
    const bitcoin::crypto::PublicKey& participant_key,
    bitcoin::amount_t total_participating_stake) noexcept {
    
    try {
        LotteryResult result;
        
        // Generate deterministic but unpredictable hash
        result.hash = HashToTarget(block_seed, participant_key);
        
        // Calculate target threshold (proportional to stake)
        // With equal lottery chances, target is same for all
        double target_threshold = 1.0 / 1000.0;  // Adjust based on expected participants
        
        // Check if this is a winning hash
        result.is_winner = IsWinningHash(result.hash, target_threshold);
        result.probability = target_threshold;
        
        // Generate proof (simplified - in production use proper VRF)
        std::ranges::copy(result.hash, result.proof.begin());
        std::ranges::fill_n(result.proof.begin() + 32, 32, 0);
        
        return result;
        
    } catch (...) {
        return std::unexpected(ParticipationError::InvalidVRF);
    }
}

std::expected<bool, ParticipationError>
VerifiableRandomFunction::VerifyLottery(
    const LotteryResult& result,
    const Seed& block_seed, 
    const bitcoin::crypto::PublicKey& participant_key) noexcept {
    
    try {
        // Recompute hash and verify it matches
        auto expected_hash = HashToTarget(block_seed, participant_key);
        return std::ranges::equal(result.hash, expected_hash);
        
    } catch (...) {
        return std::unexpected(ParticipationError::InvalidVRF);
    }
}

VerifiableRandomFunction::Output 
VerifiableRandomFunction::HashToTarget(const Seed& seed, const bitcoin::crypto::PublicKey& key) noexcept {
    // Combine block seed with participant's public key
    // This ensures deterministic but unpredictable selection
    Output result{};
    
    // Simple hash combination (in production, use proper VRF construction)
    bitcoin::crypto::SHA256 hasher;
    hasher.Write(seed.data(), seed.size());
    hasher.Write(key.data(), key.size());
    
    auto hash = hasher.Finalize();
    std::ranges::copy_n(hash.data(), result.size(), result.begin());
    
    return result;
}

bool VerifiableRandomFunction::IsWinningHash(const Output& hash, double target_threshold) noexcept {
    // Convert hash to probability value
    std::uint64_t hash_value = 0;
    for (int i = 0; i < 8; ++i) {
        hash_value = (hash_value << 8) | hash[i];
    }
    
    double probability = static_cast<double>(hash_value) / 
                        static_cast<double>(std::numeric_limits<std::uint64_t>::max());
    
    return probability < target_threshold;
}

// IP Clustering Detector Implementation  
IPClusteringDetector::ClusterAnalysis
IPClusteringDetector::AnalyzeIPClustering(
    const bitcoin::core::NetAddr& new_node,
    const std::vector<bitcoin::core::NetAddr>& existing_nodes) const noexcept {
    
    ClusterAnalysis analysis;
    analysis.recommended_mask = SubnetClass::ClassC;
    analysis.suspicious_pattern = false;
    
    // Count nodes in Class C subnet (/24)
    int nodes_in_c = CountNodesInSubnet(new_node, SubnetClass::ClassC, existing_nodes);
    analysis.node_count_in_subnet = nodes_in_c;
    
    if (nodes_in_c > 2) {
        // Suspicious: Multiple nodes in same Class C
        analysis.suspicious_pattern = true;
        analysis.recommended_mask = SubnetClass::Block20;
        analysis.analysis = std::format("Suspicious: {} nodes in /24 subnet", nodes_in_c);
        
        // Check broader pattern
        int nodes_in_20 = CountNodesInSubnet(new_node, SubnetClass::Block20, existing_nodes);
        if (nodes_in_20 > 10) {
            analysis.recommended_mask = SubnetClass::Block16;
            analysis.analysis = std::format("Attack pattern: {} nodes in /20 block", nodes_in_20);
        }
    } else {
        analysis.analysis = std::format("Normal: {} nodes in /24 subnet", nodes_in_c);
    }
    
    return analysis;
}

bool IPClusteringDetector::ShouldAllowNode(
    const bitcoin::core::NetAddr& addr,
    const WalletMetrics& metrics,
    const ClusterAnalysis& analysis) const noexcept {
    
    // Always allow if not suspicious
    if (!analysis.suspicious_pattern) {
        return true;
    }
    
    // In suspicious subnets, prioritize by coin age
    // This rewards long-term participants over new attackers
    if (metrics.coin_age_blocks > STAKE_MATURITY_BLOCKS * 10) {  // >60 days
        return true;  // Veterans always allowed
    }
    
    // For newer participants, limit based on clustering severity
    switch (analysis.recommended_mask) {
        case SubnetClass::ClassC:
            return analysis.node_count_in_subnet <= 2;
        case SubnetClass::Block20:  
            return analysis.node_count_in_subnet <= 2;  // Only 2 per provider block
        case SubnetClass::Block16:
            return analysis.node_count_in_subnet <= 2;  // Severe restriction
    }
    
    return false;
}

std::uint32_t IPClusteringDetector::GetSubnetMask(
    const bitcoin::core::NetAddr& addr, SubnetClass mask_bits) const noexcept {
    // Extract subnet based on mask bits
    std::uint32_t ip = addr.GetIPv4();  // Assuming IPv4 for now
    std::uint32_t mask = 0xFFFFFFFF << (32 - static_cast<int>(mask_bits));
    return ip & mask;
}

int IPClusteringDetector::CountNodesInSubnet(
    const bitcoin::core::NetAddr& addr,
    SubnetClass mask_bits,
    const std::vector<bitcoin::core::NetAddr>& nodes) const noexcept {
    
    std::uint32_t target_subnet = GetSubnetMask(addr, mask_bits);
    
    return std::ranges::count_if(nodes, [this, target_subnet, mask_bits](const auto& node) {
        return GetSubnetMask(node, mask_bits) == target_subnet;
    });
}

// ProofOfParticipation Implementation
ProofOfParticipation::ProofOfParticipation(NetworkState initial_state) noexcept
    : state_(std::move(initial_state)) {
}

std::expected<ProofOfParticipation::BlockCandidate, ParticipationError>
ProofOfParticipation::TryGenerateBlock(const bitcoin::crypto::KeyPair& local_keys) noexcept {
    try {
        // Generate VRF lottery ticket
        auto seed = GenerateBlockSeed();
        auto lottery = vrf_.ComputeLottery(seed, local_keys.public_key, state_.total_participating_stake);
        if (!lottery) {
            return std::unexpected(lottery.error());
        }
        
        // Check if we won the lottery
        if (!lottery->is_winner) {
            return std::unexpected(ParticipationError::InvalidVRF);  // Not our turn
        }
        
        // We won! Create block candidate
        BlockCandidate candidate;
        candidate.timestamp = std::chrono::system_clock::now();
        candidate.producer_key = local_keys.public_key;
        candidate.lottery_proof = *lottery;
        
        // Add transactions from mempool (this would integrate with existing code)
        // candidate.transactions = CollectPendingTransactions();
        // candidate.total_fees = CalculateTotalFees(candidate.transactions);
        
        return candidate;
        
    } catch (...) {
        return std::unexpected(ParticipationError::NetworkError);
    }
}

std::expected<bool, ParticipationError>
ProofOfParticipation::ValidateBlock(const BlockCandidate& candidate) noexcept {
    try {
        // Verify lottery proof
        auto seed = GenerateBlockSeed();
        auto verification = vrf_.VerifyLottery(candidate.lottery_proof, seed, candidate.producer_key);
        if (!verification || !*verification) {
            return std::unexpected(ParticipationError::InvalidVRF);
        }
        
        // Verify block timing
        if (!IsBlockTimeValid(candidate.timestamp)) {
            return false;
        }
        
        // Additional validations would go here...
        return true;
        
    } catch (...) {
        return std::unexpected(ParticipationError::NetworkError);
    }
}

VerifiableRandomFunction::Seed ProofOfParticipation::GenerateBlockSeed() const noexcept {
    // In production, this would use the previous block hash
    VerifiableRandomFunction::Seed seed{};
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Simple seed generation (improve in production)
    for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<std::uint8_t>((now >> (i * 8)) & 0xFF);
    }
    
    return seed;
}

bool ProofOfParticipation::IsBlockTimeValid(std::chrono::system_clock::time_point block_time) const noexcept {
    auto now = std::chrono::system_clock::now();
    auto time_diff = std::chrono::abs(block_time - now);
    
    // Allow blocks within reasonable time window
    return time_diff < std::chrono::minutes(5);
}

ProofOfParticipation::Stats ProofOfParticipation::GetNetworkStats() const noexcept {
    Stats stats;
    stats.total_participants = state_.participating_nodes.size();
    
    // Count eligible participants
    stats.eligible_participants = std::ranges::count_if(state_.participating_nodes,
        [](const auto& node) {
            return ParticipationValidator::ValidateParticipation(node).value_or(false);
        });
    
    if (stats.total_participants > 0) {
        bitcoin::amount_t total = 0;
        for (const auto& node : state_.participating_nodes) {
            total += node.balance;
        }
        stats.average_stake = total / stats.total_participants;
    }
    
    // Calculate decentralization index (higher = more decentralized)
    // This would be more sophisticated in production
    stats.network_decentralization_index = std::min(1.0, stats.eligible_participants / 1000.0);
    
    return stats;
}

} // namespace goldcoin::pop