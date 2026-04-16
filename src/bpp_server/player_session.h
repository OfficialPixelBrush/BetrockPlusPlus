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
#include "world/chunk.h"
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
    ConnectionState connState = ConnectionState::Handshaking;
    EntityId entityId = 0;
    std::string username;
    std::chrono::steady_clock::time_point last_packet_time = std::chrono::steady_clock::now();

    explicit PlayerSession(int socket) : stream(socket) {}
};