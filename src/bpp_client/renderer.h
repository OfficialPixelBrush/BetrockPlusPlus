/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#include "window.h"
#include <SDL3/SDL.h>

class Renderer {
public:
	explicit Renderer(Window& _window);
	~Renderer();

	void Render(int _partialTicks);

private:
	SDL_Window* window;
	SDL_GPUDevice* gpu;
};