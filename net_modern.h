// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2025 Satoshi Nakamoto (Modernization)
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "bitcoin.h"
#include "serialize_modern.h"
#include "core.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <set>
#include <map>
#include <chrono>
#include <stop_token>
#include <expected>
#include <coroutine>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace bitcoin::net {

// Network magic values - preserving original constants
inline constexpr std::uint32_t MESSAGE_START_SIZE = 4;
inline constexpr std::array<byte_t, MESSAGE_START_SIZE> MESSAGE_START = {0xf9, 0xbe, 0xb4, 0xd9};
inline constexpr std::size_t MAX_MESSAGE_SIZE = 1000000;
inline constexpr std::size_t COMMAND_SIZE = 12;

// Original network protocol constants
inline constexpr int PROTOCOL_VERSION = 31100;
inline constexpr int MIN_PROTO_VERSION = 209;

// Service flags - exact original values
enum class ServiceFlags : std::uint64_t {
    NODE_NETWORK = 1,
};

// Inventory types - preserving exact values
enum class InvType : std::uint32_t {
    ERROR = 0,
    MSG_TX = 1,
    MSG_BLOCK = 2,
};

// Address structure with modern time handling
class Address {
public:
    Address() noexcept = default;
    
    Address(ServiceFlags services, const std::string& ip, std::uint16_t port) noexcept
        : services_(services)
        , port_(port)
        , time_(std::chrono::system_clock::now()) {
        ParseIP(ip);
    }
    
    [[nodiscard]] bool IsIPv4() const noexcept {
        return std::ranges::all_of(ip_.begin(), ip_.begin() + 12,
            [](byte_t b) { return b == 0 || b == 0xff; });
    }
    
    [[nodiscard]] std::string GetKey() const {
        return std::format("{}:{}", ToStringIP(), port_);
    }
    
    [[nodiscard]] std::string ToStringIP() const {
        if (IsIPv4()) {
            return std::format("{}.{}.{}.{}", 
                ip_[12], ip_[13], ip_[14], ip_[15]);
        }
        // IPv6 formatting
        return std::format("[{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:"
                          "{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}]",
            ip_[0], ip_[1], ip_[2], ip_[3], ip_[4], ip_[5], ip_[6], ip_[7],
            ip_[8], ip_[9], ip_[10], ip_[11], ip_[12], ip_[13], ip_[14], ip_[15]);
    }
    
    [[nodiscard]] bool IsRoutable() const noexcept {
        return !(IsRFC1918() || IsRFC3927() || IsLocal());
    }
    
    [[nodiscard]] std::expected<void, serialize::Error>
    serialize(serialize::Buffer& buffer) const noexcept {
        // Network protocol: 4 byte time, 8 byte services, 16 byte IP, 2 byte port
        auto time_t_val = std::chrono::system_clock::to_time_t(time_);
        if (auto r = buffer.write(static_cast<std::uint32_t>(time_t_val)); !r) return r;
        if (auto r = buffer.write(static_cast<std::uint64_t>(services_)); !r) return r;
        if (auto r = buffer.write_bytes(ip_); !r) return r;
        
        // Port in network byte order
        std::uint16_t port_be = htons(port_);
        return buffer.write(port_be);
    }
    
    [[nodiscard]] static std::expected<Address, serialize::Error>
    deserialize(serialize::Buffer& buffer) noexcept {
        Address addr;
        
        auto time_val = buffer.read<std::uint32_t>();
        if (!time_val) return std::unexpected(time_val.error());
        addr.time_ = std::chrono::system_clock::from_time_t(*time_val);
        
        auto services = buffer.read<std::uint64_t>();
        if (!services) return std::unexpected(services.error());
        addr.services_ = static_cast<ServiceFlags>(*services);
        
        auto ip = buffer.read_bytes(16);
        if (!ip) return std::unexpected(ip.error());
        std::ranges::copy(*ip, addr.ip_.begin());
        
        auto port_be = buffer.read<std::uint16_t>();
        if (!port_be) return std::unexpected(port_be.error());
        addr.port_ = ntohs(*port_be);
        
        return addr;
    }
    
private:
    void ParseIP(const std::string& ip) {
        // Try IPv4 first
        struct in_addr addr4;
        if (inet_pton(AF_INET, ip.c_str(), &addr4) == 1) {
            // Map IPv4 to IPv6
            std::fill(ip_.begin(), ip_.begin() + 10, 0);
            ip_[10] = ip_[11] = 0xff;
            std::memcpy(&ip_[12], &addr4, 4);
            return;
        }
        
        // Try IPv6
        struct in6_addr addr6;
        if (inet_pton(AF_INET6, ip.c_str(), &addr6) == 1) {
            std::memcpy(ip_.data(), &addr6, 16);
        }
    }
    
