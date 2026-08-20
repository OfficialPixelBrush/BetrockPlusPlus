/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#ifdef ONLINE_MODE_AUTHENTICATION
#include "../bpp_shared/helpers/java/java_random.h"
#include "logger.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <format>
#include <ios>
#include <sstream>
#include <string>

struct Authentication {
	bool onlineMode = false;
	std::string proxy = "http://betacraft.ee";
	uint16_t proxyPort = 11705;
	const std::string baseUrl = "http://www.minecraft.net/game";
	static bool GlobalInit();
	static void GlobalCleanup();
	std::string GenerateAuthHash();
	bool IsRegisteredUsername(std::string _serverId, std::string _username);
};
#endif