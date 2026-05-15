/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "server_manager.h"
#include "networking/network_stream.h"
#include <cerrno>
#include <iostream>

ServerManager::ServerManager(uint16_t port) {
    // Only bind new socket if it doesn't exist already
    if (server_socket != INVALID_SOCKET)
        return;

    #if defined(_WIN32) || defined(_WIN64)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = __builtin_bswap16(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) != 0) {
        std::cerr << "Bind error: " <<  errno << std::endl;
        return;
    }
    listen(server_socket, 1);
}

ServerManager::~ServerManager() {
    // Clear all streams, which should close them
    streams.clear();
    // Close server
    if (server_socket != INVALID_SOCKET) {
        #if defined(_WIN32) || defined(_WIN64)
            shutdown(server_socket, SD_BOTH);
            closesocket(server_socket);
            // Clean-up WSA when the server closes
            WSACleanup();
        #else
            shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
        #endif
        server_socket = INVALID_SOCKET;
    }
}

// Init network stream
bool ServerManager::InitConnection() {
    int client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket != INVALID_SOCKET) {
        streams.push_back(NetworkStream(client_socket));
        return true;
    }
    return false;
}