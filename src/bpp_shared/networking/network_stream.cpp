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
    close(clientSocket);
}

template<typename T>
T NetworkStream::Read() {
    T buffer;
    // TODO: Swap endianess
    recv(clientSocket, &buffer, sizeof(T), 0);
    return buffer;
}

template<typename T>
T NetworkStream::SwapEndianess(const T& data) {
    T result;
    uint8_t* dataPtr = reinterpret_cast<uint8_t*>(const_cast<T*>(&data));
    uint8_t* resultPtr = reinterpret_cast<uint8_t*>(&result);
    for (size_t i = 0; i < sizeof(T); i++) {
        resultPtr[i] = dataPtr[sizeof(T) - 1 - i];
    }
    return result;
}

template<typename T>
void NetworkStream::Write(const T& data)
{
    // TODO: Swap endianess
    T swappedData = SwapEndianess(data);
    send(clientSocket, &swappedData, sizeof(T), 0);
}

// Automatically converts to String-16/UCS-2 when sending
void NetworkStream::Write(const std::string& str)
{
    uint16_t length = static_cast<uint16_t>(str.size());
    Write(length);
    std::vector<uint8_t> data;
    data.reserve(str.size() * 2);
    for (unsigned char c : str) {
        data.push_back(0x00);
        data.push_back(c);
    }
    send(clientSocket, data.data(), data.size(), 0);
}

void NetworkStream::Write(const PacketPreLogin& packet)
{
    Write(packet.id);
    Write(packet.username);
}

void NetworkStream::Write(const PacketLogin& packet)
{
    Write(packet.id);
    Write(packet.entityId_protocolVersion);
    Write(packet.username);
    Write(packet.worldSeed);
    Write(packet.dimension);
}

void NetworkStream::Write(const PacketPlayerPositionAndRotation& packet)
{
    Write(packet.id);
    Write(packet.x);
    Write(packet.y);
    Write(packet.camera_y);
    Write(packet.z);
    Write(packet.yaw);
    Write(packet.pitch);
    Write(packet.onGround);
}