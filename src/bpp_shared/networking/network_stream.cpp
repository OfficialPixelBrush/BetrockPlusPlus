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
void NetworkStream::Write(const T& data)
{
    // TODO: Swap endianess
    send(clientSocket, &data, sizeof(T), 0);
}

// Automatically converts to String-16/UCS-2 when sending
void NetworkStream::Write(const std::string& str)
{
    size_t length = str.size();
    int16_t lengthValue = static_cast<int16_t>(length);
    send(clientSocket, &lengthValue, sizeof(lengthValue), 0);
    send(clientSocket, str.data(), length * sizeof(wchar_t), 0);
}

void NetworkStream::Write(const Packet& packet)
{
    Write(static_cast<uint8_t>(packet.id));
    switch (packet.id) {
        case PacketId::PreLogin: {
            const PacketPreLogin& preLoginPacket = static_cast<const PacketPreLogin&>(packet);
            Write(preLoginPacket.username);
            break;
        }
        // Handle other packet types as needed
        default:
            break;
    }
}