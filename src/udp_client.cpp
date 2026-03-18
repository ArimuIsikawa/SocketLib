#include "SocketLib.hpp"
#include "SocketLib_internal.hpp"

namespace SocketLib {

udp_client::udp_client(const std::string& host, std::uint16_t port)
    : socket_(detail::from_native(detail::kInvalidSocket)), connected_(false) {
    if (!detail::ensure_socket_runtime()) {
        return;
    }

    detail::native_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
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

udp_client::~udp_client() {
    close();
}

bool udp_client::connected() const {
    return connected_;
}

bool udp_client::send(const std::string& data) {
    if (!connected_) {
        return false;
    }

#ifdef _WIN32
    const int sent = ::send(detail::to_native(socket_), data.data(), static_cast<int>(data.size()), 0);
#else
    const int sent = static_cast<int>(::send(detail::to_native(socket_), data.data(), data.size(), 0));
#endif

    return sent == static_cast<int>(data.size());
}

bool udp_client::receive(std::string& data) {
    if (!connected_) {
        return false;
    }

    char buffer[2048];
#ifdef _WIN32
    const int received = recv(detail::to_native(socket_), buffer, static_cast<int>(sizeof(buffer)), 0);
#else
    const int received = static_cast<int>(recv(detail::to_native(socket_), buffer, sizeof(buffer), 0));
#endif

    if (received <= 0) {
        return false;
    }

    data.assign(buffer, buffer + received);
    return true;
}

void udp_client::close() {
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
