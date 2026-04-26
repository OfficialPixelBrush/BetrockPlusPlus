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
#include <future>
#include <algorithm>
#include <thread>
#include <minmax.h>
#include "player_session.h"
#include "chunk_serializer.h"
#include "world/world.h"
#include "networking/packets.h"
#include "BS_thread_pool.hpp"

// ChunkSender offloads zlib chunk serialization onto a thread-pool so the
// main server tick is never blocked waiting for compression.
// enqueue() and flush() must only be called from the main thread.
struct ChunkSender {
    // One result slot per in-flight chunk.
    struct PendingChunk {
        ChunkPos                          pos;
        std::future<std::vector<uint8_t>> data;     // async compression result
        std::shared_ptr<Chunk>            chunkRef; // kept alive until flush drains pending updates
    };

    // One result slot per in-flight sub-region block update (>= 10 changes).
    struct PendingSubRegion {
        ChunkPos                          chunkPos;
        Packet::ChunkData                 header;  // pre-filled, compressedData empty until ready
        std::future<std::vector<uint8_t>> data;
    };

    // Per-session queue of in-flight full-chunk serialization jobs.
    std::unordered_map<PlayerSession*, std::vector<PendingChunk>> inFlight;

    // Per-session queue of in-flight sub-region compression jobs.
    // Drained in-order by flush() after the full-chunk queue.
    std::unordered_map<PlayerSession*, std::vector<PendingSubRegion>> subRegionFlight;

    BS::thread_pool<> pool{ 2 };

    size_t enqueue(PlayerSession& session, WorldManager& world, int batchSize = -1) {
        if (batchSize < 0) batchSize = static_cast<int>(pool.get_thread_count()) * 2;
        int cx = int(std::floor(session.position.pos.x)) >> 4;
        int cz = int(std::floor(session.position.pos.z)) >> 4;

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

        // Unload all out-of-range chunks immediately
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
            session.flushedChunks.erase(p);
            session.pendingBlockChanges.erase(p); // drop queued updates for unloaded chunk
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
        int submitted = 0;
        for (auto& p : toSend) {
            if (batchSize > 0 && submitted >= batchSize) break;
            PendingChunk pc;
            std::shared_ptr<Chunk> chunkRef = world.chunks.at(p);
            pc.pos = p;
            pc.chunkRef = chunkRef;
            pc.data = pool.submit_task([chunkRef]() {
                return ChunkSerializer::serialize(*chunkRef);
                });
            queue.push_back(std::move(pc));
            session.sentChunks.insert(p);
            ++submitted;
        }
        return static_cast<size_t>(submitted);
    }

    void sendBlockUpdates(PlayerSession& session, const ChunkPos& chunk,
        const std::vector<PendingBlock>& changes,
        std::shared_ptr<const Chunk> chunkRef = nullptr) {
        if (changes.empty()) return;

        if (changes.size() == 1) {
            const PendingBlock& pb = changes[0];
            Packet::SetBlock sb;
            sb.block = { pb.block.type, pb.block.data };
            sb.position = {
                static_cast<int32_t>(pb.block_pos.x + (chunk.x * 16)),
                static_cast<int8_t>(pb.block_pos.y),
                static_cast<int32_t>(pb.block_pos.z + (chunk.z * 16))
            };
            sb.Serialize(session.stream);
        }
        else if (changes.size() < 10) {
            auto format_multi_block = [](int8_t x, int8_t y, int8_t z) {
                return (
                    ((int16_t(x) & 0x0F) << 12) |
                    ((int16_t(z) & 0x0F) << 8) |
                    ((int16_t(y) & 0xFF))
                    );
                };
            Packet::SetMultipleBlocks smb;
            smb.chunk_position = { chunk.x, chunk.z };
            for (const auto& pb : changes) {
                smb.block_coordinates.push_back(
                    format_multi_block(pb.block_pos.x, pb.block_pos.y, pb.block_pos.z));
                smb.block_metadata.push_back(pb.block.data);
                smb.block_types.push_back(pb.block.type);
            }
            smb.number_of_blocks = static_cast<int>(smb.block_coordinates.size());
            smb.Serialize(session.stream);
        }
        else {
            // Find bounding box in chunk-local space
            auto& p0 = changes[0].block_pos;
            int xmin = p0.x, xmax = p0.x;
            int ymin = p0.y, ymax = p0.y;
            int zmin = p0.z, zmax = p0.z;
            for (const auto& pb : changes) {
                const auto& pos = pb.block_pos;
                if (pos.x > xmax) xmax = pos.x; if (pos.x < xmin) xmin = pos.x;
                if (pos.y > ymax) ymax = pos.y; if (pos.y < ymin) ymin = pos.y;
                if (pos.z > zmax) zmax = pos.z; if (pos.z < zmin) zmin = pos.z;
            }
            // Force even ySize so the client's nibble copy doesn't desync
            ymin = (ymin / 2) * 2;
            ymax = (ymax / 2 + 1) * 2 - 1;
            ymin = static_cast<int>(max(ymin, 0));
            ymax = static_cast<int>(min(ymax, CHUNK_HEIGHT - 1));

            PendingSubRegion psr;
            psr.chunkPos = chunk;
            psr.header.chunkX = chunk.x * 16 + xmin;
            psr.header.chunkZ = chunk.z * 16 + zmin;
            psr.header.chunkY = static_cast<int16_t>(ymin);
            psr.header.sizeX = static_cast<uint8_t>(xmax - xmin);
            psr.header.sizeY = static_cast<uint8_t>(ymax - ymin);
            psr.header.sizeZ = static_cast<uint8_t>(zmax - zmin);
            if (chunkRef) {
                std::shared_ptr<const Chunk> ref = chunkRef;
                psr.data = pool.submit_task(
                    [ref, xmin, xmax, ymin, ymax, zmin, zmax]() {
                        return ChunkSerializer::serialize(
                            *ref, xmin, xmax + 1, ymin, ymax + 1, zmin, zmax + 1);
                    });
            }
            subRegionFlight[&session].push_back(std::move(psr));
        }
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

            session.flushedChunks.insert(pc.pos);

            // Drain any block updates that queued up while this chunk
            // was in-flight. They go out immediately after the chunk
            // data in the same tick, so the client receives them in
            // order and applies them to freshly loaded terrain.
            auto pending = session.pendingBlockChanges.find(pc.pos);
            if (pending != session.pendingBlockChanges.end()) {
                sendBlockUpdates(session, pc.pos, pending->second, std::shared_ptr<const Chunk>(pc.chunkRef));
                session.pendingBlockChanges.erase(pending);
            }
        }

        queue = std::move(stillPending);

        // Drain in-flight sub-region compression jobs in submission order.
        auto srIt = subRegionFlight.find(&session);
        if (srIt != subRegionFlight.end()) {
            auto& srQueue = srIt->second;
            while (!srQueue.empty()) {
                auto& psr = srQueue.front();
                if (!psr.data.valid() ||
                    psr.data.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                    break;
                psr.header.compressedData = psr.data.get();
                psr.header.Serialize(session.stream);
                srQueue.erase(srQueue.begin());
            }
        }
    }

    // Remove all in-flight state for a disconnected session.
    void remove(PlayerSession& session) {
        inFlight.erase(&session);
        subRegionFlight.erase(&session);
    }
};