/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "logger.h"
#include "packet/packet_utils.h"
#include "world.h"
#include <string>
#include <thread>
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

Server::Server() : config("server.properties") {
	ServerBlock::initialize();
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

void Server::sendEntityToDimension(Dimension dim, std::shared_ptr<Entity> entity) {
	// Remove our entity from our watcher
	Dimension oldDim = entity->dim;
	if (oldDim == dim)
		return;

	// Remove the entity from the world's entity managers
	WorldManager* world = getWorldForDimension(oldDim);
	WorldManager* newWorld = getWorldForDimension(dim);
	if (!world) {
		GlobalLogger().info << "Something went seriously wrong! Couldn't find world object for dimension " << oldDim
		                    << "\n";
		return;
	}
	if (!newWorld) {
		GlobalLogger().info << "Something went seriously wrong! Couldn't find world object for dimension " << dim
		                    << "\n";
		return;
	}
	world->entityManager.removeEntity(entity->id);

	// Rebind entity stuff
	entity->dim = dim;
	entity->isDead = false;
	newWorld->entityManager.addEntity(entity);
}

void Server::sendPlayerToDimension(Dimension dim, PlayerSession& session) {
	if (dim == session.dimension)
		return;

	// Flush all of our data
	session.dimension = dim;
	session.flushedChunks.clear();
	session.sentChunks.clear();
	session.pendingBlockChanges.clear();
	session.newlyFlushed.clear();
	session.newlyUnloaded.clear();
	session.entityTracker = session.dimension == 0 ? &overworldEntityTracker : &hellEntityTracker;

	// Send a respawn packet
	Packet::Respawn pkt;
	pkt.dimension = dim;
	pkt.Serialize(session.stream);
	session.connState = ConnectionState::WaitingForSpawnChunks;
	PacketUtilities::sendInventory(session, 0, session.inventory);

	// Transfer our entity
	sendEntityToDimension(dim, session.entity);
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
	command_manager.Init(this);

	// Setup the block callback so we can send it to clients
	auto makeBlockUpdateCallback = [this](int dimensionId, auto& blockChangeMap) {
		return [this, dimensionId, &blockChangeMap](PendingBlock pendingBlock, Int32_2 chunkPos) {
			auto idxIt = chunkSessions.find(chunkKey(chunkPos, -1));
			bool anyInterested = (idxIt != chunkSessions.end() && !idxIt->second.empty());
			if (!anyInterested) {
				for (auto& session : players) {
					if (session->dimension == dimensionId && session->sentChunks.contains(chunkPos)) {
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
			blockChangeMap[chunkPos].push_back(pendingNew);
		};
	};

	auto registerEntityTrackerCallbacks = [this](EntityTracker& entityTracker, EntityManager& entityManager) {
		entityManager.onEntitySpawn = [&entityTracker](std::shared_ptr<Entity> entity) {
			if (entity->type == EntityType::PLAYER) {
				entityTracker.addPlayer(entity.get());
				return;
			}
			entityTracker.trackEntity(entity.get());
		};
		entityManager.onEntityDespawn = [&entityTracker](std::shared_ptr<Entity> entity) {
			if (entity->type == EntityType::PLAYER) {
				entityTracker.removePlayer(entity.get());
				return;
			}
			entityTracker.untrackEntity(entity.get());
		};
		entityTracker.server = this;
	};

	gameRuntime.world.onBlockUpdate = makeBlockUpdateCallback(0, chunkBlockChanges);
	gameRuntime.worldHell.onBlockUpdate = makeBlockUpdateCallback(-1, chunkBlockChangesHell);
	registerEntityTrackerCallbacks(overworldEntityTracker, gameRuntime.world.entityManager);
	registerEntityTrackerCallbacks(hellEntityTracker, gameRuntime.worldHell.entityManager);

	// Get spawn ready
	constexpr int spawn_chunk_distance = 9;
	int total_spawn_chunks = (spawn_chunk_distance + spawn_chunk_distance + 1) *
	                         (spawn_chunk_distance + spawn_chunk_distance + 1);
	GlobalLogger().info << "Server spawn is "
	                    << Int2(int(gameRuntime.world.spawnPoint.x), int(gameRuntime.world.spawnPoint.z)) << "\n";

	GlobalLogger().info << "Preparing spawn chunks..\n";
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

	// Chunks are ready to load at this point.
	auto loadSpawnChunks = [total_spawn_chunks](WorldManager& world) {
		auto start = std::chrono::steady_clock::now();
		int loaded_chunks = 0;
		while (true) {
			loaded_chunks = 0;
			// Force gen these chunks AS FAST AS POSSIBLE
			world.pumpPipeline({});
			world.pool.wait();
			world.drainGenQueue();
			world.regionManager->iopool.wait();
			world.drainLoadQueue();
			world.populateReady();
			world.lightManager.processLightQueue(world);

			for (int dx = -spawn_chunk_distance; dx <= spawn_chunk_distance; dx++) {
				for (int dz = -spawn_chunk_distance; dz <= spawn_chunk_distance; dz++) {
					Int32_2 p{ (world.spawnPoint.x >> 4) + dx, (world.spawnPoint.z >> 4) + dz };
					auto it = world.chunks.find(p);
					if (it != world.chunks.end() && it->second->state.load() >= ChunkState::Generated)
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
			if (loaded_chunks >= total_spawn_chunks)
				break;
		}
		GlobalLogger().info << "Loading spawn.. 100%\n";
	};

	GlobalLogger().info << "Loading spawn chunks for Overworld: (" << total_spawn_chunks << ")\n";
	loadSpawnChunks(gameRuntime.world);

	GlobalLogger().info << "Loading spawn chunks for Hell: (" << total_spawn_chunks << ")\n";
	loadSpawnChunks(gameRuntime.worldHell);

	float startupSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startupStart).count();
	GlobalLogger().info << "Startup Complete. (" << std::setprecision(4) << startupSeconds << "s)\n";
}

void Server::run() {
	startup();

	static constexpr auto TICK_DURATION = std::chrono::nanoseconds(std::chrono::seconds{ 1 }) / TICKS_PER_SECOND;

	using clock = std::chrono::steady_clock;

	std::chrono::nanoseconds avgTotalTickDuration{ 0 };
	int avgTickCount = 0;

	uint64_t ticks = 0;
	auto baseTime = clock::now();

	// Main tick loop
	// Heavily based on https://github.com/Minestom/Minestom/blob/59406d5b54d5221df85f381f204fbc07fd861a43/src/main/java/net/minestom/server/thread/TickSchedulerThread.java
	while (!shutdownRequested.load()) {
		auto tickStart = clock::now();
		tick();
		auto tickEnd = clock::now();

		// Sample and print average tick data
		avgTotalTickDuration += (tickEnd - tickStart);
		++avgTickCount;

		if (ticks % (TICKS_PER_SECOND * 2) == 0) {
			double avgMs = std::chrono::duration<double, std::milli>(avgTotalTickDuration).count() /
			               double(avgTickCount);
			GlobalLogger().info << "Avg MSPT: " << avgMs << " ms\n";
			avgTotalTickDuration = std::chrono::nanoseconds{ 0 };
			avgTickCount = 0;
		}

		++ticks;
		auto nextTickTime = baseTime + ticks * TICK_DURATION;
		std::this_thread::sleep_until(nextTickTime);

		// Check if the server can not keep up with the tickrate
		// if it gets too far behind, reset the ticks & baseTime
		// to avoid running too many ticks at once
		if (clock::now() > nextTickTime + MAX_TICK_CATCH_UP * TICK_DURATION) {
			baseTime = clock::now();
			ticks = 0;
			GlobalLogger().warn << "Can't keep up with ticks!";
		}
	}

	// Shutdown was requested. Save and clean up on the main thread
	stop();
	shutdownRequested.store(false); // Unblock the ctrl handler thread
}

void Server::stop() {
	if (stopped)
		return;
	stopped = true;
	GlobalLogger().info << "Server shutting down...\n";
	for (auto& session : players) {
		connStateManager.disconnectPlayer(*session, "Server Closed", *this);
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
	players.push_back(std::make_shared<PlayerSession>(clientSocket, gameRuntime));
}

void Server::tick() {
	acceptNewPlayers();
	[[maybe_unused]] const int playerCount = int(players.size());

	for (auto& session : players) {
		if (session->connState == ConnectionState::Playing)
			processIncoming(*session);
	}

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

	// Update the entity trackers
	overworldEntityTracker.tick();
	hellEntityTracker.tick();

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
		auto diffs2 = session->inventoryInteraction.tickDiff();
		if (diffs2.size() <= 5) {
			for (auto difference : diffs2) {
				PacketUtilities::sendSlot(*session, 0, difference.slot, &difference.stack);
			}
		} else {
			// Too many changes, just resend the whole inventory
			PacketUtilities::sendInventory(*session, 0, *session->inventoryInteraction.inventory);
		}

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
				PacketUtilities::sendSlot(*session, session->openWindowId, difference.slot, &difference.stack);
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
	this->disconnectClients();
}

void Server::disconnectClients() {
	// Mark clients who have timed out for removal
	auto now = std::chrono::steady_clock::now();
	for (auto& session : players) {
		if (session->connState == ConnectionState::Playing) {
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_packet_time).count();
			if (elapsed > timeout_seconds) {
				GlobalLogger().info << "Player " << session->username << " timed out\n";
				connStateManager.disconnectPlayer(*session, "Connection timed out.", *this);
				sendGlobalChatMessage("§e" + session->username + " left the game.");
			}
		} else {
			// Kill stuck handshakers
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_packet_time).count();
			if (elapsed > timeout_seconds) {
				session->stream.setConnected(false);
				GlobalLogger().info << "Disconnected dataless stream. (Most likely a prober!)\n";
			}
		}
	}

	// Force disconnect players that quit
	players.erase(std::remove_if(players.begin(), players.end(),
	                             [&](const auto& s) {
		                             if (!s->stream.isConnected()) {
			                             if (s->entity)
				                             GlobalLogger().info << "Disconnected client " << s->username
				                                                 << " with entity id " << s->entity->id << "\n";

			                             if (s->connState == ConnectionState::Playing ||
			                                 s->connState == ConnectionState::WaitingForSpawnChunks) {
				                             auto savedNbt = s->serializeToNBT();
				                             gameRuntime.saveManager.savePlayerNBT(
				                                 std::string(s->username.begin(), s->username.end()), savedNbt);
			                             }

			                             indexRemoveSession(*s);
			                             chunkSender.remove(*s);
			                             sendGlobalChatMessage("§e" + s->username + " left the game.");
			                             return true;
		                             }
		                             return false;
	                             }),
	              players.end());
};

void Server::transferPlayerDimension([[maybe_unused]] PlayerSession& session) {}

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