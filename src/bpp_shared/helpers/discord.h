/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

/**
 * @brief This exists as a way to forward messages from the Minecraft chat to Discord
 * It also supports uploading crash logs directly to discord.
 * This requires libcurl
 */

#ifdef DISCORD_INTEGRATION
#pragma once
#include <curl/curl.h>
#include <iostream>
#include <string>

class Discord {
public:
    Discord() = default;
    void Init(const std::string& _token, const std::string& _channelId);

    bool SendMessage(const std::string& _message);
    bool SendFile(const std::string& _filename, const std::string& _message = "");

private:
    static std::string RemoveMinecraftFormatting(const std::string& _input);
    static std::string EscapeJson(const std::string& _input);

    std::string token;
    std::string channelId;
    bool initialized = false;
};

Discord& GlobalDiscord();

#endif