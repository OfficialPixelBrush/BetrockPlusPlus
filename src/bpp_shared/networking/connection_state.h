/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include <cstdint>

enum class ConnectionState : uint8_t {
	Handshaking,
	LoggingIn,
	VerifyingUsername,
	WaitingForSpawnChunks,
	Playing
};