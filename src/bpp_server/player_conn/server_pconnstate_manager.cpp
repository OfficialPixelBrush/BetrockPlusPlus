/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "../packet/packet_utils.h"
#include "../server.h"
#include "version.h"

void PlayerConnStateManager::handleConnectionState(PlayerSession& session, Server& server) {
	switch (session.connState) {
	case ConnectionState::Handshaking:
		handleHandshake(session, server);
		break;
	case ConnectionState::LoggingIn:
		handleLogin(session, server);
		break;
	case ConnectionState::WaitingForSpawnChunks:
		waitForSpawnChunks(session, server);
		break;
	case ConnectionState::Playing: {
		WorldManager& sessionWorld = session.dimension == -1 ? server.gameRuntime.worldHell : server.gameRuntime.world;
		server.chunkSender.enqueue(session, sessionWorld, 16);
		server.chunkSender.flush(session);
		if (sessionWorld.elapsed_ticks % 20 == 0) {
			// Update the server time so client's don't desync
			Packet::SetTime time;
			time.time = sessionWorld.elapsed_ticks;
			time.Serialize(session.stream);
		}
		break;
	}
	}
}

void PlayerConnStateManager::handleHandshake(PlayerSession& session, [[maybe_unused]] Server& server) {
	if (!session.stream.hasData())
		return;
	PacketId packetId = session.stream.Read<PacketId>();

	if (session.stream.checkAndClearShortRead())
		return;
	if (packetId != PacketId::PreLogin)
		return;

	Packet::PreLogin incoming;
	incoming.Deserialize(session.stream);
	if (session.stream.checkAndClearShortRead()) {
		return;
	}
	session.username = incoming.username;

	Packet::PreLogin response;
	response.username = "-";
	response.Serialize(session.stream);

	GlobalLogger().info << "Player " << session.username << " is logging in.\n";

	session.connState = ConnectionState::LoggingIn;
}

void PlayerConnStateManager::handleLogin(PlayerSession& session, Server& server) {
	if (!session.stream.hasData())
		return;

	PacketId packetId = session.stream.Read<PacketId>();
	if (session.stream.checkAndClearShortRead())
		return;
	if (packetId != PacketId::Login)
		return;

	Packet::Login incoming;
	incoming.Deserialize(session.stream);
	if (session.stream.checkAndClearShortRead()) {
		return;
	}

	// Load player data before building the Login response so we know which dimension they're in
	auto playerNbt = server.gameRuntime.saveManager.getPlayerNBT(
	    std::string(session.username.begin(), session.username.end()));
	session.loadPlayerNBT(playerNbt);

	// Get the right world pointer
	WorldManager& sessionWorld = session.dimension == -1 ? server.gameRuntime.worldHell : server.gameRuntime.world;

	// Initialize our entity
	if (!session.entity)
		session.entity = std::make_shared<EntityMPPlayer>();
	session.entity->session = &session;
	session.entity->id = sessionWorld.entityManager.getNextEntityId();
	session.entity->dim = session.dimension == -1 ? Dimension::Nether : Dimension::Overworld;

	Packet::Login response;
	response.entity_id = session.entity->id;
	response.username = session.username;
	response.worldSeed = server.gameRuntime.world.seed;
	response.dimension = Dimension(session.dimension);
	response.Serialize(session.stream);

	Packet::SetSpawnPosition spawn;
	spawn.position = sessionWorld.spawnPoint;
	spawn.Serialize(session.stream);

	Packet::SetHealth health;
	health.health = 20;
	health.Serialize(session.stream);

	Packet::SetTime time;
	time.time = sessionWorld.elapsed_ticks;
	time.Serialize(session.stream);

	// Get a fresh respawn point
	auto respawnPoint = sessionWorld.getSpawnPoint(true);

	// If our session position is the default then overwrite it
	if (session.position.pos == Vec3{ -1, -1000000, -1 }) {
		session.position.pos = { float(respawnPoint.x) + 0.5, float(respawnPoint.y), float(respawnPoint.z) + 0.5 };
	}

	// Convert the feet-based respawn height into our posY convention (eye level)
	session.position.pos.y += (PLAYER_EYE_HEIGHT + 0.00001);

	// Log that we logged in!
	GlobalLogger().info << "Player " << session.username << " logged in with entity ID " << session.entity->id
	                    << " at (" << session.position.pos.x << ", " << session.position.pos.y << ", "
	                    << session.position.pos.z << ")\n";

	// Update our last trusted position
	session.entity->position = session.position.pos;
	session.pendingTeleport = session.position.pos;
	session.entity->rebuildCollider();

	// Let everyone else know we logged in
	server.sendGlobalChatMessage("§e" + session.username + " joined the game.");

	// Send our inventory
	PacketUtilities::sendInventory(session, 0, session.inventory);

	// Snapshot current contents so the tick loop's diffing (tickDiff) has a real baseline
	// to compare against, instead of starting from an empty snapshot for the whole session.
	session.inventoryInteraction.initSnapshot();

	session.connState = ConnectionState::WaitingForSpawnChunks;
}

