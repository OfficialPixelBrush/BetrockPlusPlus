
#pragma once
#include "networking/network_stream.h"
#include <vector>

// Should act as a singleton
class ServerManager {
    private:
        int server_socket = INVALID_SOCKET;
    public:
        std::vector<NetworkStream> streams;
        ServerManager(uint16_t port = 25565);
        ~ServerManager();
        bool InitConnection();
};
