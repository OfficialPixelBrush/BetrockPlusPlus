/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "discord.h"

#ifdef DISCORD_INTEGRATION

void Discord::Init(const std::string& _token, const std::string& _channelId) {
    token = _token;
    channelId = _channelId;

    if (token.empty() || channelId.empty()) {
        std::cerr << "Discord integration is enabled but discord-token or discord-channel-id "
                      "is missing/empty in server.properties; Discord messages will not be sent.\n";
        initialized = false;
        return;
    }

    initialized = true;
}

bool Discord::SendMessage(const std::string& _message) {
    if (!initialized)
        return false;

    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    std::string url = "https://discord.com/api/v10/channels/" + channelId + "/messages";

    std::string deformatted = RemoveMinecraftFormatting(_message);
    std::string json = "{\"content\":\"" + EscapeJson(deformatted) + "\"}";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers,
        ("Authorization: Bot " + token).c_str()
    );
    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK)
        std::cerr << "Discord: failed to send message: " << curl_easy_strerror(result) << "\n";

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result == CURLE_OK;
}

bool Discord::SendFile(const std::string& _filename, const std::string& _message) {
    if (!initialized)
        return false;

    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    std::string url = "https://discord.com/api/v10/channels/" + channelId + "/messages";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers,
        ("Authorization: Bot " + token).c_str()
    );

    curl_mime* mime = curl_mime_init(curl);

    // Message content
    curl_mimepart* content = curl_mime_addpart(mime);
    curl_mime_name(content, "payload_json");

    std::string json =
        "{\"content\":\"" + EscapeJson(_message) + "\"}";

    curl_mime_data(content, json.c_str(), CURL_ZERO_TERMINATED);

    // File
    curl_mimepart* file = curl_mime_addpart(mime);
    curl_mime_name(file, "files[0]");
    curl_mime_filedata(file, _filename.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK)
        std::cerr << "Discord: failed to send file '" << _filename << "': " << curl_easy_strerror(result) << "\n";

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result == CURLE_OK;
}

std::string Discord::RemoveMinecraftFormatting(const std::string& message) {
    std::string result;
    result.reserve(message.size()); for (size_t i = 0; i < message.size(); ++i) {
        // UTF-8 encoding of § is C2 A7
        if (static_cast<unsigned char>(message[i]) == 0xC2 && i + 1 < message.size() && static_cast<unsigned char>(message[i + 1]) == 0xA7) {
            i += 1;
            // Skip §
            // Skip the Minecraft formatting code
            if (i + 1 < message.size())
                i += 1;
            continue;
        } result += message[i];
    } return result;
}

std::string Discord::EscapeJson(const std::string& _input) {
    std::string result;

    for (char c : _input) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;
        }
    }

    return result;
}

Discord& GlobalDiscord() {
	static Discord discord;
	return discord;
}

#endif
