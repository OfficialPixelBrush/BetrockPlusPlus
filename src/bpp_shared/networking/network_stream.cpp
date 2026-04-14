/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "network_stream.h"

int NetworkStream::serverSocket = INVALID_SOCKET;

NetworkStream::NetworkStream(uint16_t port) {
    // Only bind new socket if it doesn't exist already
    if (serverSocket != INVALID_SOCKET)
        return;

    #if defined(_WIN32) || defined(_WIN64)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress));
    listen(serverSocket, 1);
}

bool NetworkStream::NewClient() {
    clientSocket = accept(serverSocket, nullptr, nullptr);
    return clientSocket != INVALID_SOCKET;
}

NetworkStream::~NetworkStream() {
    #if defined(_WIN32) || defined(_WIN64)
        closesocket(clientSocket);
        // TODO: Clean-up WSA when the server closes
        // WSACleanup();
    #else
        close(clientSocket);
    #endif
}

// Automatically converts to String-16/UCS-2 when sending
void NetworkStream::Write(const std::string& str)
{
    uint16_t length = static_cast<uint16_t>(str.size());
    Write(length);
    std::vector<uint8_t> data;
    data.reserve(str.size() * 2);
    for (const char c : str) {
        data.push_back(0x00);
        data.push_back(static_cast<uint8_t>(c));
    }
    #if defined(_WIN32) || defined(_WIN64)
        send(clientSocket, reinterpret_cast<const char*>(data.data()), data.size(), 0);
    #else
        send(clientSocket, data.data(), data.size(), 0);
    #endif
}

#include <iostream>

template<>
std::string NetworkStream::Read<std::string>() {
    uint16_t length = Read<uint16_t>();
    std::cout << int(length) << std::endl;
    std::string result;
    result.resize(length);
    for (uint16_t i = 0; i < length; i++) {
        Read<uint8_t>(); // skip high byte (UCS-2)
        result[i] = static_cast<char>(Read<uint8_t>());
    }
    return result;
}