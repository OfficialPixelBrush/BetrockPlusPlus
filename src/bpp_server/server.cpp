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

void Server::broadcastPlayerMovement(PlayerSession& session) {
    double dx = session.position.pos.x - session.lastBroadcastPos.x;
    double dy = session.position.pos.y - session.lastBroadcastPos.y;
    double dz = session.position.pos.z - session.lastBroadcastPos.z;

    bool moved = (dx != 0.0 || dy != 0.0 || dz != 0.0);
    bool rotated = (session.rotation.x != session.lastBroadcastRotation.x ||
        session.rotation.y != session.lastBroadcastRotation.y);

    if (!moved && !rotated) return;

    // If delta is too large, use teleport instead
    bool needsTeleport = std::abs(dx) > 4.0 || std::abs(dy) > 4.0 || std::abs(dz) > 4.0;

    int8_t qx = static_cast<int8_t>(dx * 32.0);
    int8_t qy = static_cast<int8_t>(dy * 32.0);
    int8_t qz = static_cast<int8_t>(dz * 32.0);
    int8_t qpitch = static_cast<int8_t>(session.rotation.x / 360.0f * 256.0f);
    int8_t qyaw = static_cast<int8_t>(session.rotation.y / 360.0f * 256.0f);

    for (auto& other : players) {
        if (other.get() == &session) continue;
        if (other->connState != ConnectionState::Playing) continue;

        if (needsTeleport) {
            Packet::TeleportEntity pkt;
            pkt.entity_id = session.entityId;
            pkt.position.x = static_cast<int32_t>(session.position.pos.x * 32.0);
            pkt.position.y = static_cast<int32_t>(session.position.pos.y * 32.0);
            pkt.position.z = static_cast<int32_t>(session.position.pos.z * 32.0);
            pkt.pitch = qpitch;
            pkt.yaw = qyaw;
            pkt.Serialize(other->stream);
        }
        else if (moved && rotated) {
            Packet::EntityPositionAndRotation pkt;
            pkt.entity_id = session.entityId;
            pkt.qr_position.x = qx;
            pkt.qr_position.y = qy;
            pkt.qr_position.z = qz;
            pkt.q_pitch = qpitch;
            pkt.q_yaw = qyaw;
            pkt.Serialize(other->stream);
        }
        else if (moved) {
            Packet::EntityPosition pkt;
            pkt.entity_id = session.entityId;
            pkt.qr_position.x = qx;
            pkt.qr_position.y = qy;
            pkt.qr_position.z = qz;
            pkt.Serialize(other->stream);
        }
        else {
            Packet::EntityRotation pkt;
            pkt.entity_id = session.entityId;
            pkt.q_pitch = qpitch;
            pkt.q_yaw = qyaw;
            pkt.Serialize(other->stream);
        }
    }

    session.lastBroadcastPos = session.position.pos;
    session.lastBroadcastRotation = session.rotation;
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
            sendPendingChunks(*session, 10);
            broadcastPlayerMovement(*session);
            if (tickCounter % 20 == 0) {
                Packet::KeepAlive ka;
                ka.Serialize(session->stream);
            }
            break;
        }
    }

    // Mark clients who have timed out for removal
    auto now = std::chrono::steady_clock::now();
    for (auto& session : players) {
        if (session->connState == ConnectionState::Playing) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_packet_time).count();
            if (elapsed > this->timeout_seconds) {
                std::cout << "Player " << session->username << " timed out\n";
                disconnectPlayer(*session, "Connection timed out.");
            }
        }
    }

    // Force disconnect players that quit
    players.erase(
        std::remove_if(players.begin(), players.end(), [&](const auto& s) {
            if (!s->stream.isConnected()) {
                std::cout << "Disconnected client " << s->username << " with entity id " << s->entityId << "\n";
                for (auto& other : players) {
                    if (other.get() == s.get()) continue;
                    if (other->connState != ConnectionState::Playing) continue;
                    Packet::DespawnEntity despawn;
                    despawn.entity_id = s->entityId;
                    despawn.Serialize(other->stream);
                }
                return true;
            }
            return false;
            }),
        players.end()
    );
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

    Packet::SetHealth health;
    health.health = 20;
    health.Serialize(session.stream);

    Packet::SetTime time;
    time.time = 0;
    time.Serialize(session.stream);

    session.position.pos = { 0.0, 10.0, 0.0 };
    session.connState = ConnectionState::WaitingForSpawnChunks;
}

