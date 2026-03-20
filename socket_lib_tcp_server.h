#ifndef SOCKET_LIB_TCP_SERVER_H
#define SOCKET_LIB_TCP_SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

#include <stdbool.h>

typedef struct {
    socket_t server_fd;
    socket_t client_fd;
    struct sockaddr_in address;
    int port;
    bool is_initialized;
} tcp_server;

// Функция инициализации Winsock для Windows
static inline bool tcp_server_init_wsa(void) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }
#endif
    return true;
}

// Создание TCP сервера
static inline bool tcp_server_create(tcp_server* server, int port) {
    if (!server) return false;
    
    memset(server, 0, sizeof(tcp_server));
    server->port = port;
    server->is_initialized = false;
    
    // Инициализация Winsock для Windows
    if (!tcp_server_init_wsa()) {
        return false;
    }
    
    // Создание сокета
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd == INVALID_SOCKET_VAL) {
        return false;
    }
    
    // Настройка опции сокета для переиспользования адреса
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, 
                   (const char*)&opt, sizeof(opt)) < 0) {
        SOCKET_CLOSE(server->server_fd);
        return false;
    }
    
    // Настройка адреса
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(port);
    
    // Привязка сокета
    if (bind(server->server_fd, (struct sockaddr*)&server->address, 
             sizeof(server->address)) < 0) {
        SOCKET_CLOSE(server->server_fd);
        return false;
    }
    
    // Начало прослушивания
    if (listen(server->server_fd, 3) < 0) {
        SOCKET_CLOSE(server->server_fd);
        return false;
    }
    
    server->is_initialized = true;
    return true;
}

// Принятие соединения
static inline bool tcp_server_accept(tcp_server* server) {
    if (!server || !server->is_initialized) return false;
    
    socklen_t addrlen = sizeof(server->address);
    server->client_fd = accept(server->server_fd, 
                               (struct sockaddr*)&server->address, 
                               &addrlen);
    
    return (server->client_fd != INVALID_SOCKET_VAL);
}

// Отправка данных клиенту
static inline int tcp_server_send(tcp_server* server, const void* data, size_t size) {
    if (!server || !server->is_initialized || server->client_fd == INVALID_SOCKET_VAL) {
        return -1;
    }
    
    int total_sent = 0;
    const char* buffer = (const char*)data;
    
    while (total_sent < size) {
        int sent = send(server->client_fd, buffer + total_sent, 
                        size - total_sent, 0);
        if (sent <= 0) {
            return total_sent > 0 ? total_sent : -1;
        }
        total_sent += sent;
    }
    
    return total_sent;
}

// Получение данных от клиента
static inline int tcp_server_recv(tcp_server* server, void* buffer, size_t size) {
    if (!server || !server->is_initialized || server->client_fd == INVALID_SOCKET_VAL) {
        return -1;
    }
    
    int total_received = 0;
    char* buf = (char*)buffer;
    
    while (total_received < size) {
        int received = recv(server->client_fd, buf + total_received, 
                           size - total_received, 0);
        if (received <= 0) {
            return total_received > 0 ? total_received : -1;
        }
        total_received += received;
    }
    
    return total_received;
}

// Закрытие соединения с клиентом
static inline void tcp_server_close_client(tcp_server* server) {
    if (server && server->client_fd != INVALID_SOCKET_VAL) {
        SOCKET_CLOSE(server->client_fd);
        server->client_fd = INVALID_SOCKET_VAL;
    }
}

// Закрытие сервера
static inline void tcp_server_destroy(tcp_server* server) {
    if (!server) return;
    
    if (server->client_fd != INVALID_SOCKET_VAL) {
        SOCKET_CLOSE(server->client_fd);
        server->client_fd = INVALID_SOCKET_VAL;
    }
    
    if (server->server_fd != INVALID_SOCKET_VAL) {
        SOCKET_CLOSE(server->server_fd);
        server->server_fd = INVALID_SOCKET_VAL;
    }
    
    server->is_initialized = false;
    
#ifdef _WIN32
    WSACleanup();
#endif
}

#endif // SOCKET_LIB_TCP_SERVER_H