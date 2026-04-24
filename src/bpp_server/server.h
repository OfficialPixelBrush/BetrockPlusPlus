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
#include "commands/command_manager.h"

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
    void disconnectPlayer(PlayerSession& session, const std::wstring& reason);
    void broadcastPlayerMovement(PlayerSession& session);
    void processIncoming(PlayerSession& session);

    static constexpr float TICK_DELTA = 1.0f / 20.0f;
    static constexpr int   MAX_TICKS_PER_FRAME = 10;

    WorldManager world;
    ChunkSender chunkSender;
    std::vector<std::unique_ptr<PlayerSession>> players;
    std::unordered_map<ChunkPos, std::vector<PendingBlock>> chunkBlockChanges;
    int serverSocket = -1;
    EntityId nextEntityId = 2;
    int64_t timeout_seconds = 60;
    float accumulator = 0.0f;
    CommandManager command_manager;
};