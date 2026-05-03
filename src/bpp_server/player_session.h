/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <cstdint>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "tile_entities/tile_entity.h"
#include "world/chunk.h"
#include "world/world.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "world/client_pos.h"
#include "inventory/inventories.h"

enum class ConnectionState : uint8_t {
    Handshaking,
    LoggingIn,
    WaitingForSpawnChunks,
    Playing
};

struct PlayerSession {
    NetworkStream stream;
    ClientPosition position;

    // Non-owning pointer to the server's player list; set by Server after push_back.
    // Commands use this to look up other sessions by username.
    std::vector<std::unique_ptr<PlayerSession>>* players = nullptr;

    // rotation.x = yaw, rotation.y = pitch
    Float2 rotation = { 0.0f, 0.0f };

    // Stored as integer * 32 — the same unit as the wire format.
    int32_t lastFpX = 0;
    int32_t lastFpY = 0;
    int32_t lastFpZ = 0;
    int8_t  lastYaw = 0;
    int8_t  lastPitch = 0;

    std::unordered_set<ChunkPos> sentChunks;
    std::unordered_set<ChunkPos> flushedChunks; // actually written to stream

    // Block updates that arrived while the chunk was enqueued but not yet
    // flushed. Drained by ChunkSender::flush() the moment the chunk data
    // is written, so the client never sees updates before its chunk.
    std::unordered_map<ChunkPos, std::vector<PendingBlock>> pendingBlockChanges;

    // Chunks that were written to the stream during the last flush() call.
    // Drained by Server::tick() to update the chunk-session reverse index.
    std::vector<ChunkPos> newlyFlushed;

    // Chunks that were unloaded during the last enqueue() call.
    // Drained by Server::tick() to update the chunk-session reverse index.
    std::vector<ChunkPos> newlyUnloaded;
    ConnectionState connState = ConnectionState::Handshaking;
    EntityId entityId = 0;
    std::wstring username;
    std::chrono::steady_clock::time_point last_packet_time = std::chrono::steady_clock::now();

    // Inventory
    InventoryPlayer inventory;
    // windowId=0 is always the player inventory. Non-zero means a container is open.
    int8_t openWindowId = 0;
    TileEntityChest* openChest = nullptr;

    // Transaction system: locked after a rejected click until client acks the resync.
    // While locked, all incoming clicks are rejected to prevent state corruption.
    bool          inventoryLocked = false;
    TransactionId pendingRejectAction = 0;

    explicit PlayerSession(int socket) : stream(socket) {}
};