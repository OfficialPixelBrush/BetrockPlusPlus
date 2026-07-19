/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "../player_conn/player_session.h"
#include "BS_thread_pool.hpp"
#include "chunk_serializer.h"
#include "networking/packets.h"
#include "tile_entities/tile_entity.h"
#include "world/world.h"
#include <algorithm>
#include <future>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

// ChunkSender offloads zlib chunk serialization onto a thread-pool
struct ChunkSender {
	// One result slot per in-flight chunk.
	struct PendingChunk {
		Int32_2 m_pos;
		std::future<std::vector<uint8_t>> m_data; // async compression result
		std::shared_ptr<Chunk> m_chunkRef;        // kept alive until flush drains pending updates
	};

	// One result slot per in-flight sub-region block update (>= 10 changes).
	struct PendingSubRegion {
		Int32_2 m_chunkPos;
		Packet::ChunkData m_header; // pre-filled, compressedData empty until ready
		std::future<std::vector<uint8_t>> m_data;
	};

	// Per-session queue of in-flight full-chunk serialization jobs.
	std::unordered_map<PlayerSession*, std::vector<PendingChunk>> m_inFlight;

	// Per-session queue of in-flight sub-region compression jobs.
	// Drained in-order by flush() after the full-chunk queue.
	std::unordered_map<PlayerSession*, std::vector<PendingSubRegion>> m_subRegionFlight;

	BS::thread_pool<> m_pool{ 2 };

	size_t enqueue(PlayerSession& session, WorldManager& world, int batchSize = -1) {
		if (batchSize < 0)
			batchSize = static_cast<int>(m_pool.get_thread_count()) * 2;
		int cx = int(std::floor(session.m_position.m_pos.m_x)) >> 4;
		int cz = int(std::floor(session.m_position.m_pos.m_z)) >> 4;

		int radius = world.getViewRadius();

		std::vector<Int32_2> toSend;
		for (int dx = -radius; dx <= radius; dx++) {
			for (int dz = -radius; dz <= radius; dz++) {
				Int32_2 p{ cx + dx, cz + dz };
				if (!world.m_chunks.contains(p))
					continue;
				if (session.m_sentChunks.contains(p))
					continue;
				if (world.m_chunks[p]->m_state.load() < ChunkState::Generated)
					continue;
				toSend.push_back(p);
			}
		}

		// Sort closer chunks first
		std::sort(toSend.begin(), toSend.end(), [&](const Int32_2& a, const Int32_2& b) {
			int da = std::abs(a.m_x - cx) + std::abs(a.m_z - cz);
			int db = std::abs(b.m_x - cx) + std::abs(b.m_z - cz);
			return da < db;
		});

		// Unload all out-of-range chunks immediately
		std::vector<Int32_2> toUnload;
		for (auto& p : session.m_sentChunks) {
			if (std::abs(p.m_x - cx) > radius || std::abs(p.m_z - cz) > radius)
				toUnload.push_back(p);
		}
		for (auto& p : toUnload) {
			Packet::SetChunkVisibility vis;
			vis.m_pos.m_x = p.m_x;
			vis.m_pos.m_z = p.m_z;
			vis.m_visible = false;
			vis.Serialize(session.m_stream);
			session.m_sentChunks.erase(p);
			session.m_flushedChunks.erase(p);
			session.m_pendingBlockChanges.erase(p); // drop queued updates for unloaded chunk
			session.m_newlyUnloaded.push_back(p);
		}

		// Also cancel any in-flight jobs for chunks that are now out of range
		auto& queue = m_inFlight[&session];
		queue.erase(std::remove_if(queue.begin(), queue.end(),
		                           [&](const PendingChunk& pc) {
			                           return std::abs(pc.m_pos.m_x - cx) > radius || std::abs(pc.m_pos.m_z - cz) > radius;
		                           }),
		            queue.end());

		// Rebuild toSend, excluding chunks that already have an in-flight job.
		int submitted = 0;
		for (auto& p : toSend) {
			if (batchSize > 0 && submitted >= batchSize)
				break;
			PendingChunk pc;
			std::shared_ptr<Chunk> chunkRef = world.m_chunks.at(p);
			pc.m_pos = p;
			pc.m_chunkRef = chunkRef;
			pc.m_data = m_pool.submit_task([chunkRef]() { return ChunkSerializer::serialize(*chunkRef); });
			queue.push_back(std::move(pc));
			session.m_sentChunks.insert(p);
			++submitted;
		}
		return static_cast<size_t>(submitted);
	}

