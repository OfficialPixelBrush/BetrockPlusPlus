/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "networking/packets.h"
#include <atomic>
extern std::atomic<bool> shutdownRequested;

#include "client.h"
#include <SDL3/SDL_events.h>

// This window size seems really random but its the size beta uses
Client::Client() : window({ 854, 480 }, "Betrock++", { WindowMode::WINDOWED_RESIZABLE }), renderer(window), stream(-1) {
	window.SetCursorCapture(true);
	GlobalLogger().info << "Client initialized\n";
}

void Client::Tick() {
	// Process packets sent by the server
	this->ProcessIncoming();

	// Flush outgoing packets to the server
	this->stream->FlushWriteBuffer();
}

void Client::ProcessIncoming() {
	stream->DrainToBuffer();
	while (stream->HasData()) {
		size_t packetMark = stream->Mark();
		// GlobalLogger().info << "Processing incoming packet\n";
		PacketId packetId = stream->Read<PacketId>();
		bool sendLogin = false;
		bool unknownPacket = false;

		switch (packetId) {
		case PacketId::PreLogin: {
			Packet::PreLogin resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent prelogin response: " << resp.username << "\n";
			sendLogin = true;
			break;
		}

		case PacketId::Login: {
			Packet::Login resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent login response, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::SetSpawnPosition: {
			Packet::SetSpawnPosition resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent spawn position: " << resp.position << "\n";
			break;
		}

		case PacketId::SetHealth: {
			Packet::SetHealth resp;
			resp.Deserialize(*this->stream);
			GlobalLogger().info << "Server sent health: " << resp.health << "\n";

			if (resp.health <= 0) {
				GlobalLogger().info << "Player is dead, respawning...\n";
				Packet::Respawn respawnPkt;
				respawnPkt.dimension = Dimension::Overworld;
				respawnPkt.Serialize(*this->stream);
			}

			break;
		}

		case PacketId::Respawn: {
			Packet::Respawn resp;
			resp.Deserialize(*this->stream);
			GlobalLogger().info << "Server confirmed respawn in dimension " << static_cast<int>(resp.dimension) << "\n";
			break;
		}

		case PacketId::SetTime: {
			Packet::SetTime resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent time: " << resp.time << "\n";
			break;
		}

		case PacketId::SpawnMob: {
			Packet::SpawnMob resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent mob spawn, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::EntityPosition: {
			Packet::EntityPosition resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent entity position, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::EntityRotation: {
			Packet::EntityRotation resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent entity rotation, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::EntityPositionAndRotation: {
			Packet::EntityPositionAndRotation resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent entity position and rotation, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::FillContainer: {
			Packet::FillContainer resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent fill container: " << resp.windowId << "\n";
			break;
		}

		case PacketId::SetChunkVisibility: {
			Packet::SetChunkVisibility resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent set chunk visibility: " << resp.pos << "\n";
			break;
		}

		case PacketId::Chunk: {
			Packet::ChunkData resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent chunk data: " << resp.pos << "\n";
			break;
		}

		case PacketId::SetBlock: {
			Packet::SetBlock resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent block update: " << resp.position << "\n";
			break;
		}

		case PacketId::SetMultipleBlocks: {
			Packet::SetMultipleBlocks resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent multiple block updates: " << resp.numberOfBlocks << "\n";
			break;
		}

		case PacketId::PlayerPosition: {
			Packet::PlayerPosition resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent player position: " << resp.position << "\n";
			break;
		}

		case PacketId::PlayerRotation: {
			Packet::PlayerRotation resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent player rotation: " << resp.yaw << ", " << resp.pitch << "\n";
			break;
		}

		case PacketId::PlayerPositionAndRotation: {
			Packet::PlayerPositionAndRotation resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent player position and rotation: " << resp.position << ", " << resp.yaw
			//                    << ", " << resp.pitch << "\n";
			break;
		}

		case PacketId::GameEvent: {
			Packet::GameEvent resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent game event: " << static_cast<int>(resp.eventId) << "\n";
			break;
		}

		case PacketId::SpawnItem: {
			Packet::SpawnItem resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent item spawn, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::SpawnPlayer: {
			Packet::SpawnPlayer resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent player spawn, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::SetEquipment: {
			Packet::SetEquipment resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent set equipment, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::SetSlot: {
			Packet::SetSlot resp;
			resp.Deserialize(*this->stream);
			break;
		}

		case PacketId::AddPassenger: {
			Packet::AddPassenger resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent add passenger, EID: " << resp.vehicleEntityId << "\n";
			break;
		}

		case PacketId::EntityVelocity: {
			Packet::EntityVelocity resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent entity velocity, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::DespawnEntity: {
			Packet::DespawnEntity resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent despawn entity, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::TeleportEntity: {
			Packet::TeleportEntity resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent teleport entity, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::EntityEvent: {
			Packet::EntityEvent resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent entity event, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::EntityMetadata: {
			Packet::EntityMetadata resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent entity metadata, EID: " << resp.entityId << "\n";
			break;
		}

		case PacketId::ChatMessage: {
			Packet::ChatMessage resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent chat message: " << resp.message << "\n";
			break;
		}

		case PacketId::Animation: {
			Packet::Animation resp;
			resp.Deserialize(*this->stream);
			//GlobalLogger().info << "Server sent animation, EID: " << resp.entityId << "\n";
			break;
		}

		default:
			GlobalLogger().error << "Received unknown packet ID: 0x" << std::hex << static_cast<int>(packetId) << "\n";
			stream->Rollback(packetMark);
			unknownPacket = true;
			break;
		}

		if (stream->CheckAndClearShortRead()) {
			stream->Rollback(packetMark);
			break;
		}
		if (unknownPacket)
			break;
		if (sendLogin) {
			Packet::Login login{};
			login.protocolVersion = 14;
			login.username = "BetrockClient";
			login.Serialize(*this->stream);
			GlobalLogger().info << "Sent login packet\n";
		}
	}
	// Update our last packet time for the timeout code
	this->lastPacketTime = std::chrono::steady_clock::now();
}

bool Client::LoadIntoWorld(std::string _levelPath) {
	gameRuntime.CloseCurrentWorld();
	gameRuntime.Init(_levelPath);
	return true;
}

int Client::Run() {
	const uint64_t freq = SDL_GetPerformanceFrequency();
	uint64_t lastTime = SDL_GetPerformanceCounter();

	// Try to connect to the server
	clientSocket = ClientSocketManager::Connect(this->targetIP, this->targetPort);
	if (clientSocket < 0) {
		GlobalLogger().error << "Failed to connect\n";
		return -1;
	}

	// Setup our stream so we can talk to the server
	this->stream.emplace(this->clientSocket);
	GlobalLogger().info << "Connected to " << this->targetIP << ":" << this->targetPort << "!\n";

	// Send our prelogin
	Packet::PreLogin pkt;
	pkt.username = "BetrockClient";
	pkt.Serialize(*this->stream);

	GlobalLogger().info << "Sent prelogin packet\n";

	while (!shutdownRequested.load()) {
		uint64_t ticksRan = 0;
		uint64_t now = SDL_GetPerformanceCounter();
		float delta = static_cast<float>(now - lastTime) / static_cast<float>(freq);
		lastTime = now;
		accumulator += delta;

		input.NewFrame();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT)
				shutdownRequested.store(true);

			input.HandleEvent(event);
		}

		// Run ticks until caught up
		while (accumulator >= TICK_DELTA && ticksRan < MAX_TICKS_PER_FRAME) {
			Tick();
			accumulator -= TICK_DELTA;
			ticksRan++;
		}

		// Discard leftover time if we hit the cap
		if (ticksRan >= MAX_TICKS_PER_FRAME)
			accumulator = 0.0f;

		glm::vec2 cameraInput{};
		cameraInput.y = input.IsKeyDown(SDL_SCANCODE_W) - input.IsKeyDown(SDL_SCANCODE_S);
		cameraInput.x = input.IsKeyDown(SDL_SCANCODE_D) - input.IsKeyDown(SDL_SCANCODE_A);
		camera.Move(cameraInput, delta);
		constexpr float SENSITIVITY = 0.6f;
		camera.UpdateLook(input.GetMouseDelta() * SENSITIVITY);
		renderer.Render(camera, accumulator / TICK_DELTA);
	}
	return 0;
}