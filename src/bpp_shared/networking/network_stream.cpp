/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "network_stream.h"
#include <vector>

NetworkStream::NetworkStream(int p_client_socket) {
    client_socket = p_client_socket;
}

NetworkStream::~NetworkStream() {
    if (client_socket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
        shutdown(client_socket, SD_BOTH);
        closesocket(client_socket);
        // TODO: Clean-up WSA when the server closes
        // WSACleanup();
#else
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
#endif
        client_socket = INVALID_SOCKET;
    }
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
    send(client_socket, reinterpret_cast<const char*>(data.data()), data.size(), 0);
#else
    send(client_socket, data.data(), data.size(), 0);
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

void NetworkStream::ReadBytes(uint8_t* buf, size_t len) {
    size_t received = 0;
    while (received < len) {
#if defined(_WIN32) || defined(_WIN64)
        int result = recv(client_socket, reinterpret_cast<char*>(buf + received), static_cast<int>(len - received), 0);
#else
        ssize_t result = recv(client_socket, buf + received, len - received, 0);
#endif
        if (result <= 0) {
            connected = false;
            break;
        }
        received += static_cast<size_t>(result);
    }
}

void NetworkStream::WriteBytes(const uint8_t* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
#if defined(_WIN32) || defined(_WIN64)
        int result = send(client_socket, reinterpret_cast<const char*>(buf + sent), static_cast<int>(len - sent), 0);
#else
        ssize_t result = send(client_socket, buf + sent, len - sent, 0);
#endif
        if (result <= 0) {
            connected = false;
            break;
        }
        sent += static_cast<size_t>(result);
    }
}

bool NetworkStream::hasData() {
#if defined(_WIN32) || defined(_WIN64)
    u_long bytesAvailable = 0;
    if (ioctlsocket(client_socket, FIONREAD, &bytesAvailable) == SOCKET_ERROR) {
        connected = false;
        return false;
    }
    return bytesAvailable > 0;
#else
    int bytesAvailable = 0;
    if (ioctl(client_socket, FIONREAD, &bytesAvailable) < 0) {
        connected = false;
        return false;
    }
    return bytesAvailable > 0;
#endif
}