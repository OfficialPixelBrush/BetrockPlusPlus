/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "logger.h"
#if defined(__linux__) || defined(__APPLE__) || defined(__HAIKU__)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <string>

namespace ServerSocketManager {
inline void CloseSocket(int _socket) {
#if defined(_WIN32) || defined(_WIN64)
	closesocket(_socket);
	WSACleanup();
#else
	close(_socket);
#endif
}

inline int CreateServerSocket(int _port) {
	int serverSocket = -1;
#if defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	// This allows the server to quickly restart,
	// if not enabled the OS will hold the port until the last packet time expires.
	int reuse = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

	int nodelay = 1;
	setsockopt(serverSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
		GlobalLogger().error << "**** FAILED TO BIND SOCKET! ****" << "\n";
		return -1;
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

inline int CreateClientSocket(int _socket = -1, std::string* _outIp = nullptr) {
	sockaddr_in clientAddr{};
#if defined(_WIN32) || defined(_WIN64)
	int addrLen = sizeof(clientAddr);
	SOCKET rawSocket = accept(_socket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
	if (rawSocket == INVALID_SOCKET)
		return -1;
	u_long clientMode = 1;
	ioctlsocket(rawSocket, FIONBIO, &clientMode);
	DWORD recvTimeout = 45;
	setsockopt(rawSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
	int nodelay = 1;
	setsockopt(rawSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));
	int clientSocket = static_cast<int>(rawSocket);
#else
	socklen_t addrLen = sizeof(clientAddr);
	int clientSocket = accept(_socket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
	if (clientSocket < 0)
		return -1;
	fcntl(clientSocket, F_SETFL, O_NONBLOCK);
	struct timeval recvTimeout{ 0, 45000 };
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeout), sizeof(recvTimeout));
	int nodelay = 1;
	setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
#endif

	if (_outIp) {
		char ipBuffer[INET_ADDRSTRLEN] = {};
		if (inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, sizeof(ipBuffer)))
			*_outIp = ipBuffer;
	}

	return clientSocket;
}
} // namespace ServerSocketManager