// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)  
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "core.h"
#include "crypto_modern.h"
#include "script_modern.h"
#include "main_modern.h"
#include "db_modern.h"
#include <map>
#include <set>
#include <shared_mutex>
#include <ranges>

namespace bitcoin::wallet {

// Wallet transaction status
enum class TxStatus {
    unconfirmed,
    confirmed,
    conflicted,
    abandoned
};

// Address book purpose
enum class Purpose {
    receive,
    send,
    refund
};

// Key metadata
struct KeyMetadata {
    util::time_point creation_time;
    std::optional<std::string> label;
    std::optional<std::uint32_t> account;
    bool is_change = false;
    bool is_reserved = false;
};

// Wallet transaction
class WalletTx {
public:
    core::Transaction tx;
    hash256_t block_hash;
    std::uint32_t block_height = 0;
    util::time_point time_received;
    TxStatus status = TxStatus::unconfirmed;
    
    // Metadata
    std::map<std::string, std::string, std::less<>> metadata;
    std::vector<std::pair<std::string, std::string>> order_form;
    
    [[nodiscard]] bool is_coinbase() const noexcept {
        return tx.inputs.size() == 1 && tx.inputs[0].prevout.is_null();
    }
    
    [[nodiscard]] bool is_confirmed() const noexcept {
        return status == TxStatus::confirmed;
    }
    
    [[nodiscard]] std::uint32_t get_depth() const noexcept {
        if (!is_confirmed()) return 0;
        return chain::ChainState::instance().get_best_height() - block_height + 1;
    }
    
    [[nodiscard]] amount_t get_credit() const;
    [[nodiscard]] amount_t get_debit() const;
    [[nodiscard]] amount_t get_net() const {
        return get_credit() - get_debit();
    }
};

// Key pool for pre-generated keys
class KeyPool {
public:
    explicit KeyPool(std::size_t size = 100) : target_size_(size) {}
    
    [[nodiscard]] std::expected<crypto::PrivateKey, std::string> get_key();
    void return_key(const crypto::PrivateKey& key);
    void top_up();
    
    [[nodiscard]] std::size_t size() const {
        std::shared_lock lock(mutex_);
        return keys_.size();
    }
    
private:
    mutable std::shared_mutex mutex_;
    std::deque<crypto::PrivateKey> keys_;
    std::size_t target_size_;
};

// Account structure for organizing addresses
struct Account {
    std::string name;
    std::set<crypto::Address> addresses;
    amount_t balance = 0;
    
    [[nodiscard]] bool contains_address(const crypto::Address& addr) const {
        return addresses.contains(addr);
    }
};

// Modern wallet implementation
class Wallet : public chain::WalletInterface {
public:
    explicit Wallet(const std::filesystem::path& path);
    ~Wallet() override;
    
    // Key management
    [[nodiscard]] std::expected<crypto::Address, std::string>
    get_new_address(std::string_view label = "");
    
    [[nodiscard]] bool 
    have_key(const crypto::PublicKey& pubkey) const;
    
    [[nodiscard]] std::optional<crypto::PrivateKey>
    get_key(const crypto::PublicKey& pubkey) const;
    
    [[nodiscard]] std::vector<crypto::PublicKey>
    get_public_keys() const;
    
    // Balance and transactions
    [[nodiscard]] amount_t 
    get_balance(std::optional<std::string_view> account = std::nullopt,
                std::uint32_t min_confirmations = 1) const;
    
    [[nodiscard]] std::expected<hash256_t, std::string>
    send_to_address(const crypto::Address& address, 
                   amount_t amount,
                   std::string_view comment = "");
    
    [[nodiscard]] std::vector<WalletTx>
    get_transactions(std::optional<std::string_view> account = std::nullopt,
                    std::size_t limit = 100) const;
    
    [[nodiscard]] std::optional<WalletTx>
    get_transaction(const hash256_t& hash) const;
    
    // Account management
    [[nodiscard]] std::expected<void, std::string>
    set_account(const crypto::Address& address, std::string_view account);
    
    [[nodiscard]] std::optional<std::string>
    get_account(const crypto::Address& address) const;
    
    [[nodiscard]] std::vector<Account>
    list_accounts(std::uint32_t min_confirmations = 1) const;
    
    // Address book
    [[nodiscard]] bool
    set_address_label(const crypto::Address& address, std::string_view label);
    
    [[nodiscard]] std::optional<std::string>
    get_address_label(const crypto::Address& address) const;
    
    // Backup and encryption
    [[nodiscard]] std::expected<void, std::string>
    backup(const std::filesystem::path& path) const;
    
    [[nodiscard]] std::expected<void, std::string>
    encrypt(std::string_view passphrase);
    
    [[nodiscard]] std::expected<void, std::string>
    unlock(std::string_view passphrase, std::chrono::seconds duration = std::chrono::seconds(0));
    
    [[nodiscard]] bool is_encrypted() const noexcept {
        return encrypted_master_key_.has_value();
    }
    
