/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @brief Forwards Minecraft chat messages to Discord.
 * Also supports uploading crash logs directly to Discord.
 *
 * Requires libcurl.
 */

#ifdef DISCORD_INTEGRATION
#pragma once

#include <curl/curl.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class Discord {
public:
    Discord();
    ~Discord();

    Discord(const Discord&) = delete;
    Discord& operator=(const Discord&) = delete;

    void Init(const std::string& _token, const std::string& _channelId);
    void SendMessage(const std::string& _message);
    void SendFile(const std::string& _filename, const std::string& _message = "");

private:
    void Worker();
    void Enqueue(std::function<void()> _task);

    static std::string RemoveMinecraftFormatting(const std::string& _input);
    static std::string EscapeJson(const std::string& _input);

    std::thread thread;
    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::function<void()>> queue;
    bool stopping = false;

    CURL* curl = nullptr;
    std::string token;
    std::string channelId;
    bool initialized = false;
};

Discord& GlobalDiscord();

#endif