void Server::disconnectPlayer(PlayerSession& session, const std::string& reason) {
    // Send disconnect reason to the leaving player
    Packet::Disconnect kick;
    kick.reason = reason;
    kick.Serialize(session.stream);
    session.stream.setConnected(false);

    std::cout << "Player " << session.username << " disconnected: " << reason << "\n";
}

void Server::waitForSpawnChunks(PlayerSession& session) {
    sendPendingChunks(session, 10);

    // Spawn chunk radius; 3 chunks in each direction
    int spawnChunkX = int(std::floor(session.position.pos.x)) >> 4;
    int spawnChunkZ = int(std::floor(session.position.pos.z)) >> 4;

    int radius = std::min(3, world.getViewRadius());

    int total_spawn_chunks = ((radius * 2) + 1) * ((radius * 2) + 1);
    int loaded_chunks = 0;

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dz = -radius; dz <= radius; dz++) {
            ChunkPos p{ spawnChunkX + dx, spawnChunkZ + dz };

            if (session.sentChunks.contains(p)) {
                loaded_chunks++;
            }
        }
    }

    std::cout << "Spawn chunks: " << loaded_chunks << " / " << total_spawn_chunks << "\n";

    if (loaded_chunks < total_spawn_chunks) {
        return;
    }

    std::cout << "Spawn chunks sent. Setting player position" << "\n";
    Packet::PlayerPositionAndRotation pos;
    pos.x = session.position.pos.x;
    pos.y = session.position.pos.y;
    pos.stance = session.position.pos.y + 1.62;
    pos.z = session.position.pos.z;
    pos.yaw = 0.0f;
    pos.pitch = 0.0f;
    pos.onGround = false;
    pos.Serialize(session.stream);

    // Notify all currently playing/waiting players about this new player
    for (auto& other : players) {
        if (other.get() == &session) continue;
        if (other->connState != ConnectionState::Playing &&
            other->connState != ConnectionState::WaitingForSpawnChunks) continue;

        // Send this player to others
        Packet::SpawnPlayer splayer;
        splayer.entity_id = session.entityId;
        splayer.username = session.username;
        splayer.q_position.x = static_cast<int32_t>(session.position.pos.x * 32.0);
        splayer.q_position.y = static_cast<int32_t>(session.position.pos.y * 32.0);
        splayer.q_position.z = static_cast<int32_t>(session.position.pos.z * 32.0);
        splayer.q_yaw = static_cast<int8_t>(session.rotation.x / 360.0f * 256.0f);
        splayer.q_pitch = static_cast<int8_t>(session.rotation.y / 360.0f * 256.0f);
        splayer.held_item_id = 0;
        splayer.Serialize(other->stream);

        // Send others to this player
        Packet::SpawnPlayer oplayer;
        oplayer.entity_id = other->entityId;
        oplayer.username = other->username;
        oplayer.q_position.x = static_cast<int32_t>(other->position.pos.x * 32.0);
        oplayer.q_position.y = static_cast<int32_t>(other->position.pos.y * 32.0);
        oplayer.q_position.z = static_cast<int32_t>(other->position.pos.z * 32.0);
        oplayer.q_yaw = static_cast<int8_t>(other->rotation.x / 360.0f * 256.0f);
        oplayer.q_pitch = static_cast<int8_t>(other->rotation.y / 360.0f * 256.0f);
        oplayer.held_item_id = 0;
        oplayer.Serialize(session.stream);
    }

    std::cout << "Client connected" << "\n";
    session.connState = ConnectionState::Playing;
}

