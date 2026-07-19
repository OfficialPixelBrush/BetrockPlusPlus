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

Server::Server() : config("server.properties"), m_gameRuntime(SERVER_VIEW_RADIUS) {
	ServerBlock::initialize();
	loadConfig();
	serverSocket = ServerSocketManager::createServerSocket(serverPort);
	if (serverSocket < 0) {
		GlobalLogger().m_error << "**** FAILED TO CREATE SERVER SOCKET!" << "\n";
		exit(1);
	}
	GlobalLogger().m_info << "Server initialized on port " << serverPort << "\n";
	m_gameRuntime.init(config.GetAsString("level-name"), config.GetAsString("level-seed"));
}

Server::~Server() {
	this->stop();
}

void Server::sendEntityToDimension(Dimension dim, std::shared_ptr<Entity> entity) {
	// Remove our entity from our watcher
	Dimension oldDim = entity->m_dim;
	if (oldDim == dim)
		return;

	// Remove the entity from the world's entity managers
	WorldManager* world = getWorldForDimension(oldDim);
	WorldManager* newWorld = getWorldForDimension(dim);
	if (!world) {
		GlobalLogger().m_info << "Something went seriously wrong! Couldn't find world object for dimension " << oldDim
		                    << "\n";
		return;
	}
	if (!newWorld) {
		GlobalLogger().m_info << "Something went seriously wrong! Couldn't find world object for dimension " << dim
		                    << "\n";
		return;
	}
	world->m_entityManager.removeEntity(entity->m_id);

	// Rebind entity
	entity->m_isDead = false;
	newWorld->m_entityManager.addEntity(entity);
}

void Server::sendPlayerToDimension(Dimension dim, PlayerSession& session) {
	if (dim == session.m_dimension)
		return;

	// Flush all of our dimension dependent data
	session.m_dimension = dim;
	session.m_flushedChunks.clear();
	session.m_sentChunks.clear();
	session.m_pendingBlockChanges.clear();
	session.m_newlyFlushed.clear();
	session.m_newlyUnloaded.clear();
	session.m_entityTracker = session.m_dimension == 0 ? &m_overworldEntityTracker : &m_hellEntityTracker;

	// Make sure we don't send any pending chunk updates
	m_chunkSender.m_inFlight.erase(&session);
	m_chunkSender.m_subRegionFlight.erase(&session);

	// Send a respawn packet
	Packet::Respawn pkt;
	pkt.m_dimension = dim;
	pkt.Serialize(session.m_stream);
	session.m_connState = ConnectionState::WaitingForSpawnChunks;
	PacketUtilities::sendInventory(session, 0, session.m_inventory);

	// Transfer our entity
	sendEntityToDimension(dim, session.m_entity);
}

void Server::indexAddChunk(PlayerSession& session, const Int32_2& pos) {
	auto& vec = chunkSessions[chunkKey(pos, session.m_dimension)];
	// Avoid duplicates (should never happen, but be safe)
	for (auto* p : vec)
		if (p == &session)
			return;
	vec.push_back(&session);
}

void Server::indexRemoveChunk(PlayerSession& session, const Int32_2& pos) {
	auto it = chunkSessions.find(chunkKey(pos, session.m_dimension));
	if (it == chunkSessions.end())
		return;
	auto& vec = it->second;
	vec.erase(std::remove(vec.begin(), vec.end(), &session), vec.end());
	if (vec.empty())
		chunkSessions.erase(it);
}

