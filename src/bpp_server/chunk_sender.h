/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <vector>
#include <unordered_map>
#include <mutex>
#include <future>
#include <algorithm>
#include "player_session.h"
#include "chunk_serializer.h"
#include "world/world.h"
#include "networking/packets.h"
#include "BS_thread_pool.hpp"

// ChunkSender offloads zlib chunk serialization onto a thread-pool so the
// main server tick is never blocked waiting for compression.
struct ChunkSender {
    // One result slot per in-flight chunk.
    struct PendingChunk {
        ChunkPos                          pos;
        std::future<std::vector<uint8_t>> data; // async compression result
    };

    // Per-session queue of in-flight serialization jobs.
    std::unordered_map<PlayerSession*, std::vector<PendingChunk>> inFlight;

    BS::thread_pool<> pool{ 2 }; // Only two thread

    size_t enqueue(PlayerSession& session, WorldManager& world, int batchSize = 10) {
        int cx = int(std::floor(session.position.pos.x)) >> 4;
        int cz = int(std::floor(session.position.pos.z)) >> 4;

        std::lock_guard lock(world.chunksMutex);

        int radius = world.getViewRadius();

        std::vector<ChunkPos> toSend;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                ChunkPos p{ cx + dx, cz + dz };
                if (!world.chunks.contains(p)) continue;
                if (session.sentChunks.contains(p)) continue;
                if (world.chunks[p]->state.load() < ChunkState::Generated) continue;
                toSend.push_back(p);
            }
        }

        // Sort closer chunks first
        std::sort(toSend.begin(), toSend.end(), [&](const ChunkPos& a, const ChunkPos& b) {
            int da = std::abs(a.x - cx) + std::abs(a.z - cz);
            int db = std::abs(b.x - cx) + std::abs(b.z - cz);
            return da < db;
            });

        // Unload all out-of-range chunks immediately — no batch cap.
        // Unloads are cheap (tiny packet + set erase) and must not lag behind
        // sends, otherwise stale sentChunks entries prevent world.chunks from
        // being freed and memory never comes back down.
        std::vector<ChunkPos> toUnload;
        for (auto& p : session.sentChunks) {
            if (std::abs(p.x - cx) > radius || std::abs(p.z - cz) > radius)
                toUnload.push_back(p);
        }
        for (auto& p : toUnload) {
            Packet::SetChunkVisibility vis;
            vis.chunkX = p.x;
            vis.chunkZ = p.z;
            vis.visible = false;
            vis.Serialize(session.stream);
            session.sentChunks.erase(p);
        }

        // Also cancel any in-flight jobs for chunks that are now out of range
        // so their shared_ptr refs are dropped and the chunks can be freed.
        auto& queue = inFlight[&session];
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [&](const PendingChunk& pc) {
                return std::abs(pc.pos.x - cx) > radius || std::abs(pc.pos.z - cz) > radius;
                }),
            queue.end()
        );

        // Rebuild toSend, excluding chunks that already have an in-flight job.
        // (sentChunks now only contains chunks confirmed written to the stream.)
        std::unordered_set<ChunkPos> inFlightSet;
        for (auto& pc : queue)
            inFlightSet.insert(pc.pos);

        toSend.erase(
            std::remove_if(toSend.begin(), toSend.end(), [&](const ChunkPos& p) {
                return inFlightSet.contains(p);
                }),
            toSend.end()
        );

        int submitted = 0;
        for (auto& p : toSend) {
            if (batchSize > 0 && submitted >= batchSize) break;
            // Don't add to sentChunks here — defer that to flush() so sentChunks
            // only contains chunks that have actually been written to the stream.
            PendingChunk pc;
            std::shared_ptr<Chunk> chunkRef = world.chunks.at(p);
            pc.pos = p;
            pc.data = pool.submit_task([chunkRef]() {
                return ChunkSerializer::serialize(*chunkRef);
                });
            queue.push_back(std::move(pc));
            ++submitted;
        }
        return static_cast<size_t>(submitted);
    }

    // Drains every job that is already done and writes the resulting
    // SetChunkVisibility + ChunkData packets to the session stream.
    // Jobs that aren't finished yet stay in the queue for the next tick.
    void flush(PlayerSession& session) {
        auto it = inFlight.find(&session);
        if (it == inFlight.end()) return;

        auto& queue = it->second;
        std::vector<PendingChunk> stillPending;

        for (auto& pc : queue) {
            // Non-blocking check: only consume results that are ready now.
            if (pc.data.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                stillPending.push_back(std::move(pc));
                continue;
            }

            auto compressed = pc.data.get();

            Packet::SetChunkVisibility vis;
            vis.chunkX = pc.pos.x;
            vis.chunkZ = pc.pos.z;
            vis.visible = true;
            vis.Serialize(session.stream);

            Packet::ChunkData data;
            data.chunkX = pc.pos.x * 16;
            data.chunkZ = pc.pos.z * 16;
            data.compressedData = std::move(compressed);
            data.Serialize(session.stream);
            // Mark as sent only once the data is actually on the wire.
            session.sentChunks.insert(pc.pos);
            session.flushedChunks.insert(pc.pos);
        }

        queue = std::move(stillPending);
    }

    // Remove all in-flight state for a disconnected session.
    void remove(PlayerSession& session) {
        inFlight.erase(&session);
    }
};