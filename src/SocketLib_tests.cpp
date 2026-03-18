#include "SocketLib.hpp"

#include <iostream>

int run_test_server() {
    constexpr std::uint16_t kPort = 8080;

    SocketLib::tcp_server server(kPort);
    if (!server.run()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server started on port: " << kPort << std::endl;
    std::cout << "Press enter to stop server" << std::endl;

    std::cin.get();

    server.stop();
    return 0;
}

int run_test_client() {
    SocketLib::tcp_client client("127.0.0.1", 8080);

    if (!client.connected()) {
        std::cout << "Connection fail" << std::endl;
        return 1;
    }

    if (!client.sendall("Hello, server!")) {
        std::cout << "Send failed" << std::endl;
        client.close();
        return 1;
    }

    client.close();
    return 0;
}
