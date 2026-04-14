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

class NetworkStream {
    public:
        NetworkStream(uint16_t port = 25565);
        ~NetworkStream();

        bool NewClient();

        template<typename T>
        T Read();
        
        template<typename T>
        void Write(const T& data);
        void Write(const std::string& str);
        void Write(const Packet& packet);
    private:
        // Static so the server-socket is shared across all
        // instances of NetworkStream (if multiple are created)
        // TODO: Automatically close the server socket on program exit
        static int serverSocket;
        int clientSocket = INVALID_SOCKET;
};