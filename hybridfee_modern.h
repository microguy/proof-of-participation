// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Goldcoin Hybrid Fee System - Modern C++23 Implementation
// 5% free zone + variable pricing for remaining 95%

#pragma once

#include "bitcoin.h"
#include "core.h"
#include "goldcoin_consensus.h"
#include <ranges>
#include <expected>
#include <chrono>
#include <vector>
#include <queue>
#include <unordered_map>

namespace goldcoin::fees {

// Fee system error types
enum class FeeError {
    InvalidTransaction,
    BlockFull,
    InsufficientFee,
    PriorityTooLow,
    NetworkError
};

// Transaction priority calculation (Satoshi's formula)
class PriorityCalculator {
public:
    struct InputInfo {
        bitcoin::amount_t value;
        bitcoin::height_t confirmations;
        std::chrono::system_clock::time_point time_received;
    };
    
    struct PriorityResult {
        double priority_score;
        bool qualifies_for_free;
        bitcoin::amount_t suggested_fee;
        std::string category;  // "free", "low_fee", "standard", "priority"
    };
    
    [[nodiscard]] static std::expected<PriorityResult, FeeError>
    CalculatePriority(const std::vector<InputInfo>& inputs, 
                     std::size_t transaction_size_bytes) noexcept;
    
private:
    // Goldcoin's free transaction threshold (Satoshi's original formula)
    static constexpr double FREE_TX_PRIORITY_THRESHOLD = 57'600'000.0;
    
    [[nodiscard]] static double ComputeInputPriority(const InputInfo& input) noexcept;
};

// Block space allocation manager
class BlockSpaceManager {
public:
    struct TransactionCandidate {
        bitcoin::core::Transaction tx;
        PriorityCalculator::PriorityResult priority_info;
        bitcoin::amount_t fee_paid;
        std::chrono::system_clock::time_point received_time;
        std::size_t size_bytes;
    };
    
    struct BlockTemplate {
        std::vector<TransactionCandidate> free_transactions;      // 5% zone
        std::vector<TransactionCandidate> fee_transactions;       // 95% zone
        std::size_t total_size_bytes;
        bitcoin::amount_t total_fees_collected;
        int free_zone_utilization_percent;
        int total_utilization_percent;
    };
    
    BlockSpaceManager() noexcept;
    
    // Core functionality
    [[nodiscard]] std::expected<BlockTemplate, FeeError>
    BuildBlockTemplate(const std::vector<TransactionCandidate>& mempool) noexcept;
    
    [[nodiscard]] std::expected<bitcoin::amount_t, FeeError>
    GetRecommendedFee(std::size_t tx_size_bytes, 
                     const PriorityCalculator::PriorityResult& priority) noexcept;
    
    // Statistics and monitoring
    struct FeeMarketStats {
        bitcoin::amount_t current_min_fee_rate;    // satoshis per byte
        int free_zone_pressure_percent;            // 0-100%
        int average_confirmation_blocks_free;       // for free transactions
        int average_confirmation_blocks_paid;       // for fee transactions
        bitcoin::amount_t median_fee_last_block;
        std::vector<bitcoin::amount_t> fee_percentiles;  // 25%, 50%, 75%, 95%
    };
    
    [[nodiscard]] FeeMarketStats GetMarketStats() const noexcept;
    
private:
    // Goldcoin block space allocation
    static constexpr std::size_t MAX_BLOCK_SIZE = goldcoin::consensus::MAX_BLOCK_SIZE;
    static constexpr std::size_t FREE_ZONE_SIZE = MAX_BLOCK_SIZE * 5 / 100;  // 5% = 1.6MB
    static constexpr std::size_t FEE_ZONE_SIZE = MAX_BLOCK_SIZE - FREE_ZONE_SIZE;  // 95% = 30.4MB
    
    // Fee rate calculation
    static constexpr bitcoin::amount_t BASE_FEE_RATE = 1000;  // 0.00001 GLC per byte
    static constexpr bitcoin::amount_t MIN_RELAY_FEE = 100'000;  // 0.001 GLC minimum
    
    // Current market state
    mutable std::mutex market_mutex_;
    FeeMarketStats current_stats_;
    std::queue<BlockTemplate> recent_blocks_;  // For statistics
    
    // Internal helpers
    [[nodiscard]] std::vector<TransactionCandidate> 
    SelectFreeTransactions(const std::vector<TransactionCandidate>& candidates) noexcept;
    
    [[nodiscard]] std::vector<TransactionCandidate>
    SelectFeeTransactions(const std::vector<TransactionCandidate>& remaining,
                         std::size_t available_space) noexcept;
    
    [[nodiscard]] bitcoin::amount_t
    CalculateDynamicFeeRate(int congestion_level) const noexcept;
    