	void sendBlockUpdates(PlayerSession& session, const Int32_2& chunk, const std::vector<PendingBlock>& changes,
	                      std::shared_ptr<const Chunk> chunkRef = nullptr) {
		if (changes.empty())
			return;

		if (changes.size() == 1) {
			const PendingBlock& pb = changes[0];
			Packet::SetBlock sb;
			sb.m_block = { pb.m_block.m_type, pb.m_block.m_data };
			sb.m_position = { static_cast<int32_t>(pb.m_block_pos.m_x + (chunk.m_x * 16)), static_cast<int8_t>(pb.m_block_pos.m_y),
				            static_cast<int32_t>(pb.m_block_pos.m_z + (chunk.m_z * 16)) };
			sb.Serialize(session.m_stream);
		} else if (changes.size() < 10) {
			auto format_multi_block = [](int8_t x, int8_t y, int8_t z) {
				return (((int16_t(x) & 0x0F) << 12) | ((int16_t(z) & 0x0F) << 8) | ((int16_t(y) & 0xFF)));
			};
			Packet::SetMultipleBlocks smb;
			smb.m_chunk_position = { chunk.m_x, chunk.m_z };
			for (const auto& pb : changes) {
				smb.m_block_coordinates.push_back(static_cast<int16_t>(
				    format_multi_block(int8_t(pb.m_block_pos.m_x), int8_t(pb.m_block_pos.m_y), int8_t(pb.m_block_pos.m_z))));
				smb.m_block_metadata.push_back(int8_t(pb.m_block.m_data));
				smb.m_block_types.push_back(pb.m_block.m_type);
			}
			smb.m_number_of_blocks = static_cast<int16_t>(smb.m_block_coordinates.size());
			smb.Serialize(session.m_stream);
		} else {
			// Find bounding box in chunk-local space
			auto& p0 = changes[0].m_block_pos;
			int xmin = p0.m_x, xmax = p0.m_x;
			int ymin = p0.m_y, ymax = p0.m_y;
			int zmin = p0.m_z, zmax = p0.m_z;
			for (const auto& pb : changes) {
				const auto& pos = pb.m_block_pos;
				if (pos.m_x > xmax)
					xmax = pos.m_x;
				if (pos.m_x < xmin)
					xmin = pos.m_x;
				if (pos.m_y > ymax)
					ymax = pos.m_y;
				if (pos.m_y < ymin)
					ymin = pos.m_y;
				if (pos.m_z > zmax)
					zmax = pos.m_z;
				if (pos.m_z < zmin)
					zmin = pos.m_z;
			}
			// Force even ySize so the client's nibble copy doesn't desync
			ymin = (ymin / 2) * 2;
			ymax = (ymax / 2 + 1) * 2 - 1;
			ymin = CrossPlatform::Math::max<int>(ymin, 0);
			ymax = CrossPlatform::Math::min<int>(ymax, CHUNK_HEIGHT - 1);

			PendingSubRegion psr;
			psr.m_chunkPos = chunk;
			psr.m_header.m_pos.m_x = chunk.m_x * CHUNK_WIDTH + xmin;
			psr.m_header.m_pos.m_z = chunk.m_z * CHUNK_WIDTH + zmin;
			psr.m_header.m_pos.m_y = static_cast<int16_t>(ymin);
			psr.m_header.m_size.m_x = static_cast<uint8_t>(xmax - xmin);
			psr.m_header.m_size.m_y = static_cast<uint8_t>(ymax - ymin);
			psr.m_header.m_size.m_z = static_cast<uint8_t>(zmax - zmin);
			if (chunkRef) {
				std::shared_ptr<const Chunk> ref = chunkRef;
				psr.m_data = m_pool.submit_task([ref, xmin, xmax, ymin, ymax, zmin, zmax]() {
					return ChunkSerializer::serialize(*ref, xmin, xmax + 1, ymin, ymax + 1, zmin, zmax + 1);
				});
			}
			m_subRegionFlight[&session].push_back(std::move(psr));
		}
	}

