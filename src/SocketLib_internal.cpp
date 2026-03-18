#include "SocketLib_internal.hpp"

#include <cstdlib>
#include <cstring>
#include <mutex>

namespace SocketLib {
namespace detail {

native_socket_t to_native(std::intptr_t value) {
    return static_cast<native_socket_t>(value);
}

std::intptr_t from_native(native_socket_t value) {
    return static_cast<std::intptr_t>(value);
}

void close_socket(native_socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

void shutdown_socket(native_socket_t sock) {
#ifdef _WIN32
    shutdown(sock, SD_BOTH);
#else
    shutdown(sock, SHUT_RDWR);
#endif
}

bool ensure_socket_runtime() {
#ifdef _WIN32
    static std::once_flag once;
    static bool initialized = false;

    std::call_once(once, []() {
        WSADATA data;
        initialized = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
        if (initialized) {
            std::atexit([]() { WSACleanup(); });
        }
    });

    return initialized;
#else
    return true;
#endif
}

bool resolve_ipv4(const std::string& host, std::uint16_t port, sockaddr_in& out_addr) {
    std::memset(&out_addr, 0, sizeof(out_addr));
    out_addr.sin_family = AF_INET;
    out_addr.sin_port = htons(port);

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const int rc = getaddrinfo(host.c_str(), nullptr, &hints, &result);
    if (rc != 0 || result == nullptr) {
        return false;
    }

    const auto* resolved = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    out_addr.sin_addr = resolved->sin_addr;
    freeaddrinfo(result);
    return true;
}

}  // namespace detail
}  // namespace SocketLib
