// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "core.h"
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <expected>
#include <coroutine>
#include <json/json_spirit.h>

namespace bitcoin::rpc {

// JSON-RPC 2.0 types
using Value = json_spirit::Value;
using Object = json_spirit::Object;
using Array = json_spirit::Array;

// RPC error codes - following JSON-RPC 2.0 specification
enum class ErrorCode : std::int32_t {
    // Standard JSON-RPC 2.0 errors
    PARSE_ERROR = -32700,
    INVALID_REQUEST = -32600,
    METHOD_NOT_FOUND = -32601,
    INVALID_PARAMS = -32602,
    INTERNAL_ERROR = -32603,
    
    // Bitcoin specific errors
    MISC_ERROR = -1,
    FORBIDDEN_BY_SAFE_MODE = -2,
    TYPE_ERROR = -3,
    INVALID_ADDRESS_OR_KEY = -5,
    OUT_OF_MEMORY = -7,
    INVALID_PARAMETER = -8,
    DATABASE_ERROR = -20,
    DESERIALIZATION_ERROR = -22,
    
    // Wallet errors
    WALLET_ERROR = -4,
    WALLET_INSUFFICIENT_FUNDS = -6,
    WALLET_INVALID_ACCOUNT_NAME = -11,
    WALLET_KEYPOOL_RAN_OUT = -12,
    WALLET_UNLOCK_NEEDED = -13,
    WALLET_PASSPHRASE_INCORRECT = -14,
    WALLET_WRONG_ENC_STATE = -15,
    WALLET_ENCRYPTION_FAILED = -16,
};

// RPC error structure
struct Error {
    ErrorCode code;
    std::string message;
    std::optional<Value> data;
    
    [[nodiscard]] Object to_json() const {
        Object error;
        error.emplace_back("code", static_cast<int>(code));
        error.emplace_back("message", message);
        if (data) {
            error.emplace_back("data", *data);
        }
        return error;
    }
};

// RPC method handler type
using MethodHandler = std::function<std::expected<Value, Error>(const Array& params)>;

// Async RPC handler using coroutines
struct AsyncTask {
    struct promise_type {
        std::expected<Value, Error> result;
        
        AsyncTask get_return_object() {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        
        void return_value(std::expected<Value, Error> value) {
            result = std::move(value);
        }
        
        void unhandled_exception() {
            result = std::unexpected(Error{
                ErrorCode::INTERNAL_ERROR,
                "Unhandled exception in async RPC method"
            });
        }
    };
    
    std::coroutine_handle<promise_type> h;
    
    [[nodiscard]] std::expected<Value, Error> get() {
        return h.promise().result;
    }
};

// RPC server
class Server {
public:
    static Server& instance() {
        static Server server;
        return server;
    }
    
    // Register RPC methods
    void register_method(std::string_view name, MethodHandler handler) {
        methods_[std::string(name)] = std::move(handler);
    }
    
    // Process JSON-RPC request
    [[nodiscard]] Value process_request(const Value& request);
    
    // Start/stop server
    [[nodiscard]] std::expected<void, std::string> 
    start(std::uint16_t port = 8332, std::string_view bind_addr = "127.0.0.1");
    
    void stop();
    
    // Authentication
    void set_credentials(std::string_view username, std::string_view password) {
        username_ = username;
        password_ = password;
    }
    
private:
    Server() { register_builtin_methods(); }
    
    void register_builtin_methods();
    
    [[nodiscard]] std::expected<Value, Error>
    execute_method(std::string_view method, const Array& params);
    
    [[nodiscard]] bool 
    check_auth(std::string_view auth_header) const;
    
