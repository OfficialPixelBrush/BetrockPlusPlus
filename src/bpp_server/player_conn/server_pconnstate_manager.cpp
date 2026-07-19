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
	switch (session.m_connState) {
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
		WorldManager& sessionWorld = session.m_dimension == -1 ? server.m_gameRuntime.m_worldHell : server.m_gameRuntime.m_world;
		server.m_chunkSender.enqueue(session, sessionWorld, 16);
		server.m_chunkSender.flush(session);
		if (sessionWorld.m_elapsed_ticks % 20 == 0) {
			// Update the server time so client's don't desync
			Packet::SetTime time;
			time.m_time = sessionWorld.m_elapsed_ticks;
			time.Serialize(session.m_stream);
		}
		break;
	}
	}
}

void PlayerConnStateManager::handleHandshake(PlayerSession& session, [[maybe_unused]] Server& server) {
	if (!session.m_stream.hasData())
		return;
	PacketId packetId = session.m_stream.Read<PacketId>();

	if (session.m_stream.checkAndClearShortRead())
		return;
	if (packetId != PacketId::PreLogin)
		return;

	Packet::PreLogin incoming;
	incoming.Deserialize(session.m_stream);
	if (session.m_stream.checkAndClearShortRead()) {
		return;
	}
	session.m_username = incoming.m_username;

	Packet::PreLogin response;
	response.m_username = "-";
	response.Serialize(session.m_stream);

	GlobalLogger().m_info << "Player " << session.m_username << " is logging in.\n";

	session.m_connState = ConnectionState::LoggingIn;
}

void PlayerConnStateManager::handleLogin(PlayerSession& session, Server& server) {
	if (!session.m_stream.hasData())
		return;

	PacketId packetId = session.m_stream.Read<PacketId>();
	if (session.m_stream.checkAndClearShortRead())
		return;
	if (packetId != PacketId::Login)
		return;

	Packet::Login incoming;
	incoming.Deserialize(session.m_stream);
	if (session.m_stream.checkAndClearShortRead()) {
		return;
	}

	// Load player data before building the Login response so we know which dimension they're in
	auto playerNbt = server.m_gameRuntime.m_saveManager.getPlayerNBT(
	    std::string(session.m_username.begin(), session.m_username.end()));
	session.loadPlayerNBT(playerNbt);

	// Get the right world pointer
	WorldManager& sessionWorld = session.m_dimension == -1 ? server.m_gameRuntime.m_worldHell : server.m_gameRuntime.m_world;

	// Initialize our entity
	if (!session.m_entity)
		session.m_entity = std::make_shared<EntityMPPlayer>();
	session.m_entity->m_session = &session;
	session.m_entity->m_id = sessionWorld.m_entityManager.getNextEntityId();
	session.m_entity->m_dim = session.m_dimension == -1 ? Dimension::Nether : Dimension::Overworld;

	Packet::Login response;
	response.m_entity_id = session.m_entity->m_id;
	response.m_username = session.m_username;
	response.m_worldSeed = server.m_gameRuntime.m_world.m_seed;
	response.m_dimension = Dimension(session.m_dimension);
	response.Serialize(session.m_stream);

	Packet::SetSpawnPosition spawn;
	spawn.m_position = sessionWorld.m_spawnPoint;
	spawn.Serialize(session.m_stream);

	Packet::SetHealth health;
	health.m_health = 20;
	health.Serialize(session.m_stream);

	Packet::SetTime time;
	time.m_time = sessionWorld.m_elapsed_ticks;
	time.Serialize(session.m_stream);

	// Get a fresh respawn point
	auto respawnPoint = sessionWorld.getSpawnPoint(true);

	// If our session position is the default then overwrite it
	if (session.m_position.m_pos == Vec3{ -1, -1000000, -1 }) {
		session.m_position.m_pos = { float(respawnPoint.m_x) + 0.5, float(respawnPoint.m_y), float(respawnPoint.m_z) + 0.5 };
	}

	// Convert the feet-based respawn height into our posY convention (eye level)
	session.m_position.m_pos.m_y += (PLAYER_EYE_HEIGHT + 0.00001);

	// Log that we logged in!
	GlobalLogger().m_info << "Player " << session.m_username << " logged in with entity ID " << session.m_entity->m_id
	                    << " at (" << session.m_position.m_pos.m_x << ", " << session.m_position.m_pos.m_y << ", "
	                    << session.m_position.m_pos.m_z << ")\n";

	// Update our last trusted position
	session.m_entity->m_position = session.m_position.m_pos;
	session.m_pendingTeleport = session.m_position.m_pos;
	session.m_entity->rebuildCollider();

	// Let everyone else know we logged in
	server.sendGlobalChatMessage("§e" + session.m_username + " joined the game.");

	// Send our inventory
	PacketUtilities::sendInventory(session, 0, session.m_inventory);

	// Snapshot current contents so the tick loop's diffing (tickDiff) has a real baseline
	// to compare against, instead of starting from an empty snapshot for the whole session.
	session.m_inventoryInteraction.initSnapshot();

	session.m_connState = ConnectionState::WaitingForSpawnChunks;
}

