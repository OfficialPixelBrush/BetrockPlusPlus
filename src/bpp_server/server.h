/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
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
#include "player_session.h"
#include "world/world.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "world/client_pos.h"
#include "handle_packet.h"
#include "chunk_sender.h"

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
    void disconnectPlayer(PlayerSession& session, const std::string& reason);
    void broadcastPlayerMovement(PlayerSession& session);
    void processIncoming(PlayerSession& session);

    WorldManager world;
    ChunkSender chunkSender;
    std::vector<std::unique_ptr<PlayerSession>> players;
    int serverSocket = -1;
    int tickCounter = 0;
    EntityId nextEntityId = 2;
    int64_t timeout_seconds = 60;
};