// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "util_modern.h"
#include <expected>
#include <string>
#include <vector>
#include <span>

namespace bitcoin::init {

// Initialization stages
enum class Stage {
    parse_arguments,
    setup_directories,
    load_config,
    initialize_logging,
    setup_network,
    load_blockchain,
    load_wallet,
    start_rpc,
    start_mining,
    complete
};

// Initialization options
struct Options {
    // Data directory
    std::filesystem::path data_dir;
    
    // Network
    bool testnet = false;
    bool regtest = false;
    std::uint16_t port = 8333;
    std::uint16_t rpc_port = 8332;
    std::vector<std::string> connect_nodes;
    std::vector<std::string> add_nodes;
    bool listen = true;
    bool discover = true;
    std::optional<std::string> proxy;
    
    // Mining
    bool generate = false;
    std::optional<crypto::Address> mining_address;
    std::uint32_t generation_threads = 0;
    
    // Wallet
    bool disable_wallet = false;
    std::optional<std::filesystem::path> wallet_file;
    std::optional<std::string> wallet_passphrase;
    std::uint32_t keypool_size = 100;
    
    // RPC
    bool server = false;
    std::string rpc_user;
    std::string rpc_password;
    std::vector<std::string> rpc_allow_ips;
    
    // Debugging
    bool debug = false;
    bool printtoconsole = false;
    std::optional<std::filesystem::path> debug_log_file;
    
    // Performance
    std::size_t db_cache_size = 100;  // MB
    std::uint32_t max_connections = 125;
    std::size_t max_orphan_tx = 100;
    std::size_t max_mempool_size = 300;  // MB
    
    // Security
    bool safe_mode = false;
    std::optional<std::uint32_t> alert_notify;
};

// Application context - single source of truth
class AppContext {
public:
    static AppContext& instance() {
        static AppContext ctx;
        return ctx;
    }
    
    [[nodiscard]] const Options& options() const noexcept { return options_; }
    [[nodiscard]] Stage current_stage() const noexcept { return stage_; }
    [[nodiscard]] bool is_running() const noexcept { return running_.load(); }
    
    void request_shutdown() { 
        running_.store(false);
        util::ThreadManager::instance().request_shutdown();
    }
    
private:
    friend std::expected<void, std::string> initialize(std::span<const char*> args);
    
    Options options_;
    Stage stage_ = Stage::parse_arguments;
    std::atomic<bool> running_{false};
};

// Main initialization function
[[nodiscard]] std::expected<void, std::string> 
initialize(std::span<const char*> args);

// Individual initialization steps
[[nodiscard]] std::expected<Options, std::string>
parse_command_line(std::span<const char*> args);

[[nodiscard]] std::expected<void, std::string>
setup_data_directory(const Options& options);

[[nodiscard]] std::expected<void, std::string>
load_configuration(Options& options);

[[nodiscard]] std::expected<void, std::string>
initialize_logging(const Options& options);

[[nodiscard]] std::expected<void, std::string>
initialize_network(const Options& options);

[[nodiscard]] std::expected<void, std::string>
load_block_chain(const Options& options);

[[nodiscard]] std::expected<std::shared_ptr<wallet::Wallet>, std::string>
load_wallet(const Options& options);

[[nodiscard]] std::expected<void, std::string>
start_rpc_server(const Options& options);

[[nodiscard]] std::expected<void, std::string>
start_mining(const Options& options);

// Shutdown sequence
void shutdown();

// Signal handling
void setup_signal_handlers();
void handle_signal(int signal);

// Parameter validation
[[nodiscard]] std::expected<void, std::string>
validate_options(const Options& options);

// Help and version information
[[nodiscard]] std::string get_help_message();
[[nodiscard]] std::string get_version_string();

// Command line argument parser
class ArgumentParser {
public:
    explicit ArgumentParser(std::span<const char*> args);
    
    [[nodiscard]] bool has_option(std::string_view name) const;
    
    [[nodiscard]] std::optional<std::string> 
    get_option(std::string_view name) const;
    
    [[nodiscard]] std::vector<std::string>
    get_multi_option(std::string_view name) const;
    
    template<typename T>
    [[nodiscard]] std::optional<T> 
    get_option_as(std::string_view name) const {
        auto value = get_option(name);
        if (!value) return std::nullopt;
        return util::parse_number<T>(*value);
    }
    
    [[nodiscard]] std::vector<std::string> get_positional() const {
        return positional_;
    }
    
private:
    std::map<std::string, std::vector<std::string>, std::less<>> options_;
    std::vector<std::string> positional_;
};

// Environment setup
[[nodiscard]] std::expected<void, std::string>
setup_environment();

// Daemon mode support (Unix only)
#ifndef WIN32
[[nodiscard]] std::expected<void, std::string>
daemonize();
#endif

// Windows service support
#ifdef WIN32
[[nodiscard]] std::expected<void, std::string>
install_windows_service();

[[nodiscard]] std::expected<void, std::string>
remove_windows_service();

void run_as_windows_service();
#endif

// Sanity checks
[[nodiscard]] std::expected<void, std::string>
sanity_check_environment();

[[nodiscard]] std::expected<void, std::string>
check_disk_space(const std::filesystem::path& path, std::size_t required_mb);

// Lock file to prevent multiple instances
class LockFile {
public:
    explicit LockFile(const std::filesystem::path& path);
    ~LockFile();
    
    [[nodiscard]] bool try_lock();
    void unlock();
    [[nodiscard]] bool is_locked() const noexcept { return locked_; }
    
private:
    std::filesystem::path path_;
    int fd_ = -1;
    bool locked_ = false;
};

// Application lifetime management
class Application {
public:
    [[nodiscard]] std::expected<void, std::string>
    run(std::span<const char*> args);
    
    void request_shutdown() {
        AppContext::instance().request_shutdown();
    }
    
private:
    [[nodiscard]] std::expected<void, std::string>
    main_loop();
    
    std::unique_ptr<LockFile> lock_file_;
    std::shared_ptr<wallet::Wallet> wallet_;
};

// Global application instance
[[nodiscard]] Application& app();

} // namespace bitcoin::init