void PlayerConnStateManager::disconnectPlayer(PlayerSession& session, const std::string& reason,
                                              [[maybe_unused]] Server& server) {
	// Send disconnect reason to the leaving player
	Packet::Disconnect kick;
	kick.m_reason = reason;
	kick.Serialize(session.m_stream);
	session.m_stream.setConnected(false); // This should force an NBT save
	GlobalLogger().m_info << "Player " << session.m_username << " disconnected: " << reason << "\n";
}

void PlayerConnStateManager::waitForSpawnChunks(PlayerSession& session, Server& server) {
	WorldManager& sessionWorld = session.m_dimension == -1 ? server.m_gameRuntime.m_worldHell : server.m_gameRuntime.m_world;
	server.m_chunkSender.enqueue(session, sessionWorld, server.m_flushChunkCount);
	server.m_chunkSender.flush(session);

	// Force a tiny view distance for players trying to spawn in
	session.m_position.m_viewDistanceOverride = 3;

	// Spawn chunk radius; 3 chunks in each direction
	int spawnChunkX = int(std::floor(session.m_position.m_pos.m_x)) >> 4;
	int spawnChunkZ = int(std::floor(session.m_position.m_pos.m_z)) >> 4;

	int radius = CrossPlatform::Math::min(3, sessionWorld.getViewRadius());

	int total_spawn_chunks = ((radius * 2) + 1) * ((radius * 2) + 1);
	int loaded_chunks = 0;

	for (int dx = -radius; dx <= radius; dx++) {
		for (int dz = -radius; dz <= radius; dz++) {
			Int32_2 p{ spawnChunkX + dx, spawnChunkZ + dz };
			if (session.m_flushedChunks.contains(p))
				loaded_chunks++;
		}
	}

	GlobalLogger().m_info << "Spawn chunks: " << loaded_chunks << " / " << total_spawn_chunks << "\n";

	if (loaded_chunks < total_spawn_chunks)
		return;

	GlobalLogger().m_info << "Spawn chunks sent. Setting player position\n";

	session.m_position.m_pos.m_y += 0.0625;
	Packet::PlayerPositionAndRotation pos;
	pos.m_position = session.m_position.m_pos;
	pos.m_camera_y = session.m_position.m_pos.m_y + PLAYER_EYE_HEIGHT;
	pos.m_rotation = session.m_rotation;
	pos.m_onGround = false;
	pos.Serialize(session.m_stream);

	// Update our last trusted position
	session.m_entity->m_position = session.m_position.m_pos;
	session.m_pendingTeleport = session.m_position.m_pos;
	session.m_entity->rebuildCollider();

	// Set view distance to server default
	session.m_position.m_viewDistanceOverride = 0;

	GlobalLogger().m_info << "Client connected\n";
	session.m_connState = ConnectionState::Playing;

	// Register our entity with the world
	if (!session.m_entityRegistered)
		sessionWorld.m_entityManager.addEntity(session.m_entity, session.m_entity->m_id);
	session.m_entityRegistered = true;

	// Give our player session a pointer to the entity tracker
	session.m_entityTracker = session.m_dimension == 0 ? &server.m_overworldEntityTracker : &server.m_hellEntityTracker;

	// Welcome message
	Packet::ChatMessage welcomeMsg;
	welcomeMsg.m_message = std::string("§eThis Server runs on ") + std::string(PROJECT_FULL_VERSION_LABEL);
	welcomeMsg.Serialize(session.m_stream);
}