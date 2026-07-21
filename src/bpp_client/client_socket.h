/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "logger.h"
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <string>

namespace ClientSocketManager {

inline void CloseSocket(int _socket) {
#if defined(_WIN32) || defined(_WIN64)
	closesocket(_socket);
	WSACleanup();
#else
	close(_socket);
#endif
}

// Resolves _host and opens a TCP connection to it on _port.
// Returns INVALID_SOCKET (-1) on failure.
inline int Connect(const std::string& _host, uint16_t _port) {
#if defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* resolved = nullptr;
	if (getaddrinfo(_host.c_str(), std::to_string(_port).c_str(), &hints, &resolved) != 0) {
		GlobalLogger().error << "**** FAILED TO RESOLVE HOST: " << _host << " ****" << "\n";
		return -1;
	}

	int clientSocket = -1;
	for (addrinfo* p = resolved; p != nullptr; p = p->ai_next) {
		clientSocket = static_cast<int>(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
		if (clientSocket < 0)
			continue;

		// Blocking connect (we only need this once, at startup)
		if (connect(clientSocket, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0)
			break; // connected

		CloseSocket(clientSocket);
		clientSocket = -1;
	}
	freeaddrinfo(resolved);

	if (clientSocket < 0) {
		GlobalLogger().error << "**** FAILED TO CONNECT TO " << _host << ":" << _port << " ****" << "\n";
		return -1;
	}

	// Same pattern the server uses for accepted client sockets:
	// nonblocking + a short recv timeout so Tick() never stalls.
#if defined(_WIN32) || defined(_WIN64)
	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);
	DWORD recvTimeout = 45;
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
#else
	fcntl(clientSocket, F_SETFL, O_NONBLOCK);
	struct timeval recvTimeout{ 0, 45000 };
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
#endif

	return clientSocket;
}
}