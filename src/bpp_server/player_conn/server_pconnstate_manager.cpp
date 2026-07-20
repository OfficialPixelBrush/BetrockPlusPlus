/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "../packet/packet_utils.h"
#include "../server.h"
#include "version.h"

void PlayerConnStateManager::handleConnectionState(PlayerSession& _session, Server& _server) {
	switch (_session.connState) {
	case ConnectionState::Handshaking:
		handleHandshake(_session, _server);
		break;
	case ConnectionState::LoggingIn:
		handleLogin(_session, _server);
		break;
	case ConnectionState::WaitingForSpawnChunks:
		waitForSpawnChunks(_session, _server);
		break;
	case ConnectionState::Playing: {
		WorldManager& sessionWorld = _session.dimension == -1 ? _server.gameRuntime.worldHell : _server.gameRuntime.world;
		_server.chunkSender.enqueue(_session, sessionWorld, 16);
		_server.chunkSender.flush(_session);
		if (sessionWorld.elapsed_ticks % 20 == 0) {
			// Update the server time so client's don't desync
			Packet::SetTime time;
			time.time = sessionWorld.elapsed_ticks;
			time.Serialize(_session.stream);
		}
		break;
	}
	}
}

void PlayerConnStateManager::handleHandshake(PlayerSession& _session, [[maybe_unused]] Server& _server) {
	if (!_session.stream.hasData())
		return;
	PacketId packetId = _session.stream.Read<PacketId>();

	if (_session.stream.checkAndClearShortRead())
		return;
	if (packetId != PacketId::PreLogin)
		return;

	Packet::PreLogin incoming;
	incoming.Deserialize(_session.stream);
	if (_session.stream.checkAndClearShortRead()) {
		return;
	}
	_session.username = incoming.username;

	Packet::PreLogin response;
	response.username = "-";
	response.Serialize(_session.stream);

	GlobalLogger().info << "Player " << _session.username << " is logging in.\n";

	_session.connState = ConnectionState::LoggingIn;
}

void PlayerConnStateManager::handleLogin(PlayerSession& _session, Server& _server) {
	if (!_session.stream.hasData())
		return;

	PacketId packetId = _session.stream.Read<PacketId>();
	if (_session.stream.checkAndClearShortRead())
		return;
	if (packetId != PacketId::Login)
		return;

	Packet::Login incoming;
	incoming.Deserialize(_session.stream);
	if (_session.stream.checkAndClearShortRead()) {
		return;
	}

	// Load player data before building the Login response so we know which dimension they're in
	auto playerNbt = _server.gameRuntime.saveManager.getPlayerNBT(
	    std::string(_session.username.begin(), _session.username.end()));
	_session.loadPlayerNBT(playerNbt);

	// Get the right world pointer
	WorldManager& sessionWorld = _session.dimension == -1 ? _server.gameRuntime.worldHell : _server.gameRuntime.world;

	// Initialize our entity
	if (!_session.entity)
		_session.entity = std::make_shared<EntityMPPlayer>();
	_session.entity->session = &_session;
	_session.entity->id = sessionWorld.entityManager.getNextEntityId();
	_session.entity->dim = _session.dimension == -1 ? Dimension::Nether : Dimension::Overworld;

	Packet::Login response;
	response.entity_id = _session.entity->id;
	response.username = _session.username;
	response.worldSeed = _server.gameRuntime.world.seed;
	response.dimension = Dimension(_session.dimension);
	response.Serialize(_session.stream);

	Packet::SetSpawnPosition spawn;
	spawn.position = sessionWorld.spawnPoint;
	spawn.Serialize(_session.stream);

	Packet::SetHealth health;
	health.health = 20;
	health.Serialize(_session.stream);

	Packet::SetTime time;
	time.time = sessionWorld.elapsed_ticks;
	time.Serialize(_session.stream);

	// Get a fresh respawn point
	auto respawnPoint = sessionWorld.getSpawnPoint(true);

	// If our session position is the default then overwrite it
	if (_session.position.pos == Vec3{ -1, -1000000, -1 }) {
		_session.position.pos = { float(respawnPoint.x) + 0.5, float(respawnPoint.y), float(respawnPoint.z) + 0.5 };
	}

	// Convert the feet-based respawn height into our posY convention (eye level)
	_session.position.pos.y += (PLAYER_EYE_HEIGHT + 0.00001);

	// Log that we logged in!
	GlobalLogger().info << "Player " << _session.username << " logged in with entity ID " << _session.entity->id
	                    << " at (" << _session.position.pos.x << ", " << _session.position.pos.y << ", "
	                    << _session.position.pos.z << ")\n";

	// Update our last trusted position
	_session.entity->position = _session.position.pos;
	_session.pendingTeleport = _session.position.pos;
	_session.entity->rebuildCollider();

	// Let everyone else know we logged in
	_server.sendGlobalChatMessage("§e" + _session.username + " joined the game.");

	// Send our inventory
	PacketUtilities::sendInventory(_session, 0, _session.inventory);

	// Snapshot current contents so the tick loop's diffing (tickDiff) has a real baseline
	// to compare against, instead of starting from an empty snapshot for the whole session.
	_session.inventoryInteraction.initSnapshot();

	_session.connState = ConnectionState::WaitingForSpawnChunks;
}

