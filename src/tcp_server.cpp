#include "SocketLib.hpp"
#include "SocketLib_internal.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>

namespace SocketLib {

namespace {

bool recv_exact(detail::native_socket_t sock, char* buffer, std::size_t size) {
    std::size_t total = 0;
    while (total < size) {
#ifdef _WIN32
        const int received = ::recv(sock, buffer + total, static_cast<int>(size - total), 0);
#else
        const int received = static_cast<int>(::recv(sock, buffer + total, size - total, 0));
#endif
        if (received <= 0) {
            return false;
        }
        total += static_cast<std::size_t>(received);
    }
    return true;
}

}  // namespace

tcp_server::tcp_server(std::uint16_t port)
    : port_(port), running_(false), listen_socket_(detail::from_native(detail::kInvalidSocket)) {}

tcp_server::~tcp_server() {
    stop();
}

bool tcp_server::run() {
    if (running_) {
        return true;
    }

    if (!detail::ensure_socket_runtime()) {
        return false;
    }

    detail::native_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == detail::kInvalidSocket) {
        return false;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        detail::close_socket(sock);
        return false;
    }

    if (listen(sock, SOMAXCONN) < 0) {
        detail::close_socket(sock);
        return false;
    }

    listen_socket_ = detail::from_native(sock);
    running_ = true;
    accept_thread_ = std::thread(&tcp_server::accept_loop, this);

    return true;
}

void tcp_server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    detail::native_socket_t listen_sock = detail::to_native(listen_socket_);
    if (listen_sock != detail::kInvalidSocket) {
        detail::shutdown_socket(listen_sock);
        detail::close_socket(listen_sock);
        listen_socket_ = detail::from_native(detail::kInvalidSocket);
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (std::intptr_t raw : client_sockets_) {
            detail::native_socket_t client = detail::to_native(raw);
            if (client != detail::kInvalidSocket) {
                detail::shutdown_socket(client);
                detail::close_socket(client);
            }
        }
        client_sockets_.clear();
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

bool tcp_server::is_running() const {
    return running_;
}

void tcp_server::accept_loop() {
    while (running_) {
        sockaddr_in client_addr{};
#ifdef _WIN32
        int client_len = static_cast<int>(sizeof(client_addr));
#else
        socklen_t client_len = sizeof(client_addr);
#endif

        detail::native_socket_t client = accept(
            detail::to_native(listen_socket_),
            reinterpret_cast<sockaddr*>(&client_addr),
            &client_len
        );

        if (client == detail::kInvalidSocket) {
            if (!running_) {
                break;
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            client_sockets_.push_back(detail::from_native(client));
        }

        std::thread(&tcp_server::handle_client, this, detail::from_native(client)).detach();
    }
}

void tcp_server::handle_client(std::intptr_t client_socket) {
    detail::native_socket_t client = detail::to_native(client_socket);

    while (running_) {
        std::uint32_t payload_len_net = 0;
        if (!recv_exact(client, reinterpret_cast<char*>(&payload_len_net), sizeof(payload_len_net))) {
            break;
        }

        const std::uint32_t payload_len = ntohl(payload_len_net);
        if (payload_len > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
            break;
        }

        std::string message(payload_len, '\0');
        if (payload_len > 0 &&
            !recv_exact(client, message.data(), static_cast<std::size_t>(payload_len))) {
            break;
        }

        if (message_handler != nullptr)
            message_handler(message);
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = std::remove(client_sockets_.begin(), client_sockets_.end(), client_socket);
        client_sockets_.erase(it, client_sockets_.end());
    }

    detail::close_socket(client);
}

}  // namespace SocketLib
