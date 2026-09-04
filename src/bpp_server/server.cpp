/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "base_types.h"
#include "config/list_parser.h"
#include "dimensions.h"
#include "logger.h"
#include "packet/packet_utils.h"
#include "trackers/inventory_tracker.h"
#include "world.h"
#include <future>
#include <string>
#include <thread>

#if defined(__linux__) || defined(__APPLE__) || defined(__HAIKU__)
#include <fcntl.h>
#include <iomanip>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "../bpp_utilities/compression_test.h"

#include "server.h"
#include "version.h"
#include <chrono>

#ifdef DISCORD_INTEGRATION
#include "discord.h"
#endif

Server::Server() : gameRuntime(), config("server.properties") {
	ServerBlock::Initialize();
	LoadConfig();

#ifdef DISCORD_INTEGRATION
	GlobalDiscord().Init(config.GetAsString("discord-token"), config.GetAsString("discord-channel-id"),
	                     config.GetAsString("discord-guild-id"), config.GetAsString("discord-admin-role-id"),
	                     config.GetAsString("discord-webhook-url"));
#endif

	serverSocket = ServerSocketManager::CreateServerSocket(serverPort);
	if (serverSocket < 0) {
		GlobalLogger().error << "**** FAILED TO CREATE SERVER SOCKET!" << "\n";
		exit(1);
	}
	GlobalLogger().info << "Server initialized on port " << serverPort << "\n";
	gameRuntime.Init(config.GetAsString("level-name", "world"), config.GetAsString("level-seed", "0"),
	                 config.GetAsNumber("view-distance", 8));
}

Server::~Server() {
	this->Stop();
}

void Server::SendEntityToDimension(Dimension _dim, std::shared_ptr<Entity> _entity) {
	// Remove our entity from our watcher
	Dimension oldDim = _entity->dim;
	EntityId oldId = _entity->id;
	if (oldDim == _dim)
		return;

	// Remove the entity from the world's entity managers
	WorldManager* world = GetWorldForDimension(oldDim);
	WorldManager* newWorld = GetWorldForDimension(_dim);
	world->entityManager.RemoveEntity(_entity->id);

	// Rebind entity
	_entity->isDead = false;
	newWorld->entityManager.AddEntity(_entity, oldId);
}

void Server::SendPlayerToDimension(Dimension _dim, PlayerSession& _session) {
	if (_dim == _session.dimension)
		return;

	// Flush all of our dimension dependent data
	IndexRemoveSession(_session);
	_session.dimension = _dim;
	_session.flushedChunks.clear();
	_session.sentChunks.clear();
	_session.pendingBlockChanges.clear();
	_session.newlyFlushed.clear();
	_session.newlyUnloaded.clear();
	_session.entityTracker = _session.dimension == Dimension::Overworld ? &overworldEntityTracker : &hellEntityTracker;

	// Make sure we don't send any pending chunk updates
	chunkSender.inFlight.erase(&_session);
	chunkSender.subRegionFlight.erase(&_session);

	// Send a respawn packet
	Packet::Respawn pkt;
	pkt.dimension = _dim;
	pkt.Serialize(_session.stream);
	_session.connState = ConnectionState::WaitingForSpawnChunks;

	// Update our state
	Packet::SetHealth sh;
	sh.health = _session.entity->health;
	sh.Serialize(_session.stream);
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
		    { "view-distance", "10" },
		    { "white-list", "false" },
		    //{"server-ip", ""},
		    //{"motd", "A Minecraft Server"},
		    //{"pvp","true"},
		    // use a random device to seed another prng that gives us our seed
		    { "level-seed", std::to_string(std::mt19937(std::random_device()())()) },
		    //{"spawn-animals",true}
		    { "server-port", "25565" },
#ifdef DISCORD_INTEGRATION
		    { "discord-token", "" },
		    { "discord-channel-id", "" },
		    // Optional: register slash commands to one guild instantly. Leave empty for global.
		    { "discord-guild-id", "" },
		    // Role required for privileged slash commands (e.g. /stop). Empty denies them.
		    { "discord-admin-role-id", "" },
		    // Optional: a channel webhook URL (Channel Settings -> Integrations -> Webhooks).
		    // When set, in-game chat is relayed under each player's own name + skin face
		    // instead of the bot's. Leave empty to relay chat as the bot instead.
		    { "discord-webhook-url", "" },
#endif
		    //{"allow-nether",true},
		    //{"spawn-monsters","true"},
		    //{"max-players", "-1"},
		    { "online-mode", "false" },
#ifdef BETACRAFT_HEARTBEAT
		    { "betacraft-heartbeat", "false" },
		    { "betacraft-name", "A Minecraft server" },
		    { "betacraft-description", "" },
		    { "betacraft-socket", "" },
		    { "betacraft-private-key", "" },
		    { "betacraft-category", "beta" },
		    { "betacraft-game-version", "b1.7.3" },
		    { "betacraft-protocol", "beta_14" },
		    { "betacraft-v1-version", "b1.7.3" },
		    { "betacraft-send-players", "true" },
		    { "betacraft-icon", "" },
#endif
		    //{"allow-flight","false"}
		});
		config.SaveToDisk();
	}
	//chunkDistance = config.GetAsNumber<int32_t>("view-distance");
	serverPort = config.GetAsNumber<int32_t>("server-port", 25565);
