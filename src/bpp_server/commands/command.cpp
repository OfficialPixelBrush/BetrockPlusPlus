/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "command.h"
#include <string>

std::vector<std::wstring> command;
std::wstring failureReason;

// Base Command
Command::Command(std::wstring pLabel, std::wstring pDescription, std::wstring pSyntax, bool pRequiresOp, bool pRequiresCreative) {
	this->label = pLabel;
	this->description = pDescription;
	this->syntax = pSyntax;
	this->requiresOp = pRequiresOp;
	this->requiresCreative = pRequiresCreative;
}

// Check permissions for the command
/*
std::string Command::CheckPermissions(Client *client) {
	auto player = client->GetPlayer();
	bool isOp = Betrock::Server::Instance().IsOperator(player->username);
	if (isOp) {
		return "";
	}
	if (requiresOp && !isOp) {
		return ERROR_OPERATOR;
	}
	if (requiresCreative && !player->creativeMode) {
		return ERROR_CREATIVE;
	}
	return "";
}
*/



/*
// Shows how long the server has been alive in ticks
std::string CommandUptime::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
								   Client *client) {
	DEFINE_PERMSCHECK(client)
	Respond::ChatMessage(pResponse,
						 "§7Uptime is " + std::to_string(Betrock::Server::Instance().GetUpTime()) + " Ticks");
	return "";
}

// Teleports player to coordinates or another player
std::string CommandTeleport::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
									 Client *client) {
	DEFINE_PERMSCHECK(client)
	std::vector<uint8_t> sourceResponse;
	if (pCommand.size() > 1) {
		// client that is to-be teleported
		std::string source = pCommand[1];
		auto sourceClient = Betrock::Server::Instance().FindClientByUsername(source);
		if (!sourceClient) {
			return source + " does not exist! (Source)";
		}

		// Option 1: Target coordinates
		try {
			float pitch = 0.0f;
			float yaw = 0.0f;
			int32_t x = std::stoi(pCommand[2].c_str());
			int32_t y = std::stoi(pCommand[3].c_str());
			int32_t z = std::stoi(pCommand[4].c_str());
			if (pCommand.size() > 6) {
				pitch = std::stof(pCommand[5].c_str());
				yaw = std::stof(pCommand[6].c_str());
			}
			Int3 tpGoal = {x, y, z};
			sourceClient->Teleport(sourceResponse, Int3ToVec3(tpGoal), yaw, pitch);
			sourceClient->AppendResponse(sourceResponse);
			auto sourcePlayer = sourceClient->GetPlayer();
			Respond::ChatMessage(pResponse, "§7Teleported  " + sourcePlayer->username + " to (" + std::to_string(x) +
												", " + std::to_string(y) + ", " + std::to_string(z) + ")");
			return "";
		} catch (const std::exception &e) {
			return ERROR_REASON_PARAMETERS;
		}

		// Option 2: Target client
		try {
			std::string destination = pCommand[2];
			auto destinationClient = Betrock::Server::Instance().FindClientByUsername(destination);
			if (!destinationClient) {
				return destination + " does not exist! (Destination)";
			}
			auto destinationPlayer = destinationClient->GetPlayer();
			auto sourcePlayer = sourceClient->GetPlayer();
			sourceClient->Teleport(sourceResponse, destinationPlayer->position, destinationPlayer->yaw,
								   destinationPlayer->pitch);
			sourceClient->AppendResponse(sourceResponse);
			Respond::ChatMessage(pResponse,
								 "§7Teleported " + sourcePlayer->username + " to " + destinationPlayer->username);
		} catch (const std::exception &e) {
			return e.what();
		}
	}
	return ERROR_REASON_PARAMETERS;
}

// Shows the current Server version
std::string CommandVersion::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
									Client *client) {
	DEFINE_PERMSCHECK(client)
	Respond::ChatMessage(pResponse, "§7Current " + std::string(PROJECT_NAME) + " version is " +
										std::string(PROJECT_VERSION_FULL_STRING));
	return "";
}

// Grant a player operator privlidges
std::string CommandOp::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client)

	if (pCommand.size() > 0) {
		std::string username = client->GetPlayer()->username;
		if (pCommand.size() > 1) {
			// Search for the client by username
			username = pCommand[1];
		}
		Betrock::Server::Instance().AddOperator(username);
		Respond::ChatMessage(pResponse, "§7Opping " + username);
		return "";
	}
	return ERROR_REASON_SYNTAX;
}

// Revoke a players' operator privlidges
std::string CommandDeop::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client)

	if (pCommand.size() > 0) {
		std::string username = client->GetPlayer()->username;
		if (pCommand.size() > 1) {
			// Search for the client by username
			username = pCommand[1];
		}
		Betrock::Server::Instance().RemoveOperator(username);
		Respond::ChatMessage(pResponse, "§7De-opping " + username);
		return "";
	}
	return ERROR_REASON_SYNTAX;
}

// Modify the whitelist
std::string CommandWhitelist::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
									  Client *client) {
	DEFINE_PERMSCHECK(client)
	// TODO:
	//- whitelist off
	//- whitelist on
	if (pCommand.size() > 1) {
		if (pCommand.size() > 2) {
			std::string username = pCommand[2];
			if (pCommand[1] == "add") {
				Betrock::Server::Instance().AddWhitelist(username);
				Respond::ChatMessage(pResponse, "§7Whitelisted " + username);
				return "";
			}
			if (pCommand[1] == "remove") {
				Betrock::Server::Instance().RemoveWhitelist(username);
				Respond::ChatMessage(pResponse, "§7Unwhitelisted " + username);
				return "";
			}
		}
		if (pCommand[1] == "reload") {
			auto &server = Betrock::Server::Instance();
			server.ReadWhitelist();
			Respond::ChatMessage(pResponse, "§7Reloaded Whitelist");
			return "";
		}
		if (pCommand[1] == "list") {
			auto &server = Betrock::Server::Instance();
			Respond::ChatMessage(pResponse, "§7-- Whitelisted Players --");
			std::string msg = "§7";
			auto &whitelist = server.GetWhitelist();
			for (size_t i = 0; i < whitelist.size(); i++) {
				msg += whitelist[i];
				if (i < whitelist.size() - 1) {
					msg += ", ";
				}
				if (msg.size() > 40 || i == whitelist.size() - 1) {
					Respond::ChatMessage(pResponse, msg);
					msg = "§7";
				}
			}
			return "";
		}
	}
	return ERROR_REASON_PARAMETERS;
}

// Give yourself a block or item
std::string CommandGive::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);

	if (pCommand.size() > 1) {
		int8_t amount = -1;
		int8_t metadata = 0;
		if (pCommand.size() > 2) {
			metadata = SafeStringToInt32(pCommand[2].c_str());
		}
		if (pCommand.size() > 3) {
			amount = SafeStringToInt32(pCommand[3].c_str());
		}
		int16_t itemId = SafeStringToInt32(pCommand[1].c_str());
		if ((itemId > BLOCK_AIR && itemId < BLOCK_MAX) || (itemId >= ITEM_SHOVEL_IRON && itemId < ITEM_MAX)) {
			if (!client->Give(Item{itemId, amount, metadata})) {
				return "Unable to give " + IdToLabel(itemId);
			}
			client->UpdateInventory(pResponse);
			Respond::ChatMessage(pResponse, "§7Gave " + IdToLabel(itemId));
			return "";
		} else {
			return std::to_string(itemId) + " is not a valid Item Id!";
		}
	}
	return ERROR_REASON_PARAMETERS;
}

// Kick a player from the server
std::string CommandKick::Execute(std::vector<std::string> pCommand, [[maybe_unused]] std::vector<uint8_t> &pResponse,
								 Client *client) {
	DEFINE_PERMSCHECK(client);

	if (pCommand.size() > 0) {
		std::string username = client->GetPlayer()->username;
		if (pCommand.size() > 1) {
			// Search for the client by username
			username = pCommand[1];
			client = Betrock::Server::Instance().FindClientByUsername(username);
		}
		if (client) {
			client->DisconnectClient("Kicked by " + client->GetPlayer()->username);
			// Respond::ChatMessage(pResponse, "§7Kicked " + kicked->username);
			return "";
		}
		return "Client \"" + username + "\" does not exist!";
	}
	return ERROR_REASON_SYNTAX;
}

// Get or Set Player Health
std::string CommandHealth::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();

	if (pCommand.size() > 1) {
		int32_t health = std::stoi(pCommand[1].c_str());
		player->SetHealth(pResponse, health);
		Respond::ChatMessage(pResponse, "§7Set Health to " + std::to_string(player->health));
		return "";
	}
	if (pCommand.size() == 1) {
		Respond::ChatMessage(pResponse, "§7Health is " + std::to_string(player->health));
		return "";
	}
	return ERROR_REASON_SYNTAX;
}

// List all currently online players
std::string CommandList::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
								 Client *client) {
	DEFINE_PERMSCHECK(client);

	Respond::ChatMessage(pResponse, "§7-- All players --");
	Betrock::Server::Instance().GetConnectedClientMutex();
	auto clients = Betrock::Server::Instance().GetConnectedClients();
	std::string msg = "§7";
	for (size_t i = 0; i < clients.size(); i++) {
		msg += clients[i]->GetPlayer()->username;
		if (i < clients.size() - 1) {
			msg += ", ";
		}
		if (msg.size() > MAX_CHAT_LINE_SIZE || i == clients.size() - 1) {
			Respond::ChatMessage(pResponse, msg);
			msg = "§7";
		}
	}
	return "";
}

// Toggle creative mode
std::string CommandCreative::Execute([[maybe_unused]] std::vector<std::string> pCommand,
									 std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();
	player->creativeMode = !player->creativeMode;
	Respond::ChatMessage(pResponse, "§7Set Creative to " + std::to_string(player->creativeMode));
	return "";
}

// Set the current players' pose
std::string CommandPose::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();

	if (pCommand.size() > 1) {
		if (pCommand[1] == "crouch") {
			player->crouching = !player->crouching;
		} else if (pCommand[1] == "fire") {
			player->onFire = !player->onFire;
		} else if (pCommand[1] == "sit") {
			player->sitting = !player->sitting;
		} else {
			return "Invalid pose";
		}
		std::vector<uint8_t> broadcastResponse;
		int8_t pResponseByte = (player->sitting << 2 | player->crouching << 1 | player->onFire);
		Respond::ChatMessage(pResponse, "§7Set Pose " + std::to_string(int32_t(pResponseByte)));
		Respond::EntityMetadata(broadcastResponse, player->entityId, pResponseByte);
		BroadcastToClients(broadcastResponse);
		return "";
	}
	return ERROR_REASON_PARAMETERS;
}

// Play a specified sound
std::string CommandSound::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();

	// Set the time
	if (pCommand.size() > 1) {
		int32_t sound = std::stoi(pCommand[1]);
		int32_t extraData = 0;
		if (pCommand.size() > 2) {
			extraData = std::stoi(pCommand[2]);
		}
		std::vector<uint8_t> broadcastResponse;
		Respond::Soundeffect(broadcastResponse, sound, Vec3ToInt3(player->position), extraData);
		Respond::ChatMessage(pResponse, "§7Playing Sound " + std::to_string(sound));
		BroadcastToClients(broadcastResponse);
		return "";
	}
	return ERROR_REASON_PARAMETERS;
}

// Kill the specified player
std::string CommandKill::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();

	if (pCommand.size() > 0) {
		std::string username = player->username;
		if (pCommand.size() > 1) {
			// Search for the client by username
			username = pCommand[1];
			player = Betrock::Server::Instance().FindClientByUsername(username)->GetPlayer();
		}
		if (player) {
			player->Kill(pResponse);
			Respond::ChatMessage(pResponse, "§7Killed " + player->username);
			return "";
		}
		return "Client \"" + username + "\" does not exist";
	}
	return ERROR_REASON_SYNTAX;
}

// Configure Gamerules
std::string CommandGamerule::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
									 Client *client) {
	DEFINE_PERMSCHECK(client);

	if (pCommand.size() > 1) {
		if (pCommand[1] == "doDaylightCycle") {
			doDaylightCycle = !doDaylightCycle;
			Respond::ChatMessage(pResponse, "§7Set doDaylightCycle to " + std::to_string(doDaylightCycle));
		} else if (pCommand[1] == "doTileDrops") {
			doTileDrops = !doTileDrops;
			Respond::ChatMessage(pResponse, "§7Set doTileDrops to " + std::to_string(doTileDrops));
		} else if (pCommand[1] == "keepInventory") {
			keepInventory = !keepInventory;
			Respond::ChatMessage(pResponse, "§7Set keepInventory to " + std::to_string(keepInventory));
		} else {
			return "Gamerule does not exist!";
		}
		return "";
	}
	return ERROR_REASON_PARAMETERS;
}

// Forces the server to save all loaded chunks
std::string CommandSave::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
								 Client *client) {
	DEFINE_PERMSCHECK(client);

	Respond::ChatMessage(pResponse, "§7Saving...");
	Betrock::Server::Instance().SaveAll();
	Respond::ChatMessage(pResponse, "§7Saved");
	return "";
}

// Forces the server to stop
std::string CommandStop::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
								 Client *client) {
	DEFINE_PERMSCHECK(client);

	Respond::ChatMessage(pResponse, "§7Stopping server");
	Betrock::Server::Instance().PrepareForShutdown();
	return "";
}

// Forces the server to unload chunks nobody can see
std::string CommandFree::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
								 Client *client) {
	DEFINE_PERMSCHECK(client);

	Respond::ChatMessage(pResponse, "§7Freeing Chunks");
	Betrock::Server::Instance().FreeAll();
	Respond::ChatMessage(pResponse, "§7Freed Chunks");
	return "";
}

// Shows the number of loaded chunks
std::string CommandLoaded::Execute([[maybe_unused]] std::vector<std::string> pCommand,
								   [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();

	return std::to_string(Betrock::Server::Instance().GetWorldManager(player->dimension)->world.GetNumberOfChunks()) +
		   " Chunk(s) are loaded";
}

// Shows the current memory usage in megabytes
std::string CommandUsage::Execute([[maybe_unused]] std::vector<std::string> pCommand,
								  [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);

	return GetDetailedMemoryMetricsMBString();
}

// Summon a player entity
std::string CommandSummon::Execute(std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);

	auto &server = Betrock::Server::Instance();

	if (pCommand.size() > 1) {
		// TODO: Actually implement entities that *exist*
		std::scoped_lock lock(server.GetEntityIdMutex());
		std::string username = pCommand[1];
		std::vector<uint8_t> broadcastResponse;
		Respond::NamedEntitySpawn(broadcastResponse, server.GetLatestEntityId(), username,
								  Vec3ToEntityInt3(client->GetPlayer()->position), 0, 0, 5);
		BroadcastToClients(broadcastResponse);
		Respond::ChatMessage(pResponse, "§7Summoned " + username);
		return "";
	}
	return ERROR_REASON_PARAMETERS;
}

// Check the population status of the current chunk
std::string CommandPopulated::Execute([[maybe_unused]] std::vector<std::string> pCommand,
									  [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto player = client->GetPlayer();

	// TODO: Add support for non-zero worlds
	World *w = Betrock::Server::Instance().GetWorld(0);
	if (!w)
		return "World does not exist!";
	std::shared_ptr<Chunk> c = w->GetChunk(
		Int2{
			int32_t(player->position.x / 16.0),
			int32_t(player->position.z / 16.0)
		}
	);
	if (!c)
		return "Chunk does not exist!";
	if (c->state != ChunkState::Populated)
		return "Chunk is not populated";
	if (c->state == ChunkState::Populated)
		return "Chunk is populated";
	return ERROR_REASON_ERROR;
}

// Teleport to Spawn
std::string CommandSpawn::Execute([[maybe_unused]] std::vector<std::string> pCommand, std::vector<uint8_t> &pResponse,
								  Client *client) {
	DEFINE_PERMSCHECK(client);
	client->Teleport(pResponse, Int3ToVec3(Betrock::Server::Instance().GetSpawnPoint()));
	return "";
}

// Open the desired interface
std::string CommandInterface::Execute(std::vector<std::string> pCommand,
									  [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);

	// TODO: Tracks players open Window IDs
	if (pCommand[1] == "craft") {
		client->OpenWindow(INVENTORY_CRAFTING_TABLE);
	} else if (pCommand[1] == "chest") {
		client->OpenWindow(INVENTORY_CHEST);
	}
	return "";
}

// Test the region infrastructure
std::string CommandRegion::Execute(std::vector<std::string> pCommand, [[maybe_unused]] std::vector<uint8_t> &pResponse,
								   Client *client) {
	DEFINE_PERMSCHECK(client);

	// Read in region data
	if (pCommand[1] == "load") {
		std::unique_ptr<RegionFile> rf = std::make_unique<RegionFile>(std::filesystem::current_path() / "r.0.0.mcr");
		auto root = rf->GetChunkNbt(INT2_ZERO);
		if (root) {
			std::cout << *root;
		}
		return std::to_string(rf->freeSectors.size());
	}
	return ERROR_REASON_PARAMETERS;
}

// Get the world seed
std::string CommandSeed::Execute([[maybe_unused]] std::vector<std::string> pCommand,
								 [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);

	return std::to_string(Betrock::Server::Instance().GetWorld(0)->seed);
}

// Get the latest entity id
std::string CommandEntity::Execute([[maybe_unused]] std::vector<std::string> pCommand,
								   [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	auto &server = Betrock::Server::Instance();
	std::scoped_lock lock(server.GetEntityIdMutex());

	return "Last ID was " + std::to_string(server.GetLatestEntityId());
}

// Get the number of modified chunks
std::string CommandModified::Execute([[maybe_unused]] std::vector<std::string> pCommand,
									 [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	return std::to_string(Betrock::Server::Instance().GetWorld(0)->GetNumberOfModifiedChunks());
}

// Send a custom packet
std::string CommandPacket::Execute([[maybe_unused]] std::vector<std::string> pCommand,
								   [[maybe_unused]] std::vector<uint8_t> &pResponse, Client *client) {
	DEFINE_PERMSCHECK(client);
	if (pCommand.size() < 2)
		return ERROR_REASON_PARAMETERS;
	std::vector<uint8_t> broadcast;
	bool broadcastToAll = false;
	// 0th is "packet"
	size_t part = 1;
	// Send this to all
	if (pCommand[1] == "broadcast") {
		broadcastToAll = true;
		part++;
	}
	for (part = part; part < pCommand.size(); part++) {
		uint32_t x;
		std::stringstream ss;
		ss << std::hex << pCommand[part];
		if (!(ss >> x) || x > 0xFF)
			return "Invalid hex value"; // or handle error

		std::cout << int32_t(part) << ": " << int32_t(x) << " - " << pCommand[part] << "\n";
		if (broadcastToAll)
			broadcast.push_back(static_cast<uint8_t>(x));
		else
			pResponse.push_back(static_cast<uint8_t>(x));
	}
	if (broadcastToAll)
		BroadcastToClients(broadcast);
	return "";
}
*/