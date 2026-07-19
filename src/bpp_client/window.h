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
	Window(Int2 screenSize, const std::string& title, const WindowOptions& options);
	~Window();

	// Non-copyable
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void setTitle(const std::string& title) {
		SDL_SetWindowTitle(m_handle, title.c_str());
	}
	void setCursorCapture(bool enabled) {
		SDL_SetWindowRelativeMouseMode(m_handle, enabled);
	}
	const Int2& getScreenSize() const {
		return m_screenSize;
	}
	float getAspect() const {
		return float(m_screenSize.x) / float(m_screenSize.y);
	}

	SDL_Window* getHandle() const {
		return m_handle;
	}

private:
	SDL_Window* m_handle = nullptr;
	Int2 m_screenSize;

	static void sdlLogCallback(void* userdata, int category, SDL_LogPriority priority, const char* message);
};