/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
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
    command_manager.Init();
    world.seed = 404;

    world.onBlockUpdate = [this](PendingBlock pendingBlock, ChunkPos chunkPos) {
        for (auto& session : players) {
            if (!session->sentChunks.contains(chunkPos)) continue;
            PendingBlock pendingNew = pendingBlock;
            pendingNew.block_pos = { pendingBlock.block_pos.x & 15, pendingBlock.block_pos.y, pendingBlock.block_pos.z & 15 };
            chunkBlockChanges[chunkPos].push_back(pendingNew);
        }
        };

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(25565);
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

Server::~Server() {
#if defined(_WIN32) || defined(_WIN64)
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif
}

void Server::startup() {
    printf("Initializing server startup.. \n");
    printf("Thread count: %i\n", int(world.pool.get_thread_count()));
    printf("Server spawn is (%i, %i)\n", int(spawnPoint.x), int(spawnPoint.y));
    printf("Loading 676 spawn chunks..\n");

    // Push every single spawn chunk to get ready for generation
    std::unordered_set<ChunkPos> wanted;
    for (int dx = -13; dx < 13; dx++)
        for (int dz = -13; dz < 13; dz++) {
            wanted.insert({ (spawnPoint.x >> 4) + dx, (spawnPoint.z >> 4) + dz });

            for (const auto& pos : wanted) {
                if (!world.chunks.contains(pos)) {
                    auto c = std::make_shared<Chunk>();
                    c->spawnChunk = true;
                    c->cpos = pos;
                    world.chunks.emplace(pos, std::move(c));
                }
            }
        }

    int total_spawn_chunks = 676;
    int loaded_chunks = 0;
    bool spawnDone = false;
    auto start = std::chrono::steady_clock::now();
    std::cout << "Loading spawn.. 0%\n";
    while (!spawnDone) {
        loaded_chunks = 0;
        // Force gen these chunks AS FAST AS POSSIBLE
        world.pumpPipeline(std::vector<ClientPosition>{});
        world.pool.wait();
        world.drainGenQueue();
        world.populateReady();
        // Make sure all lighting is done
        world.lightManager.processLightQueue(world);

        // Beta uses -13 to 13 with only -13 being inclusive.. for some reason.
        for (int dx = -13; dx < 13; dx++) {
            for (int dz = -13; dz < 13; dz++) {
                ChunkPos p{ (spawnPoint.x >> 4) + dx, (spawnPoint.z >> 4) + dz };
                auto it = world.chunks.find(p);
                if (it != world.chunks.end() && it->second->state.load() >= ChunkState::Generated)
                    loaded_chunks++;
            }
        }

        // Update load percentage every second
        if (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= 1.0f)
        {
            int percentLoaded = int(((float)loaded_chunks / total_spawn_chunks) * 100);
            std::cout << "Loading spawn.. " << percentLoaded << "%\n";
            start = std::chrono::steady_clock::now();
        }

        // Have we loaded all the spawn chunks?
        if (loaded_chunks >= total_spawn_chunks)
            spawnDone = true;
    }
    std::cout << "Loading spawn.. 100%\n";
    printf("Startup Complete.\n");
}

