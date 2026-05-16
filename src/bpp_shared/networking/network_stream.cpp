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

    // Switch to blocking mode
    #if defined(_WIN32) || defined(_WIN64)
        u_long mode = 0;
        ioctlsocket(client_socket, FIONBIO, &mode);
    #else
        int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
    #endif

    size_t sent = 0;
    while (sent < writeBuffer.size()) {
        size_t result = send(client_socket,
            reinterpret_cast<const char*>(writeBuffer.data() + sent),
            (writeBuffer.size() - sent), 0);
        if (result <= 0) break;
        sent += static_cast<size_t>(result);
    }
    writeBuffer.clear();

	// We close here so the client can get the packet data we just sent out before we disconnect
    #if defined(_WIN32) || defined(_WIN64)
        shutdown(client_socket, SD_SEND);
        closesocket(client_socket);
    #else
        shutdown(client_socket, SHUT_WR);
        close(client_socket);
    #endif
        client_socket = INVALID_SOCKET;
}

// String-8 Handling
void NetworkStream::Write(const std::string& str)
{
    uint16_t length = static_cast<uint16_t>(str.size());
    Write(length);
    std::vector<uint8_t> data;
    data.reserve(str.size() * 2);
    for (const char c : str) {
        data.push_back(static_cast<uint8_t>(c));
    }
    WriteBytes(data.data(), data.size());
}

std::string NetworkStream::ReadString() {
    uint16_t len = Read<uint16_t>();   // adjust to uint32_t if your protocol uses 4-byte lengths
    std::string result(len, '\0');
    ReadBytes(reinterpret_cast<uint8_t*>(result.data()), len);
    return result;
}

// String-16 Handling
void NetworkStream::Write(const std::wstring& str)
{
    uint16_t length = static_cast<uint16_t>(str.size());
    Write(length);
    std::vector<uint8_t> data;
    data.reserve(str.size());
    for (const wchar_t c : str) {
        data.push_back(static_cast<uint8_t>((c >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(c & 0xFF));
    }
    WriteBytes(data.data(), data.size());
}

std::wstring NetworkStream::ReadWString() {
    uint16_t len = Read<uint16_t>();
    
    // Read as UTF-16 (2 bytes per char) regardless of platform wchar_t size
    std::vector<uint16_t> buf(len);
    ReadBytes(reinterpret_cast<uint8_t*>(buf.data()), len * sizeof(uint16_t));
    
    std::wstring result(len, L'\0');
    for (uint16_t i = 0; i < len; i++) {
        // byteswap each UTF-16 unit, then widen to wchar_t
        result[i] = static_cast<wchar_t>(__builtin_bswap16(buf[i]));
    }
    return result;
}

void NetworkStream::ReadBytes(uint8_t* buf, size_t len) {
    size_t received = 0;
    // Drain any byte that was put back by unreadByte() first.
    if (hasUnread && received < len) {
        buf[received++] = unreadStash;
        hasUnread = false;
    }
    while (received < len) {
#if defined(_WIN32) || defined(_WIN64)
        int result = recv(client_socket, reinterpret_cast<char*>(buf + received), static_cast<int>(len - received), 0);
        if (result <= 0) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) {
                // SO_RCVTIMEO fired — packet split across ticks, retry next tick.
                shortRead = true;
            } else {
                connected = false;
            }
            break;
        }
#else
        ssize_t result = recv(client_socket, buf + received, len - received, 0);
        if (result <= 0) {
            if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT)) {
                // Timeout — packet split across ticks, retry next tick.
                shortRead = true;
            } else {
                connected = false;
            }
            break;
        }
#endif
        received += static_cast<size_t>(result);
    }
}

void NetworkStream::WriteBytes(const uint8_t* buf, size_t len) {
    // Append to the write buffer -- no syscall here.
    // The actual send() happens once per tick in flushWriteBuffer().
    writeBuffer.insert(writeBuffer.end(), buf, buf + len);
}

// TODO: Due to how this system works, a concrete length is never supplied.
// Data is read until 0x7F is hit. Ideally we should exit out if we're past
// a certain number of bytes
void NetworkStream::ReadEntityMetadata() {
    uint8_t val = Read<uint8_t>();
    while(val != 0x7F) {
        // What type the data has
        PacketData::EntityMetadata::Type type = PacketData::EntityMetadata::Type(val >> 5   );
        // Where the data goes for the relevant entity
        [[maybe_unused]] uint8_t id = uint8_t(val &  0x1F);
        switch(type) {
            case PacketData::EntityMetadata::Type::BYTE: {
                [[maybe_unused]] int8_t num = Read<int8_t>();
                break;
            }
            case PacketData::EntityMetadata::Type::SHORT: {
                [[maybe_unused]] int16_t num = Read<int16_t>();
                break;
            }
            case PacketData::EntityMetadata::Type::INTEGER: {
                [[maybe_unused]] int32_t num = Read<int32_t>();
                break;
            }
            case PacketData::EntityMetadata::Type::FLOAT: {
                [[maybe_unused]] float num = Read<float>();
                break;
            }
            case PacketData::EntityMetadata::Type::STRING: {
                [[maybe_unused]] std::string str = Read<std::string>();
                break;
            }
            case PacketData::EntityMetadata::Type::ITEM: {
                [[maybe_unused]] int16_t itemId = Read<int16_t>();
                // TODO: Check if B1.7.3 actually does
                // this for Entity Metadata too
                if (itemId != -1) {
                    Read<int8_t>();
                    Read<int16_t>();
                }
                break;
            }
            case PacketData::EntityMetadata::Type::COORINDATES: {
                [[maybe_unused]] Int3 coordinate(
                    Read<int32_t>(),
                    Read<int32_t>(),
                    Read<int32_t>()
                );
                break;
            }
            default:
                // TODO: Log that something went horribly wrong!
                break;
        }
        // Read in the next value
        val = Read<uint8_t>();
    }
}

// TODO: Implement this! Ideally we could just pass an entity into here
// and it'd take care of things automatically
void NetworkStream::WriteEntityMetadata() {
    
}

bool NetworkStream::flushWriteBuffer() {
    if (writeBuffer.empty()) return connected;
    size_t sent = 0;
    while (sent < writeBuffer.size()) {
        size_t result = send(
            client_socket,
            reinterpret_cast<const char*>(writeBuffer.data() + sent),
            writeBuffer.size() - sent,
            0
        );
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
        if (result == 0) {
            connected = false;
            break;
        }
        sent += static_cast<size_t>(result);
    }
    if (sent > 0) {
        writeBuffer.erase(
            writeBuffer.begin(),
            writeBuffer.begin() + static_cast<std::vector<unsigned char>::difference_type>(sent)
        );
    }
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