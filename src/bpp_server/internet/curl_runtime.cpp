/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "curl_runtime.h"

#if defined(ONLINE_MODE_AUTHENTICATION) || defined(BETACRAFT_HEARTBEAT)
#include <atomic>
#include <csignal>
#include <curl/curl.h>
#if defined(__linux__) || defined(__APPLE__) || defined(__HAIKU__)
#include <pthread.h>
#include <signal.h>
#endif

extern std::atomic<bool> shutdownRequested;

static std::atomic<bool> curlInitialized{ false };

static void CurlStopSignalHandler(int /*sig*/) {
	shutdownRequested.store(true);
}

void CurlRestoreStopSignals() {
#if defined(_WIN32) || defined(_WIN64)
	std::signal(SIGINT, CurlStopSignalHandler);
	std::signal(SIGTERM, CurlStopSignalHandler);
#else
	struct sigaction sa{};
	sa.sa_handler = CurlStopSignalHandler;
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
bool CurlRuntimeInit() {
	if (curlInitialized.exchange(true)) {
		CurlRestoreStopSignals();
		return true;
	}
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
		curlInitialized.store(false);
		return false;
	}
	CurlRestoreStopSignals();
	return true;
}

void CurlRuntimeCleanup() {
	if (!curlInitialized.exchange(false))
		return;
	curl_global_cleanup();
}
#endif
