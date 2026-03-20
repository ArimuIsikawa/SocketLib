#ifndef SOCKET_LIB_TCP_CLIENT_H
#define SOCKET_LIB_TCP_CLIENT_H

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
    socket_t sock_fd;
    struct sockaddr_in server_addr;
    bool is_connected;
} tcp_client;

// Функция инициализации Winsock для Windows
static inline bool tcp_client_init_wsa(void) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }
#endif
    return true;
}

// Создание и подключение TCP клиента
static inline bool tcp_client_connect(tcp_client* client, const char* server_ip, int port) {
    if (!client || !server_ip) return false;
    
    memset(client, 0, sizeof(tcp_client));
    client->is_connected = false;
    
    // Инициализация Winsock для Windows
    if (!tcp_client_init_wsa()) {
        return false;
    }
    
    // Создание сокета
    client->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->sock_fd == INVALID_SOCKET_VAL) {
        return false;
    }
    
    // Настройка адреса сервера
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);
    
    // Преобразование IP адреса
    if (inet_pton(AF_INET, server_ip, &client->server_addr.sin_addr) <= 0) {
        SOCKET_CLOSE(client->sock_fd);
        return false;
    }
    
    // Подключение к серверу
    if (connect(client->sock_fd, (struct sockaddr*)&client->server_addr, 
                sizeof(client->server_addr)) < 0) {
        SOCKET_CLOSE(client->sock_fd);
        return false;
    }
    
    client->is_connected = true;
    return true;
}

// Отправка данных серверу
static inline int tcp_client_send(tcp_client* client, const void* data, size_t size) {
    if (!client || !client->is_connected || client->sock_fd == INVALID_SOCKET_VAL) {
        return -1;
    }
    
    int total_sent = 0;
    const char* buffer = (const char*)data;
    
    while (total_sent < size) {
        int sent = send(client->sock_fd, buffer + total_sent, 
                        size - total_sent, 0);
        if (sent <= 0) {
            return total_sent > 0 ? total_sent : -1;
        }
        total_sent += sent;
    }
    
    return total_sent;
}

// Получение данных от сервера
static inline int tcp_client_recv(tcp_client* client, void* buffer, size_t size) {
    if (!client || !client->is_connected || client->sock_fd == INVALID_SOCKET_VAL) {
        return -1;
    }
    
    int total_received = 0;
    char* buf = (char*)buffer;
    
    while (total_received < size) {
        int received = recv(client->sock_fd, buf + total_received, 
                           size - total_received, 0);
        if (received <= 0) {
            return total_received > 0 ? total_received : -1;
        }
        total_received += received;
    }
    
    return total_received;
}

// Неблокирующее получение данных (сколько доступно)
static inline int tcp_client_recv_available(tcp_client* client, void* buffer, size_t size) {
    if (!client || !client->is_connected || client->sock_fd == INVALID_SOCKET_VAL) {
        return -1;
    }
    
    return recv(client->sock_fd, (char*)buffer, size, 0);
}

// Закрытие соединения
static inline void tcp_client_disconnect(tcp_client* client) {
    if (!client) return;
    
    if (client->sock_fd != INVALID_SOCKET_VAL) {
        SOCKET_CLOSE(client->sock_fd);
        client->sock_fd = INVALID_SOCKET_VAL;
    }
    
    client->is_connected = false;
    
#ifdef _WIN32
    WSACleanup();
#endif
}

// Проверка состояния соединения
static inline bool tcp_client_is_connected(tcp_client* client) {
    if (!client) return false;
    return client->is_connected;
}

#endif // SOCKET_LIB_TCP_CLIENT_H