#ifdef ONLINE_MODE_AUTHENTICATION
	auth.onlineMode = config.GetAsBoolean("online-mode", false);
#endif
#ifdef BETACRAFT_HEARTBEAT
	betacraftHeartbeat.Load(config, serverPort);
#endif
	//motd = config.GetAsString("motd");
	//maximumPlayers = config.GetAsNumber<int32_t>("max-players");
	//maximumThreads = config.GetAsNumber<int32_t>("max-generator-threads");
	useWhitelist = config.GetAsBoolean("white-list");
	operatorUsernames = ListParser::Read(ListParser::Target::Operator);
	// Bans are always enforced, unlike the whitelist, so we load them unconditionally
	bannedUsernames = ListParser::Read(ListParser::Target::BannedPlayers);
	bannedIps = ListParser::Read(ListParser::Target::BannedIps);
	if (useWhitelist) {
		LoadWhitelist();
		GlobalLogger().info << "Whitelist enabled!\n";
	}
}

void Server::LoadWhitelist() {
	if (whitelistLoaded)
		return;
	whitelistedUsernames = ListParser::Read(ListParser::Target::Whitelist);
	whitelistLoaded = true;
}

void Server::UnloadWhitelist() {
	whitelistedUsernames.clear();
	whitelistedUsernames.shrink_to_fit();
	whitelistLoaded = false;
}

void Server::SetWhitelistEnabled(bool _enabled, bool _persist) {
	if (_enabled)
		LoadWhitelist();
	const bool changed = useWhitelist != _enabled;
	useWhitelist = _enabled;
	if (!_enabled)
		UnloadWhitelist();
	if (changed)
		GlobalLogger().info << (useWhitelist ? "Whitelist enabled!\n" : "Whitelist disabled!\n");
	if (!_persist)
		return;
	config.Set("white-list", useWhitelist ? "true" : "false");
	if (!config.SaveKeyToDisk("white-list"))
		GlobalLogger().error << "Failed to persist white-list to server.properties\n";
}

bool Server::SaveWhitelist() {
	if (!whitelistLoaded)
		return true;
	return ListParser::Write(whitelistedUsernames, ListParser::Target::Whitelist);
}

bool Server::SaveOperators() {
	return ListParser::Write(operatorUsernames, ListParser::Target::Operator);
}

bool Server::SaveBannedPlayers() {
	return ListParser::Write(bannedUsernames, ListParser::Target::BannedPlayers);
}

bool Server::SaveBannedIps() {
	return ListParser::Write(bannedIps, ListParser::Target::BannedIps);
}

void Server::ReloadWhitelist() {
	if (!useWhitelist)
		return;
	whitelistedUsernames = ListParser::Read(ListParser::Target::Whitelist);
	whitelistLoaded = true;
}