void Server::indexRemoveSession(PlayerSession& session) {
	for (const auto& pos : session.m_flushedChunks)
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
	GlobalLogger().m_info << "Initializing server startup.. \n";

	// Setup commands
	command_manager.Init(this);

	// Setup the block callback so we can send it to clients
	auto makeBlockUpdateCallback = [this](int dimensionId, auto& blockChangeMap) {
		return [this, dimensionId, &blockChangeMap](PendingBlock pendingBlock, Int32_2 chunkPos) {
			auto idxIt = chunkSessions.find(chunkKey(chunkPos, -1));
			bool anyInterested = (idxIt != chunkSessions.end() && !idxIt->second.empty());
			if (!anyInterested) {
				for (auto& session : players) {
					if (session->m_dimension == dimensionId && session->m_sentChunks.contains(chunkPos)) {
						anyInterested = true;
						break;
					}
				}
			}
			if (!anyInterested)
				return;

			PendingBlock pendingNew = pendingBlock;
			pendingNew.m_block_pos = { pendingBlock.m_block_pos.m_x & 15, pendingBlock.m_block_pos.m_y,
				                     pendingBlock.m_block_pos.m_z & 15 };
			blockChangeMap[chunkPos].push_back(pendingNew);
		};
	};

	auto registerEntityTrackerCallbacks = [this](EntityTracker& entityTracker, EntityManager& entityManager) {
		entityManager.m_onEntitySpawn = [&entityTracker](std::shared_ptr<Entity> entity) {
			if (entity->m_type == EntityType::PLAYER) {
				entityTracker.addPlayer(entity.get());
				return;
			}
			entityTracker.trackEntity(entity.get());
		};
		entityManager.m_onEntityDespawn = [&entityTracker](std::shared_ptr<Entity> entity) {
			if (entity->m_type == EntityType::PLAYER) {
				entityTracker.removePlayer(entity.get());
				return;
			}
			entityTracker.untrackEntity(entity.get());
		};
		entityTracker.m_server = this;
	};

	m_gameRuntime.m_world.m_onBlockUpdate = makeBlockUpdateCallback(0, chunkBlockChanges);
	m_gameRuntime.m_worldHell.m_onBlockUpdate = makeBlockUpdateCallback(-1, chunkBlockChangesHell);
	registerEntityTrackerCallbacks(m_overworldEntityTracker, m_gameRuntime.m_world.m_entityManager);
	registerEntityTrackerCallbacks(m_hellEntityTracker, m_gameRuntime.m_worldHell.m_entityManager);

	// Get spawn ready
	int spawn_chunk_distance = this->SPAWN_CHUNK_RADIUS;
	int total_spawn_chunks = (spawn_chunk_distance + spawn_chunk_distance + 1) *
	                         (spawn_chunk_distance + spawn_chunk_distance + 1);
	GlobalLogger().m_info << "Server spawn is "
	                    << Int2(int(m_gameRuntime.m_world.m_spawnPoint.m_x), int(m_gameRuntime.m_world.m_spawnPoint.m_z)) << "\n";

	GlobalLogger().m_info << "Preparing spawn chunks..\n";
	// Push every single spawn chunk to get ready for generation
	std::unordered_set<Int32_2> wanted;
	for (int dx = -spawn_chunk_distance; dx <= spawn_chunk_distance; dx++) {
		for (int dz = -spawn_chunk_distance; dz <= spawn_chunk_distance; dz++) {
			Int32_2 pos = { (m_gameRuntime.m_world.m_spawnPoint.m_x >> 4) + dx, (m_gameRuntime.m_world.m_spawnPoint.m_z >> 4) + dz };
			wanted.insert(pos);
		}
	}

	// Actually request chunks
	for (auto pos : wanted) {
		if (!m_gameRuntime.m_world.m_chunks.contains(pos)) {
			auto c = std::make_shared<Chunk>();
			c->m_spawnChunk = true;
			c->m_cpos = pos;
			m_gameRuntime.m_world.m_chunks.emplace(pos, std::move(c));
		}
		if (!m_gameRuntime.m_worldHell.m_chunks.contains(pos)) {
			auto c = std::make_shared<Chunk>();
			c->m_spawnChunk = true;
			c->m_cpos = pos;
			m_gameRuntime.m_worldHell.m_chunks.emplace(pos, std::move(c));
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
			world.m_pool.wait();
			world.drainGenQueue();
			world.m_regionManager->m_iopool.wait();
			world.drainLoadQueue();
			world.populateReady();
			world.m_lightManager.processLightQueue(world);

			for (int dx = -spawn_chunk_distance; dx <= spawn_chunk_distance; dx++) {
				for (int dz = -spawn_chunk_distance; dz <= spawn_chunk_distance; dz++) {
					Int32_2 p{ (world.m_spawnPoint.m_x >> 4) + dx, (world.m_spawnPoint.m_z >> 4) + dz };
					auto it = world.m_chunks.find(p);
					if (it != world.m_chunks.end() && it->second->m_state.load() >= ChunkState::Generated)
						loaded_chunks++;
				}
			}

			// Update load percentage every second
			if (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= 1.0f) {
				int percentLoaded = int((float(loaded_chunks) / float(total_spawn_chunks)) * 100.0f);
				GlobalLogger().m_info << "Loading spawn.. " << percentLoaded << "%\n";
				start = std::chrono::steady_clock::now();
			}

			// Have we loaded all the spawn chunks?
			if (loaded_chunks >= total_spawn_chunks)
				break;
		}
		GlobalLogger().m_info << "Loading spawn.. 100%\n";
	};

	GlobalLogger().m_info << "Loading spawn chunks for Overworld: (" << total_spawn_chunks << ")\n";
	loadSpawnChunks(m_gameRuntime.m_world);

	GlobalLogger().m_info << "Loading spawn chunks for Hell: (" << total_spawn_chunks << ")\n";
	loadSpawnChunks(m_gameRuntime.m_worldHell);

	float startupSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startupStart).count();
	GlobalLogger().m_info << "Startup Complete. (" << std::setprecision(4) << startupSeconds << "s)\n";
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
			GlobalLogger().m_info << "Avg MSPT: " << avgMs << " ms\n";
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
			GlobalLogger().m_warn << "Can't keep up with ticks!\n";
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
	GlobalLogger().m_info << "Server shutting down...\n";
	for (auto& session : players) {
		connStateManager.disconnectPlayer(*session, "Server Closed", *this);
		session->m_stream.flushWriteBufferBlocking();
		auto savedNbt = session->serializeToNBT();
		m_gameRuntime.m_saveManager.savePlayerNBT(std::string(session->m_username.begin(), session->m_username.end()), savedNbt);
	}
	ServerSocketManager::closeSocket(serverSocket);
	m_gameRuntime.m_world.shutdown();
	m_gameRuntime.m_worldHell.shutdown();

	// Save our level file
	levelData& curLevelData = m_gameRuntime.m_saveManager.getLevelData();
	curLevelData.m_RandomSeed = m_gameRuntime.m_world.m_seed;
	curLevelData.m_spawnPoint = m_gameRuntime.m_world.m_spawnPoint;
	curLevelData.m_time = m_gameRuntime.m_world.m_elapsed_ticks;
	m_gameRuntime.m_saveManager.saveLevelFile(curLevelData);
}

