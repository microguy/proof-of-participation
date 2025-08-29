// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <source_location>
#include <stacktrace>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <stop_token>
#include <latch>
#include <barrier>
#include <semaphore>
#include <atomic>
#include <random>

namespace bitcoin::util {

// Time utilities - using std::chrono throughout
using clock = std::chrono::system_clock;
using time_point = clock::time_point;
using seconds = std::chrono::seconds;
using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;

[[nodiscard]] inline time_point now() noexcept {
    return clock::now();
}

[[nodiscard]] inline std::int64_t unix_time() noexcept {
    return std::chrono::duration_cast<seconds>(now().time_since_epoch()).count();
}

// Thread-safe random number generation
class RandomGenerator {
public:
    static RandomGenerator& instance() {
        static RandomGenerator rng;
        return rng;
    }
    
    template<std::integral T>
    [[nodiscard]] T uniform(T min, T max) {
        std::lock_guard lock(mutex_);
        std::uniform_int_distribution<T> dist(min, max);
        return dist(engine_);
    }
    
    [[nodiscard]] std::vector<byte_t> bytes(std::size_t count) {
        std::lock_guard lock(mutex_);
        std::vector<byte_t> result(count);
        std::uniform_int_distribution<int> dist(0, 255);
        std::ranges::generate(result, [&] { return static_cast<byte_t>(dist(engine_)); });
        return result;
    }
    
private:
    RandomGenerator() : engine_(std::random_device{}()) {}
    std::mt19937_64 engine_;
    std::mutex mutex_;
};

// Logging with source location
enum class LogLevel : std::uint8_t {
    debug,
    info,
    warning,
    error,
    critical
};

class Logger {
public:
    static void log(LogLevel level, 
                   std::string_view message,
                   const std::source_location& loc = std::source_location::current()) {
        auto& instance = get_instance();
        std::lock_guard lock(instance.mutex_);
        
        if (level < instance.min_level_) return;
        
        auto now = clock::now();
        auto time_t = clock::to_time_t(now);
        
        instance.stream_ << std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}:{} - {}\n",
            std::chrono::floor<seconds>(now), 
            level_string(level),
            loc.file_name(), 
            loc.line(),
            message);
    }
    
    static void set_level(LogLevel level) {
        get_instance().min_level_ = level;
    }
    
private:
    static Logger& get_instance() {
        static Logger logger;
        return logger;
    }
    
    static constexpr std::string_view level_string(LogLevel level) {
        switch (level) {
            case LogLevel::debug: return "DEBUG";
            case LogLevel::info: return "INFO";
            case LogLevel::warning: return "WARN";
            case LogLevel::error: return "ERROR";
            case LogLevel::critical: return "CRIT";
        }
        return "UNKNOWN";
    }
    
    Logger() : stream_(std::clog) {}
    
    std::mutex mutex_;
    std::ostream& stream_;
    LogLevel min_level_ = LogLevel::info;
};

// Simplified logging macros
#define LOG_DEBUG(msg) ::bitcoin::util::Logger::log(::bitcoin::util::LogLevel::debug, msg)
#define LOG_INFO(msg) ::bitcoin::util::Logger::log(::bitcoin::util::LogLevel::info, msg)
#define LOG_WARNING(msg) ::bitcoin::util::Logger::log(::bitcoin::util::LogLevel::warning, msg)
#define LOG_ERROR(msg) ::bitcoin::util::Logger::log(::bitcoin::util::LogLevel::error, msg)
#define LOG_CRITICAL(msg) ::bitcoin::util::Logger::log(::bitcoin::util::LogLevel::critical, msg)

// File system utilities
namespace fs = std::filesystem;

[[nodiscard]] inline fs::path data_dir() {
    static const fs::path dir = []() {
        if (auto home = std::getenv("HOME"); home) {
            return fs::path(home) / ".bitcoin";
        }
        return fs::current_path() / ".bitcoin";
    }();
    return dir;
}

[[nodiscard]] inline bool ensure_directory(const fs::path& path) {
    std::error_code ec;
    return fs::create_directories(path, ec);
}

// Thread utilities
class ThreadManager {
public:
    static ThreadManager& instance() {
        static ThreadManager manager;
        return manager;
    }
    
    template<typename F>
    std::jthread launch(F&& func, std::string_view name = "") {
        return std::jthread([func = std::forward<F>(func), name](std::stop_token token) {
            if (!name.empty()) {
                set_thread_name(name);
            }
            func(token);
        });
    }
    
    void request_shutdown() {
        shutdown_requested_.store(true);
        shutdown_requested_.notify_all();
    }
    