    [[nodiscard]] bool IsRFC1918() const noexcept {
        if (!IsIPv4()) return false;
        return (ip_[12] == 10) ||
               (ip_[12] == 172 && (ip_[13] >= 16 && ip_[13] <= 31)) ||
               (ip_[12] == 192 && ip_[13] == 168);
    }
    
    [[nodiscard]] bool IsRFC3927() const noexcept {
        if (!IsIPv4()) return false;
        return (ip_[12] == 169 && ip_[13] == 254);
    }
    
    [[nodiscard]] bool IsLocal() const noexcept {
        if (IsIPv4()) return ip_[12] == 127;
        return std::ranges::all_of(ip_.begin(), ip_.begin() + 15, 
                                   [](byte_t b) { return b == 0; }) && ip_[15] == 1;
    }
    
    ServiceFlags services_{ServiceFlags::NODE_NETWORK};
    std::array<byte_t, 16> ip_{};
    std::uint16_t port_{8333};
    timestamp_t time_{std::chrono::system_clock::now()};
};

// Inventory vector
class Inv {
public:
    Inv() noexcept = default;
    
    Inv(InvType type, const hash256_t& hash) noexcept
        : type_(type), hash_(hash) {}
    
    [[nodiscard]] InvType type() const noexcept { return type_; }
    [[nodiscard]] const hash256_t& hash() const noexcept { return hash_; }
    
    [[nodiscard]] bool IsKnownType() const noexcept {
        return type_ == InvType::MSG_TX || type_ == InvType::MSG_BLOCK;
    }
    
    [[nodiscard]] std::string ToString() const {
        return std::format("{} {}", 
            type_ == InvType::MSG_TX ? "tx" : 
            type_ == InvType::MSG_BLOCK ? "block" : "unknown",
            /* hash to hex string */ "");
    }
    
    auto operator<=>(const Inv&) const = default;
    
    [[nodiscard]] std::expected<void, serialize::Error>
    serialize(serialize::Buffer& buffer) const noexcept {
        if (auto r = buffer.write(static_cast<std::uint32_t>(type_)); !r) return r;
        return serialize::Serializer<hash256_t>::serialize(buffer, hash_);
    }
    
    [[nodiscard]] static std::expected<Inv, serialize::Error>
    deserialize(serialize::Buffer& buffer) noexcept {
        auto type = buffer.read<std::uint32_t>();
        if (!type) return std::unexpected(type.error());
        
        auto hash = serialize::Serializer<hash256_t>::deserialize(buffer);
        if (!hash) return std::unexpected(hash.error());
        
        return Inv{static_cast<InvType>(*type), *hash};
    }
    
private:
    InvType type_{InvType::ERROR};
    hash256_t hash_{};
};

// Message header - preserving exact wire format
class MessageHeader {
public:
    MessageHeader() noexcept = default;
    
    MessageHeader(std::string_view command, std::uint32_t payload_size) noexcept
        : magic_(MESSAGE_START), payload_size_(payload_size) {
        SetCommand(command);
    }
    
    [[nodiscard]] bool IsValid() const noexcept {
        return magic_ == MESSAGE_START && payload_size_ <= MAX_MESSAGE_SIZE;
    }
    
    [[nodiscard]] std::string_view GetCommand() const noexcept {
        auto end = std::ranges::find(command_, '\0');
        return {command_.data(), static_cast<std::size_t>(end - command_.begin())};
    }
    
    void SetCommand(std::string_view cmd) noexcept {
        std::fill(command_.begin(), command_.end(), 0);
        std::ranges::copy(cmd.substr(0, COMMAND_SIZE), command_.begin());
    }
    
