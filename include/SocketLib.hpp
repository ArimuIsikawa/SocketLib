#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace SocketLib {

class tcp_server {
public:
    explicit tcp_server(std::uint16_t port);
    ~tcp_server();

    bool run();
    void stop();
    bool is_running() const;

    void (*message_handler)(std::string message) = nullptr;
private:
    void accept_loop();
    void handle_client(std::intptr_t client_socket);

    std::uint16_t port_;
    std::atomic<bool> running_;
    std::intptr_t listen_socket_;
    std::thread accept_thread_;
    std::mutex clients_mutex_;
    std::vector<std::intptr_t> client_sockets_;
};

class tcp_client {
public:
    tcp_client(const std::string& host, std::uint16_t port);
    ~tcp_client();

    bool connected() const;
    bool sendall(const std::string& data);
    void close();

private:
    std::intptr_t socket_;
    bool connected_;
};

class udp_server {
public:
    explicit udp_server(std::uint16_t port);
    ~udp_server();

    bool run();
    void stop();
    bool is_running() const;

    bool receive(std::string& data, std::string* client_ip = nullptr, std::uint16_t* client_port = nullptr);
    bool send_to(const std::string& data, const std::string& ip, std::uint16_t port);

private:
    std::uint16_t port_;
    bool running_;
    std::intptr_t socket_;
};

class udp_client {
public:
    udp_client(const std::string& host, std::uint16_t port);
    ~udp_client();

    bool connected() const;
    bool send(const std::string& data);
    bool receive(std::string& data);
    void close();

private:
    std::intptr_t socket_;
    bool connected_;
};

}  // namespace SocketLib

int run_test_server();
int run_test_client();
