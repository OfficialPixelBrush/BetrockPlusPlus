/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#if defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <fcntl.h>
#elif defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#include <iostream>
#include <thread>
#include <chrono>
#include "server.h"

Server::Server() {
    Blocks::registerAll();
    world.seed = 12345;

    #if defined(_WIN32) || defined(_WIN64)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(25565);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(serverSocket, 8);

    #if defined(_WIN32) || defined(_WIN64)
        u_long mode = 1;
        ioctlsocket(serverSocket, FIONBIO, &mode);
    #else
        fcntl(serverSocket, F_SETFL, O_NONBLOCK);
    #endif
}

Server::~Server() {
    #if defined(_WIN32) || defined(_WIN64)
        closesocket(serverSocket);
        WSACleanup();
    #else
        close(serverSocket);
    #endif
}

void Server::run() {
    while (true) {
        acceptNewPlayers();
        tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Server::acceptNewPlayers() {
    int clientSocket = static_cast<int>(accept(serverSocket, nullptr, nullptr));

    #if defined(_WIN32) || defined(_WIN64)
        if (clientSocket == INVALID_SOCKET) return;
    #else
        if (clientSocket < 0) return;
    #endif

    std::cout << "New player connected. Total players: " << players.size() + 1 << "\n";
    players.push_back(std::make_unique<PlayerSession>(clientSocket));
}

void Server::tick() {
    std::vector<ClientPosition> positions;
    for (auto& session : players) {
        if (session->connState == ConnectionState::WaitingForSpawnChunks ||
            session->connState == ConnectionState::Playing)
            positions.push_back(session->position);
    }

    world.tick(positions);
    tickCounter++;

    for (auto& session : players) {
        switch (session->connState) {
        case ConnectionState::Handshaking:           handleHandshake(*session);     break;
        case ConnectionState::LoggingIn:             handleLogin(*session);         break;
        case ConnectionState::WaitingForSpawnChunks: waitForSpawnChunks(*session);  break;
        case ConnectionState::Playing:
            processIncoming(*session);
            sendPendingChunks(*session);
            if (tickCounter % 20 == 0) {
                Packet::KeepAlive ka;
                ka.Serialize(session->stream);
            }
            break;
        }
    }
}

void Server::handleHandshake(PlayerSession& session) {
    if (!session.stream.hasData()) return;

    PacketId packetId = session.stream.Read<PacketId>();
    if (packetId != PacketId::PreLogin) return;

    Packet::PreLogin incoming;
    incoming.Deserialize(session.stream);
    session.username = incoming.username;
    std::cout << "Player " << session.username << " is logging in.\n";

    Packet::PreLogin response;
    response.username = "-";
    response.Serialize(session.stream);

    session.connState = ConnectionState::LoggingIn;
}

void Server::handleLogin(PlayerSession& session) {
    if (!session.stream.hasData()) return;

    PacketId packetId = session.stream.Read<PacketId>();
    if (packetId != PacketId::Login) return;

    Packet::Login incoming;
    incoming.Deserialize(session.stream);

    session.entityId = nextEntityId++;
    std::cout << "Player " << session.username << " logged in with entity ID " << session.entityId << ".\n";

    Packet::Login response;
    response.entity_id = session.entityId;
    response.username = session.username;
    response.worldSeed = world.seed;
    response.dimension = Dimension::Overworld;
    response.Serialize(session.stream);

    Packet::SetSpawnPosition spawn;
    spawn.position = { 0, 64, 0 };
    spawn.Serialize(session.stream);

    session.position.pos = { 0.0, 120.0, 0.0 };
    session.connState = ConnectionState::WaitingForSpawnChunks;
}

void Server::waitForSpawnChunks(PlayerSession& session) {
    std::cout << "STATE: WaitingForSpawnChunks\n";
    bool allSent = false;
    try {
        allSent = sendPendingChunks(session);
    }
    catch (const std::exception& e) {
        std::cout << "EXCEPTION in sendPendingChunks: " << e.what() << "\n";
        return;
    }
    catch (...) {
        std::cout << "UNKNOWN EXCEPTION in sendPendingChunks\n";
        return;
    }
    std::cout << "sendPendingChunks returned: " << allSent << "\n";
    if (!allSent) return;

    std::cout << "All spawn chunks sent to player " << session.username << ".\n";

    Packet::PlayerPositionAndRotation pos;
    pos.x = 0.0;
    pos.y = 120.0;
    pos.stance = 121.62;
    pos.z = 0.0;
    pos.yaw = 0.0f;
    pos.pitch = 0.0f;
    pos.onGround = false;
    pos.Serialize(session.stream);

    session.connState = ConnectionState::Playing;
    std::cout << "connState is now: " << static_cast<int>(session.connState) << "\n"; // should print 3
    std::cout << "TRANSITIONING TO PLAYING\n";
}

void Server::processIncoming(PlayerSession& session) {
    while (session.stream.hasData()) {
        PacketId packetId = session.stream.Read<PacketId>();

		std::cout << "Recieved packet " << static_cast<int>(packetId) << " from player " << session.username << ".\n";

        switch (packetId) {
        case PacketId::KeepAlive: {
            Packet::KeepAlive ka;
            ka.Serialize(session.stream);
            break;
        }
        case PacketId::PlayerPositionAndRotation: {
            Packet::PlayerPositionAndRotation pkt;
            pkt.Deserialize(session.stream);
            session.position.pos = { pkt.x, pkt.y, pkt.z };
            break;
        }
        case PacketId::PlayerPosition: {
            Packet::PlayerPosition pkt;
            pkt.Deserialize(session.stream);
            session.position.pos = pkt.position;
            break;
        }
        case PacketId::PlayerRotation: {
            Packet::PlayerRotation pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::PlayerMovement: {
            Packet::PlayerMovement pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        default:
            return;
        }
    }
}

bool Server::sendPendingChunks(PlayerSession& session) {
    std::lock_guard lock(world.chunksMutex);
    for (auto& [pos, chunk] : world.chunks) {
        if (chunk->state.load() != ChunkState::Lit) {
            std::cout << "Chunk at " << pos.x << ", " << pos.z << " is not ready to be sent (state: " << static_cast<int>(chunk->state.load()) << ").\n";
			std::cout << "Aborting chunk sending for player " << session.username << ".\n";
            return false;
        }
        if (session.sentChunks.contains(pos)) {
            continue;
        }

        Packet::SetChunkVisibility pre;
        pre.chunkX = pos.x;
        pre.chunkZ = pos.z;
        pre.visible = true;
        pre.Serialize(session.stream);

        Packet::ChunkData data;
        data.chunkX = pos.x;
        data.chunkZ = pos.z;
        data.compressedData = ChunkSerializer::serialize(*chunk);
        data.Serialize(session.stream);

        std::cout << "Chunk sent at " << pos.x << ", " << pos.z << "\n";

        session.sentChunks.insert(pos);
    }
    return true;
}