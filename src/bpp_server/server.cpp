/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "cross_platform.h"
#include "logger.h"
#include <string>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <iomanip>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "server.h"
#include "version.h"
#include <chrono>
#include <filesystem>
#include <iostream>

Server::Server() : config("server.properties") {
	loadConfig();
	serverSocket = ServerSocketManager::createServerSocket(serverPort);
	if (serverSocket < 0) {
		GlobalLogger().error << "**** FAILED TO CREATE SERVER SOCKET!" << "\n";
		exit(1);
	}
	GlobalLogger().info << "Server initialized on port " << serverPort << "\n";
	gameRuntime.init(config.GetAsString("level-name"), config.GetAsString("level-seed"));
}

Server::~Server() {
	this->stop();
}

void Server::indexAddChunk(PlayerSession& session, const Int32_2& pos) {
	auto& vec = chunkSessions[chunkKey(pos, session.dimension)];
	// Avoid duplicates (should never happen, but be safe)
	for (auto* p : vec)
		if (p == &session)
			return;
	vec.push_back(&session);
}

void Server::indexRemoveChunk(PlayerSession& session, const Int32_2& pos) {
	auto it = chunkSessions.find(chunkKey(pos, session.dimension));
	if (it == chunkSessions.end())
		return;
	auto& vec = it->second;
	vec.erase(std::remove(vec.begin(), vec.end(), &session), vec.end());
	if (vec.empty())
		chunkSessions.erase(it);
}

void Server::indexRemoveSession(PlayerSession& session) {
	for (const auto& pos : session.flushedChunks)
		indexRemoveChunk(session, pos);
}

void Server::loadConfig() {
	if (!config.LoadFromDisk()) {
		config.Overwrite({
		    { "level-name", "world" },
		    //{"view-distance", "10"},
		    //{"white-list", "false"},
		    //{"server-ip", ""},
		    //{"motd", "A Minecraft Server"},
		    //{"pvp","true"},
		    // use a random device to seed another prng that gives us our seed
		    { "level-seed", std::to_string(std::mt19937(std::random_device()())()) },
		    //{"spawn-animals",true}
		    { "server-port", "25565" },
		    //{"allow-nether",true},
		    //{"spawn-monsters","true"},
		    //{"max-players", "-1"},
		    //{"online-mode","false"},
		    //{"allow-flight","false"}
		});
		config.SaveToDisk();
	}
	//chunkDistance = config.GetAsNumber<int32_t>("view-distance");
	serverPort = config.GetAsNumber<int32_t>("server-port");
	//motd = config.GetAsString("motd");
	//maximumPlayers = config.GetAsNumber<int32_t>("max-players");
	//maximumThreads = config.GetAsNumber<int32_t>("max-generator-threads");
	//whitelistEnabled = config.GetAsBoolean("white-list");
}

