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
#include "entities/entity_tracker.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "packet/handle_packet.h"
#include "packet/packet_dispatcher.h"
#include "player_conn/player_session.h"
#include "player_conn/server_pconnstate_manager.h"
#include "runtime.h"
#include "server_socket.h"
#include <chrono>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

class Server {
public:
	Server();
	~Server();
	void run();
	void stop();
	void processIncoming(PlayerSession& session);

	Runtime m_gameRuntime;
	ChunkSender m_chunkSender;
	int m_flushChunkCount = 10;

	std::shared_ptr<PlayerSession> getSessionById(EntityId entityId) {
		for (auto player : players) {
			if (player->m_entity && player->m_entity->m_id == entityId) {
				return player;
			}
		}
		return nullptr;
	}

	std::shared_ptr<PlayerSession> getSessionByUsername(const std::string& username) {
		for (auto player : players) {
			if (player->m_username == username) {
				return player;
			}
		}
		return nullptr;
	}

	std::string getUsernameByEntityId(EntityId id) {
		for (auto& player : players) {
			if (player->m_entity && player->m_entity->m_id == id) {
				return player->m_username;
			}
		}
		return "";
	}

	// Send a message to all players
	void sendGlobalChatMessage(std::string message) {
		for (auto& other : players) {
			if (other && other->m_connState != ConnectionState::Playing)
				continue;
			Packet::ChatMessage reply;
			reply.m_message = message;
			reply.Serialize(other->m_stream);
		}
	}

	const std::vector<std::shared_ptr<PlayerSession>>& GetPlayers() noexcept {
		return players;
	}

	void sendEntityToDimension(Dimension dim, std::shared_ptr<Entity> entity);
	void sendPlayerToDimension(Dimension dim, PlayerSession& session);

	// Entity trackers are so we can send entity updates to players and vice versa.
	EntityTracker m_overworldEntityTracker;
	EntityTracker m_hellEntityTracker;

private:
	friend bool PacketDispatcher::dispatch(PacketId packetId, PlayerSession& session, WorldManager& sessionWorld,
	                                       Server& server);
	friend void ChunkBroadcaster::broadcastBlockChanges(Server& server,
	                                                    std::unordered_map<Int32_2, std::vector<PendingBlock>>& changes,
	                                                    int8_t dimension, WorldManager& dimWorld);

	void tick();
	void startup();
	void acceptNewPlayers();
	void transferPlayerDimension(PlayerSession& session);
	void disconnectClients();

	// Config file stuff
	void loadConfig();

	// Chunk-session index helpers
	void indexAddChunk(PlayerSession& session, const Int32_2& pos);
	void indexRemoveChunk(PlayerSession& session, const Int32_2& pos);
	void indexRemoveSession(PlayerSession& session);

	WorldManager* getWorldForDimension(Dimension dim) {
		return dim == Dimension::Nether ? &this->m_gameRuntime.m_worldHell : &this->m_gameRuntime.m_world;
	}

	// Encodes chunk position + dimension into a single key for chunkSessions.
	// x = chunk X, y = chunk Z, z = dimension id
	static Int32_3 chunkKey(const Int32_2& pos, int8_t dimension) {
		return Int32_3{ pos.m_x, pos.m_z, int32_t(dimension) };
	}

	static constexpr int TICKS_PER_SECOND = 20;
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
	int64_t timeout_seconds = 60;
	CommandManager command_manager;
	bool stopped = false;
	Config config;
};