    void UpdateMarketStats(const BlockTemplate& completed_block) noexcept;
};

// Fee estimation for wallets
class FeeEstimator {
public:
    enum class ConfirmationTarget {
        NextBlock = 1,      // Highest fee, immediate confirmation
        Fast = 3,           // 6 minutes average  
        Standard = 6,       // 12 minutes average
        Economy = 12        // 24 minutes average
    };
    
    struct FeeEstimate {
        bitcoin::amount_t total_fee;
        bitcoin::amount_t fee_rate;  // satoshis per byte
        ConfirmationTarget target;
        bool likely_free;
        double confidence_percent;
        std::string explanation;
    };
    
    [[nodiscard]] static std::expected<FeeEstimate, FeeError>
    EstimateFee(std::size_t transaction_size_bytes,
               const PriorityCalculator::PriorityResult& priority,
               ConfirmationTarget target = ConfirmationTarget::Standard) noexcept;
    
    // Historical fee analysis
    [[nodiscard]] static std::vector<bitcoin::amount_t>
    GetRecentFeeRates(std::chrono::hours lookback_period = std::chrono::hours{24}) noexcept;
    
private:
    static bitcoin::amount_t CalculateTargetFee(ConfirmationTarget target, 
                                               std::size_t tx_size) noexcept;
};

// Mempool management with fee-aware organization  
class FeeAwareMempool {
public:
    struct MempoolEntry {
        bitcoin::core::Transaction transaction;
        PriorityCalculator::PriorityResult priority;
        bitcoin::amount_t fee_rate;
        std::chrono::system_clock::time_point entry_time;
        int ancestors_count;
        int descendants_count;
    };
    
    // Mempool operations
    [[nodiscard]] std::expected<void, FeeError>
    AddTransaction(const bitcoin::core::Transaction& tx) noexcept;
    
    [[nodiscard]] std::expected<void, FeeError>
    RemoveTransaction(const bitcoin::hash256_t& txid) noexcept;
    
    // Organized retrieval for block building
    [[nodiscard]] std::vector<MempoolEntry> 
    GetHighPriorityTransactions(std::size_t max_count = 1000) noexcept;
    
    [[nodiscard]] std::vector<MempoolEntry>
    GetFeePayingTransactions(bitcoin::amount_t min_fee_rate) noexcept;
    
    // Mempool statistics
    struct MempoolStats {
        std::size_t total_transactions;
        std::size_t free_eligible_count;
        std::size_t fee_paying_count;
        bitcoin::amount_t total_fees;
        std::size_t total_size_bytes;
        double average_priority;
    };
    
    [[nodiscard]] MempoolStats GetStats() const noexcept;
    
private:
    mutable std::shared_mutex mempool_mutex_;
    std::unordered_map<bitcoin::hash256_t, MempoolEntry> transactions_;
    
    // Organized indices for efficient retrieval
    std::multimap<double, bitcoin::hash256_t> priority_index_;    // priority -> txid
    std::multimap<bitcoin::amount_t, bitcoin::hash256_t> fee_index_;      // fee_rate -> txid
    
    void UpdateIndices(const bitcoin::hash256_t& txid, const MempoolEntry& entry) noexcept;
    void RemoveFromIndices(const bitcoin::hash256_t& txid) noexcept;
};

// RPC interface for fee information
class FeeRPC {
public:
    // Get current fee recommendations
    [[nodiscard]] static bitcoin::util::JsonValue 
    GetFeeEstimate(const std::vector<std::string>& params) noexcept;
    
    // Get mempool fee information
    [[nodiscard]] static bitcoin::util::JsonValue
    GetMempoolInfo() noexcept;
    
    // Get block template with fee information
    [[nodiscard]] static bitcoin::util::JsonValue
    GetBlockTemplate() noexcept;
    
    // Get fee market statistics
    [[nodiscard]] static bitcoin::util::JsonValue
    GetFeeMarketStats() noexcept;
};

// Integration with PoP consensus
class PopFeeIntegration {
public:
    // Called by PoP block generator
    [[nodiscard]] static std::expected<BlockSpaceManager::BlockTemplate, FeeError>
    PrepareBlockTransactions() noexcept;
    
    // Called after block is accepted
    static void OnBlockAccepted(const bitcoin::core::Block& block) noexcept;
    
    // Validate fee rules in PoP context
    [[nodiscard]] static std::expected<bool, FeeError>
    ValidateBlockFees(const bitcoin::core::Block& block) noexcept;
    
private:
    static inline std::unique_ptr<BlockSpaceManager> block_manager_;
    static inline std::unique_ptr<FeeAwareMempool> mempool_;
    static inline std::mutex integration_mutex_;
};

} // namespace goldcoin::fees