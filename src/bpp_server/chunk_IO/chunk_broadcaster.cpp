/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "chunk_broadcaster.h"

#include "../packet/packet_utils.h"
#include "../server.h"

namespace {
constexpr size_t MULTI_BLOCK_THRESHOLD = 64;

void DeduplicateBlockChanges(std::vector<PendingBlock>& _changes) {
	if (_changes.size() < 2)
		return;
	std::unordered_map<int, size_t> lastIndex;
	lastIndex.reserve(_changes.size());
	for (size_t i = 0; i < _changes.size(); ++i) {
		const auto& p = _changes[i].blockPos;
		int key = (p.x & 15) | ((p.z & 15) << 4) | (p.y << 8);
		lastIndex[key] = i;
	}
	if (lastIndex.size() == _changes.size())
		return;
	std::vector<PendingBlock> deduped;
	deduped.reserve(lastIndex.size());
	for (size_t i = 0; i < _changes.size(); ++i) {
		const auto& p = _changes[i].blockPos;
		int key = (p.x & 15) | ((p.z & 15) << 4) | (p.y << 8);
		if (lastIndex[key] == i)
			deduped.push_back(_changes[i]);
	}
	_changes.swap(deduped);
}
} // namespace

void ChunkBroadcaster::BroadcastBlockChanges(Server& _server,
                                             std::unordered_map<Int32_2, std::vector<PendingBlock>>& _changes,
                                             Dimension _dimension, WorldManager& _dimWorld) {
	for (auto& [chunk, blockChanges] : _changes) {
		DeduplicateBlockChanges(blockChanges);
		if (blockChanges.empty())
			continue;

		// Find which sessions care about this chunk
		// Split into flushed (send immediately) and sentOnly (queue).
		auto indexIt = _server.chunkSessions.find(Server::ChunkKey(chunk, _dimension));
		std::vector<PlayerSession*> flushedSessions;
		std::vector<PlayerSession*> sentOnlySessions;

		if (indexIt != _server.chunkSessions.end())
			flushedSessions = indexIt->second;

		// Sessions that have the chunk in-flight (sentChunks but not flushedChunks) still need to queue the updates.
		for (auto& session : _server.players) {
			if (session->connState != ConnectionState::Playing &&
			    session->connState != ConnectionState::WaitingForSpawnChunks)
				continue;
			if (session->dimension != _dimension)
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
		std::shared_ptr<Chunk> chunkRef = _dimWorld.GetChunk(chunk);

		if (blockChanges.size() == 1) {
			const PendingBlock& pb = blockChanges[0];
			Packet::SetBlock sb;
			sb.block = { pb.block.type, pb.block.data };
			sb.position = { static_cast<int32_t>(pb.blockPos.x + (chunk.x * 16)), static_cast<int8_t>(pb.blockPos.y),
				            static_cast<int32_t>(pb.blockPos.z + (chunk.z * 16)) };
			PacketUtilities::BroadcastPacket(sb, flushedSessions);
		} else if (blockChanges.size() < MULTI_BLOCK_THRESHOLD) {
			auto formatMultiBlock = [](int8_t _x, int8_t _y, int8_t _z) {
				return (((int16_t(_x) & 0x0F) << 12) | ((int16_t(_z) & 0x0F) << 8) | ((int16_t(_y) & 0xFF)));
			};
			Packet::SetMultipleBlocks smb;
			smb.chunkPosition = { chunk.x, chunk.z };
			smb.blockCoordinates.reserve(blockChanges.size());
			smb.blockMetadata.reserve(blockChanges.size());
			smb.blockTypes.reserve(blockChanges.size());
			for (const auto& pb : blockChanges) {
				smb.blockCoordinates.push_back(static_cast<int16_t>(
				    formatMultiBlock(int8_t(pb.blockPos.x), int8_t(pb.blockPos.y), int8_t(pb.blockPos.z))));
				smb.blockMetadata.push_back(int8_t(pb.block.data));
				smb.blockTypes.push_back(pb.block.type);
			}
			smb.numberOfBlocks = static_cast<int16_t>(smb.blockCoordinates.size());
			PacketUtilities::BroadcastPacket(smb, flushedSessions);
		} else {
			// Large dirty region: one compressed sub-chunk instead of hundreds of SetBlocks.
			for (auto* session : flushedSessions)
				_server.chunkSender.SendBlockUpdates(*session, chunk, blockChanges, chunkRef);
		}
	}
}
