/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "online_auth.h"

#ifdef ONLINE_MODE_AUTHENTICATION
#include "curl_runtime.h"

bool Authentication::GlobalInit() {
	return CurlRuntimeInit();
}

void Authentication::GlobalCleanup() {
	CurlRuntimeCleanup();
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
	CurlRestoreStopSignals();

	if (res != CURLE_OK)
		return false;

	return response == "YES";
}
#endif