void Server::processIncoming(PlayerSession& session) {
    while (session.stream.hasData()) {
        PacketId packetId = session.stream.Read<PacketId>();
        switch (packetId) {
        case PacketId::KeepAlive: {
            Packet::KeepAlive ka;
            ka.Serialize(session.stream);
            break;
        }
        case PacketId::ChatMessage: {
            Packet::ChatMessage pkt;
            pkt.Deserialize(session.stream);
            std::cout << pkt.message << std::endl;
            break;
        }
        case PacketId::SetTime: {
            Packet::SetTime pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::InteractWithEntity: {
            Packet::InteractWithEntity pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::Respawn: {
            Packet::Respawn pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::PlayerMovement: {
            Packet::PlayerMovement pkt;
            pkt.Deserialize(session.stream);
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
            session.rotation.x = pkt.yaw;
            session.rotation.y = pkt.pitch;
            break;
        }
        case PacketId::PlayerPositionAndRotation: {
            Packet::PlayerPositionAndRotation pkt;
            pkt.Deserialize(session.stream);
            session.position.pos = { pkt.x, pkt.y, pkt.z };
            session.rotation.x = pkt.pitch;
            session.rotation.y = pkt.yaw;
            break;
        }
        case PacketId::MineBlock: {
            Packet::MineBlock pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::PlaceBlock: {
            Packet::PlaceBlock pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::SetHotbarSlot: {
            Packet::SetHotbarSlot pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::InteractWithBlock: {
            Packet::InteractWithBlock pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::Animation: {
            Packet::Animation pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::PlayerAction: {
            Packet::PlayerAction pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::PlayerInput: {
            Packet::PlayerInput pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        /*
        case PacketId::ClickSlot: {
            Packet::ClickSlot pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        */
        case PacketId::UpdateSign: {
            Packet::UpdateSign pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::Disconnect: {
            Packet::Disconnect pkt;
            pkt.Deserialize(session.stream);
            disconnectPlayer(session, pkt.reason);
            break;
        }
        default:
            std::cout << "UNHANDLED packet 0x" << std::hex << static_cast<int>(packetId) << "\n";
            disconnectPlayer(session, "Unknown packet");
            return;
        }
    }
    // Update our last packet time for the timeout code
    session.last_packet_time = std::chrono::steady_clock::now();
}

size_t Server::sendPendingChunks(PlayerSession& session, int batchSize) {
    int spawnChunkX = int(std::floor(session.position.pos.x)) >> 4;
    int spawnChunkZ = int(std::floor(session.position.pos.z)) >> 4;

    std::lock_guard lock(world.chunksMutex);

    std::vector<ChunkPos> toSend;
    std::vector<ChunkPos> toUnload;

    int view_radius = world.getViewRadius();
    // Find what chunks we need to send to the client
    for (int cx = -view_radius; cx <= view_radius; cx++)
        for (int cz = -view_radius; cz <= view_radius; cz++) {
            ChunkPos this_chunk_pos{ cx + spawnChunkX, cz + spawnChunkZ };
            if (!world.chunks.contains(this_chunk_pos)) continue;
            if (session.sentChunks.contains(this_chunk_pos)) continue;

            // Get our chunk
            auto& this_chunk = world.chunks[this_chunk_pos];
            if (this_chunk->state.load() != ChunkState::Lit) continue;
            toSend.push_back(this_chunk_pos);
        }

    // Remove chunks that were unloaded
    for (auto& chunk_pos : session.sentChunks) {
        if (std::abs(chunk_pos.x - spawnChunkX) > view_radius ||
            std::abs(chunk_pos.z - spawnChunkZ) > view_radius)
            toUnload.push_back(chunk_pos);
    }

    // Sort by distance so closer chunks load first
    std::sort(toSend.begin(), toSend.end(), [&](const ChunkPos& a, const ChunkPos& b) {
        int da = std::abs(a.x - spawnChunkX) + std::abs(a.z - spawnChunkZ);
        int db = std::abs(b.x - spawnChunkX) + std::abs(b.z - spawnChunkZ);
        return da < db;
        });

    // Batch send chunks up until batch size 
    int sent = 0;
    for (auto& pos : toSend) {
        if (batchSize > 0 && sent >= batchSize) break;

        Packet::SetChunkVisibility pre;
        pre.chunkX = pos.x;
        pre.chunkZ = pos.z;
        pre.visible = true;
        pre.Serialize(session.stream);

        Packet::ChunkData data;
        data.chunkX = pos.x * 16;
        data.chunkZ = pos.z * 16;
        data.compressedData = ChunkSerializer::serialize(*world.chunks.at(pos));
        data.Serialize(session.stream);

        session.sentChunks.insert(pos);
        sent++;
    }

    // Batch remove chunks up until batch size
    sent = 0;
    for (auto& pos : toUnload) {
        if (batchSize > 0 && sent >= batchSize) break;

        Packet::SetChunkVisibility pre;
        pre.chunkX = pos.x;
        pre.chunkZ = pos.z;
        pre.visible = false;
        pre.Serialize(session.stream);
        session.sentChunks.erase(pos);
        sent++;
    }

    return session.sentChunks.size();
}