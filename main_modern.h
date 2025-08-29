// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "core.h"
#include "crypto_modern.h"
#include "script_modern.h"
#include "util_modern.h"
#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include <deque>
#include <ranges>
#include <memory_resource>

namespace bitcoin::chain {

// Core constants - RESTORED to Satoshi's original design
// The 1MB MAX_BLOCK_SIZE was NOT original - it was suspiciously added July 15, 2010
// In the true original, blocks were limited only by P2P message size (32MB)
// inline constexpr std::uint32_t MAX_BLOCK_SIZE = 1'000'000;  // REMOVED - NOT ORIGINAL
inline constexpr amount_t COINBASE_MATURITY = 100;
inline constexpr std::uint32_t LOCKTIME_THRESHOLD = 500'000'000;
inline constexpr std::uint32_t MAX_SCRIPT_SIZE = 10'000;
inline constexpr std::uint32_t MAX_ORPHAN_TRANSACTIONS = 10'000;

// Target spacing and difficulty adjustment
inline constexpr seconds TARGET_BLOCK_TIME{600};  // 10 minutes
inline constexpr std::uint32_t TARGET_TIMESPAN = 14 * 24 * 60 * 60;  // 2 weeks
inline constexpr std::uint32_t INTERVAL = TARGET_TIMESPAN / 600;  // 2016 blocks

// Memory pool entry
struct MemPoolEntry {
    core::Transaction tx;
    amount_t fee;
    util::time_point time;
    std::uint32_t height;
    
    [[nodiscard]] amount_t fee_rate() const noexcept {
        auto size = tx.serialized_size();
        return size > 0 ? fee / size : 0;
    }
};

// Block index - chain database entry
class BlockIndex {
public:
    // Chain position
    hash256_t hash_block;
    BlockIndex* pprev = nullptr;
    BlockIndex* pnext = nullptr;
    std::uint32_t height = 0;
    
    // Block header
    core::BlockHeader header;
    
    // Validation state
    std::uint32_t bits;
    BigNum bn_chain_work;
    util::time_point time_received;
    
    // Transaction data
    std::uint32_t file_index = 0;
    std::uint32_t block_pos = 0;
    
    [[nodiscard]] bool is_in_main_chain() const noexcept {
        return pnext != nullptr || this == get_best_index();
    }
    
    [[nodiscard]] BigNum get_block_work() const {
        BigNum bn_target;
        bn_target.set_compact(bits);
        if (bn_target <= 0) return 0;
        return (BigNum(1) << 256) / (bn_target + 1);
    }
    
    [[nodiscard]] util::time_point get_median_time_past() const {
        std::array<util::time_point, 11> times;
        int count = 0;
        const BlockIndex* pindex = this;
        
        for (int i = 0; i < 11 && pindex; i++, pindex = pindex->pprev) {
            times[count++] = pindex->header.timestamp;
        }
        
        std::ranges::sort(times | std::views::take(count));
        return times[count / 2];
    }
    
private:
    static BlockIndex* get_best_index() noexcept;
};

// UTXO (Unspent Transaction Output)
struct UTXO {
    core::OutPoint outpoint;
    core::TxOut output;
    std::uint32_t height;
    bool is_coinbase;
    
    [[nodiscard]] bool is_mature(std::uint32_t current_height) const noexcept {
        return !is_coinbase || (current_height >= height + COINBASE_MATURITY);
    }
};

// Modern chain state manager
class ChainState {
public:
    static ChainState& instance() {
        static ChainState state;
        return state;
    }
    
    // Block operations
    [[nodiscard]] std::expected<void, std::string> 
    process_block(const core::Block& block, bool check_pow = true);
    
    [[nodiscard]] std::optional<BlockIndex*> 
    get_block_index(const hash256_t& hash) const;
    
    [[nodiscard]] BlockIndex* get_best_block() const noexcept {
        std::shared_lock lock(mutex_);
        return best_index_;
    }
    
    [[nodiscard]] std::uint32_t get_best_height() const noexcept {
        std::shared_lock lock(mutex_);
        return best_index_ ? best_index_->height : 0;
    }
    
    // UTXO operations
    [[nodiscard]] std::optional<UTXO> 
    get_utxo(const core::OutPoint& outpoint) const;
    
    [[nodiscard]] amount_t 
    get_balance(const crypto::PublicKey& pubkey) const;
    
    // Memory pool operations
    [[nodiscard]] std::expected<void, std::string>
    accept_to_memory_pool(const core::Transaction& tx);
    
    [[nodiscard]] std::vector<core::Transaction>
    get_memory_pool() const;
    
    [[nodiscard]] std::optional<core::Transaction>
    get_memory_pool_tx(const hash256_t& hash) const;
    
    // Mining interface
    [[nodiscard]] std::unique_ptr<core::Block>
    create_new_block(const crypto::PublicKey& pubkey);
    
    [[nodiscard]] bool 
    check_proof_of_work(const hash256_t& hash, std::uint32_t bits) const;
    
    [[nodiscard]] std::uint32_t
    get_next_work_required(const BlockIndex* pindex) const;
    
private:
    ChainState() = default;
    
