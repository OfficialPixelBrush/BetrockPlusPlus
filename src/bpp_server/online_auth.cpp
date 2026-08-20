/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "online_auth.h"

#ifdef ONLINE_MODE_AUTHENTICATION
#include <atomic>
#include <csignal>
#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#include <signal.h>
#endif

extern std::atomic<bool> shutdownRequested;

static void AuthStopSignalHandler(int /*sig*/) {
	shutdownRequested.store(true);
}

static void RestoreStopSignals() {
#if defined(_WIN32) || defined(_WIN64)
	std::signal(SIGINT, AuthStopSignalHandler);
	std::signal(SIGTERM, AuthStopSignalHandler);
#else
	struct sigaction sa{};
	sa.sa_handler = AuthStopSignalHandler;
	sigemptyset(&sa.sa_mask);
	// No SA_RESTART: a blocked connect/wait must return so the tick loop
	// can observe shutdownRequested.
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	// Keep these blocked so the sigwait thread in main() receives them.
	// Unblocking here let libcurl/OpenSSL swallow SIGTERM on musl.
	pthread_sigmask(SIG_BLOCK, &set, nullptr);
#ifdef SIGPIPE
	std::signal(SIGPIPE, SIG_IGN);
#endif
#endif
}

// Note: MUST happen before any other threads exist, otherwise this can block all other signals on Musl
bool Authentication::GlobalInit() {
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
		return false;
	RestoreStopSignals();
	return true;
}

void Authentication::GlobalCleanup() {
	curl_global_cleanup();
}

std::string Authentication::GenerateAuthHash() {
	// Source - https://stackoverflow.com/a/5100745
	// Posted by Kornel Kisielewicz, modified by community. See post 'Timeline' for change history
	// Retrieved 2026-08-13, License - CC BY-SA 4.0
	std::stringstream stream;
	stream << std::hex << Java::Random().NextLong();
	return stream.str();
}

static size_t WriteCallback(char* _ptr, size_t _size, size_t _nmemb, void* _userdata) {
	auto* out = static_cast<std::string*>(_userdata);
	out->append(_ptr, _size * _nmemb);
	return _size * _nmemb;
}

bool Authentication::IsRegisteredUsername(std::string _serverId, std::string _username) {
	if (!onlineMode)
		return true;
	CURL* curl = curl_easy_init();
	if (!curl) {
		GlobalLogger().error << "Failed to initialize libcurl for username verification.\n";
		return false;
	}
	std::string url = std::format("{}/checkserver.jsp?user={}&serverId={}", baseUrl, _username, _serverId);
	std::string response;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Betrock++");
	// libcurl/OpenSSL otherwise install SIGALRM handlers and can block
	// SIGINT/SIGTERM in a threaded process (musl will then ignore Ctrl+C).
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYPORT, static_cast<long>(proxyPort));

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	RestoreStopSignals();

	if (res != CURLE_OK)
		return false;

	return response == "YES";
}
#endif