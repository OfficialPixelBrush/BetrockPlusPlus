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
#elif defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#include <cstdint>
#include <string>
#include <packet_ids.h>
#include <vector>
#include <bit>
#include <type_traits>
#include <cstring>

template<typename T>
T byteswap_any(T value) {
    if constexpr (std::is_enum_v<T>) {
        using U = std::underlying_type_t<T>;
        return static_cast<T>(std::byteswap(static_cast<U>(value)));
    } else if constexpr (std::is_integral_v<T>) {
        return std::byteswap(value);
    } else {
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
        NetworkStream(uint16_t port = 25565);
        ~NetworkStream();

        bool NewClient();

        template<typename T>
        T Read() {
            T buffer;
            // TODO: Swap endianess
            recv(clientSocket, &buffer, sizeof(T), 0);
            return byteswap_any(buffer);
        }
        
        template<typename T = int>
        void Write(const T& data) {
            if constexpr (std::is_same_v<T, bool>) {
                int8_t boolData = static_cast<int8_t>(data);
                send(clientSocket, &boolData, sizeof(int8_t), 0);
            } else {
                T networkData = byteswap_any(data);
                send(clientSocket, &networkData, sizeof(T), 0);
            }
        }

        // String-16 Read-Write
        template<>
        std::string Read<std::string>();
        void Write(const std::string& str);
    private:
        // Static so the server-socket is shared across all
        // instances of NetworkStream (if multiple are created)
        // TODO: Automatically close the server socket on program exit
        static int serverSocket;
        int clientSocket = INVALID_SOCKET;
};