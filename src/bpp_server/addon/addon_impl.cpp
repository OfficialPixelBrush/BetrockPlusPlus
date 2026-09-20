/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "addon_impl.h"
#include "addon_api.h"
#include "base_structs.h"
#include "blocks.h"
#include "logger.h"

void LogInfo(const char* _message) {
	GlobalLogger().info << _message << "\n";
}

void LogWarning(const char* _message) {
	GlobalLogger().warn << _message << "\n";
}

void LogError(const char* _message) {
	GlobalLogger().error << _message << "\n";
}

void PlayerSendMessage(bp_player* _player, const char* _message) {
	Packet::ChatMessage pak;
	pak.message = std::string(_message);
	pak.Serialize(_player->session->stream);
}

bp_block GetBlock(bp_world* _world, bp_block_pos _pos) {
	Int3 blockPos{ _pos.x, _pos.y, _pos.z };
	auto id = _world->manager->GetBlockId(blockPos);
	auto meta = _world->manager->GetMetadata(blockPos);
	return bp_block{ .id = id, .meta = meta };
}

void SetBlock(bp_world* _world, bp_block_pos _pos, bp_block _block) {
	Int3 blockPos{ _pos.x, _pos.y, _pos.z };
	_world->manager->SetBlock(blockPos, Block{ .type = static_cast<BlockType>(_block.id), .data = _block.meta });
}

void SendBlockUpdate(bp_world* _world, bp_block_pos _pos, bp_block _block) {
	Int3 blockPos{ _pos.x, _pos.y, _pos.z };
	if (!_world->manager->onBlockUpdate)
		return;
	auto* chunk = _world->manager->GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
	if (!chunk)
		return;
	Int3 local{ _pos.x & 15, _pos.y, _pos.z & 15 };
	_world->manager->onBlockUpdate(PendingBlock{ .block{ .type = static_cast<BlockType>(_block.id),
	                                                     .data = _block.meta },
	                                             .blockPos{ _pos.x, _pos.y, _pos.z },
	                                             .light{ chunk->GetBlockLight(local), chunk->GetSkyLight(local) } },
	                               chunk->cpos);
}

constexpr Addon* InternalGetAddon(const bp_api* _api) {
	return reinterpret_cast<Addon*>(_api->internal);
}

void* GetPlayerData(const bp_api* _api, bp_player* _player) {
	Addon* addon = InternalGetAddon(_api);
	auto it = _player->addonData.find(addon);
	if (it == _player->addonData.end())
		return nullptr;

	return it->second;
}

void SetPlayerData(const bp_api* _api, bp_player* _player, void* _data) {
	_player->addonData[InternalGetAddon(_api)] = _data;
}

bp_api MakeAddonAPI() {
	return bp_api{ .version = ADDON_API_VERSION,
		           .internal = nullptr, // Set by the addon manager
		           .log = { .info = LogInfo, .warning = LogWarning, .error = LogError },
		           .player = { .sendMessage = PlayerSendMessage },
		           .world = { .setBlock = SetBlock, .getBlock = GetBlock, .sendBlockUpdate = SendBlockUpdate },
		           .data = { .setPlayer = SetPlayerData, .getPlayer = GetPlayerData } };
}