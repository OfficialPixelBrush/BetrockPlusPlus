/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

#include "config/config.h"
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
    void stop();

private:
    void tick();
    void startup();
    void acceptNewPlayers();
    void handleHandshake(PlayerSession& session);
    void handleLogin(PlayerSession& session);
    void waitForSpawnChunks(PlayerSession& session);
    void disconnectPlayer(PlayerSession& session, const std::wstring& reason);
    void broadcastPlayerMovement(PlayerSession& session);
    void processIncoming(PlayerSession& session);

    // Config file stuff
    void loadConfig();

    // Chunk-session index helpers
    void indexAddChunk(PlayerSession& session, const Int32_2& pos);
    void indexRemoveChunk(PlayerSession& session, const Int32_2& pos);
    void indexRemoveSession(PlayerSession& session);

    void closeSocket() const {
        #if defined(_WIN32) || defined(_WIN64)
        closesocket(serverSocket);
        WSACleanup();
        #else
        close(serverSocket);
        #endif
    }

    void createServerSocket(int port) {
        // This is horrific to look at but it works
        #if defined(_WIN32) || defined(_WIN64)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            std::cerr << "**** FAILED TO BIND TO PORT!" << std::endl;
        }
        listen(serverSocket, 8);
        #if defined(_WIN32) || defined(_WIN64)
        u_long mode = 1;
        ioctlsocket(serverSocket, FIONBIO, &mode);
        #else
        fcntl(serverSocket, F_SETFL, O_NONBLOCK);
        #endif
    }

    int createClientSocket() const {
        #if defined(_WIN32) || defined(_WIN64)
        SOCKET rawSocket = accept(serverSocket, nullptr, nullptr);
        if (rawSocket == INVALID_SOCKET) return -1;
        u_long clientMode = 1;
        ioctlsocket(rawSocket, FIONBIO, &clientMode);
        DWORD recvTimeout = 45;
        setsockopt(rawSocket, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
        int clientSocket = static_cast<int>(rawSocket);
        #else
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) return -1;
        fcntl(clientSocket, F_SETFL, O_NONBLOCK);
        struct timeval recvTimeout { 0, 45000 };
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
        #endif
        return clientSocket;
    }

    static constexpr float TICK_DELTA = 1.0f / 20.0f;
    static constexpr int   MAX_TICKS_PER_FRAME = 10;

    WorldManager world;
    ChunkSender chunkSender;
    std::vector<std::unique_ptr<PlayerSession>> players;
    std::unordered_map<Int32_2, std::vector<PendingBlock>> chunkBlockChanges;

    // Reverse index: which sessions currently have a given chunk loaded.
    // Maintained in sync with session.flushedChunks so block-change dispatch
    // can skip chunks that no player has received, avoiding a full player scan.
    std::unordered_map<Int32_2, std::vector<PlayerSession*>> chunkSessions;
    int serverSocket = -1;
	int serverPort = 25565;
    EntityId nextEntityId = 2;
    int64_t timeout_seconds = 60;
    int flushChunkCount = 10;
    float accumulator = 0.0f;
    float tickTimeAccum = 0.0f;
    int   tickCount = 0;
    CommandManager command_manager;
    bool stopped = false;
    Config config;
};