void Server::acceptNewPlayers() {
	auto clientSocket = ServerSocketManager::createClientSocket(serverSocket);
	if (clientSocket < 0)
		return;
	players.push_back(std::make_shared<PlayerSession>(clientSocket, m_gameRuntime));
}

void Server::tick() {
	acceptNewPlayers();
	[[maybe_unused]] const int playerCount = int(players.size());

	for (auto& session : players) {
		if (session->m_connState == ConnectionState::Playing)
			processIncoming(*session);
	}

	std::vector<ClientPosition> overworldPositions;
	std::vector<ClientPosition> netherPositions;
	for (auto& session : players) {
		if (session->m_connState == ConnectionState::WaitingForSpawnChunks ||
		    session->m_connState == ConnectionState::Playing) {
			if (session->m_dimension == -1)
				netherPositions.push_back(session->m_position);
			else
				overworldPositions.push_back(session->m_position);
		}
	}
	m_gameRuntime.m_world.tick(overworldPositions);
	m_gameRuntime.m_world.update(overworldPositions);
	m_gameRuntime.m_worldHell.tick(netherPositions);
	m_gameRuntime.m_worldHell.update(netherPositions);

	// Send all of the block changes that have accumulated since the last tick, then clear the list.
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChanges;
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChangesHell;
	localBlockChanges.swap(chunkBlockChanges);
	localBlockChangesHell.swap(chunkBlockChangesHell);

	// Update the entity trackers
	m_overworldEntityTracker.tick();
	m_hellEntityTracker.tick();

	// Handle connection state for each player
	for (auto& session : players) {
		connStateManager.handleConnectionState(*session, *this);

		// Drain chunk-session index updates that ChunkSender recorded.
		for (const auto& pos : session->m_newlyFlushed)
			indexAddChunk(*session, pos);
		session->m_newlyFlushed.clear();
		for (const auto& pos : session->m_newlyUnloaded)
			indexRemoveChunk(*session, pos);
		session->m_newlyUnloaded.clear();

		// Check inventory diffs
		auto diffs2 = session->m_inventoryInteraction.tickDiff();
		if (diffs2.size() <= 5) {
			for (auto difference : diffs2) {
				PacketUtilities::sendSlot(*session, 0, difference.m_slot, &difference.m_stack);
			}
		} else {
			// Too many changes, just resend the whole inventory
			PacketUtilities::sendInventory(*session, 0, *session->m_inventoryInteraction.m_inventory);
		}

		if (!session->m_activeInteraction)
			continue;

		// Force close windows that reference tile entities that have been deleted
		if (!session->m_activeInteraction->canExist()) {
			PacketUtilities::CloseContainer(*session);
			continue;
		}

		// Send each differing slot
		auto diffs = session->m_activeInteraction->tickDiff();
		if (diffs.size() <= 5) {
			for (auto difference : diffs) {
				PacketUtilities::sendSlot(*session, session->m_openWindowId, difference.m_slot, &difference.m_stack);
			}
		} else {
			// Too many changes, just resend the whole inventory
			PacketUtilities::sendInventory(*session, session->m_openWindowId, *session->m_activeInteraction->m_inventory);
		}

		if (this->m_gameRuntime.m_world.m_elapsed_ticks % 40 == 0) {
			// Save periodically
			auto savedNbt = session->serializeToNBT();
			m_gameRuntime.m_saveManager.savePlayerNBT(std::string(session->m_username.begin(), session->m_username.end()),
			                                      savedNbt);
		}
	}

	// Dispatch block changes.
	ChunkBroadcaster::broadcastBlockChanges(*this, localBlockChanges, 0, m_gameRuntime.m_world);
	ChunkBroadcaster::broadcastBlockChanges(*this, localBlockChangesHell, -1, m_gameRuntime.m_worldHell);

	// Flush all pending outgoing data to the socket once per tick.
	for (auto& session : players) {
		session->m_stream.flushWriteBuffer();
	}
	this->disconnectClients();
}

