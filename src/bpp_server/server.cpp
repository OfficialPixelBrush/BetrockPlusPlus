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

Server::Server() : gameRuntime(serverViewRadius), config("server.properties") {
	ServerBlock::Initialize();
	LoadConfig();
	serverSocket = ServerSocketManager::CreateServerSocket(serverPort);
	if (serverSocket < 0) {
		GlobalLogger().error << "**** FAILED TO CREATE SERVER SOCKET!" << "\n";
		exit(1);
	}
	GlobalLogger().info << "Server initialized on port " << serverPort << "\n";
	gameRuntime.Init(config.GetAsString("level-name"), config.GetAsString("level-seed"));
}

Server::~Server() {
	this->Stop();
}

void Server::SendEntityToDimension(Dimension _dim, std::shared_ptr<Entity> _entity) {
	// Remove our entity from our watcher
	Dimension oldDim = _entity->dim;
	if (oldDim == _dim)
		return;

	// Remove the entity from the world's entity managers
	WorldManager* world = GetWorldForDimension(oldDim);
	WorldManager* newWorld = GetWorldForDimension(_dim);
	if (!world) {
		GlobalLogger().info << "Something went seriously wrong! Couldn't find world object for dimension " << oldDim
		                    << "\n";
		return;
	}
	if (!newWorld) {
		GlobalLogger().info << "Something went seriously wrong! Couldn't find world object for dimension " << _dim
		                    << "\n";
		return;
	}
	world->entityManager.RemoveEntity(_entity->id);

	// Rebind entity
	_entity->isDead = false;
	newWorld->entityManager.AddEntity(_entity);
}

void Server::SendPlayerToDimension(Dimension _dim, PlayerSession& _session) {
	if (_dim == _session.dimension)
		return;

	// Flush all of our dimension dependent data
	_session.dimension = _dim;
	_session.flushedChunks.clear();
	_session.sentChunks.clear();
	_session.pendingBlockChanges.clear();
	_session.newlyFlushed.clear();
	_session.newlyUnloaded.clear();
	_session.entityTracker = _session.dimension == 0 ? &overworldEntityTracker : &hellEntityTracker;

	// Make sure we don't send any pending chunk updates
	chunkSender.inFlight.erase(&_session);
	chunkSender.subRegionFlight.erase(&_session);

	// Send a respawn packet
	Packet::Respawn pkt;
	pkt.dimension = _dim;
	pkt.Serialize(_session.stream);
	_session.connState = ConnectionState::WaitingForSpawnChunks;
	PacketUtilities::SendInventory(_session, 0, _session.inventory);

	// Transfer our entity
	SendEntityToDimension(_dim, _session.entity);
}

void Server::IndexAddChunk(PlayerSession& _session, const Int32_2& _pos) {
	auto& vec = chunkSessions[ChunkKey(_pos, _session.dimension)];
	// Avoid duplicates (should never happen, but be safe)
	for (auto* p : vec)
		if (p == &_session)
			return;
	vec.push_back(&_session);
}

void Server::IndexRemoveChunk(PlayerSession& _session, const Int32_2& _pos) {
	auto it = chunkSessions.find(ChunkKey(_pos, _session.dimension));
	if (it == chunkSessions.end())
		return;
	auto& vec = it->second;
	vec.erase(std::remove(vec.begin(), vec.end(), &_session), vec.end());
	if (vec.empty())
		chunkSessions.erase(it);
}

void Server::IndexRemoveSession(PlayerSession& _session) {
	for (const auto& pos : _session.flushedChunks)
		IndexRemoveChunk(_session, pos);
}