void Server::startup() {
	auto startupStart = std::chrono::steady_clock::now();
	GlobalLogger().info << "Initializing server startup.. \n";

	// Setup commands
	command_manager.Init();

	// Setup the block callback so we can send it to clients
	gameRuntime.world.onBlockUpdate = [this](PendingBlock pendingBlock, Int32_2 chunkPos) {
		// Only enqueue if at least one session knows about this chunk.
		auto idxIt = chunkSessions.find(chunkKey(chunkPos, 0));
		bool anyInterested = (idxIt != chunkSessions.end() && !idxIt->second.empty());
		if (!anyInterested) {
			// Any session with this chunk in-flight?
			for (auto& session : players) {
				if (session->dimension == 0 && session->sentChunks.contains(chunkPos)) {
					anyInterested = true;
					break;
				}
			}
		}
		if (!anyInterested)
			return;

		PendingBlock pendingNew = pendingBlock;
		pendingNew.block_pos = { pendingBlock.block_pos.x & 15, pendingBlock.block_pos.y,
			                     pendingBlock.block_pos.z & 15 };
		chunkBlockChanges[chunkPos].push_back(pendingNew);
	};

	gameRuntime.worldHell.onBlockUpdate = [this](PendingBlock pendingBlock, Int32_2 chunkPos) {
		auto idxIt = chunkSessions.find(chunkKey(chunkPos, -1));
		bool anyInterested = (idxIt != chunkSessions.end() && !idxIt->second.empty());
		if (!anyInterested) {
			for (auto& session : players) {
				if (session->dimension == -1 && session->sentChunks.contains(chunkPos)) {
					anyInterested = true;
					break;
				}
			}
		}
		if (!anyInterested)
			return;

		PendingBlock pendingNew = pendingBlock;
		pendingNew.block_pos = { pendingBlock.block_pos.x & 15, pendingBlock.block_pos.y,
			                     pendingBlock.block_pos.z & 15 };
		chunkBlockChangesHell[chunkPos].push_back(pendingNew);
	};

	// Get spawn ready
	constexpr int spawn_chunk_distance = 4;
	int total_spawn_chunks = (spawn_chunk_distance + spawn_chunk_distance + 1) *
	                         (spawn_chunk_distance + spawn_chunk_distance + 1);
	int loaded_chunks = 0;
	bool spawnDone = false;
	auto start = std::chrono::steady_clock::now();
	GlobalLogger().info << "Server spawn is "
	                    << Int2(int(gameRuntime.world.spawnPoint.x), int(gameRuntime.world.spawnPoint.z)) << "\n";
	GlobalLogger().info << "Loading spawn chunks for Overworld: (" << total_spawn_chunks << ")\n";

	// Push every single spawn chunk to get ready for generation
	std::unordered_set<Int32_2> wanted;
	for (int dx = -spawn_chunk_distance; dx <= spawn_chunk_distance; dx++) {
		for (int dz = -spawn_chunk_distance; dz <= spawn_chunk_distance; dz++) {
			Int32_2 pos = { (gameRuntime.world.spawnPoint.x >> 4) + dx, (gameRuntime.world.spawnPoint.z >> 4) + dz };
			wanted.insert(pos);
		}
	}

	// Actually request chunks
	for (auto pos : wanted) {
		if (!gameRuntime.world.chunks.contains(pos)) {
			auto c = std::make_shared<Chunk>();
			c->spawnChunk = true;
			c->cpos = pos;
			gameRuntime.world.chunks.emplace(pos, std::move(c));
		}
		if (!gameRuntime.worldHell.chunks.contains(pos)) {
			auto c = std::make_shared<Chunk>();
			c->spawnChunk = true;
			c->cpos = pos;
			gameRuntime.worldHell.chunks.emplace(pos, std::move(c));
		}
	}

	// Load the overworld
	while (!spawnDone) {
		loaded_chunks = 0;
		// Force gen these chunks AS FAST AS POSSIBLE
		gameRuntime.world.pumpPipeline({});
		gameRuntime.world.pool.wait();
		gameRuntime.world.drainGenQueue();
		gameRuntime.world.regionManager->iopool.wait();
		gameRuntime.world.drainLoadQueue();
		gameRuntime.world.populateReady();
		gameRuntime.world.lightManager.processLightQueue(gameRuntime.world);
		// Make sure all lighting is done
		gameRuntime.world.lightManager.processLightQueue(gameRuntime.world);

		for (int dx = -spawn_chunk_distance; dx <= spawn_chunk_distance; dx++) {
			for (int dz = -spawn_chunk_distance; dz <= spawn_chunk_distance; dz++) {
				Int32_2 p{ (gameRuntime.world.spawnPoint.x >> 4) + dx, (gameRuntime.world.spawnPoint.z >> 4) + dz };
				auto it = gameRuntime.world.chunks.find(p);
				if (it != gameRuntime.world.chunks.end() && it->second->state.load() >= ChunkState::Generated)
					loaded_chunks++;
			}
		}

		// Update load percentage every second
		if (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= 1.0f) {
			int percentLoaded = int((float(loaded_chunks) / float(total_spawn_chunks)) * 100.0f);
			GlobalLogger().info << "Loading spawn.. " << percentLoaded << "%\n";
			start = std::chrono::steady_clock::now();
		}

		// Have we loaded all the spawn chunks?
		spawnDone = loaded_chunks >= total_spawn_chunks;
	}
	GlobalLogger().info << "Loading spawn.. 100%\n";
	GlobalLogger().info << "Loading spawn chunks for Hell: (" << total_spawn_chunks << ")\n";
	spawnDone = false;
	// Load the hell dimension
	while (!spawnDone) {
		loaded_chunks = 0;
		// Force gen these chunks AS FAST AS POSSIBLE
		gameRuntime.worldHell.pumpPipeline({});
		gameRuntime.worldHell.pool.wait();
		gameRuntime.worldHell.drainGenQueue();
		gameRuntime.worldHell.regionManager->iopool.wait();
		gameRuntime.worldHell.drainLoadQueue();
		gameRuntime.worldHell.populateReady();
		gameRuntime.worldHell.lightManager.processLightQueue(gameRuntime.worldHell);
		// Make sure all lighting is done
		gameRuntime.worldHell.lightManager.processLightQueue(gameRuntime.worldHell);
		for (int dx = -spawn_chunk_distance; dx <= spawn_chunk_distance; dx++) {
			for (int dz = -spawn_chunk_distance; dz <= spawn_chunk_distance; dz++) {
				Int32_2 p{ (gameRuntime.worldHell.spawnPoint.x >> 4) + dx,
					       (gameRuntime.worldHell.spawnPoint.z >> 4) + dz };
				auto it = gameRuntime.worldHell.chunks.find(p);
				if (it != gameRuntime.worldHell.chunks.end() && it->second->state.load() >= ChunkState::Generated)
					loaded_chunks++;
			}
		}

		// Update load percentage every second
		if (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= 1.0f) {
			int percentLoaded = int((float(loaded_chunks) / float(total_spawn_chunks)) * 100.0f);
			GlobalLogger().info << "Loading spawn.. " << percentLoaded << "%\n";
			start = std::chrono::steady_clock::now();
		}

		// Have we loaded all the spawn chunks?
		spawnDone = loaded_chunks >= total_spawn_chunks;
	}
	GlobalLogger().info << "Loading spawn.. 100%\n";

	float startupSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startupStart).count();
	GlobalLogger().info << "Startup Complete. (" << std::setprecision(4) << startupSeconds << "s)\n";
}

