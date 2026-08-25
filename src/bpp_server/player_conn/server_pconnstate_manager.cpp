/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "../packet/packet_utils.h"
#include "../server.h"
#include "username.h"
#include "version.h"

#ifdef DISCORD_INTEGRATION
#include "discord.h"
#endif

void PlayerConnStateManager::HandleConnectionState(PlayerSession& _session, Server& _server) {
	switch (_session.connState) {
	case ConnectionState::Handshaking:
		HandleHandshake(_session, _server);
		break;
	case ConnectionState::LoggingIn:
		HandleLogin(_session, _server);
		break;
	case ConnectionState::VerifyingUsername:
		HandleVerifyingUsername(_session, _server);
		break;
	case ConnectionState::WaitingForSpawnChunks:
		WaitForSpawnChunks(_session, _server);
		break;
	case ConnectionState::Playing: {
		WorldManager& sessionWorld = _session.dimension == Dimension::Nether ? _server.gameRuntime.worldHell
		                                                                     : _server.gameRuntime.world;
		_server.chunkSender.Enqueue(_session, sessionWorld, 16);
		_server.chunkSender.Flush(_session);
		if (sessionWorld.elapsedTicks % 20 == 0) {
			// Update the server time so client's don't desync
			Packet::SetTime time;
			time.time = sessionWorld.elapsedTicks;
			time.Serialize(_session.stream);
		}
		break;
	}
	}
}

void PlayerConnStateManager::HandleHandshake(PlayerSession& _session, [[maybe_unused]] Server& _server) {
	if (!_session.stream.HasData())
		return;
	PacketId packetId = _session.stream.Read<PacketId>();

	if (_session.stream.CheckAndClearShortRead())
		return;
	if (packetId != PacketId::PreLogin)
		return;

	Packet::PreLogin incoming;
	incoming.Deserialize(_session.stream);
	if (_session.stream.CheckAndClearShortRead()) {
		return;
	}

	// NOTE: Ornite hyjacks the PreLogin packet to report which mods it has,
	// so we shouldn't use the PreLogin username to identification.
	// This works on Vanilla Beta 1.7.3, so we should replicate this.
	if (incoming.username.starts_with("^@"))
		GlobalLogger().info << "Modified client! Provided mods: " << incoming.username << '\n';

	// Authentication
	_session.serverId = "-";
#ifdef ONLINE_MODE_AUTHENTICATION
	if (_server.auth.onlineMode)
		_session.serverId = _server.auth.GenerateAuthHash();
#endif

	Packet::PreLogin response;
	response.serverId = _session.serverId;
	response.Serialize(_session.stream);

	_session.connState = ConnectionState::LoggingIn;
}

void PlayerConnStateManager::HandleLogin(PlayerSession& _session, Server& _server) {
	if (!_session.stream.HasData())
		return;

	PacketId packetId = _session.stream.Read<PacketId>();
	if (_session.stream.CheckAndClearShortRead())
		return;
	if (packetId != PacketId::Login)
		return;

	Packet::Login incoming;
	incoming.Deserialize(_session.stream);
	if (_session.stream.CheckAndClearShortRead()) {
		return;
	}
	// Check if the username contains any invalid characters
	if (!IsValidUsername(incoming.username)) {
		DisconnectPlayer(_session, "Invalid username!", _server);
		return;
	}
	_session.username = incoming.username;

	// User isn't whitelisted, reject
	if (_server.useWhitelist && std::find(_server.whitelistedUsernames.begin(), _server.whitelistedUsernames.end(),
	                                      incoming.username) == _server.whitelistedUsernames.end()) {
		DisconnectPlayer(_session, "You're not whitelisted!", _server);
		return;
	}

	GlobalLogger().info << "Player " << _session.username << " is logging in.\n";

#ifdef ONLINE_MODE_AUTHENTICATION
	if (_server.auth.onlineMode) {
		_session.pendingAuthFuture = _server.auth.IsRegisteredUsernameAsync(_session.serverId, _session.username);
		_session.authStartTime = std::chrono::steady_clock::now();
		_session.connState = ConnectionState::VerifyingUsername;
		return;
	}
#endif

	FinishLogin(_session, _server);
}