void Server::Startup() {
	auto startupStart = std::chrono::steady_clock::now();
	GlobalLogger().info << "Initializing server startup.. \n";

	// Init auth
#ifdef ONLINE_MODE_AUTHENTICATION
	if (!auth.onlineMode) {
		GlobalLogger().warn << "**** SERVER IS RUNNING IN OFFLINE/INSECURE MODE!\n"
		                    << "The server will make no attempt to authenticate usernames. Beware.\n"
		                    << "While this makes the game possible to play without internet access,\n"
		                    << "it also opens up the ability for hackers to connect with any username they choose.\n"
		                    << "To change this, set \"online-mode\" to \"true\" in the server.properties file.\n";
	}
#endif

	// Setup commands
	commandManager.Init(this);

	// Setup the block callback so we can send it to clients
	auto makeBlockUpdateCallback = [this](Dimension _dimensionId, auto& _blockChangeMap) {
		return [this, _dimensionId, &_blockChangeMap](PendingBlock _pendingBlock, Int32_2 _chunkPos) {
			auto idxIt = chunkSessions.find(ChunkKey(_chunkPos, _dimensionId));
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

	auto registerExplosionCallback = [this](WorldManager& _world, EntityTracker& _entityTracker) {
		_world.onExplosion = [&](Vec3 _pos, float _size, std::unordered_set<Int3>& _blockPositions, Entity* _exploder) {
			if (!_exploder)
				return;
			Packet::Explosion pkt;
			pkt.position = _pos;
			pkt.numberOfDestroyedBlocks = _blockPositions.size();
			pkt.radius = _size;

			Int3 blockPos = { int(_pos.x), int(_pos.y), int(_pos.z) };
			for (auto& pos : _blockPositions) {
				pkt.destroyedBlocks.push_back(static_cast<int8_t>(pos.x - blockPos.x));
				pkt.destroyedBlocks.push_back(static_cast<int8_t>(pos.y - blockPos.y));
				pkt.destroyedBlocks.push_back(static_cast<int8_t>(pos.z - blockPos.z));
			}
			_entityTracker.SendPacketToViewers(pkt, _exploder->id);
		};
	};

	gameRuntime.world.onBlockUpdate = makeBlockUpdateCallback(Dimension::Overworld, chunkBlockChanges);
	gameRuntime.worldHell.onBlockUpdate = makeBlockUpdateCallback(Dimension::Nether, chunkBlockChangesHell);

	gameRuntime.world.onWorldEvent = [this](PacketData::WorldEvent _eventType, Int3 _position, int32_t _data,
	                                        PlayerSession* _triggeringSession) {
		WorldEventBroadcaster::BroadcastWorldEvent(*this, _eventType, _position, _data, Dimension::Overworld,
		                                           _triggeringSession);
	};
	gameRuntime.worldHell.onWorldEvent = [this](PacketData::WorldEvent _eventType, Int3 _position, int32_t _data,
	                                            PlayerSession* _triggeringSession) {
		WorldEventBroadcaster::BroadcastWorldEvent(*this, _eventType, _position, _data, Dimension::Nether,
		                                           _triggeringSession);
	};

	registerEntityTrackerCallbacks(overworldEntityTracker, gameRuntime.world.entityManager);
	registerEntityTrackerCallbacks(hellEntityTracker, gameRuntime.worldHell.entityManager);

	registerExplosionCallback(gameRuntime.world, overworldEntityTracker);
	registerExplosionCallback(gameRuntime.worldHell, hellEntityTracker);

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
			_world.PopulateReady(INT_MAX);
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

	/*
	std::vector<Chunk*> chunks;
	for (auto& c : gameRuntime.world.chunks) {
		chunks.push_back(c.second.get());
	}
	auto result = ChunkBenchmark::Benchmark(chunks, 100);
	ChunkBenchmark::Print(result);
	*/

	GlobalLogger().info << "Loading spawn chunks for Hell: (" << totalSpawnChunks << ")\n";
	loadSpawnChunks(gameRuntime.worldHell);

	float startupSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startupStart).count();
	GlobalLogger().info << "Startup Complete. (" << std::setprecision(4) << startupSeconds << "s)\n";
#ifdef DISCORD_INTEGRATION
	GlobalDiscord().SendServerNotice("Server started!", Discord::EmbedColor::Green);
#endif
#ifdef BETACRAFT_HEARTBEAT
	betacraftHeartbeat.Start();
#endif
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
			averageTickMs = std::chrono::duration<double, std::milli>(avgTotalTickDuration).count() /
			                double(avgTickCount);
			avgTotalTickDuration = std::chrono::nanoseconds{ 0 };
			avgTickCount = 0;
		}

		++ticks;
		auto nextTickTime = baseTime + ticks * TICK_DURATION;
		// Slice the wait so shutdownRequested is observed even if libc++
		// restarts sleep_until() after EINTR (musl + libcurl).
		while (!shutdownRequested.load() && Clock::now() < nextTickTime) {
			auto remaining = nextTickTime - Clock::now();
			auto slice = remaining < std::chrono::milliseconds(10) ? remaining : std::chrono::milliseconds(10);
			std::this_thread::sleep_for(slice);
		}
		if (shutdownRequested.load())
			break;

		// Check if the server can not keep up with the tickrate
		// if it gets too far behind, reset the ticks & baseTime
		// to avoid running too many ticks at once
		if (Clock::now() > nextTickTime + MAX_TICK_CATCH_UP * TICK_DURATION) {
			baseTime = Clock::now();
			ticks = 0;
			auto overshoot = std::chrono::duration<double, std::milli>(Clock::now() - nextTickTime).count();
			GlobalLogger().warn << "Can't keep up with ticks! (tick avg " << averageTickMs << "ms, overshoot "
			                    << overshoot << "ms, budget "
			                    << std::chrono::duration<double, std::milli>(TICK_DURATION).count() << "ms)\n";
		}
	}

	// Wait for any in-flight write flushes before saving and cleaning up
	while (!writePool.wait_for(std::chrono::milliseconds(100))) {
		if (shutdownRequested.load())
			break;
	}

	// Shutdown was requested. Save and clean up on the main thread
	Stop();
	shutdownRequested.store(false); // Unblock the ctrl handler thread
}

