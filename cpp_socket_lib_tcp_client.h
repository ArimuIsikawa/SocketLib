#ifndef CPP_SOCKET_LIB_TCP_CLIENT_H
#define CPP_SOCKET_LIB_TCP_CLIENT_H

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define SOCKET_CLOSE(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
    #define SOCKET_CLOSE(s) close(s)
#endif

namespace socket_lib {

// RAII класс для инициализации Winsock
class WinsockInitializer {
private:
    static int ref_count;
    
public:
    WinsockInitializer() {
#ifdef _WIN32
        if (ref_count == 0) {
            WSADATA wsa_data;
            if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
                ref_count = -1;
            } else {
                ref_count = 1;
            }
        } else if (ref_count > 0) {
            ref_count++;
        }
#endif
    }
    
    ~WinsockInitializer() {
#ifdef _WIN32
        if (ref_count > 0) {
            ref_count--;
            if (ref_count == 0) {
                WSACleanup();
            }
        }
#endif
    }
    
    bool is_initialized() const {
#ifdef _WIN32
        return ref_count > 0;
#else
        return true;
#endif
    }
    
    WinsockInitializer(const WinsockInitializer&) = delete;
    WinsockInitializer& operator=(const WinsockInitializer&) = delete;
};

#ifdef _WIN32
int WinsockInitializer::ref_count = 0;
#endif

// Класс TCP клиента
class TCPClient {
private:
    socket_t sock_fd;
    struct sockaddr_in server_addr;
    bool connected;
    std::unique_ptr<WinsockInitializer> wsa_init;
    
public:
    TCPClient() : sock_fd(INVALID_SOCKET_VAL), connected(false) {
#ifdef _WIN32
        wsa_init = std::make_unique<WinsockInitializer>();
#endif
        memset(&server_addr, 0, sizeof(server_addr));
    }
    
    ~TCPClient() {
        disconnect();
    }
    
    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;
    
    TCPClient(TCPClient&& other) noexcept
        : sock_fd(other.sock_fd), server_addr(other.server_addr),
          connected(other.connected), wsa_init(std::move(other.wsa_init)) {
        other.sock_fd = INVALID_SOCKET_VAL;
        other.connected = false;
    }
    
    TCPClient& operator=(TCPClient&& other) noexcept {
        if (this != &other) {
            disconnect();
            sock_fd = other.sock_fd;
            server_addr = other.server_addr;
            connected = other.connected;
            wsa_init = std::move(other.wsa_init);
            other.sock_fd = INVALID_SOCKET_VAL;
            other.connected = false;
        }
        return *this;
    }
    
    int connect(const std::string& server_ip, int port) {
        if (connected) {
            return -1;
        }
        
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == INVALID_SOCKET_VAL) {
            return -1;
        }
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            SOCKET_CLOSE(sock_fd);
            sock_fd = INVALID_SOCKET_VAL;
            return -1;
        }
        
        if (::connect(sock_fd, (struct sockaddr*)&server_addr, 
                      sizeof(server_addr)) < 0) {
            SOCKET_CLOSE(sock_fd);
            sock_fd = INVALID_SOCKET_VAL;
            return -1;
        }
        
        connected = true;
        return 0;
    }
    
    int connect(const char* server_ip, int port) {
        return connect(std::string(server_ip), port);
    }
    
    int send(const void* data, size_t size) {
        if (!connected) {
            return -1;
        }
        
        size_t total_sent = 0;
        const char* buffer = static_cast<const char*>(data);
        
        while (total_sent < size) {
            int sent = ::send(sock_fd, buffer + total_sent, 
                             size - total_sent, 0);
            
            if (sent <= 0) {
                return -1;
            }
            
            total_sent += sent;
        }
        
        return static_cast<int>(total_sent);
    }
    
    int send(const std::string& data) {
        return send(data.c_str(), data.length());
    }
    
    int recv(void* buffer, size_t size) {
        if (!connected) {
            return -1;
        }
        
        size_t total_received = 0;
        char* buf = static_cast<char*>(buffer);
        
        while (total_received < size) {
            int received = ::recv(sock_fd, buf + total_received, 
                                 size - total_received, 0);
            
            if (received <= 0) {
                if (total_received > 0) {
                    return static_cast<int>(total_received);
                }
                return -1;
            }
            
            total_received += received;
        }
        
        return static_cast<int>(total_received);
    }
    
    int recv(std::vector<char>& buffer, size_t size) {
        buffer.resize(size);
        return recv(buffer.data(), size);
    }
    
    int recv_string(std::string& out_str, size_t size) {
        std::vector<char> buffer(size + 1, 0);
        int received = recv(buffer.data(), size);
        if (received > 0) {
            buffer[received] = '\0';
            out_str = std::string(buffer.data());
            return received;
        }
        return received;
    }
    
    int recv_available(void* buffer, size_t size) {
        if (!connected) {
            return -1;
        }
        
        int received = ::recv(sock_fd, static_cast<char*>(buffer), size, 0);
        if (received < 0) {
            return -1;
        }
        
        return received;
    }
    
    int set_timeout(int seconds) {
        if (!connected) {
            return -1;
        }
        
#ifdef _WIN32
        DWORD timeout = seconds * 1000;
        if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO,
                      (const char*)&timeout, sizeof(timeout)) < 0) {
            return -1;
        }
#else
        struct timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO,
                      &timeout, sizeof(timeout)) < 0) {
            return -1;
        }
#endif
        return 0;
    }
    
    void disconnect() {
        if (connected && sock_fd != INVALID_SOCKET_VAL) {
            SOCKET_CLOSE(sock_fd);
            sock_fd = INVALID_SOCKET_VAL;
            connected = false;
        }
    }
    
    bool is_connected() const {
        if (!connected) return false;
        
        char buffer;
        int result = ::recv(sock_fd, &buffer, 0, MSG_PEEK);
        return result != SOCKET_ERROR_VAL;
    }
    
    socket_t get_fd() const { return sock_fd; }
    std::string get_server_ip() const {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server_addr.sin_addr, ip_str, sizeof(ip_str));
        return std::string(ip_str);
    }
    
    int get_port() const { return ntohs(server_addr.sin_port); }
};

} // namespace socket_lib

#endif // CPP_SOCKET_LIB_TCP_CLIENT_H