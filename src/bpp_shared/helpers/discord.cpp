/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "discord.h"
#include "logger.h"

#ifdef DISCORD_INTEGRATION

Discord::Discord() {
	thread = std::thread(&Discord::Worker, this);
}

Discord::~Discord() {
	{
		std::lock_guard lock(mutex);
		stopping = true;
	}

	condition.notify_one();

	if (thread.joinable())
		thread.join();
}

void Discord::Enqueue(std::function<void()> _task) {
	{
		std::lock_guard lock(mutex);

		if (stopping)
			return;

		queue.push(std::move(_task));
	}

	condition.notify_one();
}

void Discord::Worker() {
	while (true) {
		std::function<void()> task;

		{
			std::unique_lock lock(mutex);

			condition.wait(lock, [this] { return stopping || !queue.empty(); });

			if (stopping && queue.empty())
				break;

			task = std::move(queue.front());
			queue.pop();
		}

		task();
	}

	if (curl) {
		curl_easy_cleanup(curl);
		curl = nullptr;
	}
}

void Discord::Init(const std::string& _token, const std::string& _channelId) {
	Enqueue([this, token = _token, channelId = _channelId] {
		this->token = token;
		this->channelId = channelId;

		if (token.empty() || channelId.empty()) {
			GlobalLogger().warn << "Discord integration is enabled but discord-token or discord-channel-id "
			                       "is missing/empty in server.properties; Discord messages will not be sent.\n";

			initialized = false;
			return;
		}

		if (!curl) {
			curl = curl_easy_init();

			if (!curl) {
				GlobalLogger().error << "Discord: failed to initialize CURL\n";

				initialized = false;
				return;
			}
		}

		initialized = true;
	});
}

void Discord::SendMessage(const std::string& _message) {
	Enqueue([this, message = _message] {
		if (!initialized || !curl)
			return;

		std::string url = "https://discord.com/api/v10/channels/" + channelId + "/messages";

		std::string deformatted = RemoveMinecraftFormatting(message);

		std::string json = "{\"content\":\"" + EscapeJson(deformatted) + "\"}";

		struct curl_slist* headers = nullptr;

		std::string authorization = "Authorization: Bot " + token;

		headers = curl_slist_append(headers, authorization.c_str());

		headers = curl_slist_append(headers, "Content-Type: application/json");

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		                 [](char*, size_t size, size_t nmemb, void*) { return size * nmemb; });

		CURLcode result = curl_easy_perform(curl);

		if (result != CURLE_OK) {
			GlobalLogger().warn << "Discord: failed to send message: " << curl_easy_strerror(result) << "\n";
		}

		curl_slist_free_all(headers);
	});
}

void Discord::SendFile(const std::string& _filename, const std::string& _message) {
	Enqueue([this, filename = _filename, message = _message] {
		if (!initialized || !curl)
			return;

		std::string url = "https://discord.com/api/v10/channels/" + channelId + "/messages";

		struct curl_slist* headers = nullptr;

		std::string authorization = "Authorization: Bot " + token;

		headers = curl_slist_append(headers, authorization.c_str());

		curl_mime* mime = curl_mime_init(curl);

		// Message content
		curl_mimepart* content = curl_mime_addpart(mime);

		curl_mime_name(content, "payload_json");

		std::string json = "{\"content\":\"" + EscapeJson(message) + "\"}";

		curl_mime_data(content, json.c_str(), CURL_ZERO_TERMINATED);

		// File
		curl_mimepart* file = curl_mime_addpart(mime);

		curl_mime_name(file, "files[0]");
		curl_mime_filedata(file, filename.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		                 [](char*, size_t size, size_t nmemb, void*) { return size * nmemb; });

		CURLcode result = curl_easy_perform(curl);

		if (result != CURLE_OK) {
			GlobalLogger().warn << "Discord: failed to send file '" << filename << "': " << curl_easy_strerror(result)
			                    << "\n";
		}

		curl_mime_free(mime);
		curl_slist_free_all(headers);
	});
}

