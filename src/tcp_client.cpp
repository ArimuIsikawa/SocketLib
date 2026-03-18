#include "SocketLib.hpp"
#include "SocketLib_internal.hpp"

#include <cstdint>
#include <limits>

namespace SocketLib {

namespace {

bool send_exact(detail::native_socket_t sock, const char* data, std::size_t size) {
    std::size_t total = 0;
    while (total < size) {
#ifdef _WIN32
        const int sent = ::send(sock, data + total, static_cast<int>(size - total), 0);
#else
        const int sent = static_cast<int>(::send(sock, data + total, size - total, 0));
#endif
        if (sent <= 0) {
            return false;
        }
        total += static_cast<std::size_t>(sent);
    }
    return true;
}

}  // namespace

tcp_client::tcp_client(const std::string& host, std::uint16_t port)
    : socket_(detail::from_native(detail::kInvalidSocket)), connected_(false) {
    if (!detail::ensure_socket_runtime()) {
        return;
    }

    detail::native_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == detail::kInvalidSocket) {
        return;
    }

    sockaddr_in addr{};
    if (!detail::resolve_ipv4(host, port, addr)) {
        detail::close_socket(sock);
        return;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        detail::close_socket(sock);
        return;
    }

    socket_ = detail::from_native(sock);
    connected_ = true;
}

tcp_client::~tcp_client() {
    close();
}

bool tcp_client::connected() const {
    return connected_;
}

bool tcp_client::sendall(const std::string& data) {
    if (!connected_) {
        return false;
    }

    if (data.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }

    const auto payload_len = static_cast<std::uint32_t>(data.size());
    const std::uint32_t payload_len_net = htonl(payload_len);
    detail::native_socket_t sock = detail::to_native(socket_);

    if (!send_exact(sock, reinterpret_cast<const char*>(&payload_len_net), sizeof(payload_len_net))) {
        return false;
    }

    return payload_len == 0 || send_exact(sock, data.data(), data.size());
}

void tcp_client::close() {
    if (!connected_) {
        return;
    }

    detail::native_socket_t sock = detail::to_native(socket_);
    if (sock != detail::kInvalidSocket) {
        detail::close_socket(sock);
    }

    socket_ = detail::from_native(detail::kInvalidSocket);
    connected_ = false;
}

}  // namespace SocketLib
