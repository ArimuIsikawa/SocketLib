#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace SocketLib {
namespace detail {

#ifdef _WIN32
using native_socket_t = SOCKET;
constexpr native_socket_t kInvalidSocket = INVALID_SOCKET;
#else
using native_socket_t = int;
constexpr native_socket_t kInvalidSocket = -1;
#endif

native_socket_t to_native(std::intptr_t value);
std::intptr_t from_native(native_socket_t value);

void close_socket(native_socket_t sock);
void shutdown_socket(native_socket_t sock);

bool ensure_socket_runtime();
bool resolve_ipv4(const std::string& host, std::uint16_t port, sockaddr_in& out_addr);

}  // namespace detail
}  // namespace SocketLib
