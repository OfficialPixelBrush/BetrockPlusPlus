/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "renderer.h"
#include "logger.h"
#include "window.h"
#include <SDL3/SDL_video.h>
// This is should be defined only once
#define SOKOL_GFX_IMPL
#define SOKOL_GLCORE
#include "sokol_gfx.h"

Renderer::Renderer(Window& _window) : window(_window) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Required for macOS
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	glContext = SDL_GL_CreateContext(window.GetHandle());

	// We don't need anything extra for OpenGL
	sg_setup(sg_desc{ .logger = sg_logger{ .func = SokolLogCallback } });
}

Renderer::~Renderer() {
	sg_shutdown();
	SDL_GL_DestroyContext(glContext);
}

sg_swapchain Renderer::GetSwapchain() {
	Int2 framebufferSize = window.GetFramebufferSize();
	return { .invalid = false,
		     .width = framebufferSize.x,
		     .height = framebufferSize.y,
		     .sample_count = 4,
		     .color_format = SG_PIXELFORMAT_RGBA8,
		     .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
		     .gl = { .framebuffer = 0 } }; // 0 is our window
}

void Renderer::Render(int _partialTicks) {
	sg_pass_action passAction{};
	passAction.colors[0].load_action = SG_LOADACTION_CLEAR;
	passAction.colors[0].clear_value = { 1, 0, 0, 1 };

	sg_begin_pass(sg_pass{ .action = passAction, .swapchain = GetSwapchain() });

	//TODO: Render stuff

	sg_end_pass();

	// Frame end
	sg_commit();
	window.SwapBuffers();
}

void Renderer::SokolLogCallback(const char* _tag, uint32_t _logLevel, uint32_t _logItemId, const char* _messageOrNull,
                                uint32_t _lineNr, const char* _filenameOrNull, void* _userData) {
	if (!_messageOrNull)
		return;

	LogLevel level;
	switch (_logLevel) {
	case 0: // panic
	case 1: // error
		level = LOG_ERROR;
		break;
	case 2: // warning
		level = LOG_WARNING;
		break;
	case 3: // info
		level = LOG_INFO;
		break;
	default:
		level = LOG_DEBUG;
		break;
	}

	GlobalLogger().Log(_messageOrNull, level);
}