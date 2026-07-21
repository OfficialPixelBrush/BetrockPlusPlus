/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include <atomic>
extern std::atomic<bool> shutdownRequested;

#include <SDL3/SDL_events.h>
#include "client.h"

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
	while (stream->HasData()) {
		PacketId packetId = stream->Read<PacketId>();

		if (false) {
			return;
		}

		if (stream->CheckAndClearShortRead()) {
			break;
		}
	}
	// Update our last packet time for the timeout code
	this->lastPacketTime = std::chrono::steady_clock::now();
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
		if (ticksRan == MAX_TICKS_PER_FRAME)
			accumulator = 0.0f;

		renderer.Render(accumulator / TICK_DELTA);
	}
	return 0;
}