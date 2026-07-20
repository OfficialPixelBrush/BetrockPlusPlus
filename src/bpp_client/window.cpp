/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>

#include "logger.h"
#include "misc.h"
#include "window.h"

void Window::sdlLogCallback(void* _userdata, int _category, SDL_LogPriority _priority, const char* _message) {
	LogLevel level;
	switch (_priority) {
	case SDL_LOG_PRIORITY_INFO:
		level = LOG_INFO;
		break;
	case SDL_LOG_PRIORITY_WARN:
		level = LOG_WARNING;
		break;
	case SDL_LOG_PRIORITY_ERROR:
	case SDL_LOG_PRIORITY_CRITICAL:
		level = LOG_ERROR;
		break;
	default:
		level = LOG_DEBUG;
		break;
	}
	GlobalLogger().Log(_message, level);
}

Window::Window(Int2 _screenSize, const std::string& _title, const WindowOptions& _options) : screenSize(_screenSize) {
	SDL_SetLogOutputFunction(sdlLogCallback, nullptr);
#ifndef NDEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif

	if (!SDL_Init(SDL_INIT_VIDEO))
		THROW_SDL_ERROR("Failed to initialize SDL!");

	SDL_WindowFlags windowFlags;
	switch (_options.windowMode) {
	case WindowMode::WINDOWED_RESIZABLE:
		windowFlags = SDL_WINDOW_RESIZABLE;
		break;
	case WindowMode::FULLSCREEN:
		windowFlags = SDL_WINDOW_FULLSCREEN;
		break;
	default:
		windowFlags = 0;
		break;
	}

	handle = SDL_CreateWindow(_title.c_str(), screenSize.x, screenSize.y, windowFlags);
	if (!handle)
		THROW_SDL_ERROR("Failed to create SDL Window!");

	GlobalLogger().info << "Window initialized!\n";
}

Window::~Window() {
	SDL_DestroyWindow(handle);
	SDL_Quit();
}