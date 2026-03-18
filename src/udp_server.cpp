#include "SocketLib.hpp"
#include "SocketLib_internal.hpp"

namespace SocketLib {

udp_server::udp_server(std::uint16_t port)
    : port_(port), running_(false), socket_(detail::from_native(detail::kInvalidSocket)) {}

udp_server::~udp_server() {
    stop();
}

bool udp_server::run() {
    if (running_) {
        return true;
    }

    if (!detail::ensure_socket_runtime()) {
        return false;
    }

    detail::native_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == detail::kInvalidSocket) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        detail::close_socket(sock);
        return false;
    }

    socket_ = detail::from_native(sock);
    running_ = true;
    return true;
}

void udp_server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    detail::native_socket_t sock = detail::to_native(socket_);
    if (sock != detail::kInvalidSocket) {
        detail::close_socket(sock);
    }

    socket_ = detail::from_native(detail::kInvalidSocket);
}

bool udp_server::is_running() const {
    return running_;
}

bool udp_server::receive(std::string& data, std::string* client_ip, std::uint16_t* client_port) {
    if (!running_) {
        return false;
    }

    sockaddr_in from{};
#ifdef _WIN32
    int from_len = static_cast<int>(sizeof(from));
#else
    socklen_t from_len = sizeof(from);
#endif

    char buffer[2048];
#ifdef _WIN32
    const int received = recvfrom(detail::to_native(socket_), buffer, static_cast<int>(sizeof(buffer)), 0,
                                  reinterpret_cast<sockaddr*>(&from), &from_len);
#else
    const int received = static_cast<int>(recvfrom(detail::to_native(socket_), buffer, sizeof(buffer), 0,
                                                   reinterpret_cast<sockaddr*>(&from), &from_len));
#endif

    if (received <= 0) {
        return false;
    }

    data.assign(buffer, buffer + received);

    if (client_ip != nullptr) {
        char ip_buffer[INET_ADDRSTRLEN];
        const char* txt = inet_ntop(AF_INET, &from.sin_addr, ip_buffer, sizeof(ip_buffer));
        *client_ip = (txt != nullptr) ? txt : "";
    }

    if (client_port != nullptr) {
        *client_port = ntohs(from.sin_port);
    }

    return true;
}

bool udp_server::send_to(const std::string& data, const std::string& ip, std::uint16_t port) {
    if (!running_) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        return false;
    }

#ifdef _WIN32
    const int sent = sendto(detail::to_native(socket_), data.data(), static_cast<int>(data.size()), 0,
                            reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
#else
    const int sent = static_cast<int>(sendto(detail::to_native(socket_), data.data(), data.size(), 0,
                                             reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
#endif

    return sent == static_cast<int>(data.size());
}

}  // namespace SocketLib