void Server::disconnectClients() {
	// Mark clients who have timed out for removal
	auto now = std::chrono::steady_clock::now();
	for (auto& session : players) {
		if (session->m_connState == ConnectionState::Playing) {
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->m_last_packet_time).count();
			if (elapsed > timeout_seconds) {
				GlobalLogger().m_info << "Player " << session->m_username << " timed out\n";
				connStateManager.disconnectPlayer(*session, "Connection timed out.", *this);
				sendGlobalChatMessage("§e" + session->m_username + " left the game.");
			}
		} else {
			// Kill stuck handshakers
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->m_last_packet_time).count();
			if (elapsed > timeout_seconds) {
				session->m_stream.setConnected(false);
				GlobalLogger().m_info << "Disconnected dataless stream. (Most likely a prober!)\n";
			}
		}
	}

	// Force disconnect players that quit
	players.erase(std::remove_if(players.begin(), players.end(),
	                             [&](const auto& s) {
		                             if (!s->m_stream.isConnected()) {
			                             if (s->m_entity)
				                             GlobalLogger().m_info << "Disconnected client " << s->m_username
				                                                 << " with entity id " << s->m_entity->m_id << "\n";

			                             if (s->m_connState == ConnectionState::Playing ||
			                                 s->m_connState == ConnectionState::WaitingForSpawnChunks) {
				                             auto savedNbt = s->serializeToNBT();
				                             m_gameRuntime.m_saveManager.savePlayerNBT(
				                                 std::string(s->m_username.begin(), s->m_username.end()), savedNbt);
			                             }

			                             indexRemoveSession(*s);
			                             m_chunkSender.remove(*s);
			                             sendGlobalChatMessage("§e" + s->m_username + " left the game.");
			                             return true;
		                             }
		                             return false;
	                             }),
	              players.end());
};

void Server::transferPlayerDimension([[maybe_unused]] PlayerSession& session) {}

void Server::processIncoming(PlayerSession& session) {
	WorldManager& sessionWorld = session.m_dimension == -1 ? m_gameRuntime.m_worldHell : m_gameRuntime.m_world;

	while (session.m_stream.hasData()) {
		PacketId packetId = session.m_stream.Read<PacketId>();

		if (!PacketDispatcher::dispatch(packetId, session, sessionWorld, *this))
			return; // session is dead, or sent an unknown packet

		if (session.m_stream.checkAndClearShortRead()) {
			break;
		}
	}
	// Update our last packet time for the timeout code
	session.m_last_packet_time = std::chrono::steady_clock::now();
}