void Discord::SendMessageSync(const std::string& _message) {
	if (token.empty() || channelId.empty())
		return;

	// Do not trust whatever the parent process's worker thread left behind in libcurl's global state
	curl_global_cleanup();
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
		return;

	CURL* localCurl = curl_easy_init();
	if (!localCurl) {
		curl_global_cleanup();
		return;
	}

	std::string url = "https://discord.com/api/v10/channels/" + channelId + "/messages";
	std::string deformatted = RemoveMinecraftFormatting(_message);
	std::string json = "{\"content\":\"" + EscapeJson(deformatted) + "\"}";

	struct curl_slist* headers = nullptr;
	std::string authorization = "Authorization: Bot " + token;
	headers = curl_slist_append(headers, authorization.c_str());
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(localCurl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(localCurl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(localCurl, CURLOPT_POST, 1L);
	curl_easy_setopt(localCurl, CURLOPT_POSTFIELDS, json.c_str());
	curl_easy_setopt(localCurl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(localCurl, CURLOPT_TIMEOUT, 10L); // Don't hang the crashing process forever
	curl_easy_setopt(localCurl, CURLOPT_WRITEFUNCTION,
	                 [](char*, size_t size, size_t nmemb, void*) { return size * nmemb; });

	curl_easy_perform(localCurl);

	curl_slist_free_all(headers);
	curl_easy_cleanup(localCurl);
	curl_global_cleanup();
}

void Discord::SendFileSync(const std::string& _filename, const std::string& _message) {
	if (token.empty() || channelId.empty())
		return;

	curl_global_cleanup();
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
		return;

	CURL* localCurl = curl_easy_init();
	if (!localCurl) {
		curl_global_cleanup();
		return;
	}

	std::string url = "https://discord.com/api/v10/channels/" + channelId + "/messages";

	struct curl_slist* headers = nullptr;
	std::string authorization = "Authorization: Bot " + token;
	headers = curl_slist_append(headers, authorization.c_str());

	curl_mime* mime = curl_mime_init(localCurl);

	// Message content
	curl_mimepart* content = curl_mime_addpart(mime);
	curl_mime_name(content, "payload_json");
	std::string json = "{\"content\":\"" + EscapeJson(_message) + "\"}";
	curl_mime_data(content, json.c_str(), CURL_ZERO_TERMINATED);

	// File
	curl_mimepart* file = curl_mime_addpart(mime);
	curl_mime_name(file, "files[0]");
	curl_mime_filedata(file, _filename.c_str());

	curl_easy_setopt(localCurl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(localCurl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(localCurl, CURLOPT_MIMEPOST, mime);
	curl_easy_setopt(localCurl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(localCurl, CURLOPT_TIMEOUT, 15L);
	curl_easy_setopt(localCurl, CURLOPT_WRITEFUNCTION,
	                 [](char*, size_t size, size_t nmemb, void*) { return size * nmemb; });

	curl_easy_perform(localCurl);

	curl_mime_free(mime);
	curl_slist_free_all(headers);
	curl_easy_cleanup(localCurl);
	curl_global_cleanup();
}

std::string Discord::RemoveMinecraftFormatting(const std::string& _input) {
	std::string result;
	result.reserve(_input.size());

	for (size_t i = 0; i < _input.size(); ++i) {
		// UTF-8 encoding of § is C2 A7
		if (static_cast<unsigned char>(_input[i]) == 0xC2 && i + 1 < _input.size() &&
		    static_cast<unsigned char>(_input[i + 1]) == 0xA7) {
			// Skip §
			i += 1;

			// Skip Minecraft formatting code
			if (i + 1 < _input.size())
				i += 1;

			continue;
		}

		result += _input[i];
	}

	return result;
}

std::string Discord::EscapeJson(const std::string& _input) {
	std::string result;

	for (char c : _input) {
		switch (c) {
		case '"':
			result += "\\\"";
			break;
		case '\\':
			result += "\\\\";
			break;
		case '\n':
			result += "\\n";
			break;
		case '\r':
			result += "\\r";
			break;
		case '\t':
			result += "\\t";
			break;
		default:
			result += c;
			break;
		}
	}

	return result;
}

Discord& GlobalDiscord() {
	static Discord discord;
	return discord;
}

#endif