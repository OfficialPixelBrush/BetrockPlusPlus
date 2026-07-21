/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "client_pos.h"
#include "input.h"
#include "renderer.h"
#include "window.h"
#include "logger.h"
#include "assets.h"
#include "BS_thread_pool.hpp"
#include "server.h"
#include "client_socket.h"
#include "networking/network_stream.h"
#include "packet/handle_client_packet.h"
#include "packet/client_packet_dispatcher.h"

class Client {
public:
	Client();
	int Run();

private:
	static constexpr float TICK_DELTA = 1.0f / 20.0f;
	static constexpr int MAX_TICKS_PER_FRAME = 10;

	void Tick();
	void Render([[maybe_unused]] float _partialTick);
	void ProcessIncoming();

	Window window;
	Input input;
	Renderer renderer;
	AssetManager assetManager;
	float accumulator = 0.0f;

	// Network
	std::string targetIP = "127.0.0.1";
	int targetPort = 25565;
	int clientSocket = -1;
	std::optional<NetworkStream> stream;
	std::chrono::steady_clock::time_point lastPacketTime = std::chrono::steady_clock::now();

	// Not used currently
	BS::thread_pool<> serverThread{ 1 };
};