/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "logger.h"
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace ServerSocketManager {
inline void closeSocket(int socket) {
#if defined(_WIN32) || defined(_WIN64)
	closesocket(socket);
	WSACleanup();
#else
	close(socket);
#endif
}

inline int createServerSocket(int port) {
	int serverSocket = -1;
#if defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
		GlobalLogger().warn << "**** FAILED TO BIND SOCKET! ****" << "\n";
	}
	listen(serverSocket, 8);

#if defined(_WIN32) || defined(_WIN64)
	u_long mode = 1;
	ioctlsocket(serverSocket, FIONBIO, &mode);
#else
	fcntl(serverSocket, F_SETFL, O_NONBLOCK);
#endif

	return serverSocket;
}

inline int createClientSocket(int socket = -1) {
#if defined(_WIN32) || defined(_WIN64)
	SOCKET rawSocket = accept(socket, nullptr, nullptr);
	if (rawSocket == INVALID_SOCKET)
		return -1;
	u_long clientMode = 1;
	ioctlsocket(rawSocket, FIONBIO, &clientMode);
	DWORD recvTimeout = 45;
	setsockopt(rawSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
	int clientSocket = static_cast<int>(rawSocket);
#else
	int clientSocket = accept(socket, nullptr, nullptr);
	if (clientSocket < 0)
		return -1;
	fcntl(clientSocket, F_SETFL, O_NONBLOCK);
	struct timeval recvTimeout{ 0, 45000 };
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
#endif

	return clientSocket;
}
} // namespace ServerSocketManager