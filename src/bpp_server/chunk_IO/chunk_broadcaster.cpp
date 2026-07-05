/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "chunk_broadcaster.h"

#include "../server.h"

void ChunkBroadcaster::broadcastBlockChanges(Server& server, std::unordered_map<Int32_2, std::vector<PendingBlock>>& changes,
                                              int8_t dimension, WorldManager& dimWorld) {
	for (auto& [chunk, blockChanges] : changes) {
		// Find which sessions care about this chunk
		// Split into flushed (send immediately) and sentOnly (queue).
		auto indexIt = server.chunkSessions.find(Server::chunkKey(chunk, dimension));
		std::vector<PlayerSession*> flushedSessions;
		std::vector<PlayerSession*> sentOnlySessions;

		if (indexIt != server.chunkSessions.end())
			flushedSessions = indexIt->second;

		// Sessions that have the chunk in-flight (sentChunks but not flushedChunks) still need to queue the updates.
		for (auto& session : server.players) {
			if (session->connState != ConnectionState::Playing &&
			    session->connState != ConnectionState::WaitingForSpawnChunks)
				continue;
			if (session->dimension != dimension)
				continue;
			if (session->flushedChunks.contains(chunk))
				continue; // already in flushedSessions
			if (session->sentChunks.contains(chunk)) {
				sentOnlySessions.push_back(session.get());
			}
		}

		// Queue updates for sessions still waiting on the chunk to flush.
		for (auto* session : sentOnlySessions) {
			auto& q = session->pendingBlockChanges[chunk];
			q.insert(q.end(), blockChanges.begin(), blockChanges.end());
		}
		if (flushedSessions.empty())
			continue;

		// Capture chunk ref once for sub-region jobs.
		std::shared_ptr<Chunk> chunkRef = dimWorld.getChunk(chunk);

		if (blockChanges.size() == 1) {
			// Single block change: serialise once, raw-copy to every session.
			const PendingBlock& pb = blockChanges[0];
			Packet::SetBlock sb;
			sb.block = { pb.block.type, pb.block.data };
			sb.position = { static_cast<int32_t>(pb.block_pos.x + (chunk.x * 16)), static_cast<int8_t>(pb.block_pos.y),
				            static_cast<int32_t>(pb.block_pos.z + (chunk.z * 16)) };
			// Serialise into a temporary buffer, then send to all sessions.
			NetworkStream tmpStream(-1);
			sb.Serialize(tmpStream);
			const auto& buf = tmpStream.getRawWriteBuffer();
			for (auto* session : flushedSessions)
				session->stream.writeRaw(buf.data(), buf.size());
		} else if (blockChanges.size() < 10) {
			// Multi-block packet
			auto format_multi_block = [](int8_t x, int8_t y, int8_t z) {
				return (((int16_t(x) & 0x0F) << 12) | ((int16_t(z) & 0x0F) << 8) | ((int16_t(y) & 0xFF)));
			};
			Packet::SetMultipleBlocks smb;
			smb.chunk_position = { chunk.x, chunk.z };
			for (const auto& pb : blockChanges) {
				smb.block_coordinates.push_back(static_cast<int16_t>(
				    format_multi_block(int8_t(pb.block_pos.x), int8_t(pb.block_pos.y), int8_t(pb.block_pos.z))));
				smb.block_metadata.push_back(int8_t(pb.block.data));
				smb.block_types.push_back(pb.block.type);
			}
			smb.number_of_blocks = static_cast<int16_t>(smb.block_coordinates.size());
			NetworkStream tmpStream(-1);
			smb.Serialize(tmpStream);
			const auto& buf = tmpStream.getRawWriteBuffer();
			for (auto* session : flushedSessions)
				session->stream.writeRaw(buf.data(), buf.size());
		} else {
			// Sub-region: compression is async per-session via ChunkSender.
			for (auto* session : flushedSessions)
				server.chunkSender.sendBlockUpdates(*session, chunk, blockChanges, chunkRef);
		}
	}
}
