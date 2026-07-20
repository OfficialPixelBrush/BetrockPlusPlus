/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <cassert>
#include <string>

#include "numeric_structs.h"

enum class WindowMode {
	WINDOWED,
	WINDOWED_RESIZABLE,
	FULLSCREEN
};

struct WindowOptions {
	WindowMode windowMode;
};

class Window {
public:
	Window(Int2 _screenSize, const std::string& _title, const WindowOptions& _options);
	~Window();

	// Non-copyable
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void setTitle(const std::string& _title) {
		SDL_SetWindowTitle(handle, _title.c_str());
	}
	void setCursorCapture(bool _enabled) {
		SDL_SetWindowRelativeMouseMode(handle, _enabled);
	}
	const Int2& getScreenSize() const {
		return screenSize;
	}
	float getAspect() const {
		return float(screenSize.x) / float(screenSize.y);
	}

	SDL_Window* getHandle() const {
		return handle;
	}

private:
	SDL_Window* handle = nullptr;
	Int2 screenSize;

	static void sdlLogCallback(void* _userdata, int _category, SDL_LogPriority _priority, const char* _message);
};