    // Chain validation
    [[nodiscard]] std::expected<void, std::string>
    connect_block(const core::Block& block, BlockIndex* pindex);
    
    [[nodiscard]] std::expected<void, std::string>
    disconnect_block(BlockIndex* pindex);
    
    [[nodiscard]] bool
    reorganize(BlockIndex* pnew);
    
    [[nodiscard]] std::expected<void, std::string>
    validate_transaction(const core::Transaction& tx, bool check_inputs = true) const;
    
    // UTXO management
    void update_utxo_set(const core::Transaction& tx, std::uint32_t height, bool is_coinbase);
    void remove_utxo(const core::OutPoint& outpoint);
    
    mutable std::shared_mutex mutex_;
    
    // Chain state
    BlockIndex* genesis_index_ = nullptr;
    BlockIndex* best_index_ = nullptr;
    std::unordered_map<hash256_t, std::unique_ptr<BlockIndex>> map_block_index_;
    
    // UTXO set - the key data structure
    std::unordered_map<core::OutPoint, UTXO> utxo_set_;
    
    // Memory pool
    std::unordered_map<hash256_t, MemPoolEntry> mempool_;
    std::deque<hash256_t> mempool_order_;  // For FIFO eviction
    
    // Orphan transactions
    std::unordered_map<hash256_t, core::Transaction> orphan_transactions_;
    std::unordered_map<core::OutPoint, hash256_t> orphan_by_prev_;
};

// Transaction verification
class TxVerifier {
public:
    struct Context {
        const core::Transaction& tx;
        const std::vector<core::TxOut>& prev_outputs;
        std::uint32_t input_index;
        std::uint32_t flags;
    };
    
    [[nodiscard]] static std::expected<void, std::string>
    verify_script(const Script& script_sig, 
                  const Script& script_pubkey,
                  const Context& ctx);
    
    [[nodiscard]] static std::expected<amount_t, std::string>
    check_transaction_sanity(const core::Transaction& tx);
    
    [[nodiscard]] static bool
    is_standard(const core::Transaction& tx);
    
    [[nodiscard]] static bool
    is_final(const core::Transaction& tx, std::uint32_t block_height, util::time_point block_time);
};

// Mining operations
class Miner {
public:
    struct Options {
        crypto::PublicKey pubkey;
        std::uint32_t extra_nonce = 0;
        std::optional<std::uint32_t> max_iterations;
    };
    
    [[nodiscard]] static std::optional<core::Block>
    mine_block(std::unique_ptr<core::Block> pblock, 
               const Options& options,
               std::stop_token stop_token);
    
    [[nodiscard]] static hash256_t
    hash_block_header(const core::BlockHeader& header);
    
private:
    static void increment_extra_nonce(core::Block& block, std::uint32_t& extra_nonce);
};

// Wallet integration hooks
class WalletInterface {
public:
    virtual ~WalletInterface() = default;
    
    virtual void inventory_received(const hash256_t& hash, const core::Transaction& tx) = 0;
    virtual void block_connected(const core::Block& block, const BlockIndex* pindex) = 0;
    virtual void block_disconnected(const core::Block& block, const BlockIndex* pindex) = 0;
    virtual void set_best_chain(const BlockIndex* pindex) = 0;
};

// Global wallet interface registration
inline void register_wallet(std::shared_ptr<WalletInterface> wallet);
inline void unregister_wallet(std::shared_ptr<WalletInterface> wallet);

// Validation flags
enum ValidationFlags : std::uint32_t {
    SCRIPT_VERIFY_NONE = 0,
    SCRIPT_VERIFY_P2SH = (1U << 0),
    SCRIPT_VERIFY_STRICTENC = (1U << 1),
    SCRIPT_VERIFY_DERSIG = (1U << 2),
    SCRIPT_VERIFY_LOW_S = (1U << 3),
    SCRIPT_VERIFY_NULLDUMMY = (1U << 4),
    SCRIPT_VERIFY_SIGPUSHONLY = (1U << 5),
};

// Fee estimation
class FeeEstimator {
public:
    [[nodiscard]] static amount_t 
    estimate_fee(std::size_t tx_size, std::uint32_t confirmation_target = 6);
    
    static void 
    record_transaction(const core::Transaction& tx, amount_t fee, std::uint32_t confirm_height);
    
private:
    struct FeeStats {
        std::deque<amount_t> recent_fees;
        static constexpr std::size_t MAX_SAMPLES = 100;
    };
    
    static inline std::array<FeeStats, 10> fee_buckets_;
};

// Chain parameters (would be different for testnet/regtest)
struct ChainParams {
    hash256_t genesis_hash;
    std::uint32_t default_port;
    std::string network_id;
    std::vector<std::string> seed_nodes;
    BigNum proof_of_work_limit;
    std::uint32_t subsidy_halving_interval;
    
    [[nodiscard]] static const ChainParams& main();
    [[nodiscard]] static const ChainParams& testnet();
    [[nodiscard]] static const ChainParams& regtest();
};

// Initialize chain state with genesis block
[[nodiscard]] std::expected<void, std::string> initialize_chain();

// Shutdown and cleanup
void shutdown_chain();

} // namespace bitcoin::chain