void Server::run() {
	startup();
	auto lastTime = std::chrono::steady_clock::now();

	while (!shutdownRequested.load()) {
		int ticks_ran = 0;

		auto now = std::chrono::steady_clock::now();
		float delta = std::chrono::duration<float>(now - lastTime).count();
		lastTime = now;

		accumulator += delta;

		while (accumulator >= TICK_DELTA && ticks_ran <= MAX_TICKS_PER_FRAME) {
			auto tickStart = std::chrono::steady_clock::now();
			tick();
			float tickMs = std::chrono::duration<float>(std::chrono::steady_clock::now() - tickStart).count() * 1000.0f;
			tickTimeAccum += tickMs;
			tickCount++;
			if (tickCount >= 40) {
				GlobalLogger().info << std::setprecision(2)
				                    << "Avg tick: " << (double(tickTimeAccum) / double(tickCount)) << " ms\n";
				tickTimeAccum = 0.0f;
				tickCount = 0;
			}
			accumulator -= TICK_DELTA;
			ticks_ran++;
		}

		if (ticks_ran == MAX_TICKS_PER_FRAME) {
			accumulator = 0.0f;
			GlobalLogger().warn << "Can't keep up!";
		}
	}

	// Shutdown was requested. Save and clean up on the main thread
	stop();
	shutdownRequested.store(false); // unblock the ctrl handler thread
}

void Server::stop() {
	if (stopped)
		return;
	stopped = true;
	GlobalLogger().info << "Server shutting down...\n";
	for (auto& session : players) {
		connStateManager.disconnectPlayer(*session, L"Server Closed", *this);
		session->stream.flushWriteBufferBlocking();
		auto savedNbt = session->serializeToNBT();
		gameRuntime.saveManager.savePlayerNBT(std::string(session->username.begin(), session->username.end()), savedNbt);
	}
	ServerSocketManager::closeSocket(serverSocket);
	gameRuntime.world.shutdown();
	gameRuntime.worldHell.shutdown();

	// Save our level file
	levelData& curLevelData = gameRuntime.saveManager.getLevelData();
	curLevelData.RandomSeed = gameRuntime.world.seed;
	curLevelData.spawnPoint = gameRuntime.world.spawnPoint;
	curLevelData.time = gameRuntime.world.elapsed_ticks;
	gameRuntime.saveManager.saveLevelFile(curLevelData);
}

void Server::acceptNewPlayers() {
	auto clientSocket = ServerSocketManager::createClientSocket(serverSocket);
	if (clientSocket < 0)
		return;
	players.push_back(std::make_shared<PlayerSession>(clientSocket));
	players.back()->players = &players;
}