void PlayerConnStateManager::disconnectPlayer(PlayerSession& _session, const std::string& _reason,
                                              [[maybe_unused]] Server& _server) {
	// Send disconnect reason to the leaving player
	Packet::Disconnect kick;
	kick.reason = _reason;
	kick.Serialize(_session.stream);
	_session.stream.setConnected(false); // This should force an NBT save
	GlobalLogger().info << "Player " << _session.username << " disconnected: " << _reason << "\n";
}

void PlayerConnStateManager::waitForSpawnChunks(PlayerSession& _session, Server& _server) {
	WorldManager& sessionWorld = _session.dimension == -1 ? _server.gameRuntime.worldHell : _server.gameRuntime.world;
	_server.chunkSender.enqueue(_session, sessionWorld, _server.flushChunkCount);
	_server.chunkSender.flush(_session);

	// Force a tiny view distance for players trying to spawn in
	_session.position.viewDistanceOverride = 3;

	// Spawn chunk radius; 3 chunks in each direction
	int spawnChunkX = int(std::floor(_session.position.pos.x)) >> 4;
	int spawnChunkZ = int(std::floor(_session.position.pos.z)) >> 4;

	int radius = CrossPlatform::Math::min(3, sessionWorld.getViewRadius());

	int total_spawn_chunks = ((radius * 2) + 1) * ((radius * 2) + 1);
	int loaded_chunks = 0;

	for (int dx = -radius; dx <= radius; dx++) {
		for (int dz = -radius; dz <= radius; dz++) {
			Int32_2 p{ spawnChunkX + dx, spawnChunkZ + dz };
			if (_session.flushedChunks.contains(p))
				loaded_chunks++;
		}
	}

	GlobalLogger().info << "Spawn chunks: " << loaded_chunks << " / " << total_spawn_chunks << "\n";

	if (loaded_chunks < total_spawn_chunks)
		return;

	GlobalLogger().info << "Spawn chunks sent. Setting player position\n";

	_session.position.pos.y += 0.0625;
	Packet::PlayerPositionAndRotation pos;
	pos.position = _session.position.pos;
	pos.camera_y = _session.position.pos.y + PLAYER_EYE_HEIGHT;
	pos.rotation = _session.rotation;
	pos.onGround = false;
	pos.Serialize(_session.stream);

	// Update our last trusted position
	_session.entity->position = _session.position.pos;
	_session.pendingTeleport = _session.position.pos;
	_session.entity->rebuildCollider();

	// Set view distance to server default
	_session.position.viewDistanceOverride = 0;

	GlobalLogger().info << "Client connected\n";
	_session.connState = ConnectionState::Playing;

	// Register our entity with the world
	if (!_session.entityRegistered)
		sessionWorld.entityManager.addEntity(_session.entity, _session.entity->id);
	_session.entityRegistered = true;

	// Give our player session a pointer to the entity tracker
	_session.entityTracker = _session.dimension == 0 ? &_server.overworldEntityTracker : &_server.hellEntityTracker;

	// Welcome message
	Packet::ChatMessage welcomeMsg;
	welcomeMsg.message = std::string("§eThis Server runs on ") + std::string(PROJECT_FULL_VERSION_LABEL);
	welcomeMsg.Serialize(_session.stream);
}