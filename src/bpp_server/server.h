/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

#if defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <fcntl.h>
#elif defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <unordered_set>
#include "world/world.h"
#include "world/chunk_serializer.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "world/client_pos.h"

enum class ConnectionState : uint8_t {
    Handshaking,
    LoggingIn,
    WaitingForSpawnChunks,
    Playing
};

struct PlayerSession {
    NetworkStream stream;
    ClientPosition position;
    std::unordered_set<ChunkPos> sentChunks;
    ConnectionState connState = ConnectionState::Handshaking;
    EntityId entityId = 0;
    std::string username;

    explicit PlayerSession(int socket) : stream(socket) {}
};

class Server {
public:
    Server();
    ~Server();
    void run();

private:
    void tick();
    void acceptNewPlayers();
    void handleHandshake(PlayerSession& session);
    void handleLogin(PlayerSession& session);
    void waitForSpawnChunks(PlayerSession& session);
    size_t sendPendingChunks(PlayerSession& session, int batchSize);
    void processIncoming(PlayerSession& session);

    WorldManager world;
    std::vector<std::unique_ptr<PlayerSession>> players;
    int serverSocket = -1;
    int tickCounter = 0;
    EntityId nextEntityId = 1;
};