/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#define INVALID_SOCKET -1
#if defined(__linux__)
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <cstdint>
#include <string>
#include <packet_ids.h>
#include <bit>
#include <type_traits>
#include <cstring>
#include <vector>
#include "helpers/byteswap_compat.h"

template<typename T>
inline T byteswap_any(T value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "byteswap_any: only trivially copyable types allowed");
    if constexpr (sizeof(T) == 1) {
        return value;
    }
    else if constexpr (sizeof(T) == 2) {
        uint16_t tmp;
        std::memcpy(&tmp, &value, 2);
        tmp = __builtin_bswap16(tmp);
        std::memcpy(&value, &tmp, 2);
    }
    else if constexpr (sizeof(T) == 4) {
        uint32_t tmp;
        std::memcpy(&tmp, &value, 4);
        tmp = __builtin_bswap32(tmp);
        std::memcpy(&value, &tmp, 4);
    }
    else if constexpr (sizeof(T) == 8) {
        uint64_t tmp;
        std::memcpy(&tmp, &value, 8);
        tmp = __builtin_bswap64(tmp);
        std::memcpy(&value, &tmp, 8);
    }
    else {
        static_assert(sizeof(T) <= 8, "byteswap_any: unsupported type size");
    }
    return value;
}

class NetworkStream {
public:
    NetworkStream(int client_socket);
    ~NetworkStream();
    bool NewClient();

    template<typename T>
    T Read() {
        static_assert(std::is_trivially_copyable_v<T>,
            "NetworkStream::Read<T>: use Read<std::string>() or Read<std::wstring>() for string types");
        T buffer{};
#if defined(_WIN32) || defined(_WIN64)
        int result = recv(client_socket, reinterpret_cast<char*>(&buffer), sizeof(T), 0);
#else
        ssize_t result = recv(client_socket, &buffer, sizeof(T), 0);
#endif
        if (result <= 0) connected = false;
        return byteswap_any(buffer);
    }

    template<typename T = int>
    void Write(const T& data) {
        if constexpr (std::is_same_v<T, bool>) {
            int8_t boolData = static_cast<int8_t>(data);
            WriteBytes(reinterpret_cast<const uint8_t*>(&boolData), sizeof(int8_t));
        }
        else {
            T networkData = byteswap_any(data);
            WriteBytes(reinterpret_cast<const uint8_t*>(&networkData), sizeof(T));
        }
    }

    void setConnected(bool val) { connected = val; }
    bool isConnected() const { return connected; }

    // String-8 Read-Write
    std::string  ReadString();
    void         Write(const std::string& str);

    // String-16 Read-Write
    std::wstring ReadWString();
    void         Write(const std::wstring& str);

    // Raw byte buffer Read-Write (no endian conversion)
    void ReadBytes(uint8_t* buf, size_t len);

    // Append bytes to the per-session write buffer (no syscall).
    void WriteBytes(const uint8_t* buf, size_t len);

    // Handles Entity Metadata Interpreting
    void ReadEntityMetadata();

    // Handles Entity Metadata Conversion
    void WriteEntityMetadata();

    // Flush the write buffer to the socket once per tick.
    // Returns false if the connection was lost.
    bool flushWriteBuffer();
    bool hasData();

    // Append pre-serialised bytes directly to the write buffer.
    // Used for shared-packet broadcast: serialise once, copy to N sessions.
    void writeRaw(const uint8_t* data, size_t len) { WriteBytes(data, len); }

    // Read-only view of the pending write buffer.
    // Valid only until the next Write*/writeRaw/flushWriteBuffer call.
    const std::vector<uint8_t>& getRawWriteBuffer() const { return writeBuffer; }

private:
    int client_socket = INVALID_SOCKET;
    bool connected = true;
    std::vector<uint8_t> writeBuffer;
};

// Out-of-class explicit specializations (GCC/Clang require these outside the class body)
template<>
inline std::string NetworkStream::Read<std::string>() {
    return ReadString();
}

template<>
inline std::wstring NetworkStream::Read<std::wstring>() {
    return ReadWString();
}