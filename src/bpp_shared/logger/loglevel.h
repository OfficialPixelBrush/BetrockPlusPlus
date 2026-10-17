/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <cstdint>
enum LogLevel : int8_t {
	LOG_NONE = 0,
	LOG_MESSAGE = 1,
	LOG_INFO = 2,
	LOG_WARNING = 4,
	LOG_ERROR = 8,
	LOG_DEBUG = 16,
	LOG_ALL = LOG_DEBUG | LOG_ERROR | LOG_WARNING | LOG_INFO | LOG_MESSAGE
};