void PlayerConnStateManager::HandleVerifyingUsername(PlayerSession& _session, [[maybe_unused]] Server& _server) {
#ifdef ONLINE_MODE_AUTHENTICATION
	using namespace std::chrono_literals;
	constexpr auto K_AUTH_TIMEOUT = 20s;

	if (!_session.pendingAuthFuture.valid()) {
		DisconnectPlayer(_session, "Failed to verify username!", _server);
		return;
	}

	if (_session.pendingAuthFuture.wait_for(0s) != std::future_status::ready) {
		if (std::chrono::steady_clock::now() - _session.authStartTime > K_AUTH_TIMEOUT)
			DisconnectPlayer(_session, "Login verification timed out!", _server);
		return; // Not ready yet - check again next tick.
	}

	const bool verified = _session.pendingAuthFuture.get();
	if (!verified) {
		DisconnectPlayer(_session, "Failed to verify username!", _server);
		return;
	}
#endif
	FinishLogin(_session, _server);
}

void PlayerConnStateManager::FinishLogin(PlayerSession& _session, Server& _server) {
	// Initialize our entity first as the player session depends on it
	if (!_session.entity)
		_session.entity = std::make_shared<EntityMPPlayer>();

	// Load player data before building the Login response so we know which dimension they're in
	auto playerNbt = _server.gameRuntime.saveManager.GetPlayerNbt(
	    std::string(_session.username.begin(), _session.username.end()));
	_session.LoadPlayerNbt(playerNbt);

	// Get the right world pointer
	WorldManager& sessionWorld = _session.dimension == Dimension::Nether ? _server.gameRuntime.worldHell
	                                                                     : _server.gameRuntime.world;

	_session.entity->session = &_session;
	_session.entity->id = sessionWorld.entityManager.GetNextEntityId();
	_session.entity->dim = _session.dimension == Dimension::Nether ? Dimension::Nether : Dimension::Overworld;

	Packet::Login response;
	response.entityId = _session.entity->id;
	response.username = _session.username;
	response.worldSeed = _server.gameRuntime.world.seed;
	response.dimension = Dimension(_session.dimension);
	response.Serialize(_session.stream);

	Packet::SetSpawnPosition spawn;
	spawn.position = sessionWorld.spawnPoint;
	spawn.Serialize(_session.stream);

	Packet::SetHealth health;
	health.health = _session.entity->health;
	health.Serialize(_session.stream);

	Packet::SetTime time;
	time.time = sessionWorld.elapsedTicks;
	time.Serialize(_session.stream);

	// Get a fresh respawn point
	auto respawnPoint = sessionWorld.GetSpawnPoint(true);

	// If our session position is the default then overwrite it
	if (_session.position.pos == Vec3{ -1, -1000000, -1 }) {
		_session.position.pos = { float(respawnPoint.x) + 0.5, float(respawnPoint.y), float(respawnPoint.z) + 0.5 };
	}

	// Small nudge
	_session.position.pos.y += 0.01;

	// Log that we logged in!
	GlobalLogger().info << "Player " << _session.username << " logged in with entity ID " << _session.entity->id
	                    << " at (" << _session.position.pos.x << ", " << _session.position.pos.y << ", "
	                    << _session.position.pos.z << ")\n";

	// Update our last trusted position and rotation
	_session.entity->position = _session.position.pos;
	_session.entity->rotationYaw = _session.rotation.x;
	_session.entity->rotationPitch = _session.rotation.y;
	_session.pendingTeleport = _session.position.pos;
	_session.entity->RebuildCollider();

	// Let everyone else know we logged in
	_server.SendGlobalChatMessage("§e" + _session.username + " joined the game.", false);
#ifdef DISCORD_INTEGRATION
	GlobalDiscord().SendPlayerJoinMessage(_session.username);
#endif

	// Send our inventory
	PacketUtilities::SendInventory(_session, 0, _session.inventory);

	// Snapshot current contents so the Tick loop's diffing (TickDiff) has a real baseline
	// to compare against, instead of starting from an empty snapshot for the whole session.
	_session.inventoryInteraction.InitSnapshot();

	_session.connState = ConnectionState::WaitingForSpawnChunks;
}

