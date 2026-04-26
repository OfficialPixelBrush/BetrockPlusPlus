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

template<typename T>
T byteswap_any(T value) {
    if constexpr (std::is_enum_v<T>) {
        using U = std::underlying_type_t<T>;
        return static_cast<T>(std::byteswap(static_cast<U>(value)));
    }
    else if constexpr (std::is_integral_v<T>) {
        return std::byteswap(value);
    }
    else {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8, "byteswap_any: unsupported type size");
        using U = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
        U tmp;
        std::memcpy(&tmp, &value, sizeof(U));
        tmp = std::byteswap(tmp);
        std::memcpy(&value, &tmp, sizeof(U));
        return value;
    }
}

class NetworkStream {
public:
    NetworkStream(int client_socket);
    ~NetworkStream();

    bool NewClient();

    template<typename T>
    T Read() {
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
    template<>
    std::string Read<std::string>();
    void Write(const std::string& str);
    // String-16 Read-Write
    template<>
    std::wstring Read<std::wstring>();
    void Write(const std::wstring& str);
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
private:
    // Static so the server-socket is shared across all
    // instances of NetworkStream (if multiple are created)
    // TODO: Automatically close the server socket on program exit
    int client_socket = INVALID_SOCKET;
    bool connected = true;
    // Outgoing data is accumulated here and sent in one syscall per tick,
    // so the main thread is never blocked waiting for the kernel send buffer.
    std::vector<uint8_t> writeBuffer;
};