void Server::run() {
    startup();
    auto lastTime = std::chrono::steady_clock::now();

    while (true) {
        int ticks_ran = 0;

        auto now = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        accumulator += delta;

        while (accumulator >= TICK_DELTA && ticks_ran <= MAX_TICKS_PER_FRAME) {
            auto tickStart = std::chrono::steady_clock::now();
            tick();
            float tickMs = std::chrono::duration<float>(std::chrono::steady_clock::now() - tickStart).count() * 1000.0f;
            if (tickMs > 50.0f) {
                printf("Can't keep up! Tick took %i ms\n", int(tickMs));
            }
            tickTimeAccum += tickMs;
            tickCount++;
            if (tickCount >= 40) {
                printf("Avg tick: %.2f ms\n", tickTimeAccum / tickCount);
                tickTimeAccum = 0.0f;
                tickCount = 0;
            }
            accumulator -= TICK_DELTA;
            ticks_ran++;
        }

        if (ticks_ran == MAX_TICKS_PER_FRAME)
            accumulator = 0.0f;

        acceptNewPlayers();
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
    players.back()->players = &players;
}

// Position deltas are computed as fixed-point integers (coord * 32) to avoid
// floating-point drift. Threshold of 128 fp units (= 4 blocks) triggers an
// absolute TeleportEntity instead of a relative move packet.

// The client's handleEntityTeleport adds 1/64 to the decoded Y
// (serverPosY/32 + 1/64), so we subtract that offset when building the
// teleport Y so the entity lands on feet position, not 1/64 above it.
// Relative move packets do NOT have this offset (handleEntity uses /32 only).
void Server::broadcastPlayerMovement(PlayerSession& session) {
    // Convert current position to fixed-point integers — wire scale
    int32_t newFpX = static_cast<int32_t>(session.position.pos.x * 32.0);
    // Subtract 1/64 offset: client adds (serverPosY/32 + 1/64) on teleport,
    // so encode as (feet_y - 1/64) * 32 = feet_y*32 - 0.5, truncated
    int32_t newFpY = static_cast<int32_t>((session.position.pos.y - (1.0 / 64.0)) * 32.0);
    int32_t newFpZ = static_cast<int32_t>(session.position.pos.z * 32.0);
    // rotation.x = yaw, rotation.y = pitch
    int8_t newYaw = static_cast<int8_t>(session.rotation.x / 360.0f * 256.0f);
    int8_t newPitch = static_cast<int8_t>(session.rotation.y / 360.0f * 256.0f);

    // Relative-move Y does not have the 1/64 offset
    int32_t relFpY = static_cast<int32_t>(session.position.pos.y * 32.0);

    int32_t dx = newFpX - session.lastFpX;
    int32_t dy = relFpY - session.lastFpY;
    int32_t dz = newFpZ - session.lastFpZ;

    bool moved = (dx != 0 || dy != 0 || dz != 0);
    bool rotated = (newYaw != session.lastYaw || newPitch != session.lastPitch);

    if (!moved && !rotated) return;

    // If delta is too large, use teleport instead (128 fp units = 4 blocks)
    bool needsTeleport = (std::abs(dx) > 128 || std::abs(dy) > 128 || std::abs(dz) > 128);

    for (auto& other : players) {
        if (other.get() == &session) continue;
        if (other->connState != ConnectionState::Playing) continue;

        if (needsTeleport) {
            Packet::TeleportEntity pkt;
            pkt.entity_id = session.entityId;
            pkt.position.x = newFpX;
            pkt.position.y = newFpY;
            pkt.position.z = newFpZ;
            pkt.yaw = newYaw;
            pkt.pitch = newPitch;
            pkt.Serialize(other->stream);
        }
        else if (moved && rotated) {
            Packet::EntityPositionAndRotation pkt;
            pkt.entity_id = session.entityId;
            pkt.qr_position.x = static_cast<int8_t>(dx);
            pkt.qr_position.y = static_cast<int8_t>(dy);
            pkt.qr_position.z = static_cast<int8_t>(dz);
            pkt.q_yaw = newYaw;
            pkt.q_pitch = newPitch;
            pkt.Serialize(other->stream);
        }
        else if (moved) {
            Packet::EntityPosition pkt;
            pkt.entity_id = session.entityId;
            pkt.qr_position.x = static_cast<int8_t>(dx);
            pkt.qr_position.y = static_cast<int8_t>(dy);
            pkt.qr_position.z = static_cast<int8_t>(dz);
            pkt.Serialize(other->stream);
        }
        else {
            Packet::EntityRotation pkt;
            pkt.entity_id = session.entityId;
            pkt.q_yaw = newYaw;
            pkt.q_pitch = newPitch;
            pkt.Serialize(other->stream);
        }
    }

    // Update last-broadcast state using the same Y as relative moves
    session.lastFpX = newFpX;
    session.lastFpY = relFpY;
    session.lastFpZ = newFpZ;
    session.lastYaw = newYaw;
    session.lastPitch = newPitch;
}

void Server::tick() {
    std::vector<ClientPosition> positions;
    for (auto& session : players) {
        if (session->connState == ConnectionState::WaitingForSpawnChunks ||
            session->connState == ConnectionState::Playing) {
            positions.push_back(session->position);
        }
    }
    world.tick(positions);
    world.update(positions);

    // Block changes accumulate in chunkBlockChanges during setBlock() calls
    // (all on main thread). Swap out here so the loop below sees a stable snapshot.
    std::unordered_map<ChunkPos, std::vector<PendingBlock>> localBlockChanges;
    localBlockChanges.swap(chunkBlockChanges);

    for (auto& session : players) {
        switch (session->connState) {
        case ConnectionState::Handshaking:           handleHandshake(*session);    break;
        case ConnectionState::LoggingIn:             handleLogin(*session);        break;
        case ConnectionState::WaitingForSpawnChunks: waitForSpawnChunks(*session); break;
        case ConnectionState::Playing:
            processIncoming(*session);
            chunkSender.enqueue(*session, world, 10);
            chunkSender.flush(*session);
            broadcastPlayerMovement(*session);
            if (world.elapsed_ticks % 20 == 0) {
                // Keep alive packet or else beta fusses
                Packet::KeepAlive ka;
                ka.Serialize(session->stream);

                // Update the server time so client's don't desync
                Packet::SetTime time;
                time.time = world.elapsed_ticks;
                time.Serialize(session->stream);
            }

            break;
        }
    }

    // Dispatch block changes to all sessions that know about each chunk.
    for (auto it = localBlockChanges.begin(); it != localBlockChanges.end(); ++it) {
        const ChunkPos& chunk = it->first;
        const std::vector<PendingBlock>& changes = it->second;
        // Capture as shared_ptr so the async compression job keeps the chunk
        // alive until the pool thread finishes.
        std::shared_ptr chunkRef = world.getChunk(chunk);

        for (auto& session : players) {
            // Include WaitingForSpawnChunks: their chunks are in-flight so
            // updates queue in pendingBlockChanges and drain via flush().
            if (session->connState != ConnectionState::Playing &&
                session->connState != ConnectionState::WaitingForSpawnChunks) continue;

            if (session->flushedChunks.contains(chunk)) {
                chunkSender.sendBlockUpdates(*session, chunk, changes, chunkRef);
            }
            else if (session->sentChunks.contains(chunk)) {
                auto& q = session->pendingBlockChanges[chunk];
                q.insert(q.end(), changes.begin(), changes.end());
            }
        }
    }

    // Flush all pending outgoing data to the socket once per tick.
    for (auto& session : players) {
        session->stream.flushWriteBuffer();
    }

    // Mark clients who have timed out for removal
    auto now = std::chrono::steady_clock::now();
    for (auto& session : players) {
        if (session->connState == ConnectionState::Playing) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - session->last_packet_time).count();
            if (elapsed > timeout_seconds) {
                std::wcout << L"Player " << session->username << L" timed out\n";
                disconnectPlayer(*session, L"Connection timed out.");
            }
        }
    }

    // Force disconnect players that quit
    players.erase(
        std::remove_if(players.begin(), players.end(), [&](const auto& s) {
            if (!s->stream.isConnected()) {
                std::wcout << L"Disconnected client " << s->username
                    << L" with entity id " << s->entityId << L"\n";
                chunkSender.remove(*s);
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
    std::wcout << L"Player " << session.username << L" is logging in.\n";

    Packet::PreLogin response;
    response.username = L"-";
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
    std::wcout << L"Player " << session.username << L" logged in with entity ID " << session.entityId << L".\n";
    Packet::Login response;
    response.entity_id = session.entityId;
    response.username = session.username;
    response.worldSeed = world.seed;
    response.dimension = Dimension::Overworld;
    response.Serialize(session.stream);

    Packet::SetSpawnPosition spawn;
    spawn.position = { 0, 200, 0 };
    spawn.Serialize(session.stream);

    Packet::SetHealth health;
    health.health = 20;
    health.Serialize(session.stream);

    Packet::SetTime time;
    time.time = world.elapsed_ticks;
    time.Serialize(session.stream);

    session.position.pos = { 0.0, 200.0, 0.0 };
    session.connState = ConnectionState::WaitingForSpawnChunks;
}

void Server::disconnectPlayer(PlayerSession& session, const std::wstring& reason) {
    // Send disconnect reason to the leaving player
    Packet::Disconnect kick;
    kick.reason = reason;
    kick.Serialize(session.stream);
    session.stream.setConnected(false);

    std::wcout << L"Player " << session.username << L" disconnected: " << reason << L"\n";
}

void Server::waitForSpawnChunks(PlayerSession& session) {
    chunkSender.enqueue(session, world, 10);
    chunkSender.flush(session);

    // Spawn chunk radius; 3 chunks in each direction
    int spawnChunkX = int(std::floor(session.position.pos.x)) >> 4;
    int spawnChunkZ = int(std::floor(session.position.pos.z)) >> 4;

    int radius = min(3, world.getViewRadius());

    int total_spawn_chunks = ((radius * 2) + 1) * ((radius * 2) + 1);
    int loaded_chunks = 0;

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dz = -radius; dz <= radius; dz++) {
            ChunkPos p{ spawnChunkX + dx, spawnChunkZ + dz };
            if (session.flushedChunks.contains(p))
                loaded_chunks++;
        }
    }

    std::cout << "Spawn chunks: " << loaded_chunks << " / " << total_spawn_chunks << "\n";

    if (loaded_chunks < total_spawn_chunks)
        return;

    std::cout << "Spawn chunks sent. Setting player position\n";

    Packet::PlayerPositionAndRotation pos;
    pos.x = session.position.pos.x;
    pos.y = session.position.pos.y;
    pos.stance = session.position.pos.y + 1.62;
    pos.z = session.position.pos.z;
    pos.yaw = 0.0f;
    pos.pitch = 0.0f;
    pos.onGround = false;
    pos.Serialize(session.stream);

    // Initialise fixed-point tracking from actual spawn position.
    // Use the same Y encoding as relative moves (no 1/64 offset).
    session.lastFpX = static_cast<int32_t>(session.position.pos.x * 32.0);
    session.lastFpY = static_cast<int32_t>(session.position.pos.y * 32.0);
    session.lastFpZ = static_cast<int32_t>(session.position.pos.z * 32.0);
    session.lastYaw = 0;
    session.lastPitch = 0;

    // Notify all currently playing/waiting players about this new player
    for (auto& other : players) {
        if (other.get() == &session) continue;
        if (other->connState != ConnectionState::Playing &&
            other->connState != ConnectionState::WaitingForSpawnChunks) continue;

        // Send this player to others using fixed-point last-broadcast values
        {
            Packet::SpawnPlayer pkt;
            pkt.entity_id = session.entityId;
            pkt.username = session.username;
            pkt.q_position.x = session.lastFpX;
            // SpawnPlayer Y decoded as serverPosY/32 (no 1/64 offset on client)
            pkt.q_position.y = session.lastFpY;
            pkt.q_position.z = session.lastFpZ;
            pkt.q_yaw = session.lastYaw;
            pkt.q_pitch = session.lastPitch;
            pkt.held_item_id = 0;
            pkt.Serialize(other->stream);
        }

        // Send others to this player using their fixed-point last-broadcast values.
        // Follow up with TeleportEntity so position is exact — the 1/64 offset
        // in handleEntityTeleport will correctly place the entity at feet level.
        {
            Packet::SpawnPlayer pkt;
            pkt.entity_id = other->entityId;
            pkt.username = other->username;
            pkt.q_position.x = other->lastFpX;
            pkt.q_position.y = other->lastFpY;
            pkt.q_position.z = other->lastFpZ;
            pkt.q_yaw = other->lastYaw;
            pkt.q_pitch = other->lastPitch;
            pkt.held_item_id = 0;
            pkt.Serialize(session.stream);

            // TeleportEntity Y = (feet_y - 1/64) * 32 so client decodes correctly
            Packet::TeleportEntity tel;
            tel.entity_id = other->entityId;
            tel.position.x = other->lastFpX;
            tel.position.y = static_cast<int32_t>(
                (other->position.pos.y - (1.0 / 64.0)) * 32.0);
            tel.position.z = other->lastFpZ;
            tel.yaw = other->lastYaw;
            tel.pitch = other->lastPitch;
            tel.Serialize(session.stream);
        }
    }

    std::cout << "Client connected\n";
    session.connState = ConnectionState::Playing;
}

void Server::processIncoming(PlayerSession& session) {
    while (session.stream.hasData()) {
        PacketId packetId = session.stream.Read<PacketId>();
        switch (packetId) {
        case PacketId::KeepAlive: {
            Packet::KeepAlive pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::KeepAlive(pkt, session);
            break;
        }
        case PacketId::ChatMessage: {
            Packet::ChatMessage pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::ChatMessage(pkt, session, players, command_manager);
            break;
        }
        case PacketId::SetTime: {
            // Server-authoritative; client should not send this — consume and ignore.
            Packet::SetTime pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::InteractWithEntity: {
            Packet::InteractWithEntity pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::InteractWithEntity(pkt, session);
            break;
        }
        case PacketId::Respawn: {
            Packet::Respawn pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::Respawn(pkt, session);
            break;
        }
        case PacketId::PlayerMovement: {
            Packet::PlayerMovement pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerMovement(pkt, session);
            break;
        }
        case PacketId::PlayerPosition: {
            Packet::PlayerPosition pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerPosition(pkt, session);
            break;
        }
        case PacketId::PlayerRotation: {
            Packet::PlayerRotation pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerRotation(pkt, session);
            break;
        }
        case PacketId::PlayerPositionAndRotation: {
            Packet::PlayerPositionAndRotation pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerPositionAndRotation(pkt, session);
            break;
        }
        case PacketId::MineBlock: {
            Packet::MineBlock pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::MineBlock(pkt, session, world, players);
            break;
        }
        case PacketId::PlaceBlock: {
            Packet::PlaceBlock pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlaceBlock(pkt, session, world, players);
            break;
        }
        case PacketId::SetHotbarSlot: {
            Packet::SetHotbarSlot pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::SetHotbarSlot(pkt, session);
            break;
        }
        case PacketId::InteractWithBlock: {
            Packet::InteractWithBlock pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::InteractWithBlock(pkt, session, world);
            break;
        }
        case PacketId::Animation: {
            Packet::Animation pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::Animation(pkt, session, players);
            break;
        }
        case PacketId::PlayerAction: {
            Packet::PlayerAction pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerAction(pkt, session);
            break;
        }
        case PacketId::PlayerInput: {
            // Client-only packet; consume and ignore.
            Packet::PlayerInput pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::CloseContainer: {
            Packet::CloseContainer pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::CloseContainer(pkt, session);
            break;
        }
        case PacketId::ClickSlot: {
            Packet::ClickSlot pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::ClickSlot(pkt, session);
            break;
        }
        case PacketId::UpdateSign: {
            Packet::UpdateSign pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::UpdateSign(pkt, session, world);
            break;
        }
        case PacketId::Disconnect: {
            Packet::Disconnect pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::Disconnect(pkt, session);
            return; // session is dead; stop processing
        }
        default:
            std::cout << "UNHANDLED packet 0x" << std::hex
                << static_cast<int>(packetId) << "\n";
            disconnectPlayer(session, L"Unknown packet");
            return;
        }
    }
    // Update our last packet time for the timeout code
    session.last_packet_time = std::chrono::steady_clock::now();
}