void Server::tick() {
	acceptNewPlayers();
	const int playerCount = int(players.size());
	std::vector<ClientPosition> overworldPositions;
	std::vector<ClientPosition> netherPositions;
	for (auto& session : players) {
		if (session->connState == ConnectionState::WaitingForSpawnChunks ||
		    session->connState == ConnectionState::Playing) {
			if (session->dimension == -1)
				netherPositions.push_back(session->position);
			else
				overworldPositions.push_back(session->position);
		}
	}
	gameRuntime.world.tick(overworldPositions);
	gameRuntime.world.update(overworldPositions);
	gameRuntime.worldHell.tick(netherPositions);
	gameRuntime.worldHell.update(netherPositions);

	// Send all of the block changes that have accumulated since the last tick, then clear the list.
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChanges;
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChangesHell;
	localBlockChanges.swap(chunkBlockChanges);
	localBlockChangesHell.swap(chunkBlockChangesHell);

	// Handle connection state for each player
	for (auto& session : players) {
		connStateManager.handleConnectionState(*session, *this);

		// Drain chunk-session index updates that ChunkSender recorded.
		for (const auto& pos : session->newlyFlushed)
			indexAddChunk(*session, pos);
		session->newlyFlushed.clear();
		for (const auto& pos : session->newlyUnloaded)
			indexRemoveChunk(*session, pos);
		session->newlyUnloaded.clear();

		// Check inventory diffs
		if (!session->activeInteraction)
			continue;

		// Force close windows that reference tile entities that have been deleted
		if (!session->activeInteraction->canExist()) {
			PacketUtilities::CloseContainer(*session);
			continue;
		}

		// Send each differing slot
		auto diffs = session->activeInteraction->tickDiff();
		if (diffs.size() <= 5) {
			for (auto difference : diffs) {
				ItemStack invalid{ ITEM_INVALID };
				ItemStack* item = difference.stack.has_value() ? &difference.stack.value() : &invalid;
				PacketUtilities::sendSlot(*session, session->openWindowId, difference.slot, item);
			}
		} else {
			// Too many changes, just resend the whole inventory
			PacketUtilities::sendInventory(*session, session->openWindowId, *session->activeInteraction->inventory);
		}
	}

	// Dispatch block changes.
	ChunkBroadcaster::broadcastBlockChanges(*this, localBlockChanges, 0, gameRuntime.world);
	ChunkBroadcaster::broadcastBlockChanges(*this, localBlockChangesHell, -1, gameRuntime.worldHell);

	// Flush all pending outgoing data to the socket once per tick.
	for (auto& session : players) {
		session->stream.flushWriteBuffer();
	}

	// Mark clients who have timed out for removal
	auto now = std::chrono::steady_clock::now();
	for (auto& session : players) {
		if (session->connState == ConnectionState::Playing) {
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_packet_time).count();
			if (elapsed > timeout_seconds) {
				GlobalLogger().info << L"Player " << session->username << L" timed out\n";
				connStateManager.disconnectPlayer(*session, L"Connection timed out.", *this);
			}
		} else {
			// Kill stuck handshakers
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_packet_time).count();
			if (elapsed > timeout_seconds) {
				session->stream.setConnected(false);
				GlobalLogger().info << L"Disconnected dataless stream. (Most likely a prober!)\n";
			}
		}
	}

	// Force disconnect players that quit
	players.erase(std::remove_if(players.begin(), players.end(),
	                             [&](const auto& s) {
		                             if (!s->stream.isConnected()) {
			                             GlobalLogger().info << L"Disconnected client " << s->username
			                                                 << L" with entity id " << s->entityId << L"\n";

			                             if (s->connState == ConnectionState::Playing ||
			                                 s->connState == ConnectionState::WaitingForSpawnChunks) {
				                             auto savedNbt = s->serializeToNBT();
				                             gameRuntime.saveManager.savePlayerNBT(
				                                 std::string(s->username.begin(), s->username.end()), savedNbt);
			                             }

			                             indexRemoveSession(*s);
			                             chunkSender.remove(*s);
			                             for (auto& other : players) {
				                             if (other.get() == s.get())
					                             continue;
				                             if (other->connState != ConnectionState::Playing)
					                             continue;
				                             Packet::DespawnEntity despawn;
				                             despawn.entity_id = s->entityId;
				                             despawn.Serialize(other->stream);
			                             }
			                             return true;
		                             }
		                             return false;
	                             }),
	              players.end());
}

void Server::transferPlayerDimension(PlayerSession& session) {
}

void Server::processIncoming(PlayerSession& session) {
	WorldManager& sessionWorld = session.dimension == -1 ? gameRuntime.worldHell : gameRuntime.world;

	while (session.stream.hasData()) {
		PacketId packetId = session.stream.Read<PacketId>();

		if (!PacketDispatcher::dispatch(packetId, session, sessionWorld, *this))
			return; // session is dead, or sent an unknown packet

		if (session.stream.checkAndClearShortRead()) {
			break;
		}
	}
	// Update our last packet time for the timeout code
	session.last_packet_time = std::chrono::steady_clock::now();
}