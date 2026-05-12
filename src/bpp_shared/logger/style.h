/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <cstdint>
#include <string>

#define STYLE_BOLD                "\033[1m"
#define STYLE_ITALIC              "\033[3m"
#define STYLE_UNDERLINE           "\033[4m"
#define STYLE_STRIKETHROUGH       "\033[9m"

#define STYLE_FOREGROUND_BLACK    "\033[30m"
#define STYLE_FOREGROUND_RED      "\033[31m"
#define STYLE_FOREGROUND_GREEN    "\033[32m"
#define STYLE_FOREGROUND_YELLOW   "\033[33m"
#define STYLE_FOREGROUND_BLUE     "\033[34m"
#define STYLE_FOREGROUND_PURPLE   "\033[35m"
#define STYLE_FOREGROUND_CYAN     "\033[36m"
#define STYLE_FOREGROUND_WHITE    "\033[37m"

#define STYLE_BACKGROUND_BLACK    "\033[40m"
#define STYLE_BACKGROUND_RED      "\033[41m"
#define STYLE_BACKGROUND_GREEN    "\033[42m"
#define STYLE_BACKGROUND_YELLOW   "\033[43m"
#define STYLE_BACKGROUND_BLUE     "\033[44m"
#define STYLE_BACKGROUND_PURPLE   "\033[45m"
#define STYLE_BACKGROUND_CYAN     "\033[46m"
#define STYLE_BACKGROUND_WHITE    "\033[47m"

#define STYLE_RESET               "\033[0m"

std::string FormatToStyle(int8_t format);
std::string HandleFormattingCodes(const std::string& input);