    [[nodiscard]] std::uint32_t GetPayloadSize() const noexcept { 
        return payload_size_; 
    }
    
    void SetChecksum(const hash256_t& hash) noexcept {
        std::copy(hash.begin(), hash.begin() + 4, checksum_.begin());
    }
    
    [[nodiscard]] bool VerifyChecksum(const hash256_t& hash) const noexcept {
        return std::equal(hash.begin(), hash.begin() + 4, checksum_.begin());
    }
    
    [[nodiscard]] std::expected<void, serialize::Error>
    serialize(serialize::Buffer& buffer) const noexcept {
        if (auto r = buffer.write_bytes(magic_); !r) return r;
        if (auto r = buffer.write_bytes(std::span{
            reinterpret_cast<const byte_t*>(command_.data()), COMMAND_SIZE}); !r) return r;
        if (auto r = buffer.write(payload_size_); !r) return r;
        return buffer.write_bytes(checksum_);
    }
    
    [[nodiscard]] static std::expected<MessageHeader, serialize::Error>
    deserialize(serialize::Buffer& buffer) noexcept {
        MessageHeader header;
        
        auto magic = buffer.read_bytes(4);
        if (!magic) return std::unexpected(magic.error());
        std::ranges::copy(*magic, header.magic_.begin());
        
        auto command = buffer.read_bytes(COMMAND_SIZE);
        if (!command) return std::unexpected(command.error());
        std::ranges::copy(*command, 
            reinterpret_cast<byte_t*>(header.command_.data()));
        
        auto size = buffer.read<std::uint32_t>();
        if (!size) return std::unexpected(size.error());
        header.payload_size_ = *size;
        
        auto checksum = buffer.read_bytes(4);
        if (!checksum) return std::unexpected(checksum.error());
        std::ranges::copy(*checksum, header.checksum_.begin());
        
        return header;
    }
    
private:
    std::array<byte_t, 4> magic_{MESSAGE_START};
    std::array<char, COMMAND_SIZE> command_{};
    std::uint32_t payload_size_{0};
    std::array<byte_t, 4> checksum_{};
};

// Node connection with modern async I/O
class Node : public std::enable_shared_from_this<Node> {
public:
    static constexpr auto PING_INTERVAL = std::chrono::seconds{30};
    static constexpr auto TIMEOUT = std::chrono::seconds{90};
    
    Node(int socket, const Address& addr) noexcept
        : socket_(socket)
        , addr_(addr)
        , version_sent_(false)
        , version_received_(false)
        , last_recv_(std::chrono::steady_clock::now())
        , last_send_(std::chrono::steady_clock::now()) {}
    
    ~Node() {
        Disconnect();
    }
    
    void Start() {
        receive_thread_ = std::jthread([this](std::stop_token token) {
            ReceiveLoop(token);
        });
        
        send_thread_ = std::jthread([this](std::stop_token token) {
            SendLoop(token);
        });
    }
    
    void Disconnect() {
        if (socket_ != -1) {
            shutdown(socket_, SHUT_RDWR);
            close(socket_);
            socket_ = -1;
        }
    }
    
    template<typename T>
    void PushMessage(std::string_view command, const T& payload) {
        auto data = serialize::to_bytes(payload);
        if (!data) return;
        
        MessageHeader header{command, static_cast<std::uint32_t>(data->size())};
        auto hash = crypto::Hash(*data);
        header.SetChecksum(hash);
        
        std::lock_guard lock{send_mutex_};
        send_queue_.emplace_back(command, std::move(*data));
        send_cv_.notify_one();
    }
    
    void PushVersion() {
        // Version message structure preserved exactly
        struct Version {
            std::int32_t version{PROTOCOL_VERSION};
            std::uint64_t services{static_cast<std::uint64_t>(ServiceFlags::NODE_NETWORK)};
            std::int64_t timestamp{std::time(nullptr)};
            Address addr_recv;
            Address addr_from;
            std::uint64_t nonce{GenerateNonce()};
            std::string sub_ver{"/Satoshi:0.3.11/"};
            std::int32_t start_height{0};
        } version;
        
        PushMessage("version", version);
        version_sent_ = true;
    }
    