    [[nodiscard]] bool is_locked() const noexcept {
        return is_encrypted() && !master_key_.has_value();
    }
    
    // Chain interface implementation
    void inventory_received(const hash256_t& hash, const core::Transaction& tx) override;
    void block_connected(const core::Block& block, const chain::BlockIndex* pindex) override;
    void block_disconnected(const core::Block& block, const chain::BlockIndex* pindex) override;
    void set_best_chain(const chain::BlockIndex* pindex) override;
    
    // Transaction creation
    [[nodiscard]] std::expected<core::Transaction, std::string>
    create_transaction(const std::vector<std::pair<crypto::Address, amount_t>>& recipients,
                      amount_t fee_rate = 0);
    
    // Coin selection
    struct CoinSelectionResult {
        std::vector<core::OutPoint> selected_coins;
        amount_t total_value;
        amount_t change;
    };
    
    [[nodiscard]] std::expected<CoinSelectionResult, std::string>
    select_coins(amount_t target, amount_t fee_rate = 0) const;
    
private:
    // Database operations
    [[nodiscard]] bool load_wallet();
    [[nodiscard]] bool save_key(const crypto::PrivateKey& key, const KeyMetadata& metadata);
    [[nodiscard]] bool save_transaction(const WalletTx& wtx);
    
    // Transaction processing
    void add_to_wallet(const core::Transaction& tx, const hash256_t& block_hash = {});
    void update_transaction(const hash256_t& hash, const hash256_t& block_hash, std::uint32_t height);
    
    // Key generation
    [[nodiscard]] crypto::PrivateKey generate_new_key();
    
    // Encryption helpers
    [[nodiscard]] bool encrypt_keys();
    [[nodiscard]] bool decrypt_keys();
    
    mutable std::shared_mutex mutex_;
    
    // Wallet database
    std::unique_ptr<db::CWalletDB> db_;
    
    // Keys and addresses
    std::map<crypto::PublicKey, crypto::PrivateKey> keys_;
    std::map<crypto::PublicKey, KeyMetadata> key_metadata_;
    std::map<crypto::Address, std::string> address_book_;
    
    // Transactions
    std::map<hash256_t, WalletTx> transactions_;
    
    // Accounts
    std::map<std::string, Account, std::less<>> accounts_;
    
    // Key pool
    KeyPool key_pool_;
    
    // Encryption
    std::optional<std::vector<byte_t>> encrypted_master_key_;
    std::optional<crypto::AESKey> master_key_;
    util::time_point unlock_time_;
    
    // Best known block
    hash256_t best_block_hash_;
    std::uint32_t best_block_height_ = 0;
};

// Transaction builder for complex transactions
class TransactionBuilder {
public:
    TransactionBuilder& add_input(const core::OutPoint& outpoint, 
                                  const Script& script_sig = {});
    
    TransactionBuilder& add_output(const crypto::Address& address, amount_t amount);
    TransactionBuilder& add_output(const Script& script_pubkey, amount_t amount);
    
    TransactionBuilder& set_locktime(std::uint32_t locktime);
    TransactionBuilder& set_version(std::uint32_t version);
    
    [[nodiscard]] std::expected<core::Transaction, std::string> build() const;
    
    [[nodiscard]] amount_t calculate_fee(amount_t fee_rate) const;
    
private:
    std::vector<core::TxIn> inputs_;
    std::vector<core::TxOut> outputs_;
    std::uint32_t version_ = 1;
    std::uint32_t locktime_ = 0;
};

// Watch-only wallet support
class WatchOnlyWallet {
public:
    void add_watch_address(const crypto::Address& address);
    void remove_watch_address(const crypto::Address& address);
    
    [[nodiscard]] bool is_watching(const crypto::Address& address) const;
    [[nodiscard]] std::vector<crypto::Address> get_watch_addresses() const;
    
    [[nodiscard]] amount_t get_balance() const;
    [[nodiscard]] std::vector<core::Transaction> get_transactions() const;
    
private:
    std::set<crypto::Address> watch_addresses_;
    std::map<hash256_t, core::Transaction> transactions_;
};

// Deterministic wallet support (BIP32 precursor)
class DeterministicWallet {
public:
    explicit DeterministicWallet(const std::vector<byte_t>& seed);
    
    [[nodiscard]] crypto::PrivateKey derive_key(std::uint32_t index) const;
    [[nodiscard]] crypto::Address derive_address(std::uint32_t index) const;
    
    [[nodiscard]] std::vector<crypto::Address> 
    get_addresses(std::uint32_t start = 0, std::uint32_t count = 20) const;
    
private:
    std::vector<byte_t> seed_;
    mutable std::map<std::uint32_t, crypto::PrivateKey> derived_keys_;
};

// Fee recommendation
[[nodiscard]] amount_t recommend_fee(std::size_t tx_size, std::uint32_t confirmation_target = 6);

// Address validation
[[nodiscard]] bool is_valid_address(std::string_view address);
[[nodiscard]] bool is_valid_bitcoin_address(const crypto::Address& address);

} // namespace bitcoin::wallet