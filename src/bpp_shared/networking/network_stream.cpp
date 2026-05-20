/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "network_stream.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include <vector>


// ============================================================
// NetworkStream (write-side only now)
// ============================================================

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

void NetworkStream::flushWriteBufferBlocking() {
    if (writeBuffer.empty() || client_socket == INVALID_SOCKET) return;

#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 0;
    ioctlsocket(client_socket, FIONBIO, &mode);
#else
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
#endif

    size_t sent = 0;
    while (sent < writeBuffer.size()) {
        int result = send(client_socket,
            reinterpret_cast<const char*>(writeBuffer.data() + sent),
            static_cast<int>(writeBuffer.size() - sent), 0);
        if (result <= 0) break;
        sent += static_cast<size_t>(result);
    }
    writeBuffer.clear();

#if defined(_WIN32) || defined(_WIN64)
    shutdown(client_socket, SD_SEND);
    closesocket(client_socket);
#else
    shutdown(client_socket, SHUT_WR);
    close(client_socket);
#endif
    client_socket = INVALID_SOCKET;
}

void NetworkStream::Write(const std::string& str) {
    uint16_t length = static_cast<uint16_t>(str.size());
    Write(length);
    WriteBytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

void NetworkStream::Write(const std::wstring& str) {
    uint16_t length = static_cast<uint16_t>(str.size());
    Write(length);
    std::vector<uint8_t> data;
    data.reserve(str.size() * 2);
    for (const wchar_t c : str) {
        data.push_back(static_cast<uint8_t>((c >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(c & 0xFF));
    }
    WriteBytes(data.data(), data.size());
}

void NetworkStream::WriteBytes(const uint8_t* data, size_t len) {
    writeBuffer.insert(writeBuffer.end(), data, data + len);
}

void NetworkStream::WriteEntityMetadata() {
    // TODO: implement
}

bool NetworkStream::flushWriteBuffer() {
    if (writeBuffer.empty()) return connected;
    size_t sent = 0;
    while (sent < writeBuffer.size()) {
        int result = send(client_socket,
            reinterpret_cast<const char*>(writeBuffer.data() + sent),
            static_cast<int>(writeBuffer.size() - sent), 0);
        if (result < 0) {
#if defined(_WIN32) || defined(_WIN64)
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) break;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
#endif
            connected = false;
            break;
        }
        if (result == 0) { connected = false; break; }
        sent += static_cast<size_t>(result);
    }
    if (sent > 0)
        writeBuffer.erase(writeBuffer.begin(),
            writeBuffer.begin() + static_cast<std::ptrdiff_t>(sent));
    return connected;
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
