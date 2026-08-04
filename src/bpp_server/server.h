/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <atomic>
extern std::atomic<bool> shutdownRequested;

#include "blocks/serverBlockBehaviors.h"
#include "chunk_IO/chunk_broadcaster.h"
#include "chunk_IO/chunk_sender.h"
#include "commands/command_manager.h"
#include "config/config.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "packet/handle_packet.h"
#include "packet/packet_dispatcher.h"
#include "player_conn/player_session.h"
#include "player_conn/server_pconnstate_manager.h"
#include "runtime.h"
#include "server_socket.h"
#include "trackers/entity_tracker.h"
#include "strings/ucs2.h"
#include "items/item_properties.h"
#include "items/tool_properties.h"
#include <chrono>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

class Server {
public:
	Server();
	~Server();
	void Run();
	void Stop();
	void StopTimeout(float _secondsUntilShutdown);
	void ResetTimeout();
	void ProcessIncoming(PlayerSession& _session);

private:
	int serverViewRadius = 8;
	int spawnChunkRadius = 5;
	uint16_t shutdownTimer = 0;

public:
	Runtime gameRuntime;
	ChunkSender chunkSender;
	int flushChunkCount = 10;

	void SavePlayer(const std::string& _username) {
		auto PlayerSession = GetSessionByUsername(_username);
		if (!PlayerSession)
			GlobalLogger().error << "Failed to save player data for " << _username << "!\n";

		auto savedNbt = PlayerSession->SerializeToNbt();
		gameRuntime.saveManager.SavePlayerNbt(std::string(PlayerSession->username.begin(), PlayerSession->username.end()), savedNbt);
	}

	void DisconnectPlayer(const std::string& _reason, PlayerSession& _session) {
		this->connStateManager.DisconnectPlayer(_session, _reason, *this);
	}

	std::shared_ptr<PlayerSession> GetSessionById(EntityId _entityId) {
		for (auto player : players) {
			if (player->entity && player->entity->id == _entityId) {
				return player;
			}
		}
		return nullptr;
	}

	std::shared_ptr<PlayerSession> GetSessionByUsername(const std::string& _username) {
		for (auto player : players) {
			if (player->username == _username) {
				return player;
			}
		}
		return nullptr;
	}

	std::string GetUsernameByEntityId(EntityId _id) {
		for (auto& player : players) {
			if (player->entity && player->entity->id == _id) {
				return player->username;
			}
		}
		return "";
	}

	// Send a message to all players
	void SendGlobalChatMessage(std::string _message) {
		for (auto& other : players) {
			if (other && other->connState != ConnectionState::Playing)
				continue;
			Packet::ChatMessage reply;
			reply.message = _message;
			reply.Serialize(other->stream);
		}
		GlobalLogger().msg << StripFormatting(_message) << "\n";
	}

	const std::vector<std::shared_ptr<PlayerSession>>& GetPlayers() noexcept {
		return players;
	}

	WorldManager* GetWorldForDimension(Dimension _dim) {
		return _dim == Dimension::Nether ? &this->gameRuntime.worldHell : &this->gameRuntime.world;
	}

	EntityTracker* GetEntityTrackerForDimension(Dimension _dim) {
		return _dim == Dimension::Nether ? &this->hellEntityTracker : &this->overworldEntityTracker;
	}

	void TryForceBreak(PlayerSession& _session, WorldManager& _world);

	void SendEntityToDimension(Dimension _dim, std::shared_ptr<Entity> _entity);
	void SendPlayerToDimension(Dimension _dim, PlayerSession& _session);

	// Entity trackers are so we can send entity updates to players and vice versa.
	EntityTracker overworldEntityTracker;
	EntityTracker hellEntityTracker;
	// Performance metric
	double averageTickMs = 0.0;
	static constexpr int TICKS_PER_SECOND = 20;

private:
	friend bool PacketDispatcher::Dispatch(PacketId _packetId, PlayerSession& _session, WorldManager& _sessionWorld,
	                                       Server& _server);
	friend void ChunkBroadcaster::BroadcastBlockChanges(Server& _server,
	                                                    std::unordered_map<Int32_2, std::vector<PendingBlock>>& _changes,
	                                                    int8_t _dimension, WorldManager& _dimWorld);

	void Tick();
	void Startup();
	void AcceptNewPlayers();
	void DisconnectClients();

	// When a player breaks a block
	void OnPlayerBlockBreak(PlayerSession& _session, WorldManager& _world) {
		// Actually break this block
		auto finishMiningWithTool = [&](ItemStack* _held, BlockType _block) {
			if (!_held)
				return;
			auto it = Items::toolBehavior.find(_held->id);
			if (it != Items::toolBehavior.end() && it->second.onBlockFinishMining)
				it->second.onBlockFinishMining(_held, _block);
		};

		auto blockId = _session.pendingBlockBreak->lastBlock;
		auto blockPos = _session.pendingBlockBreak->lastBlockPos;
		ItemStack* heldItem = _session.inventory.GetHeldItem();

		_session.pendingBlockBreak.reset();
		if (!Items::CanPlayerHarvest(heldItem, blockId)) {
			_world.SetBlock(blockPos, BLOCK_AIR);
			return;
		}

		if (_session.entity) {
			if (auto func = Blocks::blockBehaviors[blockId].onBlockDestroyedByPlayer) {
				func(_world, blockPos, *_session.entity);
			}
		}

		finishMiningWithTool(heldItem, blockId);

		// Send the particle packet
		Packet::WorldEvent pkt;
		pkt.eventId = PacketData::WorldEvent::BLOCK_BREAK;
		pkt.data = blockId;
		pkt.position = { blockPos.x, int8_t(blockPos.y), blockPos.z };
		_session.entityTracker->SendPacketToViewers(pkt, _session.entity->id);
	}

	// Config file stuff
	void LoadConfig();

	// Chunk-session index helpers
	void IndexAddChunk(PlayerSession& _session, const Int32_2& _pos);
	void IndexRemoveChunk(PlayerSession& _session, const Int32_2& _pos);
	void IndexRemoveSession(PlayerSession& _session);

	// Update our block breaking state
	void UpdateBlockBreaking(PlayerSession& _session, WorldManager& _world);

	// Encodes chunk position + dimension into a single key for chunkSessions.
	// x = chunk X, y = chunk Z, z = dimension id
	static Int32_3 ChunkKey(const Int32_2& _pos, int8_t _dimension) {
		return Int32_3{ _pos.x, _pos.z, int32_t(_dimension) };
	}
	static constexpr int MAX_TICK_CATCH_UP = 5;

	PlayerConnStateManager connStateManager;

	// Global players and dimensional players
	std::vector<std::shared_ptr<PlayerSession>> players;

	// Block change tracking
	std::unordered_map<Int32_2, std::vector<PendingBlock>> chunkBlockChanges;
	std::unordered_map<Int32_2, std::vector<PendingBlock>> chunkBlockChangesHell;

	// Which sessions currently have a given chunk loaded?
	std::unordered_map<Int32_3, std::vector<PlayerSession*>> chunkSessions;

	// Server specifics
	int serverSocket = -1;
	int serverPort = 25565;
	int64_t timeoutSeconds = 60;
	CommandManager commandManager;
	bool stopped = false;
	Config config;
};