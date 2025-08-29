// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Goldcoin Hybrid Fee System Implementation
// Following Satoshi's priority formula with Goldcoin's 5% free zone

#include "hybridfee_modern.h"
#include "util_modern.h" 
#include <algorithm>
#include <cmath>
#include <numeric>
#include <format>

namespace goldcoin::fees {

// PriorityCalculator Implementation
std::expected<PriorityCalculator::PriorityResult, FeeError>
PriorityCalculator::CalculatePriority(
    const std::vector<InputInfo>& inputs, 
    std::size_t transaction_size_bytes) noexcept {
    
    try {
        if (inputs.empty() || transaction_size_bytes == 0) {
            return std::unexpected(FeeError::InvalidTransaction);
        }
        
        PriorityResult result{};
        
        // Calculate total input priority using Satoshi's formula:
        // Priority = Σ(input_value × confirmations) / transaction_size
        double total_priority = 0.0;
        
        for (const auto& input : inputs) {
            total_priority += ComputeInputPriority(input);
        }
        
        // Divide by transaction size (in bytes)
        result.priority_score = total_priority / static_cast<double>(transaction_size_bytes);
        
        // Check if qualifies for free transaction
        result.qualifies_for_free = (result.priority_score >= FREE_TX_PRIORITY_THRESHOLD);
        
        // Categorize and suggest fee
        if (result.qualifies_for_free) {
            result.category = "free";
            result.suggested_fee = 0;
        } else {
            // Calculate suggested fee based on priority deficit
            double priority_ratio = result.priority_score / FREE_TX_PRIORITY_THRESHOLD;
            
            if (priority_ratio > 0.5) {
                result.category = "low_fee";
                result.suggested_fee = transaction_size_bytes * 500;  // 0.000005 GLC per byte
            } else if (priority_ratio > 0.1) {
                result.category = "standard";  
                result.suggested_fee = transaction_size_bytes * 1000; // 0.00001 GLC per byte
            } else {
                result.category = "priority";
                result.suggested_fee = transaction_size_bytes * 2000; // 0.00002 GLC per byte
            }
        }
        
        return result;
        
    } catch (...) {
        return std::unexpected(FeeError::NetworkError);
    }
}

double PriorityCalculator::ComputeInputPriority(const InputInfo& input) noexcept {
    // Satoshi's formula: value (in satoshis) × age (in confirmations)
    return static_cast<double>(input.value) * static_cast<double>(input.confirmations);
}

// BlockSpaceManager Implementation
BlockSpaceManager::BlockSpaceManager() noexcept {
    // Initialize with conservative stats
    current_stats_.current_min_fee_rate = BASE_FEE_RATE;
    current_stats_.free_zone_pressure_percent = 0;
    current_stats_.average_confirmation_blocks_free = 1;
    current_stats_.average_confirmation_blocks_paid = 1;
    current_stats_.median_fee_last_block = 0;
    current_stats_.fee_percentiles = {0, 0, 0, 0}; // 25%, 50%, 75%, 95%
}

std::expected<BlockSpaceManager::BlockTemplate, FeeError>
BlockSpaceManager::BuildBlockTemplate(const std::vector<TransactionCandidate>& mempool) noexcept {
    try {
        BlockTemplate template_block{};
        
        // Step 1: Select high-priority transactions for free zone (5%)
        template_block.free_transactions = SelectFreeTransactions(mempool);
        
        // Calculate used space in free zone
        std::size_t free_zone_used = std::accumulate(
            template_block.free_transactions.begin(),
            template_block.free_transactions.end(),
            0UZ,
            [](std::size_t sum, const auto& tx) { return sum + tx.size_bytes; }
        );
        
        // Step 2: Select fee-paying transactions for remaining space
        std::size_t remaining_space = MAX_BLOCK_SIZE - free_zone_used;
        
        // Get candidates not already in free zone
        std::vector<TransactionCandidate> remaining_candidates;
        std::ranges::copy_if(mempool, std::back_inserter(remaining_candidates),
            [&](const auto& candidate) {
                return !std::ranges::any_of(template_block.free_transactions,
                    [&](const auto& free_tx) {
                        return free_tx.tx.GetHash() == candidate.tx.GetHash();
                    });
            });
        
        template_block.fee_transactions = SelectFeeTransactions(remaining_candidates, remaining_space);
        
        // Calculate totals
        template_block.total_size_bytes = free_zone_used;
        template_block.total_fees_collected = 0;
        
        for (const auto& tx : template_block.fee_transactions) {
            template_block.total_size_bytes += tx.size_bytes;
            template_block.total_fees_collected += tx.fee_paid;
        }
        
        // Calculate utilization percentages
        template_block.free_zone_utilization_percent = 
            static_cast<int>((free_zone_used * 100) / FREE_ZONE_SIZE);
        template_block.total_utilization_percent = 
            static_cast<int>((template_block.total_size_bytes * 100) / MAX_BLOCK_SIZE);
        
        // Update market statistics
        UpdateMarketStats(template_block);
        
        return template_block;
        
    } catch (...) {
        return std::unexpected(FeeError::NetworkError);
    }
}

std::vector<BlockSpaceManager::TransactionCandidate>
BlockSpaceManager::SelectFreeTransactions(const std::vector<TransactionCandidate>& candidates) noexcept {
    
    // Filter candidates that qualify for free transactions
    std::vector<TransactionCandidate> free_eligible;
    std::ranges::copy_if(candidates, std::back_inserter(free_eligible),
        [](const auto& candidate) {
            return candidate.priority_info.qualifies_for_free;
        });
    
    // Sort by priority (highest first)
    std::ranges::sort(free_eligible, [](const auto& a, const auto& b) {
        return a.priority_info.priority_score > b.priority_info.priority_score;
    });
    
    // Fill free zone (5% of block space)
    std::vector<TransactionCandidate> selected;
    std::size_t used_space = 0;
    
    for (const auto& candidate : free_eligible) {
        if (used_space + candidate.size_bytes <= FREE_ZONE_SIZE) {
            selected.push_back(candidate);
            used_space += candidate.size_bytes;
        } else {
            break; // Free zone full
        }
    }
    
    return selected;
}

std::vector<BlockSpaceManager::TransactionCandidate>
BlockSpaceManager::SelectFeeTransactions(const std::vector<TransactionCandidate>& candidates,
                                        std::size_t available_space) noexcept {
    
    // Filter candidates with fees
    std::vector<TransactionCandidate> fee_paying;
    std::ranges::copy_if(candidates, std::back_inserter(fee_paying),
        [](const auto& candidate) {
            return candidate.fee_paid > 0;
        });
    
    // Sort by fee rate (highest first), then by time received (oldest first)
    std::ranges::sort(fee_paying, [](const auto& a, const auto& b) {
        bitcoin::amount_t fee_rate_a = a.fee_paid * 1000 / a.size_bytes;  // per KB
        bitcoin::amount_t fee_rate_b = b.fee_paid * 1000 / b.size_bytes;
        
        if (fee_rate_a != fee_rate_b) {
            return fee_rate_a > fee_rate_b;
        }
        return a.received_time < b.received_time; // Older first
    });
    
    // Fill remaining block space
    std::vector<TransactionCandidate> selected;
    std::size_t used_space = 0;
    
    for (const auto& candidate : fee_paying) {
        if (used_space + candidate.size_bytes <= available_space) {
            selected.push_back(candidate);
            used_space += candidate.size_bytes;
        } else {
            // Try to fit smaller transactions
            continue;
        }
    }
    
    return selected;
}

std::expected<bitcoin::amount_t, FeeError>
BlockSpaceManager::GetRecommendedFee(std::size_t tx_size_bytes,
                                   const PriorityCalculator::PriorityResult& priority) noexcept {
    try {
        // If qualifies for free, no fee needed
        if (priority.qualifies_for_free) {
            return 0;
        }
        
        // Calculate base fee
        bitcoin::amount_t base_fee = tx_size_bytes * BASE_FEE_RATE;
        
        // Adjust based on current congestion
        std::lock_guard lock(market_mutex_);
        
        int congestion_level = current_stats_.free_zone_pressure_percent;
        bitcoin::amount_t dynamic_rate = CalculateDynamicFeeRate(congestion_level);
        
        bitcoin::amount_t recommended_fee = tx_size_bytes * dynamic_rate;
        
        // Ensure minimum relay fee
        return std::max(recommended_fee, MIN_RELAY_FEE);
        
    } catch (...) {
        return std::unexpected(FeeError::NetworkError);
    }
}

bitcoin::amount_t 
BlockSpaceManager::CalculateDynamicFeeRate(int congestion_level) const noexcept {
    // Dynamic fee rate based on network congestion
    // congestion_level: 0-100%
    
    bitcoin::amount_t base_rate = BASE_FEE_RATE;
    
    if (congestion_level < 50) {
        // Low congestion: base rate
        return base_rate;
    } else if (congestion_level < 80) {
        // Medium congestion: 2x base rate
        return base_rate * 2;
    } else if (congestion_level < 95) {
        // High congestion: 5x base rate  
        return base_rate * 5;
    } else {
        // Extreme congestion: 10x base rate
        return base_rate * 10;
    }
}

void BlockSpaceManager::UpdateMarketStats(const BlockTemplate& completed_block) noexcept {
    std::lock_guard lock(market_mutex_);
    
    // Update congestion metrics
    current_stats_.free_zone_pressure_percent = completed_block.free_zone_utilization_percent;
    
    // Calculate median fee
    std::vector<bitcoin::amount_t> fee_rates;
    for (const auto& tx : completed_block.fee_transactions) {
        if (tx.size_bytes > 0) {
            bitcoin::amount_t rate = (tx.fee_paid * 1000) / tx.size_bytes; // per KB
            fee_rates.push_back(rate);
        }
    }
    
    if (!fee_rates.empty()) {
        std::ranges::sort(fee_rates);
        current_stats_.median_fee_last_block = fee_rates[fee_rates.size() / 2];
        
        // Calculate percentiles
        if (fee_rates.size() >= 4) {
            current_stats_.fee_percentiles = {
                fee_rates[fee_rates.size() / 4],       // 25th percentile
                current_stats_.median_fee_last_block,   // 50th percentile  
                fee_rates[fee_rates.size() * 3 / 4],   // 75th percentile
                fee_rates[fee_rates.size() * 95 / 100] // 95th percentile
            };
        }
    }
    
    // Update minimum fee rate for next block
    current_stats_.current_min_fee_rate = CalculateDynamicFeeRate(
        current_stats_.free_zone_pressure_percent);
    
    // Store recent block for historical analysis
    recent_blocks_.push(completed_block);
    while (recent_blocks_.size() > 144) {  // Keep ~5 hours of blocks
        recent_blocks_.pop();
    }
}

BlockSpaceManager::FeeMarketStats BlockSpaceManager::GetMarketStats() const noexcept {
    std::lock_guard lock(market_mutex_);
    return current_stats_;
}

// FeeEstimator Implementation
std::expected<FeeEstimator::FeeEstimate, FeeError>
FeeEstimator::EstimateFee(std::size_t transaction_size_bytes,
                         const PriorityCalculator::PriorityResult& priority,
                         ConfirmationTarget target) noexcept {
    try {
        FeeEstimate estimate{};
        estimate.target = target;
        estimate.likely_free = priority.qualifies_for_free;
        
        if (priority.qualifies_for_free) {
            // High priority transaction - likely free
            estimate.total_fee = 0;
            estimate.fee_rate = 0;
            estimate.confidence_percent = 95.0;
            estimate.explanation = std::format(
                "High priority ({}), qualifies for free 5% zone",
                static_cast<int>(priority.priority_score)
            );
        } else {
            // Calculate fee based on target
            estimate.fee_rate = CalculateTargetFee(target, transaction_size_bytes);
            estimate.total_fee = (estimate.fee_rate * transaction_size_bytes) / 1000; // per KB to total
            
            // Confidence decreases with faster targets for low priority
            switch (target) {
                case ConfirmationTarget::NextBlock:
                    estimate.confidence_percent = 90.0;
                    break;
                case ConfirmationTarget::Fast:
                    estimate.confidence_percent = 85.0;
                    break;
                case ConfirmationTarget::Standard:
                    estimate.confidence_percent = 95.0;
                    break;
                case ConfirmationTarget::Economy:
                    estimate.confidence_percent = 75.0;
                    break;
            }
            
            estimate.explanation = std::format(
                "Priority too low ({:.0f}), estimated fee for {} confirmation",
                priority.priority_score, static_cast<int>(target)
            );
        }
        
        return estimate;
        
    } catch (...) {
        return std::unexpected(FeeError::NetworkError);
    }
}

bitcoin::amount_t FeeEstimator::CalculateTargetFee(ConfirmationTarget target, std::size_t tx_size) noexcept {
    // Base fee rates per KB based on confirmation target
    switch (target) {
        case ConfirmationTarget::NextBlock:
            return 10'000;  // 0.0001 GLC per KB - premium rate
        case ConfirmationTarget::Fast:
            return 5'000;   // 0.00005 GLC per KB - fast rate  
        case ConfirmationTarget::Standard:
            return 1'000;   // 0.00001 GLC per KB - standard rate
        case ConfirmationTarget::Economy:
            return 500;     // 0.000005 GLC per KB - economy rate
    }
    return 1'000; // fallback
}

} // namespace goldcoin::fees