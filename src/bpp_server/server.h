/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

#include <atomic>
extern std::atomic<bool> shutdownRequested;

#include "config/config.h"
#include "chunk_broadcaster.h"
#include "chunk_sender.h"
#include "commands/command_manager.h"
#include "handle_packet.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "packet_dispatcher.h"
#include "player_session.h"
#include "runtime.h"
#include "server_socket.h"
#include "server_pconnstate_manager.h"
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
	void broadcastPlayerMovement(PlayerSession& session);

	Runtime gameRuntime;
	ChunkSender chunkSender;
	int flushChunkCount = 10;
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

	// Config file stuff
	void loadConfig();

	// Chunk-session index helpers
	void indexAddChunk(PlayerSession& session, const Int32_2& pos);
	void indexRemoveChunk(PlayerSession& session, const Int32_2& pos);
	void indexRemoveSession(PlayerSession& session);

	// Dimensional player list helpers
	void indexAddDimensionalPlayer(PlayerSession& session);
	void indexRemoveDimensionalPlayer(PlayerSession& session);

	// Encodes chunk position + dimension into a single key for chunkSessions.
	// x = chunk X, y = chunk Z, z = dimension id
	static Int32_3 chunkKey(const Int32_2& pos, int8_t dimension) {
		return Int32_3{ pos.x, pos.z, int32_t(dimension) };
	}

	static constexpr float TICK_DELTA = 1.0f / 20.0f;
	static constexpr int MAX_TICKS_PER_FRAME = 10;

	PlayerConnStateManager connStateManager;

	// Global players and dimensional players
	std::vector<std::shared_ptr<PlayerSession>> players;

	// Block change tracking
	std::unordered_map<Int32_2, std::vector<PendingBlock>> chunkBlockChanges;
	std::unordered_map<Int32_2, std::vector<PendingBlock>> chunkBlockChangesHell;

	// Which sessions currently have a given chunk loaded?
	std::unordered_map<Int32_3, std::vector<PlayerSession*>> chunkSessions;
	int serverSocket = -1;
	int serverPort = 25565;
	int64_t timeout_seconds = 60;
	float accumulator = 0.0f;
	float tickTimeAccum = 0.0f;
	int tickCount = 0;
	CommandManager command_manager;
	bool stopped = false;
	Config config;
};