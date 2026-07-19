/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "chunk_broadcaster.h"

#include "../server.h"

void ChunkBroadcaster::broadcastBlockChanges(Server& server,
                                             std::unordered_map<Int32_2, std::vector<PendingBlock>>& changes,
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
			if (session->m_connState != ConnectionState::Playing &&
			    session->m_connState != ConnectionState::WaitingForSpawnChunks)
				continue;
			if (session->m_dimension != dimension)
				continue;
			if (session->m_flushedChunks.contains(chunk))
				continue; // already in flushedSessions
			if (session->m_sentChunks.contains(chunk)) {
				sentOnlySessions.push_back(session.get());
			}
		}

		// Queue updates for sessions still waiting on the chunk to flush.
		for (auto* session : sentOnlySessions) {
			auto& q = session->m_pendingBlockChanges[chunk];
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
			sb.m_block = { pb.m_block.m_type, pb.m_block.m_data };
			sb.m_position = { static_cast<int32_t>(pb.m_block_pos.m_x + (chunk.m_x * 16)), static_cast<int8_t>(pb.m_block_pos.m_y),
				            static_cast<int32_t>(pb.m_block_pos.m_z + (chunk.m_z * 16)) };
			// Serialise into a temporary buffer, then send to all sessions.
			NetworkStream tmpStream(-1);
			sb.Serialize(tmpStream);
			const auto& buf = tmpStream.getRawWriteBuffer();
			for (auto* session : flushedSessions)
				session->m_stream.writeRaw(buf.data(), buf.size());
		} else if (blockChanges.size() < 10) {
			// Multi-block packet
			auto format_multi_block = [](int8_t x, int8_t y, int8_t z) {
				return (((int16_t(x) & 0x0F) << 12) | ((int16_t(z) & 0x0F) << 8) | ((int16_t(y) & 0xFF)));
			};
			Packet::SetMultipleBlocks smb;
			smb.m_chunk_position = { chunk.m_x, chunk.m_z };
			for (const auto& pb : blockChanges) {
				smb.m_block_coordinates.push_back(static_cast<int16_t>(
				    format_multi_block(int8_t(pb.m_block_pos.m_x), int8_t(pb.m_block_pos.m_y), int8_t(pb.m_block_pos.m_z))));
				smb.m_block_metadata.push_back(int8_t(pb.m_block.m_data));
				smb.m_block_types.push_back(pb.m_block.m_type);
			}
			smb.m_number_of_blocks = static_cast<int16_t>(smb.m_block_coordinates.size());
			NetworkStream tmpStream(-1);
			smb.Serialize(tmpStream);
			const auto& buf = tmpStream.getRawWriteBuffer();
			for (auto* session : flushedSessions)
				session->m_stream.writeRaw(buf.data(), buf.size());
		} else {
			// Sub-region: compression is async per-session via ChunkSender.
			for (auto* session : flushedSessions)
				server.m_chunkSender.sendBlockUpdates(*session, chunk, blockChanges, chunkRef);
		}
	}
}
