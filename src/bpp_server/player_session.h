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
#include "inventory/inventory_interaction.h"

enum class ConnectionState : uint8_t {
    Handshaking,
    LoggingIn,
    WaitingForSpawnChunks,
    Playing
};

struct PlayerSession {
    NetworkStream stream;
    ClientPosition position;

    // Commands use this to look up other sessions by username.
    // Should probably handle this in the server!!
    std::vector<std::unique_ptr<PlayerSession>>* players = nullptr;

    // rotation.x = yaw, rotation.y = pitch
    Float2 rotation = { 0.0f, 0.0f };

    // Stored as integer * 32
    int32_t lastFpX = 0;
    int32_t lastFpY = 0;
    int32_t lastFpZ = 0;
    int8_t  lastYaw = 0;
    int8_t  lastPitch = 0;

    std::unordered_set<Int32_2> sentChunks;
    std::unordered_set<Int32_2> flushedChunks; // actually written to stream

    // Block updates that arrived while the chunk was enqueued but not yet flushed.
    std::unordered_map<Int32_2, std::vector<PendingBlock>> pendingBlockChanges;

    // Chunks that were written to the stream during the last flush() call.
    std::vector<Int32_2> newlyFlushed;

    // Chunks that were unloaded during the last enqueue() call.
    std::vector<Int32_2> newlyUnloaded;
    ConnectionState connState = ConnectionState::Handshaking;
    EntityId entityId = 0;
    std::wstring username;
    std::chrono::steady_clock::time_point last_packet_time = std::chrono::steady_clock::now();

    // Inventory
    InventoryPlayer            inventory;
    PlayerInventoryInteraction inventoryInteraction;
    std::unique_ptr<InventoryInteraction> activeInteraction = nullptr;

    // windowId = 0 is always the player inventory. Non-zero means a container is open.
    // ranges from 0-127 and wraps
    int8_t openWindowId = 0;
    int8_t getNextWindowId() {
        openWindowId = (openWindowId + 1) % 128;
        return openWindowId;
    }

    // Lock after a rejected click until client acknowledges the resync.
    // While locked, all incoming clicks are rejected to prevent state corruption.
    bool          inventoryLocked = false;
    TransactionId pendingTransactionId = 0;
    WindowId      pendingWindowId = 0;

    explicit PlayerSession(int socket) : stream(socket), inventoryInteraction(&inventory) {}
};