void Server::LoadConfig() {
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

void Server::Startup() {
	auto startupStart = std::chrono::steady_clock::now();
	GlobalLogger().info << "Initializing server startup.. \n";

	// Setup commands
	commandManager.Init(this);

	// Setup the block callback so we can send it to clients
	auto makeBlockUpdateCallback = [this](int _dimensionId, auto& _blockChangeMap) {
		return [this, _dimensionId, &_blockChangeMap](PendingBlock _pendingBlock, Int32_2 _chunkPos) {
			auto idxIt = chunkSessions.find(ChunkKey(_chunkPos, -1));
			bool anyInterested = (idxIt != chunkSessions.end() && !idxIt->second.empty());
			if (!anyInterested) {
				for (auto& session : players) {
					if (session->dimension == _dimensionId && session->sentChunks.contains(_chunkPos)) {
						anyInterested = true;
						break;
					}
				}
			}
			if (!anyInterested)
				return;

			PendingBlock pendingNew = _pendingBlock;
			pendingNew.blockPos = { _pendingBlock.blockPos.x & 15, _pendingBlock.blockPos.y,
				                    _pendingBlock.blockPos.z & 15 };
			_blockChangeMap[_chunkPos].push_back(pendingNew);
		};
	};

	auto registerEntityTrackerCallbacks = [this](EntityTracker& _entityTracker, EntityManager& _entityManager) {
		_entityManager.onEntitySpawn = [&_entityTracker](std::shared_ptr<Entity> _entity) {
			if (_entity->type == EntityType::PLAYER) {
				_entityTracker.AddPlayer(_entity.get());
				return;
			}
			_entityTracker.TrackEntity(_entity.get());
		};
		_entityManager.onEntityDespawn = [&_entityTracker](std::shared_ptr<Entity> _entity) {
			if (_entity->type == EntityType::PLAYER) {
				_entityTracker.RemovePlayer(_entity.get());
				return;
			}
			_entityTracker.UntrackEntity(_entity.get());
		};
		_entityTracker.server = this;
	};

	gameRuntime.world.onBlockUpdate = makeBlockUpdateCallback(0, chunkBlockChanges);
	gameRuntime.worldHell.onBlockUpdate = makeBlockUpdateCallback(-1, chunkBlockChangesHell);
	registerEntityTrackerCallbacks(overworldEntityTracker, gameRuntime.world.entityManager);
	registerEntityTrackerCallbacks(hellEntityTracker, gameRuntime.worldHell.entityManager);

	// Get spawn ready
	int spawnChunkDistance = this->spawnChunkRadius;
	int totalSpawnChunks = (spawnChunkDistance + spawnChunkDistance + 1) * (spawnChunkDistance + spawnChunkDistance + 1);
	GlobalLogger().info << "Server spawn is "
	                    << Int2(int(gameRuntime.world.spawnPoint.x), int(gameRuntime.world.spawnPoint.z)) << "\n";

	GlobalLogger().info << "Preparing spawn chunks..\n";
	// Push every single spawn chunk to get ready for generation
	std::unordered_set<Int32_2> wanted;
	for (int dx = -spawnChunkDistance; dx <= spawnChunkDistance; dx++) {
		for (int dz = -spawnChunkDistance; dz <= spawnChunkDistance; dz++) {
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
	auto loadSpawnChunks = [totalSpawnChunks, spawnChunkDistance](WorldManager& _world) {
		auto start = std::chrono::steady_clock::now();
		int loadedChunks = 0;
		while (true) {
			loadedChunks = 0;
			// Force gen these chunks AS FAST AS POSSIBLE
			_world.PumpPipeline({});
			_world.pool.wait();
			_world.DrainGenQueue();
			_world.regionManager->iopool.wait();
			_world.DrainLoadQueue();
			_world.PopulateReady();
			_world.lightManager.ProcessLightQueue(_world);

			for (int dx = -spawnChunkDistance; dx <= spawnChunkDistance; dx++) {
				for (int dz = -spawnChunkDistance; dz <= spawnChunkDistance; dz++) {
					Int32_2 p{ (_world.spawnPoint.x >> 4) + dx, (_world.spawnPoint.z >> 4) + dz };
					auto it = _world.chunks.find(p);
					if (it != _world.chunks.end() && it->second->state.load() >= ChunkState::Generated)
						loadedChunks++;
				}
			}

			// Update load percentage every second
			if (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= 1.0f) {
				int percentLoaded = int((float(loadedChunks) / float(totalSpawnChunks)) * 100.0f);
				GlobalLogger().info << "Loading spawn.. " << percentLoaded << "%\n";
				start = std::chrono::steady_clock::now();
			}

			// Have we loaded all the spawn chunks?
			if (loadedChunks >= totalSpawnChunks)
				break;
		}
		GlobalLogger().info << "Loading spawn.. 100%\n";
	};

	GlobalLogger().info << "Loading spawn chunks for Overworld: (" << totalSpawnChunks << ")\n";
	loadSpawnChunks(gameRuntime.world);

	GlobalLogger().info << "Loading spawn chunks for Hell: (" << totalSpawnChunks << ")\n";
	loadSpawnChunks(gameRuntime.worldHell);

	float startupSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startupStart).count();
	GlobalLogger().info << "Startup Complete. (" << std::setprecision(4) << startupSeconds << "s)\n";
}

void Server::Run() {
	Startup();

	static constexpr auto TICK_DURATION = std::chrono::nanoseconds(std::chrono::seconds{ 1 }) / TICKS_PER_SECOND;

	using Clock = std::chrono::steady_clock;

	std::chrono::nanoseconds avgTotalTickDuration{ 0 };
	int avgTickCount = 0;

	uint64_t ticks = 0;
	auto baseTime = Clock::now();

	// Main Tick loop
	// Heavily based on https://github.com/Minestom/Minestom/blob/59406d5b54d5221df85f381f204fbc07fd861a43/src/main/java/net/minestom/server/thread/TickSchedulerThread.java
	while (!shutdownRequested.load()) {
		auto tickStart = Clock::now();
		Tick();
		auto tickEnd = Clock::now();

		// Sample and print average Tick data
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
		if (Clock::now() > nextTickTime + MAX_TICK_CATCH_UP * TICK_DURATION) {
			baseTime = Clock::now();
			ticks = 0;
			GlobalLogger().warn << "Can't keep up with ticks!\n";
		}
	}

	// Shutdown was requested. Save and clean up on the main thread
	Stop();
	shutdownRequested.store(false); // Unblock the ctrl handler thread
}

void Server::Stop() {
	if (stopped)
		return;
	stopped = true;
	GlobalLogger().info << "Server shutting down...\n";
	for (auto& session : players) {
		connStateManager.DisconnectPlayer(*session, "Server Closed", *this);
		session->stream.FlushWriteBufferBlocking();
		auto savedNbt = session->SerializeToNbt();
		gameRuntime.saveManager.SavePlayerNbt(std::string(session->username.begin(), session->username.end()), savedNbt);
	}
	ServerSocketManager::CloseSocket(serverSocket);
	gameRuntime.world.Shutdown();
	gameRuntime.worldHell.Shutdown();

	// Save our level file
	LevelData& curLevelData = gameRuntime.saveManager.GetLevelData();
	curLevelData.randomSeed = gameRuntime.world.seed;
	curLevelData.spawnPoint = gameRuntime.world.spawnPoint;
	curLevelData.time = gameRuntime.world.elapsedTicks;
	gameRuntime.saveManager.SaveLevelFile(curLevelData);
}

void Server::AcceptNewPlayers() {
	auto clientSocket = ServerSocketManager::CreateClientSocket(serverSocket);
	if (clientSocket < 0)
		return;
	players.push_back(std::make_shared<PlayerSession>(clientSocket, gameRuntime));
}

void Server::Tick() {
	AcceptNewPlayers();
	[[maybe_unused]] const int playerCount = int(players.size());

	for (auto& session : players) {
		if (session->connState == ConnectionState::Playing)
			ProcessIncoming(*session);
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
	gameRuntime.world.Tick(overworldPositions);
	gameRuntime.world.Update(overworldPositions);
	gameRuntime.worldHell.Tick(netherPositions);
	gameRuntime.worldHell.Update(netherPositions);

	// Send all of the block changes that have accumulated since the last Tick, then clear the list.
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChanges;
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChangesHell;
	localBlockChanges.swap(chunkBlockChanges);
	localBlockChangesHell.swap(chunkBlockChangesHell);

	// Update the entity trackers
	overworldEntityTracker.Tick();
	hellEntityTracker.Tick();

	// Handle connection state for each player
	for (auto& session : players) {
		connStateManager.HandleConnectionState(*session, *this);

		// Drain chunk-session index updates that ChunkSender recorded.
		for (const auto& pos : session->newlyFlushed)
			IndexAddChunk(*session, pos);
		session->newlyFlushed.clear();
		for (const auto& pos : session->newlyUnloaded)
			IndexRemoveChunk(*session, pos);
		session->newlyUnloaded.clear();

		// Check inventory diffs
		auto diffs2 = session->inventoryInteraction.TickDiff();
		if (diffs2.size() <= 5) {
			for (auto difference : diffs2) {
				PacketUtilities::SendSlot(*session, 0, difference.slot, &difference.stack);
			}
		} else {
			// Too many changes, just resend the whole inventory
			PacketUtilities::SendInventory(*session, 0, *session->inventoryInteraction.inventory);
		}

		if (!session->activeInteraction)
			continue;

		// Force close windows that reference tile entities that have been deleted
		if (!session->activeInteraction->CanExist()) {
			PacketUtilities::CloseContainer(*session);
			continue;
		}

		// Send each differing slot
		auto diffs = session->activeInteraction->TickDiff();
		if (diffs.size() <= 5) {
			for (auto difference : diffs) {
				PacketUtilities::SendSlot(*session, session->openWindowId, difference.slot, &difference.stack);
			}
		} else {
			// Too many changes, just resend the whole inventory
			PacketUtilities::SendInventory(*session, session->openWindowId, *session->activeInteraction->inventory);
		}

		if (this->gameRuntime.world.elapsedTicks % 40 == 0) {
			// Save periodically
			auto savedNbt = session->SerializeToNbt();
			gameRuntime.saveManager.SavePlayerNbt(std::string(session->username.begin(), session->username.end()),
			                                      savedNbt);
		}
	}

	// Dispatch block changes.
	ChunkBroadcaster::BroadcastBlockChanges(*this, localBlockChanges, 0, gameRuntime.world);
	ChunkBroadcaster::BroadcastBlockChanges(*this, localBlockChangesHell, -1, gameRuntime.worldHell);

	// Flush all pending outgoing data to the socket once per Tick.
	for (auto& session : players) {
		session->stream.FlushWriteBuffer();
	}
	this->DisconnectClients();
}

void Server::DisconnectClients() {
	// Mark clients who have timed out for removal
	auto now = std::chrono::steady_clock::now();
	for (auto& session : players) {
		if (session->connState == ConnectionState::Playing) {
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->lastPacketTime).count();
			if (elapsed > timeoutSeconds) {
				GlobalLogger().info << "Player " << session->username << " timed out\n";
				connStateManager.DisconnectPlayer(*session, "Connection timed out.", *this);
				SendGlobalChatMessage("§e" + session->username + " left the game.");
			}
		} else {
			// Kill stuck handshakers
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->lastPacketTime).count();
			if (elapsed > timeoutSeconds) {
				session->stream.SetConnected(false);
				GlobalLogger().info << "Disconnected dataless stream. (Most likely a prober!)\n";
			}
		}
	}

	// Force disconnect players that quit
	players.erase(std::remove_if(players.begin(), players.end(),
	                             [&](const auto& _s) {
		                             if (!_s->stream.IsConnected()) {
			                             if (_s->entity)
				                             GlobalLogger().info << "Disconnected client " << _s->username
				                                                 << " with entity id " << _s->entity->id << "\n";

			                             if (_s->connState == ConnectionState::Playing ||
			                                 _s->connState == ConnectionState::WaitingForSpawnChunks) {
				                             auto savedNbt = _s->SerializeToNbt();
				                             gameRuntime.saveManager.SavePlayerNbt(
				                                 std::string(_s->username.begin(), _s->username.end()), savedNbt);
			                             }

			                             IndexRemoveSession(*_s);
			                             chunkSender.Remove(*_s);
			                             SendGlobalChatMessage("§e" + _s->username + " left the game.");
			                             return true;
		                             }
		                             return false;
	                             }),
	              players.end());
};

void Server::TransferPlayerDimension([[maybe_unused]] PlayerSession& _session) {}

void Server::ProcessIncoming(PlayerSession& _session) {
	WorldManager& sessionWorld = _session.dimension == -1 ? gameRuntime.worldHell : gameRuntime.world;

	while (_session.stream.HasData()) {
		PacketId packetId = _session.stream.Read<PacketId>();

		if (!PacketDispatcher::Dispatch(packetId, _session, sessionWorld, *this))
			return; // session is dead, or sent an unknown packet

		if (_session.stream.CheckAndClearShortRead()) {
			break;
		}
	}
	// Update our last packet time for the timeout code
	_session.lastPacketTime = std::chrono::steady_clock::now();
}