	// Drains every job that is already done and writes the resulting
	// SetChunkVisibility + ChunkData packets to the session stream.
	// Jobs that aren't finished yet stay in the queue for the next tick.
	void flush(PlayerSession& session) {
		auto it = m_inFlight.find(&session);
		if (it == m_inFlight.end())
			return;

		auto& queue = it->second;
		std::vector<PendingChunk> stillPending;

		for (auto& pc : queue) {
			// Non-blocking check: only consume results that are ready now.
			if (pc.m_data.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
				stillPending.push_back(std::move(pc));
				continue;
			}

			auto compressed = pc.m_data.get();

			Packet::SetChunkVisibility vis;
			vis.m_pos.m_x = pc.m_pos.m_x;
			vis.m_pos.m_z = pc.m_pos.m_z;
			vis.m_visible = true;
			vis.Serialize(session.m_stream);

			Packet::ChunkData data;
			data.m_pos.m_x = pc.m_pos.m_x * 16;
			data.m_pos.m_z = pc.m_pos.m_z * 16;
			data.m_compressedData = std::move(compressed);
			data.Serialize(session.m_stream);

			session.m_flushedChunks.insert(pc.m_pos);
			session.m_newlyFlushed.push_back(pc.m_pos);

			// Drain any block updates that queued up while this chunk
			// was in-flight. They go out immediately after the chunk
			// data in the same tick, so the client receives them in
			// order and applies them to freshly loaded terrain.
			auto pending = session.m_pendingBlockChanges.find(pc.m_pos);
			if (pending != session.m_pendingBlockChanges.end()) {
				sendBlockUpdates(session, pc.m_pos, pending->second, std::shared_ptr<const Chunk>(pc.m_chunkRef));
				session.m_pendingBlockChanges.erase(pending);
			}

			// Signs need to inform the client when they are loaded
			for (auto te : pc.m_chunkRef->m_tileEntities) {
				if (te->m_type != TileType::SIGN)
					continue;
				auto sign = std::static_pointer_cast<TileEntitySign>(te);

				Packet::UpdateSign pkt;
				pkt.m_position = SlimInt3<int16_t>{ sign->m_position.m_x, static_cast<short>(sign->m_position.m_y),
					                              sign->m_position.m_z };
				pkt.m_lines[0] = sign->m_text1;
				pkt.m_lines[1] = sign->m_text2;
				pkt.m_lines[2] = sign->m_text3;
				pkt.m_lines[3] = sign->m_text4;

				pkt.Serialize(session.m_stream);
			}
		}

		queue = std::move(stillPending);

		// Drain in-flight sub-region compression jobs in submission order.
		auto srIt = m_subRegionFlight.find(&session);
		if (srIt != m_subRegionFlight.end()) {
			auto& srQueue = srIt->second;
			while (!srQueue.empty()) {
				auto& psr = srQueue.front();
				if (!psr.m_data.valid() || psr.m_data.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
					break;
				psr.m_header.m_compressedData = psr.m_data.get();
				psr.m_header.Serialize(session.m_stream);
				srQueue.erase(srQueue.begin());
			}
		}
	}

	// Remove all in-flight state for a disconnected session.
	void remove(PlayerSession& session) {
		m_inFlight.erase(&session);
		m_subRegionFlight.erase(&session);
	}
};