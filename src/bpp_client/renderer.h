/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#include "sokol_gfx.h"
#include "window.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

class Renderer {
public:
	explicit Renderer(Window& _window);
	~Renderer();

	void Render(int _partialTicks);

private:
	Window& window;
	SDL_GLContext glContext;

	sg_swapchain GetSwapchain();

	static void SokolLogCallback(const char* _tag,              // always "sg"
	                             uint32_t _logLevel,           // 0=panic, 1=error, 2=warning, 3=info
	                             uint32_t _logItemId,         // SG_LOGITEM_*
	                             const char* _messageOrNull,  // a message string, may be nullptr in release mode
	                             uint32_t _lineNr,             // line number in sokol_gfx.h
	                             const char* _filenameOrNull, // source filename, may be nullptr in release mode
	                             void* _userData);
};