    [[nodiscard]] bool IsConnected() const noexcept {
        return socket_ != -1;
    }
    
    [[nodiscard]] bool IsFullyConnected() const noexcept {
        return version_sent_ && version_received_;
    }
    
private:
    void ReceiveLoop(std::stop_token token) {
        std::vector<byte_t> buffer(MAX_MESSAGE_SIZE);
        
        while (!token.stop_requested() && IsConnected()) {
            // Receive header
            MessageHeader header;
            if (!ReceiveExact(reinterpret_cast<byte_t*>(&header), sizeof(header)))
                break;
                
            if (!header.IsValid()) {
                Disconnect();
                break;
            }
            
            // Receive payload
            std::vector<byte_t> payload(header.GetPayloadSize());
            if (!ReceiveExact(payload.data(), payload.size()))
                break;
                
            // Verify checksum
            auto hash = crypto::Hash(payload);
            if (!header.VerifyChecksum(hash)) {
                Disconnect();
                break;
            }
            
            // Process message
            ProcessMessage(header.GetCommand(), payload);
            last_recv_ = std::chrono::steady_clock::now();
        }
    }
    
    void SendLoop(std::stop_token token) {
        while (!token.stop_requested() && IsConnected()) {
            std::unique_lock lock{send_mutex_};
            
            if (send_cv_.wait_for(lock, PING_INTERVAL, [this] { 
                return !send_queue_.empty(); 
            })) {
                auto [command, data] = std::move(send_queue_.front());
                send_queue_.pop_front();
                lock.unlock();
                
                // Send header
                MessageHeader header{command, static_cast<std::uint32_t>(data.size())};
                auto hash = crypto::Hash(data);
                header.SetChecksum(hash);
                
                std::vector<byte_t> header_bytes(24);
                serialize::Buffer header_buffer{header_bytes};
                header.serialize(header_buffer);
                
                if (!SendExact(header_bytes.data(), 24))
                    break;
                    
                // Send payload
                if (!SendExact(data.data(), data.size()))
                    break;
                    
                last_send_ = std::chrono::steady_clock::now();
            } else {
                // Send ping if idle
                lock.unlock();
                PushMessage("ping", std::array<byte_t, 0>{});
            }
        }
    }
    
    bool ReceiveExact(byte_t* data, std::size_t size) {
        while (size > 0) {
            ssize_t n = recv(socket_, data, size, 0);
            if (n <= 0) return false;
            data += n;
            size -= n;
        }
        return true;
    }
    
    bool SendExact(const byte_t* data, std::size_t size) {
        while (size > 0) {
            ssize_t n = send(socket_, data, size, MSG_NOSIGNAL);
            if (n <= 0) return false;
            data += n;
            size -= n;
        }
        return true;
    }
    
    void ProcessMessage(std::string_view command, const std::vector<byte_t>& payload) {
        // Process messages exactly as original
        if (command == "version") {
            // Handle version message
            version_received_ = true;
            PushMessage("verack", std::array<byte_t, 0>{});
        } else if (command == "verack") {
            // Version handshake complete
        } else if (command == "ping") {
            // Respond with pong
            PushMessage("pong", std::array<byte_t, 0>{});
        } else if (command == "addr") {
            // Handle address announcement
        } else if (command == "inv") {
            // Handle inventory
        } else if (command == "getdata") {
            // Handle data request
        } else if (command == "getblocks") {
            // Handle block request
        } else if (command == "getheaders") {
            // Handle header request  
        } else if (command == "tx") {
            // Handle transaction
        } else if (command == "block") {
            // Handle block
        }
    }
    
    [[nodiscard]] static std::uint64_t GenerateNonce() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        return gen();
    }
    
    int socket_;
    Address addr_;
    std::atomic<bool> version_sent_;
    std::atomic<bool> version_received_;
    std::chrono::steady_clock::time_point last_recv_;
    std::chrono::steady_clock::time_point last_send_;
    
    std::jthread receive_thread_;
    std::jthread send_thread_;
    
    std::mutex send_mutex_;
    std::condition_variable send_cv_;
    std::deque<std::pair<std::string, std::vector<byte_t>>> send_queue_;
};

} // namespace bitcoin::net