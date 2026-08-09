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

	// Synchronous, self-contained equivalents safe to call from CrashCatch's post-fork
	// crash handler (see linuxSignalHandler in CrashCatch.hpp).
	void SendMessageSync(const std::string& _message);
	void SendFileSync(const std::string& _filename, const std::string& _message = "");

	// Static, instance-free versions of the above. These take token/channelId as
	// parameters instead of reading them off a live Discord object, so they can be
	// called from a fresh, single-threaded, freshly-exec'd process (no GlobalDiscord(),
	// no worker thread, no queue/mutex) - see CrashUploadMain() in main.cpp.
	//
	// IMPORTANT: never call these from a fork()'d child that hasn't exec'd. libcurl and
	// the TLS/DNS libraries it uses keep global locks that may be held by *other*
	// threads in the parent (e.g. GlobalDiscord()'s worker thread). fork() only
	// duplicates the calling thread; any lock those other threads held is inherited
	// "stuck" in the child forever, since the thread that would release it doesn't
	// exist there. Touching curl's global state (curl_global_init/cleanup, or even
	// curl_easy_init the first time) in that situation can deadlock or - as seen in
	// the observed segfault inside libcurl.so - crash outright. Only call these after
	// an exec(), which discards all inherited process/thread state.
	static void SendMessageSyncStatic(const std::string& _token, const std::string& _channelId,
	                                   const std::string& _message);
	static void SendFileSyncStatic(const std::string& _token, const std::string& _channelId,
	                                const std::string& _filename, const std::string& _message = "");

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