    [[nodiscard]] bool shutdown_requested() const noexcept {
        return shutdown_requested_.load();
    }
    
    void wait_for_shutdown() {
        shutdown_requested_.wait(false);
    }
    
private:
    static void set_thread_name(std::string_view name) {
        #ifdef __linux__
        pthread_setname_np(pthread_self(), name.data());
        #endif
    }
    
    std::atomic<bool> shutdown_requested_{false};
};

// Format utilities - replace printf with std::format
template<typename... Args>
[[nodiscard]] std::string format(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}

// Safe integer operations
template<std::integral T>
[[nodiscard]] constexpr std::optional<T> safe_add(T a, T b) noexcept {
    T result;
    if (__builtin_add_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
}

template<std::integral T>
[[nodiscard]] constexpr std::optional<T> safe_multiply(T a, T b) noexcept {
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
}

// Memory utilities
template<typename T>
[[nodiscard]] constexpr T* align_up(T* ptr, std::size_t alignment) noexcept {
    auto addr = reinterpret_cast<std::uintptr_t>(ptr);
    addr = (addr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<T*>(addr);
}

// String utilities
[[nodiscard]] inline std::string to_lower(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    std::ranges::transform(str, std::back_inserter(result), 
                           [](char c) { return std::tolower(c); });
    return result;
}

[[nodiscard]] inline std::string to_upper(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    std::ranges::transform(str, std::back_inserter(result),
                           [](char c) { return std::toupper(c); });
    return result;
}

// Parse utilities with better error handling
template<typename T>
[[nodiscard]] std::optional<T> parse_number(std::string_view str) {
    T value{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return value;
    }
    return std::nullopt;
}

// Config management
class Config {
public:
    static Config& instance() {
        static Config config;
        return config;
    }
    
    [[nodiscard]] std::optional<std::string> get(std::string_view key) const {
        std::shared_lock lock(mutex_);
        if (auto it = values_.find(std::string(key)); it != values_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    void set(std::string_view key, std::string_view value) {
        std::unique_lock lock(mutex_);
        values_[std::string(key)] = std::string(value);
    }
    
    template<typename T>
    [[nodiscard]] std::optional<T> get_as(std::string_view key) const {
        auto value = get(key);
        if (!value) return std::nullopt;
        return parse_number<T>(*value);
    }
    
private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::string, std::less<>> values_;
};

// Global flags as atomics instead of extern bools
inline std::atomic<bool> g_debug{false};
inline std::atomic<bool> g_daemon{false};
inline std::atomic<bool> g_server{false};
inline std::atomic<bool> g_proxy{false};
inline std::atomic<bool> g_generate_bitcoins{false};

// Network endianness utilities
template<std::integral T>
[[nodiscard]] constexpr T to_little_endian(T value) noexcept {
    if constexpr (std::endian::native == std::endian::big) {
        return std::byteswap(value);
    }
    return value;
}

template<std::integral T>
[[nodiscard]] constexpr T from_little_endian(T value) noexcept {
    if constexpr (std::endian::native == std::endian::big) {
        return std::byteswap(value);
    }
    return value;
}

// Error handling
[[noreturn]] inline void fatal_error(std::string_view message,
                                     const std::source_location& loc = std::source_location::current()) {
    LOG_CRITICAL(std::format("Fatal error at {}:{} - {}", 
                             loc.file_name(), loc.line(), message));
    std::cerr << "Fatal: " << message << "\n";
    std::cerr << "Location: " << loc.file_name() << ":" << loc.line() << "\n";
    std::cerr << "Stack trace:\n" << std::stacktrace::current() << "\n";
    std::abort();
}

// Scope guards for RAII
template<typename F>
class ScopeGuard {
public:
    explicit ScopeGuard(F&& f) : func_(std::forward<F>(f)), active_(true) {}
    ~ScopeGuard() { if (active_) func_(); }
    
    ScopeGuard(ScopeGuard&& other) noexcept 
        : func_(std::move(other.func_)), active_(other.active_) {
        other.active_ = false;
    }
    
    void dismiss() noexcept { active_ = false; }
    
private:
    F func_;
    bool active_;
};

template<typename F>
[[nodiscard]] auto make_scope_guard(F&& f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

// Performance timing
class Timer {
public:
    Timer() : start_(clock::now()) {}
    
    [[nodiscard]] milliseconds elapsed() const {
        return std::chrono::duration_cast<milliseconds>(clock::now() - start_);
    }
    
    void reset() { start_ = clock::now(); }
    
private:
    time_point start_;
};

} // namespace bitcoin::util