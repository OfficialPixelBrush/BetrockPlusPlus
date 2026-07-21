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

	static void SokolLogCallback(const char* tag,              // always "sg"
	                             uint32_t log_level,           // 0=panic, 1=error, 2=warning, 3=info
	                             uint32_t log_item_id,         // SG_LOGITEM_*
	                             const char* message_or_null,  // a message string, may be nullptr in release mode
	                             uint32_t line_nr,             // line number in sokol_gfx.h
	                             const char* filename_or_null, // source filename, may be nullptr in release mode
	                             void* user_data);
};