    std::map<std::string, MethodHandler, std::less<>> methods_;
    std::string username_;
    std::string password_;
    std::atomic<bool> running_{false};
};

// Built-in RPC methods

// Block chain information
[[nodiscard]] std::expected<Value, Error> getblockcount(const Array& params);
[[nodiscard]] std::expected<Value, Error> getbestblockhash(const Array& params);
[[nodiscard]] std::expected<Value, Error> getdifficulty(const Array& params);
[[nodiscard]] std::expected<Value, Error> getblock(const Array& params);
[[nodiscard]] std::expected<Value, Error> getblockhash(const Array& params);
[[nodiscard]] std::expected<Value, Error> gettransaction(const Array& params);

// Mining
[[nodiscard]] std::expected<Value, Error> getgenerate(const Array& params);
[[nodiscard]] std::expected<Value, Error> setgenerate(const Array& params);
[[nodiscard]] std::expected<Value, Error> gethashespersec(const Array& params);
[[nodiscard]] std::expected<Value, Error> getwork(const Array& params);

// Wallet
[[nodiscard]] std::expected<Value, Error> getbalance(const Array& params);
[[nodiscard]] std::expected<Value, Error> getnewaddress(const Array& params);
[[nodiscard]] std::expected<Value, Error> getaccountaddress(const Array& params);
[[nodiscard]] std::expected<Value, Error> getaddressesbyaccount(const Array& params);
[[nodiscard]] std::expected<Value, Error> sendtoaddress(const Array& params);
[[nodiscard]] std::expected<Value, Error> listtransactions(const Array& params);
[[nodiscard]] std::expected<Value, Error> listaccounts(const Array& params);
[[nodiscard]] std::expected<Value, Error> listreceivedbyaddress(const Array& params);
[[nodiscard]] std::expected<Value, Error> listreceivedbyaccount(const Array& params);
[[nodiscard]] std::expected<Value, Error> backupwallet(const Array& params);
[[nodiscard]] std::expected<Value, Error> validateaddress(const Array& params);

// Network
[[nodiscard]] std::expected<Value, Error> getconnectioncount(const Array& params);
[[nodiscard]] std::expected<Value, Error> getpeerinfo(const Array& params);
[[nodiscard]] std::expected<Value, Error> addnode(const Array& params);
[[nodiscard]] std::expected<Value, Error> getnetworkinfo(const Array& params);

// Raw transactions
[[nodiscard]] std::expected<Value, Error> getrawtransaction(const Array& params);
[[nodiscard]] std::expected<Value, Error> sendrawtransaction(const Array& params);
[[nodiscard]] std::expected<Value, Error> decoderawtransaction(const Array& params);

// Utility
[[nodiscard]] std::expected<Value, Error> getinfo(const Array& params);
[[nodiscard]] std::expected<Value, Error> stop(const Array& params);
[[nodiscard]] std::expected<Value, Error> help(const Array& params);

// Parameter parsing utilities
template<typename T>
[[nodiscard]] std::expected<T, Error> 
parse_param(const Value& value, std::string_view name) {
    try {
        if constexpr (std::is_same_v<T, std::string>) {
            if (value.type() != json_spirit::str_type) {
                return std::unexpected(Error{
                    ErrorCode::TYPE_ERROR,
                    std::format("Parameter '{}' must be a string", name)
                });
            }
            return value.get_str();
        } else if constexpr (std::is_integral_v<T>) {
            if (value.type() != json_spirit::int_type) {
                return std::unexpected(Error{
                    ErrorCode::TYPE_ERROR,
                    std::format("Parameter '{}' must be an integer", name)
                });
            }
            return static_cast<T>(value.get_int());
        } else if constexpr (std::is_floating_point_v<T>) {
            if (value.type() != json_spirit::real_type) {
                return std::unexpected(Error{
                    ErrorCode::TYPE_ERROR,
                    std::format("Parameter '{}' must be a number", name)
                });
            }
            return static_cast<T>(value.get_real());
        } else if constexpr (std::is_same_v<T, bool>) {
            if (value.type() != json_spirit::bool_type) {
                return std::unexpected(Error{
                    ErrorCode::TYPE_ERROR,
                    std::format("Parameter '{}' must be a boolean", name)
                });
            }
            return value.get_bool();
        }
    } catch (const std::exception& e) {
        return std::unexpected(Error{
            ErrorCode::INVALID_PARAMS,
            std::format("Failed to parse parameter '{}': {}", name, e.what())
        });
    }
}

// Safe parameter access
template<typename T>
[[nodiscard]] std::expected<T, Error>
get_param(const Array& params, std::size_t index, std::string_view name) {
    if (index >= params.size()) {
        return std::unexpected(Error{
            ErrorCode::INVALID_PARAMS,
            std::format("Missing required parameter '{}'", name)
        });
    }
    return parse_param<T>(params[index], name);
}

// Optional parameter access
template<typename T>
[[nodiscard]] std::optional<T>
get_optional_param(const Array& params, std::size_t index, T default_value = T{}) {
    if (index >= params.size()) {
        return default_value;
    }
    auto result = parse_param<T>(params[index], "optional");
    return result.has_value() ? *result : default_value;
}

// Convert Bitcoin types to JSON
[[nodiscard]] Object block_to_json(const core::Block& block);
[[nodiscard]] Object transaction_to_json(const core::Transaction& tx);
[[nodiscard]] Object script_to_json(const Script& script);

// HTTP server integration
class HTTPServer {
public:
    struct Request {
        std::string method;
        std::string uri;
        std::map<std::string, std::string, std::less<>> headers;
        std::string body;
    };
    
    struct Response {
        int status_code = 200;
        std::map<std::string, std::string, std::less<>> headers;
        std::string body;
    };
    
    [[nodiscard]] Response handle_request(const Request& request);
    
private:
    [[nodiscard]] bool check_basic_auth(const Request& request) const;
};

} // namespace bitcoin::rpc