void PlayerConnStateManager::DisconnectPlayer(PlayerSession& _session, const std::string& _reason,
                                              [[maybe_unused]] Server& _server) {
	// Send disconnect reason to the leaving player
	Packet::Disconnect kick;
	kick.reason = _reason;
	kick.Serialize(_session.stream);
	_session.stream.SetConnected(false);
	_server.SavePlayer(_session.username);
	GlobalLogger().info << "Player " << (_session.username.empty() ? "(username not yet set)" : _session.username)
	                    << " disconnected: " << _reason << "\n";
}

void PlayerConnStateManager::WaitForSpawnChunks(PlayerSession& _session, Server& _server) {
	WorldManager& sessionWorld = _session.dimension == Dimension::Nether ? _server.gameRuntime.worldHell
	                                                                     : _server.gameRuntime.world;
	_server.chunkSender.Enqueue(_session, sessionWorld, _server.flushChunkCount);
	_server.chunkSender.Flush(_session);

	// Force a tiny view distance for players trying to spawn in
	_session.position.viewDistanceOverride = 3;

	// Spawn chunk radius; 3 chunks in each direction
	int spawnChunkX = int(std::floor(_session.position.pos.x)) >> 4;
	int spawnChunkZ = int(std::floor(_session.position.pos.z)) >> 4;

	int radius = CrossPlatform::Math::Min(3, sessionWorld.GetViewRadius());

	int totalSpawnChunks = ((radius * 2) + 1) * ((radius * 2) + 1);
	int loadedChunks = 0;

	for (int dx = -radius; dx <= radius; dx++) {
		for (int dz = -radius; dz <= radius; dz++) {
			Int32_2 p{ spawnChunkX + dx, spawnChunkZ + dz };
			if (_session.flushedChunks.contains(p))
				loadedChunks++;
		}
	}

	int percent = std::ceil((float(loadedChunks) / float(totalSpawnChunks)) * 100.0f);

	if (loadedChunks % (totalSpawnChunks / 4) == 0)
		GlobalLogger().info << "Spawn chunks: " << percent << "%\n";

	if (loadedChunks < totalSpawnChunks)
		return;

	GlobalLogger().info << "Spawn chunks sent.\n";

	Packet::PlayerPositionAndRotation pos;
	pos.position = { _session.position.pos.x, _session.position.pos.y + PLAYER_EYE_HEIGHT, _session.position.pos.z };
	pos.cameraY = _session.position.pos.y;
	pos.rotation = _session.rotation;
	pos.onGround = false;
	pos.Serialize(_session.stream);

	// Update our last trusted position
	_session.entity->position = _session.position.pos;
	_session.pendingTeleport = _session.position.pos;
	_session.entity->RebuildCollider();

	// Set view distance to server default
	_session.position.viewDistanceOverride = 0;

	GlobalLogger().info << "Client connected\n";
	_session.connState = ConnectionState::Playing;

	// Register our entity with the world
	if (!_session.entityRegistered)
		sessionWorld.entityManager.AddEntity(_session.entity, _session.entity->id);
	_session.entityRegistered = true;

	// Give our player session a pointer to the entity tracker
	_session.entityTracker = _session.dimension == Dimension::Overworld ? &_server.overworldEntityTracker
	                                                                    : &_server.hellEntityTracker;

	// Welcome message
	Packet::ChatMessage welcomeMsg;
	welcomeMsg.message = std::string("§eThis Server runs on ") + std::string(PROJECT_FULL_VERSION_LABEL);
	welcomeMsg.Serialize(_session.stream);
}