void Server::Stop() {
	if (stopped)
		return;
	stopped = true;
#ifdef BETACRAFT_HEARTBEAT
	betacraftHeartbeat.Stop();
#endif
#ifdef DISCORD_INTEGRATION
	//GlobalDiscord().SendServerNotice("Server stopped!", Discord::EmbedColor::Red);
	// TODO: The server often shuts down too fast for the embed to get sent!
	GlobalDiscord().Shutdown("Server stopped!");
#endif
	GlobalLogger().info << "Server shutting down...\n";
	for (auto& session : players) {
		connStateManager.DisconnectPlayer(*session, "Server Closed", *this);
		session->stream.FlushWriteBufferBlocking();
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

	// Save operator, whitelist, and ban updates
	SaveWhitelist();
	SaveOperators();
	SaveBannedPlayers();
	SaveBannedIps();
}

void Server::AcceptNewPlayers() {
	std::string clientIp;
	auto clientSocket = ServerSocketManager::CreateClientSocket(serverSocket, &clientIp);
	if (clientSocket < 0)
		return;
	auto session = std::make_shared<PlayerSession>(clientSocket, gameRuntime);
	session->ipAddress = clientIp;
	players.push_back(std::move(session));
}

void Server::StopTimeout(float _secondsUntilShutdown) {
	shutdownTimer = static_cast<uint16_t>(_secondsUntilShutdown * static_cast<float>(TICKS_PER_SECOND));
}

void Server::ResetTimeout() {
	shutdownTimer = 0;
}

void Server::Tick() {
	// Wait for the previous tick's write flushes to finish. The main thread
	// owns the session write buffers for the duration of the tick; the write
	// thread may only touch them after Tick has submitted its flushes.
	writePool.wait();
#ifdef DISCORD_INTEGRATION
	GlobalDiscord().Drain(*this);
#endif
	AcceptNewPlayers();
	[[maybe_unused]] const int playerCount = int(players.size());

	std::vector<ClientPosition> overworldPositions;
	std::vector<ClientPosition> netherPositions;
	for (auto& session : players) {
		session->stream.DrainToBuffer();
		if (session->connState == ConnectionState::WaitingForSpawnChunks ||
		    session->connState == ConnectionState::Playing) {
			// Process our packets
			ProcessIncoming(*session);

			// Register with the correct dimension
			if (session->dimension == Dimension::Nether)
				netherPositions.push_back(session->position);
			else
				overworldPositions.push_back(session->position);

			// Update our break state
			this->UpdateBlockBreaking(*session, *GetWorldForDimension(session->dimension));

			// Autosave every 2 seconds
			if (gameRuntime.world.tickScheduler.currentTick % 40 == 0) {
				SavePlayer(session->username);
			}
		}

		connStateManager.HandleConnectionState(*session, *this);

		// Drain chunk-session index updates that ChunkSender recorded
		for (const auto& pos : session->newlyFlushed)
			IndexAddChunk(*session, pos);
		session->newlyFlushed.clear();
		for (const auto& pos : session->newlyUnloaded)
			IndexRemoveChunk(*session, pos);
		session->newlyUnloaded.clear();
	}
	// Inventory tracker
	InventoryTracker::Tick(*this);

	// Worlds
	gameRuntime.world.Tick(overworldPositions);
	gameRuntime.world.Update(overworldPositions);
	gameRuntime.worldHell.Tick(netherPositions);
	gameRuntime.worldHell.Update(netherPositions);

	// Send all of the block changes that have accumulated since the last Tick, then clear the list.
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChanges;
	std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChangesHell;
	localBlockChanges.swap(chunkBlockChanges);
	localBlockChangesHell.swap(chunkBlockChangesHell);

	// Entity trackers
	overworldEntityTracker.Tick();
	hellEntityTracker.Tick();

	// Dispatch block changes
	ChunkBroadcaster::BroadcastBlockChanges(*this, localBlockChanges, Dimension::Overworld, gameRuntime.world);
	ChunkBroadcaster::BroadcastBlockChanges(*this, localBlockChangesHell, Dimension::Nether, gameRuntime.worldHell);

	// Disconnect timed-out clients; the removed sessions are returned so we
	// can send their last packets (e.g. a disconnect reason) before they die
	std::vector<std::shared_ptr<PlayerSession>> removedSessions = this->DisconnectClients();

	// Flush pending outgoing data on the write thread so socket I/O never
	// holds up the main tick thread
	std::vector<std::future<void>> flushFutures;
	for (auto& session : players) {
		if (session->stream.GetRawWriteBuffer().empty())
			continue;
		auto sessionRef = session;
		flushFutures.push_back(writePool.submit_task([sessionRef]() { sessionRef->stream.FlushWriteBuffer(); }));
	}

	// Sessions removed this tick get one final synchronous flush (their
	// pending data is small) before their shared_ptr drops and they are destroyed
	for (auto& session : removedSessions)
		session->stream.FlushWriteBuffer();

	// TODO: This is rather fragile!
	// Countdown
	if (shutdownTimer > 1)
		shutdownTimer--;
	if (shutdownTimer == 1)
		shutdownRequested.store(true);

#ifdef BETACRAFT_HEARTBEAT
	if (betacraftHeartbeat.Enabled() && gameRuntime.world.tickScheduler.currentTick % TICKS_PER_SECOND == 0) {
		BetacraftHeartbeatSnapshot snap;
		snap.maxPlayers = config.GetAsNumber<int>("max-players", 20);
		if (snap.maxPlayers < 0)
			snap.maxPlayers = 20;
		snap.onlineMode = config.GetAsBoolean("online-mode", false);
		for (const auto& session : players) {
			if (session->connState != ConnectionState::Playing)
				continue;
			++snap.onlinePlayers;
			if (!session->username.empty())
				snap.playerNames.push_back(session->username);
		}
		betacraftHeartbeat.UpdateSnapshot(snap);
	}
#endif
}

void Server::TryForceBreak(PlayerSession& _session, WorldManager& _world) {
	if (!_session.pendingBlockBreak.has_value())
		return;

	if (_session.pendingBlockBreak->damage < 0.7f) {
		// We missed the client break, so we need to force it to break on the server side
		_session.pendingBlockBreak->clientBreakMissed = true;
		return;
	}

	// Success!
	OnPlayerBlockBreak(_session, _world);
}

void Server::UpdateBlockBreaking(PlayerSession& _session, WorldManager& _world) {
	if (!_session.pendingBlockBreak.has_value())
		return;

	auto miningConditions = [&]() -> std::pair<bool, bool> {
		bool inWater = _session.entity && _session.entity->HeadInWater();
		bool onGround = !_session.entity || _session.entity->onGround;
		return { inWater, onGround };
	};

	auto finishMiningWithTool = [&](ItemStack* _held, BlockType _block) {
		if (!_held)
			return;
		auto it = Items::toolBehavior.find(_held->id);
		if (it != Items::toolBehavior.end() && it->second.onBlockFinishMining)
			it->second.onBlockFinishMining(_held, _block);
	};

	auto [inWater, onGround] = miningConditions();
	auto blockId = _session.pendingBlockBreak->lastBlock;
	auto blockPos = _session.pendingBlockBreak->lastBlockPos;
	ItemStack* heldItem = _session.inventory.GetHeldItem();

	auto damagePerTick = Items::BlockDamagePerTick(heldItem, blockId, inWater, onGround);

	// Client breaks immediately when blockStrength >= 1
	if (damagePerTick >= 1.0f) {
		if (_session.entity) {
			if (auto func = Blocks::blockBehaviors[blockId].onBlockDestroyedByPlayer) {
				func(_world, blockPos, *_session.entity);
			} else {
				Blocks::BreakAndDropBlock(_world, blockPos);
			}
		}
		finishMiningWithTool(heldItem, blockId);
		_session.pendingBlockBreak.reset();
		return;
	}

	// Dont break if our damage per tick is too low
	if (damagePerTick <= 0.0f) {
		_session.pendingBlockBreak.reset();
		return;
	}

	// See if we can actually break this block
	if (_session.pendingBlockBreak->damage < 1.0f || !_session.pendingBlockBreak->clientBreakMissed) {
		_session.pendingBlockBreak->damage = std::min(1.0f, _session.pendingBlockBreak->damage + damagePerTick);
		return;
	}

	OnPlayerBlockBreak(_session, _world);
}

std::vector<std::shared_ptr<PlayerSession>> Server::DisconnectClients() {
	// Mark clients who have timed out for removal
	auto now = std::chrono::steady_clock::now();
	for (auto& session : players) {
		if (session->connState == ConnectionState::Playing) {
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->lastPacketTime).count();
			if (elapsed > timeoutSeconds) {
				GlobalLogger().info << "Player " << session->username << " timed out\n";
				connStateManager.DisconnectPlayer(*session, "Connection timed out.", *this);
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
	std::vector<std::shared_ptr<PlayerSession>> removed;
	players.erase(std::remove_if(players.begin(), players.end(),
	                             [&](const auto& _s) {
		                             if (!_s->stream.IsConnected()) {
			                             if (_s->entity) {
				                             GlobalLogger().info << "Disconnected client " << _s->username
				                                                 << " with entity id " << _s->entity->id << "\n";
				                             SendGlobalChatMessage("§e" + _s->username + " left the game.", false);
#ifdef DISCORD_INTEGRATION
				                             GlobalDiscord().SendPlayerLeaveMessage(_s->username);
#endif
				                             if (_s->entity->entityManager)
					                             _s->entity->entityManager->RemoveEntity(_s->entity->id);
			                             }
			                             IndexRemoveSession(*_s);
			                             chunkSender.Remove(*_s);
			                             removed.push_back(_s);
			                             return true;
		                             }
		                             return false;
	                             }),
	              players.end());
	return removed;
};

void Server::ProcessIncoming(PlayerSession& _session) {
	WorldManager& sessionWorld = _session.dimension == Dimension::Nether ? gameRuntime.worldHell : gameRuntime.world;

	while (_session.stream.HasData()) {
		size_t packetMark = _session.stream.Mark();

		PacketId packetId = _session.stream.Read<PacketId>();

		if (_session.stream.CheckAndClearShortRead()) {
			// Not even the ID has fully arrived yet.
			_session.stream.Rollback(packetMark);
			break;
		}

		if (!PacketDispatcher::Dispatch(packetId, _session, sessionWorld, *this))
			return; // session is dead, or sent an unknown packet

		if (_session.stream.CheckAndClearShortRead()) {
			// The packet wasn't full.
			_session.stream.Rollback(packetMark);
			break;
		}
	}

	// Update our last packet time for the timeout code
	_session.lastPacketTime = std::chrono::steady_clock::now();
}