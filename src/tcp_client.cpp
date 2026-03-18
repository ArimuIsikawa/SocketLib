#include "SocketLib.hpp"
#include "SocketLib_internal.hpp"

namespace SocketLib {

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

    const char* ptr = data.data();
    std::size_t left = data.size();

    while (left > 0) {
#ifdef _WIN32
        const int sent = ::send(detail::to_native(socket_), ptr, static_cast<int>(left), 0);
#else
        const int sent = static_cast<int>(::send(detail::to_native(socket_), ptr, left, 0));
#endif
        if (sent <= 0) {
            return false;
        }

        ptr += sent;
        left -= static_cast<std::size_t>(sent);
    }

    return true;
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
