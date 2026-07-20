/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "server_manager.h"
#include "logger.h"
#include "networking/network_stream.h"
#include <cerrno>

ServerManager::ServerManager(uint16_t _port) {
	// Only bind new socket if it doesn't exist already
	if (serverSocket != INVALID_SOCKET)
		return;

#if defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) != 0) {
		GlobalLogger().error << "Bind error: " << errno << std::endl;
		return;
	}
	listen(serverSocket, 1);
}

ServerManager::~ServerManager() {
	// Clear all streams, which should close them
	streams.clear();
	// Close server
	if (serverSocket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
		shutdown(serverSocket, SD_BOTH);
		closesocket(serverSocket);
		// Clean-up WSA when the server closes
		WSACleanup();
#else
		shutdown(serverSocket, SHUT_RDWR);
		close(serverSocket);
#endif
		serverSocket = INVALID_SOCKET;
	}
}

// Init network stream
bool ServerManager::InitConnection() {
	int clientSocket = accept(serverSocket, nullptr, nullptr);
	if (clientSocket != INVALID_SOCKET) {
		streams.push_back(NetworkStream(clientSocket));
		return true;
	}
	return false;
}