void PlayerConnStateManager::disconnectPlayer(PlayerSession& session, const std::string& reason,
                                              [[maybe_unused]] Server& server) {
	// Send disconnect reason to the leaving player
	Packet::Disconnect kick;
	kick.reason = reason;
	kick.Serialize(session.stream);
	session.stream.setConnected(false); // This should force an NBT save
	GlobalLogger().info << "Player " << session.username << " disconnected: " << reason << "\n";
}

void PlayerConnStateManager::waitForSpawnChunks(PlayerSession& session, Server& server) {
	WorldManager& sessionWorld = session.dimension == -1 ? server.gameRuntime.worldHell : server.gameRuntime.world;
	server.chunkSender.enqueue(session, sessionWorld, server.flushChunkCount);
	server.chunkSender.flush(session);

	// Force a tiny view distance for players trying to spawn in
	session.position.viewDistanceOverride = 3;

	// Spawn chunk radius; 3 chunks in each direction
	int spawnChunkX = int(std::floor(session.position.pos.x)) >> 4;
	int spawnChunkZ = int(std::floor(session.position.pos.z)) >> 4;

	int radius = CrossPlatform::Math::min(3, sessionWorld.getViewRadius());

	int total_spawn_chunks = ((radius * 2) + 1) * ((radius * 2) + 1);
	int loaded_chunks = 0;

	for (int dx = -radius; dx <= radius; dx++) {
		for (int dz = -radius; dz <= radius; dz++) {
			Int32_2 p{ spawnChunkX + dx, spawnChunkZ + dz };
			if (session.flushedChunks.contains(p))
				loaded_chunks++;
		}
	}

	GlobalLogger().info << "Spawn chunks: " << loaded_chunks << " / " << total_spawn_chunks << "\n";

	if (loaded_chunks < total_spawn_chunks)
		return;

	GlobalLogger().info << "Spawn chunks sent. Setting player position\n";

	session.position.pos.y += 0.0625;
	Packet::PlayerPositionAndRotation pos;
	pos.position = session.position.pos;
	pos.camera_y = session.position.pos.y + PLAYER_EYE_HEIGHT;
	pos.rotation = session.rotation;
	pos.onGround = false;
	pos.Serialize(session.stream);

	// Update our last trusted position
	session.entity->position = session.position.pos;
	session.pendingTeleport = session.position.pos;
	session.entity->rebuildCollider();

	// Set view distance to server default
	session.position.viewDistanceOverride = 0;

	GlobalLogger().info << "Client connected\n";
	session.connState = ConnectionState::Playing;

	// Register our entity with the world
	if (!session.entityRegistered)
		sessionWorld.entityManager.addEntity(session.entity, session.entity->id);
	session.entityRegistered = true;

	// Give our player session a pointer to the entity tracker
	session.entityTracker = session.dimension == 0 ? &server.overworldEntityTracker : &server.hellEntityTracker;

	// Welcome message
	Packet::ChatMessage welcomeMsg;
	welcomeMsg.message = std::string("§eThis Server runs on ") + std::string(PROJECT_FULL_VERSION_LABEL);
	welcomeMsg.Serialize(session.stream);
}